/*
 * (C) 2008 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This header is the Boost.Log library implementation, see the library documentation
 * at http://www.boost.org/libs/log/doc/log.html.
 */
/*!
 * \file   debug_output_backend.hpp
 * \author Andrey Semashev
 * \date   07.11.2008
 * 
 * The header contains a logging sink backend that outputs log records to the debugger.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_DEBUG_OUTPUT_BACKEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_DEBUG_OUTPUT_BACKEND_HPP_INCLUDED_

#include <string>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/filters/basic_filters.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sinks {

/*!
 * \brief An implementation of a logging sink backend that outputs to the debugger
 *
 * The sink uses Windows API in order to write log records as debug messages, if the
 * application process is run under debugger. The sink backend also provides a specific
 * filter that allows to check whether the debugger is available and thus elide unnecessary
 * formatting.
 */
template< typename CharT >
class BOOST_LOG_EXPORT basic_debug_output_backend :
    public basic_formatting_sink_backend< CharT >
{
    //! Base type
    typedef basic_formatting_sink_backend< CharT > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type to be used as a message text holder
    typedef typename base_type::target_string_type target_string_type;
    //! Attribute values view type
    typedef typename base_type::values_view_type values_view_type;

public:
    //! \cond
    //! A filter that checks whether the debugger is available
    class debugger_presence_filter :
        public filters::basic_filter< char_type, debugger_presence_filter >
    {
    public:
        BOOST_LOG_EXPORT bool operator() (values_view_type const& values) const;
    };
    //! \endcond

public:
    /*!
     * Constructor. Initializes the sink backend.
     */
	basic_debug_output_backend();
    /*!
     * Destructor
     */
    ~basic_debug_output_backend();

    /*!
     * \return A filter that checks whether the debugger is available
     */
    debugger_presence_filter get_debugger_presence_filter() const;

private:
    //! The method puts the formatted message to debugger
    void do_write_message(values_view_type const& values, target_string_type const& formatted_message);
};

#ifdef BOOST_LOG_USE_CHAR
typedef basic_debug_output_backend< char > debug_output_backend;      //!< Convenience typedef for narrow-character logging
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_debug_output_backend< wchar_t > wdebug_output_backend;  //!< Convenience typedef for wide-character logging
#endif

} // namespace sinks

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SINKS_DEBUG_OUTPUT_BACKEND_HPP_INCLUDED_
