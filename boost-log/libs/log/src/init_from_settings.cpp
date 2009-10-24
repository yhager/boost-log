/*!
 * (C) 2009 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * \file   init_from_settings.cpp
 * \author Andrey Semashev
 * \date   11.10.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#ifndef BOOST_LOG_NO_SETTINGS_PARSERS_SUPPORT

#if defined(_MSC_VER)
// 'const int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4800)
#endif

#include <map>
#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <typeinfo>
#include <stdexcept>
#include <algorithm>

#if !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_SPIRIT_THREADSAFE)
#define BOOST_SPIRIT_THREADSAFE
#endif // !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_SPIRIT_THREADSAFE)

#include "windows_version.hpp"
#include <boost/any.hpp>
#include <boost/bind.hpp>
#include <boost/limits.hpp>
#include <boost/cstdint.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_unsigned.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/log/detail/code_conversion.hpp>
#include <boost/log/detail/throw_exception.hpp>
#include <boost/log/detail/singleton.hpp>
#include <boost/log/detail/universal_path.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sinks.hpp>
#if defined(BOOST_WINDOWS) && defined(BOOST_LOG_USE_WINNT6_API)
#include <boost/log/sinks/nt6_event_log_backend.hpp>
#endif // BOOST_WINDOWS && BOOST_LOG_USE_WINNT6_API
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/log/utility/init/from_settings.hpp>
#include <boost/log/utility/init/filter_parser.hpp>
#include <boost/log/utility/init/formatter_parser.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/thread/locks.hpp>
#include <boost/log/detail/light_rw_mutex.hpp>
#include <boost/log/detail/shared_lock_guard.hpp>
#endif
#include "parser_utils.hpp"

#if defined(BOOST_WINDOWS) && defined(BOOST_LOG_USE_WINNT6_API)
#include <guiddef.h>
#endif // defined(BOOST_WINDOWS) && defined(BOOST_LOG_USE_WINNT6_API)

namespace bsc = boost::spirit::classic;

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace {

    //! Throws an exception when a parameter type is not valid
    template< typename CharT >
    void BOOST_LOG_NORETURN throw_invalid_type(const CharT* param_name, log::type_info_wrapper const& param_type)
    {
        std::string name = log::aux::to_narrow(param_name);
        boost::log::aux::throw_exception(std::invalid_argument(
            "Invalid parameter \"" + name + "\" type: " + param_type.pretty_name()));
    }

    //! Throws an exception when a parameter value is not valid
    template< typename CharT >
    void BOOST_LOG_NORETURN throw_invalid_value(const CharT* param_name)
    {
        std::string name = log::aux::to_narrow(param_name);
        boost::log::aux::throw_exception(std::invalid_argument(
            "Invalid parameter \"" + name + "\" value"));
    }

    //! Extracts a filesystem path from any
    template< typename CharT >
    inline log::aux::universal_path any_cast_to_path(const CharT* param_name, any const& val)
    {
        std::type_info const& type = val.type();
        if (type == typeid(filesystem::path))
            return log::aux::to_universal_path(any_cast< filesystem::path >(val));
        else if (type == typeid(std::string))
            return log::aux::to_universal_path(any_cast< std::string >(val));
#if !defined(BOOST_FILESYSTEM_NARROW_ONLY)
        else if (type == typeid(filesystem::wpath))
            return log::aux::to_universal_path(any_cast< filesystem::wpath >(val));
        else if (type == typeid(std::wstring))
            return log::aux::to_universal_path(any_cast< std::wstring >(val));
#endif //!defined(BOOST_FILESYSTEM_NARROW_ONLY)
        else
            throw_invalid_type(param_name, type);
    }
    //! Extracts an integral value from any
    template< typename IntT, typename CharT >
    inline IntT any_cast_to_int(const CharT* param_name, any const& val)
    {
        typedef std::basic_string< CharT > string_type;
        std::type_info const& type = val.type();
        if (type == typeid(short))
            return static_cast< IntT >(any_cast< short >(val));
        else if (type == typeid(unsigned short))
            return static_cast< IntT >(any_cast< unsigned short >(val));
        else if (type == typeid(int))
            return static_cast< IntT >(any_cast< int >(val));
        else if (type == typeid(unsigned int))
            return static_cast< IntT >(any_cast< unsigned int >(val));
        else if (type == typeid(long))
            return static_cast< IntT >(any_cast< long >(val));
        else if (type == typeid(unsigned long))
            return static_cast< IntT >(any_cast< unsigned long >(val));
#if !defined(BOOST_NO_INT64_T)
        else if (type == typeid(long long))
            return static_cast< IntT >(any_cast< long long >(val));
        else if (type == typeid(unsigned long long))
            return static_cast< IntT >(any_cast< unsigned long long >(val));
#endif // !defined(BOOST_NO_INT64_T)
        else if (type == typeid(string_type))
        {
            string_type value = any_cast< string_type >(val);
            IntT res = 0;
            typedef typename mpl::if_<
                is_unsigned< IntT >,
                bsc::uint_parser< IntT >,
                bsc::int_parser< IntT >
            >::type int_parser_t;
            int_parser_t int_p;
            if (bsc::parse(value.begin(), value.end(), int_p[bsc::assign_a(res)]).full)
                return res;
            else
                throw_invalid_value(param_name);
        }
        else
            throw_invalid_type(param_name, type);
    }

    //! Extracts a boolean value from any
    template< typename CharT >
    inline bool any_cast_to_bool(const CharT* param_name, any const& val)
    {
        typedef std::basic_string< CharT > string_type;
        typedef log::aux::char_constants< CharT > char_constants;
        std::type_info const& type = val.type();
        if (type == typeid(bool))
            return any_cast< bool >(val);
        else if (type == typeid(string_type))
        {
            string_type value = any_cast< string_type >(val);
            bool res = false;
            if (bsc::parse(value.begin(), value.end(), (
                bsc::int_p[bsc::assign_a(res)] ||
                bsc::as_lower_d[ bsc::str_p(char_constants::true_keyword()) ][bsc::assign_a(res, true)] ||
                bsc::as_lower_d[ bsc::str_p(char_constants::false_keyword()) ][bsc::assign_a(res, false)]
            )).full)
            {
                return res;
            }
            else
                throw_invalid_value(param_name);
        }
        else
            throw_invalid_type(param_name, type);
    }

    //! Extracts a filter from any
    template< typename CharT >
    inline function1<
        bool,
        basic_attribute_values_view< CharT > const&
    > any_cast_to_filter(const CharT* param_name, any const& val)
    {
        typedef std::basic_string< CharT > string_type;
        typedef basic_attribute_values_view< CharT > values_view_type;
        typedef function1< bool, values_view_type const& > filter_type1;
        typedef function< bool (values_view_type const&) > filter_type2;

        std::type_info const& type = val.type();
        if (type == typeid(string_type))
            return parse_filter(any_cast< string_type >(val));
        else if (type == typeid(filter_type1))
            return any_cast< filter_type1 >(val);
        else if (type == typeid(filter_type2))
            return any_cast< filter_type2 >(val);
        else
            throw_invalid_type(param_name, type);
    }

    //! Extracts a formatter from any
    template< typename CharT >
    inline function2<
        void,
        std::basic_ostream< CharT >&,
        basic_record< CharT > const&
    > any_cast_to_formatter(const CharT* param_name, any const& val)
    {
        typedef std::basic_string< CharT > string_type;
        typedef std::basic_ostream< CharT > stream_type;
        typedef basic_record< CharT > record_type;
        typedef function2< void, stream_type&, record_type const& > formatter_type1;
        typedef function< void (stream_type&, record_type const&) > formatter_type2;

        std::type_info const& type = val.type();
        if (type == typeid(string_type))
            return parse_formatter(any_cast< string_type >(val));
        else if (type == typeid(formatter_type1))
            return any_cast< formatter_type1 >(val);
        else if (type == typeid(formatter_type2))
            return any_cast< formatter_type2 >(val);
        else
            throw_invalid_type(param_name, type);
    }

    //! Extracts a formatter from any
    template< typename CharT >
    inline std::string any_cast_to_address(const CharT* param_name, any const& val)
    {
        typedef std::basic_string< CharT > string_type;

        std::type_info const& type = val.type();
        if (type == typeid(asio::ip::address))
            return any_cast< asio::ip::address >(val).to_string();
        else if (type == typeid(asio::ip::address_v4))
            return any_cast< asio::ip::address_v4 >(val).to_string();
        else if (type == typeid(asio::ip::address_v6))
            return any_cast< asio::ip::address_v6 >(val).to_string();
        else if (type == typeid(string_type))
            return log::aux::to_narrow(any_cast< string_type >(val));
        else
            throw_invalid_type(param_name, type);
    }

#if defined(BOOST_WINDOWS) && defined(BOOST_LOG_USE_WINNT6_API)

    //! The function parses GUIDS from strings in form "{631EF147-9A84-4e33-8ADD-BCE174C4DBD9}"
    template< typename CharT >
    GUID any_cast_to_guid(const CharT* param_name, any const& val)
    {
        typedef CharT char_type;
        typedef std::basic_string< char_type > string_type;

        std::type_info const& type = val.type();
        if (type == typeid(GUID))
            return any_cast< GUID >(val);
        else if (type == typeid(string_type))
        {
            const char_type
                open_bracket = static_cast< char_type >('{'),
                close_bracket = static_cast< char_type >('}'),
                dash = static_cast< char_type >('-');

            const bsc::uint_parser< unsigned char, 16, 2, 2 > octet_p;
            const bsc::uint_parser< unsigned short, 16, 4, 4 > word_p;
            const bsc::uint_parser< unsigned long, 16, 8, 8 > dword_p;

            GUID g;
            string_type str = any_cast< string_type >(val);
            bsc::parse_info< const char_type* > result =
                bsc::parse(
                    str.c_str(),
                    str.c_str() + str.size(),
                    (
                        bsc::ch_p(open_bracket) >>
                        dword_p[bsc::assign_a(g.Data1)] >>
                        bsc::ch_p(dash) >>
                        word_p[bsc::assign_a(g.Data2)] >>
                        bsc::ch_p(dash) >>
                        word_p[bsc::assign_a(g.Data3)] >>
                        bsc::ch_p(dash) >>
                        octet_p[bsc::assign_a(g.Data4[0])] >>
                        octet_p[bsc::assign_a(g.Data4[1])] >>
                        bsc::ch_p(dash) >>
                        octet_p[bsc::assign_a(g.Data4[2])] >>
                        octet_p[bsc::assign_a(g.Data4[3])] >>
                        octet_p[bsc::assign_a(g.Data4[4])] >>
                        octet_p[bsc::assign_a(g.Data4[5])] >>
                        octet_p[bsc::assign_a(g.Data4[6])] >>
                        octet_p[bsc::assign_a(g.Data4[7])] >>
                        bsc::ch_p(close_bracket)
                    )
                );

            if (!result.full)
            {
                boost::log::aux::throw_exception(std::invalid_argument("Could not recognize CLSID from string "
                    + log::aux::to_narrow(str)));
            }

            return g;
        }
        else
            throw_invalid_type(param_name, type);
    }

#endif // defined(BOOST_WINDOWS) && defined(BOOST_LOG_USE_WINNT6_API)


//! The supported sinks repository
template< typename CharT >
struct sinks_repository :
    public log::aux::lazy_singleton< sinks_repository< CharT > >
{
    typedef log::aux::lazy_singleton< sinks_repository< CharT > > base_type;

#if !defined(BOOST_LOG_BROKEN_FRIEND_TEMPLATE_INSTANTIATIONS)
    friend class log::aux::lazy_singleton< sinks_repository< CharT > >;
#else
    friend class base_type;
#endif

    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef boost::log::aux::char_constants< char_type > constants;
    typedef std::map< string_type, any > params_t;
    typedef function1< shared_ptr< sinks::sink< char_type > >, params_t const& > sink_factory;
    typedef std::map< string_type, sink_factory > sink_factories;

#if !defined(BOOST_LOG_NO_THREADS)
    //! Synchronization mutex
    log::aux::light_rw_mutex m_Mutex;
#endif
    //! Map of the sink factories
    sink_factories m_Factories;

    //! The function constructs a sink from the settings
    shared_ptr< sinks::sink< char_type > > construct_sink_from_settings(params_t const& params)
    {
        typename params_t::const_iterator dest = params.find(constants::sink_destination_param_name());
        if (dest != params.end() && dest->second.type() == typeid(typename sink_factories::key_type))
        {
            typename sink_factories::key_type dest_name =
                boost::any_cast< typename sink_factories::key_type >(dest->second);
            BOOST_LOG_EXPR_IF_MT(log::aux::shared_lock_guard< log::aux::light_rw_mutex > _(m_Mutex);)
            typename sink_factories::const_iterator it = m_Factories.find(dest_name);
            if (it != m_Factories.end())
            {
                return it->second(params);
            }
            else
            {
                boost::log::aux::throw_exception(std::runtime_error("The sink destination is not supported"));
            }
        }
        else
        {
            boost::log::aux::throw_exception(std::runtime_error("The sink destination is not set"));
        }
    }

    static void init_instance()
    {
        sinks_repository& instance = base_type::get_instance();
        instance.m_Factories[constants::text_file_destination()] =
            &sinks_repository< char_type >::default_text_file_sink_factory;
        instance.m_Factories[constants::console_destination()] =
            &sinks_repository< char_type >::default_console_sink_factory;
        instance.m_Factories[constants::syslog_destination()] =
            &sinks_repository< char_type >::default_syslog_sink_factory;
#ifdef BOOST_WINDOWS
        instance.m_Factories[constants::debugger_destination()] =
            &sinks_repository< char_type >::default_debugger_sink_factory;
        instance.m_Factories[constants::simple_event_log_destination()] =
            &sinks_repository< char_type >::default_simple_event_log_sink_factory;
#ifdef BOOST_LOG_USE_WINNT6_API
        instance.m_Factories[constants::simple_nt6_event_log_destination()] =
            &sinks_repository< char_type >::default_simple_nt6_event_log_sink_factory;
#endif // BOOST_LOG_USE_WINNT6_API
#endif // BOOST_WINDOWS
    }

private:
    sinks_repository() {}

    //! The function constructs a sink that writes log records to a text file
    static shared_ptr< sinks::sink< char_type > > default_text_file_sink_factory(params_t const& params)
    {
        typedef sinks::basic_text_file_backend< char_type > backend_t;
        typedef typename backend_t::path_type path_type;
        shared_ptr< backend_t > backend = boost::make_shared< backend_t >();

        // FileName
        typename params_t::const_iterator it = params.find(constants::file_name_param_name());
        if (it != params.end() && !it->second.empty())
        {
            backend->set_file_name_pattern(
                any_cast_to_path(constants::file_name_param_name(), it->second));
        }
        else
            boost::log::aux::throw_exception(std::runtime_error("File name is not specified"));

        // File rotation size
        it = params.find(constants::rotation_size_param_name());
        if (it != params.end() && !it->second.empty())
        {
            backend->set_rotation_size(
                any_cast_to_int< uintmax_t >(constants::rotation_size_param_name(), it->second));
        }

        // File rotation interval
        it = params.find(constants::rotation_interval_param_name());
        if (it != params.end() && !it->second.empty())
        {
            backend->set_rotation_interval(posix_time::seconds(
                any_cast_to_int< unsigned int >(constants::rotation_interval_param_name(), it->second)));
        }

        // Auto flush
        it = params.find(constants::auto_flush_param_name());
        if (it != params.end() && !it->second.empty())
        {
            backend->auto_flush(
                any_cast_to_bool(constants::auto_flush_param_name(), it->second));
        }

        // File collector parameters
        // Target directory
        it = params.find(constants::target_param_name());
        if (it != params.end() && !it->second.empty())
        {
            path_type target_dir = any_cast_to_path(constants::target_param_name(), it->second);

            // Max total size
            uintmax_t max_size = (std::numeric_limits< uintmax_t >::max)();
            it = params.find(constants::max_size_param_name());
            if (it != params.end() && !it->second.empty())
                max_size = any_cast_to_int< uintmax_t >(constants::max_size_param_name(), it->second);

            // Min free space
            uintmax_t space = 0;
            it = params.find(constants::min_free_space_param_name());
            if (it != params.end() && !it->second.empty())
                space = any_cast_to_int< uintmax_t >(constants::min_free_space_param_name(), it->second);

            backend->set_file_collector(sinks::file::make_collector(
                keywords::target = target_dir,
                keywords::max_size = max_size,
                keywords::min_free_space = space));

            // Scan for log files
            it = params.find(constants::scan_for_files_param_name());
            if (it != params.end() && !it->second.empty())
            {
                if (it->second.type() == typeid(sinks::file::scan_method))
                    backend->scan_for_files(any_cast< sinks::file::scan_method >(it->second));
                else if (it->second.type() == typeid(string_type))
                {
                    string_type value = any_cast< string_type >(it->second);
                    if (value == constants::scan_method_all())
                        backend->scan_for_files(sinks::file::scan_all);
                    else if (value == constants::scan_method_matching())
                        backend->scan_for_files(sinks::file::scan_matching);
                    else
                    {
                        boost::log::aux::throw_exception(std::invalid_argument(
                            "File scan method \"" + boost::log::aux::to_narrow(value) + "\" is not supported"));
                    }
                }
                else
                    throw_invalid_type(constants::scan_for_files_param_name(), it->second.type());
            }
        }

        return init_sink(backend, params);
    }

    //! The function constructs a sink that writes log records to the console
    static shared_ptr< sinks::sink< char_type > > default_console_sink_factory(params_t const& params)
    {
        // Construct the backend
        typedef sinks::basic_text_ostream_backend< char_type > backend_t;
        shared_ptr< backend_t > backend = boost::make_shared< backend_t >();
        backend->add_stream(
            shared_ptr< typename backend_t::stream_type >(&constants::get_console_log_stream(), empty_deleter()));

        return init_text_ostream_sink(backend, params);
    }

    //! The function constructs a sink that writes log records to the syslog service
    static shared_ptr< sinks::sink< char_type > > default_syslog_sink_factory(params_t const& params)
    {
        // Construct the backend
        typedef sinks::basic_syslog_backend< char_type > backend_t;
        shared_ptr< backend_t > backend = boost::make_shared< backend_t >();

        // For now we use only the default level mapping. Will add support for configuration later.
        backend->set_severity_mapper(
            sinks::syslog::basic_direct_severity_mapping< char_type >(constants::default_level_attribute_name()));

        // Setup local and remote addresses
        typename params_t::const_iterator it = params.find(constants::local_address_param_name());
        if (it != params.end() && !it->second.empty())
            backend->set_local_address(any_cast_to_address(constants::local_address_param_name(), it->second));

        it = params.find(constants::target_address_param_name());
        if (it != params.end() && !it->second.empty())
            backend->set_target_address(any_cast_to_address(constants::local_address_param_name(), it->second));

        return init_sink(backend, params);
    }

#ifdef BOOST_WINDOWS

    //! The function constructs a sink that writes log records to the debugger
    static shared_ptr< sinks::sink< char_type > > default_debugger_sink_factory(params_t const& params)
    {
        // Construct the backend
        typedef sinks::basic_debug_output_backend< char_type > backend_t;
        shared_ptr< backend_t > backend = boost::make_shared< backend_t >();

        return init_sink(backend, params);
    }

    //! The function constructs a sink that writes log records to the Windows NT Event Log
    static shared_ptr< sinks::sink< char_type > > default_simple_event_log_sink_factory(params_t const& params)
    {
        typedef sinks::basic_simple_event_log_backend< char_type > backend_t;

        // Determine the log name
        string_type log_name = backend_t::get_default_log_name();
        typename params_t::const_iterator it = params.find(constants::log_name_param_name());
        if (it != params.end() && !it->second.empty())
        {
            if (it->second.type() == typeid(string_type))
                log_name = any_cast< string_type >(it->second);
            else
                throw_invalid_type(constants::log_name_param_name(), it->second.type());
        }

        // Determine the log source name
        string_type source_name = backend_t::get_default_source_name();
        it = params.find(constants::source_name_param_name());
        if (it != params.end() && !it->second.empty())
        {
            if (it->second.type() == typeid(string_type))
                source_name = any_cast< string_type >(it->second);
            else
                throw_invalid_type(constants::source_name_param_name(), it->second.type());
        }

        // Determine the force flag
        sinks::event_log::registration_mode reg_mode = sinks::event_log::on_demand;
        it = params.find(constants::registration_param_name());
        if (it != params.end() && !it->second.empty())
        {
            if (it->second.type() == typeid(sinks::event_log::registration_mode))
                reg_mode = any_cast< sinks::event_log::registration_mode >(it->second);
            else if (it->second.type() == typeid(string_type))
            {
                string_type value = any_cast< string_type >(it->second);
                if (value == constants::registration_never())
                    reg_mode = sinks::event_log::never;
                else if (value == constants::registration_on_demand())
                    reg_mode = sinks::event_log::on_demand;
                else if (value == constants::registration_forced())
                    reg_mode = sinks::event_log::forced;
                else
                {
                    boost::log::aux::throw_exception(std::invalid_argument(
                        "The registration mode \"" + log::aux::to_narrow(value) + "\" is not supported"));
                }
            }
            else
                throw_invalid_type(constants::registration_param_name(), it->second.type());
        }

        // Construct the backend
        shared_ptr< backend_t > backend(boost::make_shared< backend_t >((
            keywords::log_name = log_name,
            keywords::log_source = source_name,
            keywords::registration = reg_mode)));

        // For now we use only the default event type mapping. Will add support for configuration later.
        backend->set_event_type_mapper(
            sinks::event_log::basic_direct_event_type_mapping< char_type >(constants::default_level_attribute_name()));

        return init_sink(backend, params);
    }

#ifdef BOOST_LOG_USE_WINNT6_API

    //! The function constructs a sink that writes log records to the Windows NT 6 Event Log
    static shared_ptr< sinks::sink< char_type > > default_simple_nt6_event_log_sink_factory(params_t const& params)
    {
        typedef experimental::sinks::basic_simple_nt6_event_log_backend< char_type > backend_t;

        // Determine the provider GUID
        GUID provider_id = backend_t::get_default_provider_id();
        typename params_t::const_iterator it = params.find(constants::provider_id_param_name());
        if (it != params.end() && !it->second.empty())
            provider_id = any_cast_to_guid(constants::provider_id_param_name(), it->second);

        // Construct the backend
        shared_ptr< backend_t > backend(boost::make_shared< backend_t >(boost::cref(provider_id)));

        // For now we use only the default level mapping. Will add support for configuration later.
        backend->set_severity_mapper(
            experimental::sinks::etw::basic_direct_severity_mapping< char_type >(constants::default_level_attribute_name()));

        return init_sink(backend, params);
    }

#endif // BOOST_LOG_USE_WINNT6_API
#endif // BOOST_WINDOWS

    //! The function initializes common parameters of text stream sink and returns the constructed sink
    static shared_ptr< sinks::sink< char_type > > init_text_ostream_sink(
        shared_ptr< sinks::basic_text_ostream_backend< char_type > > const& backend, params_t const& params)
    {
        typedef sinks::basic_text_ostream_backend< char_type > backend_t;

        // AutoFlush
        typename params_t::const_iterator it = params.find(constants::auto_flush_param_name());
        if (it != params.end() && !it->second.empty())
            backend->auto_flush(any_cast_to_bool(constants::auto_flush_param_name(), it->second));

        return init_sink(backend, params);
    }

    //! The function initializes common parameters of a formatting sink and returns the constructed sink
    template< typename BackendT >
    static shared_ptr< sinks::sink< char_type > > init_sink(shared_ptr< BackendT > const& backend, params_t const& params)
    {
        typedef BackendT backend_t;

        // Filter
        typedef typename sinks::sink< char_type >::filter_type filter_type;
        filter_type filt;
        typename params_t::const_iterator it = params.find(constants::filter_param_name());
        if (it != params.end() && !it->second.empty())
        {
            filt = any_cast_to_filter(constants::filter_param_name(), it->second);
        }

        // Formatter
        it = params.find(constants::format_param_name());
        if (it != params.end() && !it->second.empty())
        {
            backend->set_formatter(any_cast_to_formatter(constants::format_param_name(), it->second));
        }

        shared_ptr< sinks::basic_sink_frontend< char_type > > p;

#if !defined(BOOST_LOG_NO_THREADS)
        // Asynchronous. TODO: make it more flexible.
        bool async = false;
        it = params.find(constants::asynchronous_param_name());
        if (it != params.end() && !it->second.empty())
        {
            async = any_cast_to_bool(constants::asynchronous_param_name(), it->second);
        }

        // Construct the frontend, considering Asynchronous parameter
        if (!async)
            p = boost::make_shared< sinks::synchronous_sink< backend_t > >(backend);
        else
            p = boost::make_shared< sinks::asynchronous_sink< backend_t > >(backend);
#else
        // When multithreading is disabled we always use the unlocked sink frontend
        p = boost::make_shared< sinks::unlocked_sink< backend_t > >(backend);
#endif

        p->set_filter(filt);

        return p;
    }
};

//! The function applies the settings to the logging core
template< typename CharT >
void apply_core_settings(std::map< std::basic_string< CharT >, any > const& params)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::map< string_type, any > params_t;
    typedef aux::char_constants< char_type > constants;
    typedef basic_core< char_type > core_t;
    shared_ptr< core_t > core = core_t::get();

    // Filter
    typename params_t::const_iterator it = params.find(constants::filter_param_name());
    if (it != params.end() && !it->second.empty())
        core->set_filter(any_cast_to_filter(constants::filter_param_name(), it->second));
    else
        core->reset_filter();

    // DisableLogging
    it = params.find(constants::core_disable_logging_param_name());
    if (it != params.end() && !it->second.empty())
        core->set_logging_enabled(!any_cast_to_bool(constants::core_disable_logging_param_name(), it->second));
    else
        core->set_logging_enabled(true);
}

} // namespace


//! The function initializes the logging library from a settings container
template< typename CharT >
void init_from_settings(basic_settings< CharT > const& setts)
{
    typedef basic_settings< CharT > settings_type;
    typedef typename settings_type::char_type char_type;
    typedef typename settings_type::string_type string_type;
    typedef basic_core< char_type > core_t;
    typedef sinks_repository< char_type > sinks_repo_t;
    typedef boost::log::aux::char_constants< char_type > constants;

    // Apply core settings
    typename settings_type::sections_type const& sections = setts.sections();
    typename settings_type::sections_type::const_iterator it =
        sections.find(constants::core_section_name());
    if (it != sections.end())
        apply_core_settings(it->second);

    // Construct and initialize sinks
    sinks_repo_t& sinks_repo = sinks_repo_t::get();
    string_type sink_prefix = constants::sink_section_name_prefix();
    std::vector< shared_ptr< sinks::sink< char_type > > > new_sinks;
    for (it = setts.sections().begin(); it != setts.sections().end(); ++it)
    {
        if (it->first.compare(0, sink_prefix.size(), sink_prefix) == 0)
            new_sinks.push_back(sinks_repo.construct_sink_from_settings(it->second));
    }
    std::for_each(new_sinks.begin(), new_sinks.end(), bind(&core_t::add_sink, core_t::get(), _1));
}


//! The function registers a factory for a sink
template< typename CharT >
void register_sink_factory(
    const CharT* sink_name,
    function1<
        shared_ptr< sinks::sink< CharT > >,
        std::map< std::basic_string< CharT >, any > const&
    > const& factory)
{
    sinks_repository< CharT >& repo = sinks_repository< CharT >::get();
    BOOST_LOG_EXPR_IF_MT(lock_guard< log::aux::light_rw_mutex > _(repo.m_Mutex);)
    repo.m_Factories[sink_name] = factory;
}

#ifdef BOOST_LOG_USE_CHAR
template BOOST_LOG_SETUP_EXPORT
void register_sink_factory< char >(
    const char* sink_name,
    function1<
        shared_ptr< sinks::sink< char > >,
        std::map< std::basic_string< char >, any > const&
    > const& factory);
template BOOST_LOG_SETUP_EXPORT void init_from_settings< char >(basic_settings< char > const& setts);
#endif

#ifdef BOOST_LOG_USE_WCHAR_T
template BOOST_LOG_SETUP_EXPORT
void register_sink_factory< wchar_t >(
    const wchar_t* sink_name,
    function1<
        shared_ptr< sinks::sink< wchar_t > >,
        std::map< std::basic_string< wchar_t >, any > const&
    > const& factory);
template BOOST_LOG_SETUP_EXPORT void init_from_settings< wchar_t >(basic_settings< wchar_t > const& setts);
#endif

} // namespace log

} // namespace boost

#endif // BOOST_LOG_NO_SETTINGS_PARSERS_SUPPORT