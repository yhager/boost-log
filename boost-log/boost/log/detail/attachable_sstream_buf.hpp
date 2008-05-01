/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   attachable_sstream_buf.hpp
 * \author Andrey Semashev
 * \date   29.07.2007
 * 
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTACHABLE_SSTREAM_BUF_HPP_INCLUDED_
#define BOOST_LOG_ATTACHABLE_SSTREAM_BUF_HPP_INCLUDED_

#include <memory>
#include <string>
#include <streambuf>
#include <boost/log/detail/prologue.hpp>

#ifndef BOOST_LOG_DEFAULT_ATTACHABLE_SSTREAM_BUF_SIZE
//! The size (in chars) of a stream buffer used by logger. It affects logger object size.
//! \note The Boost.Log library should be rebuilt once this value is modified.
#define BOOST_LOG_DEFAULT_ATTACHABLE_SSTREAM_BUF_SIZE 16
#endif // BOOST_LOG_DEFAULT_ATTACHABLE_SSTREAM_BUF_SIZE

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
#endif // _MSC_VER

namespace boost {

namespace log {

namespace aux {

    //! A streambuf that puts the formatted data to an external string
    template<
        typename CharT,
        typename TraitsT = std::char_traits< CharT >,
        typename AllocatorT = std::allocator< CharT >
    >
    class basic_ostringstreambuf :
        public std::basic_streambuf< CharT, TraitsT >
    {
        //! Base type
        typedef std::basic_streambuf< CharT, TraitsT > base_type;

    public:
        //! Character type
        typedef typename base_type::char_type char_type;
        //! Traits type
        typedef typename base_type::traits_type traits_type;
        //! String type
        typedef std::basic_string< char_type, traits_type, AllocatorT > string_type;
        //! Int type
        typedef typename base_type::int_type int_type;

    private:
        //! A reference to the string that will be filled
        string_type& m_Storage;
        //! A buffer used to temporarily store output
        char_type m_Buffer[BOOST_LOG_DEFAULT_ATTACHABLE_SSTREAM_BUF_SIZE];

    private:
        //! Copy constructor (closed)
        basic_ostringstreambuf(basic_ostringstreambuf const& that);
        //! Assignment (closed)
        basic_ostringstreambuf& operator= (basic_ostringstreambuf const& that);

    public:
        //! Constructor
        explicit basic_ostringstreambuf(string_type& storage) : m_Storage(storage)
        {
            base_type::setp(m_Buffer, m_Buffer + (sizeof(m_Buffer) / sizeof(*m_Buffer)));
        }

    protected:
        //! Puts all buffered data to the string
        int sync()
        {
            register char_type* pBase = this->pbase();
            register char_type* pPtr = this->pptr();
            if (pBase != pPtr)
            {
                m_Storage.append(pBase, pPtr);
                this->pbump(static_cast< int >(pBase - pPtr));
            }
            return 0;
        }
        //! Puts an unbuffered character to the string
        int_type overflow(int_type c)
        {
            basic_ostringstreambuf::sync();
            if (!traits_type::eq_int_type(c, traits_type::eof()))
            {
                m_Storage.push_back(traits_type::to_char_type(c));
                return c;
            }
            else
                return traits_type::not_eof(c);
        }
        //! Puts a character sequence to the string
        std::streamsize xsputn(const char_type* s, std::streamsize n)
        {
            basic_ostringstreambuf::sync();
            typedef typename string_type::size_type string_size_type;
            register const string_size_type max_storage_left =
                m_Storage.max_size() - m_Storage.size();
            if (static_cast< string_size_type >(n) < max_storage_left)
            {
                m_Storage.append(s, static_cast< string_size_type >(n));
                return n;
            }
            else
            {
                m_Storage.append(s, max_storage_left);
                return static_cast< std::streamsize >(max_storage_left);
            }
        }
    };

} // namespace aux

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_ATTACHABLE_SSTREAM_BUF_HPP_INCLUDED_
