/*!
 * (C) 2009 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * \file   text_file_backend.cpp
 * \author Andrey Semashev
 * \date   09.06.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#include <memory>
#include <string>
#include <locale>
#include <ostream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

#if !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_SPIRIT_THREADSAFE)
#define BOOST_SPIRIT_THREADSAFE
#endif // !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_SPIRIT_THREADSAFE)

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/compatibility/cpp_c_headers/cctype>
#include <boost/compatibility/cpp_c_headers/cwctype>
#include <boost/compatibility/cpp_c_headers/ctime>
#include <boost/compatibility/cpp_c_headers/cstdio>
#include <boost/compatibility/cpp_c_headers/cstdlib>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/log/detail/snprintf.hpp>
#include <boost/log/detail/throw_exception.hpp>
#include <boost/log/attributes/time_traits.hpp>
#include <boost/log/sinks/text_file_backend.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sinks {

namespace {

    typedef boost::log::aux::universal_path::string_type path_string_type;
    typedef path_string_type::value_type path_char_type;

    //! The function generates a temporary file name
    boost::log::aux::universal_path make_temp_file_name()
    {
        char str[L_tmpnam + 1];
        const char* name = NULL;
        boost::log::aux::universal_path res;
        unsigned int n = 0;
        do
        {
            using namespace std; // in case if tmpnam is in std namespace
            name = tmpnam(str);
            if (name)
                res = boost::log::aux::to_universal_path(name);
            else
                break;
        }
        while (filesystem::exists(res) && ++n < TMP_MAX);

        if (!name || n >= TMP_MAX)
        {
            boost::log::aux::throw_exception(
                std::runtime_error("failed to generate a suitable temporary file name"));
        }

        return res;
    }

    //! An auxiliary traits that contain various constants and functions regarding string and character operations
    template< typename CharT >
    struct file_controller_traits;

    template< >
    struct file_controller_traits< char >
    {
        enum
        {
            percent = '%',
            number_placeholder = 'N',
            day_placeholder = 'd',
            month_placeholder = 'm',
            year_placeholder = 'y',
            full_year_placeholder = 'Y',
            frac_sec_placeholder = 'f',
            seconds_placeholder = 'S',
            minutes_placeholder = 'M',
            hours_placeholder = 'H',
            space = ' ',
            plus = '+',
            minus = '-',
            zero = '0',
            dot = '.'
        };
        static bool is_digit(char c)
        {
            return (isdigit(c) != 0);
        }
    };

    template< >
    struct file_controller_traits< wchar_t >
    {
        enum
        {
            percent = L'%',
            number_placeholder = L'N',
            day_placeholder = L'd',
            month_placeholder = L'm',
            year_placeholder = L'y',
            full_year_placeholder = L'Y',
            frac_sec_placeholder = L'f',
            seconds_placeholder = L'S',
            minutes_placeholder = L'M',
            hours_placeholder = L'H',
            space = L' ',
            plus = L'+',
            minus = L'-',
            zero = L'0',
            dot = L'.'
        };
        static bool is_digit(wchar_t c)
        {
            return (iswdigit(c) != 0);
        }
    };

    //! Date and time formatter
    class date_and_time_formatter
    {
    public:
        typedef path_string_type result_type;

    private:
        typedef date_time::time_facet< posix_time::ptime, path_char_type > time_facet_type;

    private:
        time_facet_type* m_pFacet;
        mutable std::basic_ostringstream< path_char_type > m_Stream;

    public:
        //! Constructor
        date_and_time_formatter() : m_pFacet(NULL)
        {
            std::auto_ptr< time_facet_type > pFacet(new time_facet_type());
            m_pFacet = pFacet.get();
            std::locale loc(m_Stream.getloc(), m_pFacet);
            pFacet.release();
            m_Stream.imbue(loc);
        }
        //! Copy constructor
        date_and_time_formatter(date_and_time_formatter const& that) :
            m_pFacet(that.m_pFacet)
        {
            m_Stream.imbue(that.m_Stream.getloc());
        }
        //! The method formats the current date and time according to the format string str and writes the result into it
        path_string_type operator()(path_string_type const& pattern, unsigned int counter) const
        {
            m_pFacet->format(pattern.c_str());
            m_Stream.str(path_string_type());
            m_Stream << boost::log::attributes::local_time_traits::get_clock();
            if (m_Stream.good())
            {
                return m_Stream.str();
            }
            else
            {
                m_Stream.clear();
                return pattern;
            }
        }
    };

    //! The functor formats the file counter into the file name
    class file_counter_formatter
    {
    public:
        typedef path_string_type result_type;

    private:
        //! The position in the pattern where the file counter placeholder is
        path_string_type::size_type m_FileCounterPosition;
        //! The file counter formatting stream
        mutable std::basic_ostringstream< path_char_type > m_Stream;

    public:
        //! Initializing constructor
        file_counter_formatter(path_string_type::size_type pos, unsigned int width, unsigned int precision) :
            m_FileCounterPosition(pos)
        {
            typedef file_controller_traits< path_char_type > traits_t;
            if (width != (std::numeric_limits< unsigned int >::max)())
                m_Stream.width(width);
            if (precision != (std::numeric_limits< unsigned int >::max)())
                m_Stream.precision(precision);
            m_Stream.fill(traits_t::zero);
        }
        //! Copy constructor
        file_counter_formatter(file_counter_formatter const& that) :
            m_FileCounterPosition(that.m_FileCounterPosition)
        {
            m_Stream.width(that.m_Stream.width());
            m_Stream.precision(that.m_Stream.precision());
            m_Stream.fill(that.m_Stream.fill());
        }

        //! The function formats the file counter into the file name
        path_string_type operator()(path_string_type const& pattern, unsigned int counter) const
        {
            path_string_type file_name = pattern;

            m_Stream.str(path_string_type());
            m_Stream << counter;
            file_name.insert(m_FileCounterPosition, m_Stream.str());

            return file_name;
        }
    };

    //! The function returns the pattern as the file name
    class empty_formatter
    {
    public:
        typedef path_string_type result_type;

    private:
        path_string_type m_Pattern;

    public:
        //! Initializing constructor
        explicit empty_formatter(path_string_type const& pattern) : m_Pattern(pattern)
        {
        }
        //! Copy constructor
        empty_formatter(empty_formatter const& that) : m_Pattern(that.m_Pattern)
        {
        }

        //! The function returns the pattern as the file name
        path_string_type const& operator() (unsigned int) const
        {
            return m_Pattern;
        }
    };

    //! The function parses the format placeholder for file counter
    bool parse_counter_placeholder(
        path_string_type::const_iterator& it,
        path_string_type::const_iterator end,
        unsigned int& width,
        unsigned int& precision)
    {
        typedef file_controller_traits< path_char_type > traits_t;
        spirit::classic::parse_info< path_string_type::const_iterator > result = spirit::classic::parse(it, end,
        (
            !(
                spirit::classic::ch_p(static_cast< path_char_type >(traits_t::zero)) |
                spirit::classic::ch_p(static_cast< path_char_type >(traits_t::plus)) |
                spirit::classic::ch_p(static_cast< path_char_type >(traits_t::minus)) |
                spirit::classic::ch_p(static_cast< path_char_type >(traits_t::space))
            ) >>
            !spirit::classic::uint_p[spirit::classic::assign_a(width)] >>
            !(
                spirit::classic::ch_p(static_cast< path_char_type >(traits_t::dot)) >>
                spirit::classic::uint_p[spirit::classic::assign_a(precision)]
            ) >>
            spirit::classic::ch_p(static_cast< path_char_type >(traits_t::number_placeholder))
        ));

        if (result.hit)
        {
            it = result.stop;
            return true;
        }
        else
            return false;
    }

    //! The function matches the file name and the pattern
    bool match_pattern(path_string_type const& file_name, path_string_type const& pattern)
    {
        typedef file_controller_traits< path_char_type > traits_t;
        path_string_type::const_iterator
            f_it = file_name.begin(),
            f_end = file_name.end(),
            p_it = pattern.begin(),
            p_end = pattern.end();
        bool placeholder_expected = false;
        while (f_it != f_end && p_it != p_end)
        {
            path_char_type p_c = *p_it, f_c = *f_it;
            if (!placeholder_expected)
            {
                if (p_c == traits_t::percent)
                {
                    placeholder_expected = true;
                    ++p_it;
                }
                else if (p_c == f_c)
                {
                    ++p_it;
                    ++f_it;
                }
                else
                    return false;
            }
            else
            {
                struct local
                {
                    // Verifies that the string contains exactly n digits
                    static bool scan_digits(
                        path_string_type::const_iterator& it,
                        path_string_type::const_iterator end,
                        unsigned int n)
                    {
                        for (; n > 0; --n)
                        {
                            path_char_type c = *it++;
                            if (!traits_t::is_digit(c) || it == end)
                                return false;
                        }
                        return true;
                    }
                };

                switch (p_c)
                {
                case traits_t::percent: // An escaped '%'
                    if (p_c == f_c)
                    {
                        ++p_it;
                        ++f_it;
                        break;
                    }
                    else
                        return false;

                case traits_t::seconds_placeholder: // Date/time components with 2-digits width
                case traits_t::minutes_placeholder:
                case traits_t::hours_placeholder:
                case traits_t::day_placeholder:
                case traits_t::month_placeholder:
                case traits_t::year_placeholder:
                    if (!local::scan_digits(f_it, f_end, 2))
                        return false;
                    ++p_it;
                    break;

                case traits_t::full_year_placeholder: // Date/time components with 4-digits width
                    if (!local::scan_digits(f_it, f_end, 4))
                        return false;
                    ++p_it;
                    break;

                case traits_t::frac_sec_placeholder: // Fraction seconds width is configuration-dependent
                    typedef posix_time::ptime::time_system_type::time_rep_type::resolution_traits posix_resolution_traits;
                    if (!local::scan_digits(f_it, f_end, posix_resolution_traits::num_fractional_digits()))
                    {
                        return false;
                    }
                    ++p_it;
                    break;

                default: // This should be the file counter placeholder or some unsupported placeholder
                    {
                        path_string_type::const_iterator p = p_it;
                        unsigned int width = (std::numeric_limits< unsigned int >::max)(), precision = 0;
                        if (!parse_counter_placeholder(p, p_end, width, precision))
                        {
                            boost::log::aux::throw_exception(std::logic_error(
                                "unsupported placeholder used in pattern for file scanning"));
                        }

                        // There will be at least precision and at most width digits
                        if (precision > width)
                            precision = width;
                        if (!local::scan_digits(f_it, f_end, precision))
                            return false;
                        for (unsigned int extra_digits = width - precision; extra_digits > 0; --extra_digits)
                        {
                            path_char_type c = *f_it;
                            if (!traits_t::is_digit(c) || ++f_it == f_end)
                                break;
                        }

                        p_it = ++p;
                    }
                    break;
                }

                placeholder_expected = false;
            }
        }

        return (f_it == f_end && p_it == p_end);
    }

} // namespace

fifo_file_collector::fifo_file_collector()
{
    // Yeah, not too cute. The empty_arg_list class should really be public.
    construct(parameter::aux::empty_arg_list());
}

fifo_file_collector::fifo_file_collector(fifo_file_collector const& that) :
    m_MaxSize(that.m_MaxSize),
    m_MinFreeSpace(that.m_MinFreeSpace),
    m_StorageDir(that.m_StorageDir),
    m_FileNameGenerator(that.m_FileNameGenerator),
    m_Files(that.m_Files),
    m_FileCounter(that.m_FileCounter),
    m_TotalSize(that.m_TotalSize)
{
}

//! Constructor implementation
void fifo_file_collector::construct(
    uintmax_t max_size, uintmax_t min_free_space, boost::log::aux::universal_path const& pattern, file_scan_method scan)
{
    m_MaxSize = max_size;
    m_MinFreeSpace = min_free_space;
    m_StorageDir = pattern.branch_path();
    path_string_type name_pattern = pattern.leaf();

    m_TotalSize = static_cast< uintmax_t >(0);

    // Let's try to find the file counter placeholder
    typedef file_controller_traits< path_char_type > traits_t;
    unsigned int placeholder_count = 0;
    unsigned int width = (std::numeric_limits< unsigned int >::max)();
    unsigned int precision = (std::numeric_limits< unsigned int >::max)();
    bool counter_found = false;
    path_string_type::size_type counter_pos = 0;
    path_string_type::const_iterator end = name_pattern.end();
    path_string_type::const_iterator it = name_pattern.begin();

    do
    {
        it = std::find(it, end, static_cast< path_char_type >(traits_t::percent));
        if (it == end)
            break;
        path_string_type::const_iterator placeholder_begin = it++;
        if (it == end)
            break;
        if (*it == traits_t::percent)
        {
            // An escaped percent detected
            ++it;
            continue;
        }

        ++placeholder_count;

        if (!counter_found && parse_counter_placeholder(it, end, width, precision))
        {
            // We've found the file counter placeholder in the pattern
            counter_found = true;
            counter_pos = placeholder_begin - name_pattern.begin();
            name_pattern.erase(counter_pos, it - placeholder_begin);
            --placeholder_count;
            it = name_pattern.begin() + counter_pos;
            end = name_pattern.end();
        }
    }
    while (it != end);

    // Construct the formatter functor
    unsigned int choice = (static_cast< unsigned int >(placeholder_count > 0) << 1) |
                          static_cast< unsigned int >(counter_found);
    switch (choice)
    {
    case 1: // Only counter placeholder in the pattern
        m_FileNameGenerator = boost::bind(file_counter_formatter(counter_pos, width, precision), name_pattern, _1);
        break;
    case 2: // Only date/time placeholders in the pattern
        m_FileNameGenerator = boost::bind(date_and_time_formatter(), name_pattern, _1);
        break;
    case 3: // Counter and date/time placeholder in the pattern
        m_FileNameGenerator = boost::bind(date_and_time_formatter(),
            boost::bind(file_counter_formatter(counter_pos, width, precision), name_pattern, _1), _1);
        break;
    default: // No placeholders detected
        m_FileNameGenerator = empty_formatter(name_pattern);
        break;
    }

    scan_for_files(scan, scan == scan_all ? pattern.parent_path() : pattern);
}

fifo_file_collector& fifo_file_collector::operator= (fifo_file_collector that)
{
    this->swap(that);
    return *this;
}

void fifo_file_collector::swap(fifo_file_collector& that)
{
    std::swap(m_MaxSize, that.m_MaxSize);
    std::swap(m_MinFreeSpace, that.m_MinFreeSpace);
    m_StorageDir.swap(that.m_StorageDir);
    m_FileNameGenerator.swap(that.m_FileNameGenerator);
    m_Files.swap(that.m_Files);
    std::swap(m_FileCounter, that.m_FileCounter);
    std::swap(m_TotalSize, that.m_TotalSize);
}

void fifo_file_collector::operator() (boost::log::aux::universal_path const& p)
{
    // Let's construct the new file name
    file_info info;
    info.m_TimeStamp = filesystem::last_write_time(p);
    info.m_Size = filesystem::file_size(p);
    do
    {
        path_string_type file_name = m_FileNameGenerator(m_FileCounter++);
        info.m_Path = m_StorageDir / file_name;
    }
    while (filesystem::exists(info.m_Path));

    // Check if an old file should be erased
    uintmax_t free_space = m_MinFreeSpace ? filesystem::space(m_StorageDir).available : static_cast< uintmax_t >(0);
    file_list::iterator it = m_Files.begin(), end = m_Files.end();
    while (it != end &&
        (m_TotalSize + info.m_Size > m_MaxSize || (m_MinFreeSpace && m_MinFreeSpace > free_space)))
    {
        file_info& old_info = *it;
        if (filesystem::exists(old_info.m_Path) && filesystem::is_regular_file(old_info.m_Path))
        {
            try
            {
                filesystem::remove(old_info.m_Path);
                // Free space has to be queried as it may not increase equally
                // to the erased file size on compressed filesystems
                if (m_MinFreeSpace)
                    free_space = filesystem::space(m_StorageDir).available;
                m_TotalSize -= old_info.m_Size;
                m_Files.erase(it++);
            }
            catch (system::system_error&)
            {
                // Can't erase the file. Maybe it's locked? Never mind...
                ++it;
            }
        }
        else
        {
            // If it's not a file or is absent, just remove it from the list
            m_TotalSize -= old_info.m_Size;
            m_Files.erase(it++);
        }
    }

    // Move/rename the file to the target storage
    filesystem::create_directories(m_StorageDir);
    filesystem::rename(p, info.m_Path);

    m_Files.push_back(info);
    m_TotalSize += info.m_Size;
}

void fifo_file_collector::scan_for_files(file_scan_method method, boost::log::aux::universal_path const& pattern)
{
    if (method != no_scan)
    {
        boost::log::aux::universal_path dir;
        path_string_type mask;
        if (method == scan_matching)
        {
            dir = pattern.parent_path();
            mask = pattern.leaf();
        }
        else
            dir = pattern;

        if (filesystem::exists(dir) && filesystem::is_directory(dir))
        {
            typedef filesystem::basic_directory_iterator< boost::log::aux::universal_path > dir_iterator;
            file_list files;
            dir_iterator it(dir), end;
            uintmax_t total_size = 0;
            for (; it != end; ++it)
            {
                file_info info;
                info.m_Path = *it;
                if (filesystem::is_regular_file(info.m_Path))
                {
                    // Check that there are no duplicates in the resulting list
                    struct local
                    {
                        static bool equivalent(boost::log::aux::universal_path const& left, file_info const& right)
                        {
                            return filesystem::equivalent(left, right.m_Path);
                        }
                    };
                    if (std::find_if(m_Files.begin(), m_Files.end(),
                        boost::bind(&local::equivalent, boost::cref(info.m_Path), _1)) == m_Files.end())
                    {
                        // Check that the file name matches the pattern
                        if (method != scan_matching || match_pattern(info.m_Path.leaf(), mask))
                        {
                            info.m_Size = filesystem::file_size(info.m_Path);
                            total_size += info.m_Size;
                            info.m_TimeStamp = filesystem::last_write_time(info.m_Path);
                            files.push_back(info);
                        }
                    }
                }
            }

            // Sort files chronologically
            m_Files.splice(m_Files.end(), files);
            m_TotalSize = total_size;
            files.sort(boost::bind(&file_info::m_TimeStamp, _1) < boost::bind(&file_info::m_TimeStamp, _2));
        }
    }
}

//! Sink implementation data
template< typename CharT >
struct basic_text_file_backend< CharT >::implementation
{
    //! File open mode
    std::ios_base::openmode m_FileOpenMode;
    //! Temporary file name
    boost::log::aux::universal_path m_FileName;
    //! Temporary file name to be used since next file rotation
    optional< boost::log::aux::universal_path > m_NextFileName;
    //! File stream
    filesystem::basic_ofstream< CharT > m_File;
    //! Characters written
    uintmax_t m_CharactersWritten;

    //! File collector functional object
    file_collector_type m_FileCollector;

    //! The maximum file size, in characters written to the stream
    uintmax_t m_MaxFileSize;
    //! The flag shows if every written record should be flushed
    bool m_AutoFlush;

    implementation(
        boost::log::aux::universal_path const& temp,
        std::ios_base::openmode mode,
        uintmax_t max_size,
        bool auto_flush
    ) :
        m_FileOpenMode(mode),
        m_FileName(temp),
        m_CharactersWritten(0),
        m_MaxFileSize(max_size),
        m_AutoFlush(auto_flush)
    {
    }
};

//! Constructor. No streams attached to the constructed backend, auto flush feature disabled.
template< typename CharT >
basic_text_file_backend< CharT >::basic_text_file_backend()
{
    // Yeah, not too cute. The empty_arg_list class should really be public.
    construct(parameter::aux::empty_arg_list());
}

//! Destructor
template< typename CharT >
basic_text_file_backend< CharT >::~basic_text_file_backend()
{
    try
    {
        // Attempt to put the temporary file into storage
        if (m_pImpl->m_File.is_open() && m_pImpl->m_CharactersWritten > 0)
            rotate_file();
    }
    catch (...)
    {
    }

    delete m_pImpl;
}

//! Constructor implementation
template< typename CharT >
void basic_text_file_backend< CharT >::construct(
    boost::log::aux::universal_path const& temp,
    std::ios_base::openmode mode,
    uintmax_t max_size,
    bool auto_flush)
{
    m_pImpl = new implementation(temp, mode, max_size, auto_flush);
}


template< typename CharT >
void basic_text_file_backend< CharT >::max_file_size(uintmax_t size)
{
    m_pImpl->m_MaxFileSize = size;
}

template< typename CharT >
void basic_text_file_backend< CharT >::auto_flush(bool f)
{
    m_pImpl->m_AutoFlush = f;
}

template< typename CharT >
void basic_text_file_backend< CharT >::do_consume(
    record_type const& record, target_string_type const& formatted_message)
{
    if ((m_pImpl->m_File.is_open()
        && m_pImpl->m_CharactersWritten + formatted_message.size() >= m_pImpl->m_MaxFileSize)
        || !m_pImpl->m_File.good())
    {
        rotate_file();
    }

    if (!m_pImpl->m_File.is_open())
    {
        if (m_pImpl->m_FileName.empty())
            m_pImpl->m_FileName = make_temp_file_name();

        m_pImpl->m_File.open(m_pImpl->m_FileName, m_pImpl->m_FileOpenMode);
        if (!!m_pImpl->m_File.is_open())
        {
            boost::throw_exception(filesystem::basic_filesystem_error< boost::log::aux::universal_path >(
                "failed to open file for writing",
                m_pImpl->m_FileName,
                system::error_code(system::errc::io_error, system::get_generic_category())));
        }
    }

    m_pImpl->m_File.write(formatted_message.data(), static_cast< std::streamsize >(formatted_message.size()));
    m_pImpl->m_File.put(static_cast< typename target_string_type::value_type >('\n'));

    m_pImpl->m_CharactersWritten += formatted_message.size() + 1;

    if (m_pImpl->m_AutoFlush)
        m_pImpl->m_File.flush();
}

template< typename CharT >
void basic_text_file_backend< CharT >::set_temp_file_name(boost::log::aux::universal_path const& temp)
{
    if (m_pImpl->m_File.is_open())
    {
        m_pImpl->m_NextFileName = temp;
    }
    else
    {
        m_pImpl->m_FileName = temp;
        m_pImpl->m_NextFileName = none;
    }
}

template< typename CharT >
void basic_text_file_backend< CharT >::rotate_file()
{
    m_pImpl->m_File.close();
    m_pImpl->m_File.clear();
    m_pImpl->m_CharactersWritten = 0;
    if (!m_pImpl->m_FileCollector.empty())
        m_pImpl->m_FileCollector(m_pImpl->m_FileName);

    if (!!m_pImpl->m_NextFileName)
    {
        m_pImpl->m_FileName = m_pImpl->m_NextFileName.get();
        m_pImpl->m_NextFileName = none;
    }
}

template< typename CharT >
void basic_text_file_backend< CharT >::open_mode(std::ios_base::openmode mode)
{
    mode |= std::ios_base::out;
    mode &= ~std::ios_base::in;
    if ((mode & (std::ios_base::trunc | std::ios_base::app)) == 0)
        mode |= std::ios_base::trunc;
    m_pImpl->m_FileOpenMode = mode;
}

template< typename CharT >
void basic_text_file_backend< CharT >::set_file_collector(file_collector_type const& collector)
{
    m_pImpl->m_FileCollector = collector;
}

//! Explicitly instantiate sink backend implementation
#ifdef BOOST_LOG_USE_CHAR
template class BOOST_LOG_EXPORT basic_text_file_backend< char >;
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
template class BOOST_LOG_EXPORT basic_text_file_backend< wchar_t >;
#endif

} // namespace sinks

} // namespace log

} // namespace boost
