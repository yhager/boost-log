/*
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This header is the Boost.Log library implementation, see the library documentation
 * at http://www.boost.org/libs/log/doc/log.html.
 */
/*!
 * \file   severity_channel_logger.hpp
 * \author Andrey Semashev
 * \date   28.02.2008
 * 
 * The header contains implementation of a logger with severity level and channel support.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SOURCES_SEVERITY_CHANNEL_LOGGER_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_SEVERITY_CHANNEL_LOGGER_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/channel_logger.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sources {

#ifndef BOOST_LOG_DOXYGEN_PASS

#ifdef BOOST_LOG_USE_CHAR

//! Narrow-char logger with severity level and channel support
BOOST_LOG_DECLARE_LOGGER(severity_channel_logger, (basic_severity_logger)(basic_channel_logger));

#if !defined(BOOST_LOG_NO_THREADS)
//! Narrow-char thread-safe logger with severity level and channel support
BOOST_LOG_DECLARE_LOGGER_MT(severity_channel_logger_mt, (basic_severity_logger)(basic_channel_logger));
#endif

#endif

#ifdef BOOST_LOG_USE_WCHAR_T

//! Wide-char logger with severity level and channel support
BOOST_LOG_DECLARE_WLOGGER(wseverity_channel_logger, (basic_severity_logger)(basic_channel_logger));

#if !defined(BOOST_LOG_NO_THREADS)
//! Wide-char thread-safe logger with severity level and channel support
BOOST_LOG_DECLARE_WLOGGER_MT(wseverity_channel_logger_mt, (basic_severity_logger)(basic_channel_logger));
#endif

#endif

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief Narrow-char logger. Functionally equivalent to \c basic_severity_logger and \c basic_channel_logger.
 * 
 * See \c basic_severity_logger and \c basic_channel_logger class templates for a more detailed description
 */
class severity_channel_logger :
    public basic_severity_logger<
        basic_channel_logger<
            basic_logger< char, severity_channel_logger, single_thread_model >
        >
    >
{
public:
    /*!
     * Default constructor
     */
    severity_channel_logger();
    /*!
     * Copy constructor
     */
    severity_channel_logger(severity_channel_logger const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit severity_channel_logger(ArgsT... const& args);
    /*!
     * Assignment operator
     */
    severity_channel_logger& operator= (severity_channel_logger const& that)
    /*!
     * Swaps two loggers
     */
    void swap(severity_channel_logger& that);
};

/*!
 * \brief Narrow-char thread-safe logger. Functionally equivalent to \c basic_severity_logger and \c basic_channel_logger.
 * 
 * See \c basic_severity_logger and \c basic_channel_logger class templates for a more detailed description
 */
class severity_channel_logger_mt :
    public basic_severity_logger<
        basic_channel_logger<
            basic_logger< char, severity_channel_logger_mt, multi_thread_model >
        >
    >
{
public:
    /*!
     * Default constructor
     */
    severity_channel_logger_mt();
    /*!
     * Copy constructor
     */
    severity_channel_logger_mt(severity_channel_logger_mt const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit severity_channel_logger_mt(ArgsT... const& args);
    /*!
     * Assignment operator
     */
    severity_channel_logger_mt& operator= (severity_channel_logger_mt const& that)
    /*!
     * Swaps two loggers
     */
    void swap(severity_channel_logger_mt& that);
};

/*!
 * \brief Wide-char logger. Functionally equivalent to \c basic_severity_logger and \c basic_channel_logger.
 * 
 * See \c basic_severity_logger and \c basic_channel_logger class templates for a more detailed description
 */
class wseverity_channel_logger :
    public basic_severity_logger<
        basic_channel_logger<
            basic_logger< wchar_t, wseverity_channel_logger, single_thread_model >
        >
    >
{
public:
    /*!
     * Default constructor
     */
    wseverity_channel_logger();
    /*!
     * Copy constructor
     */
    wseverity_channel_logger(wseverity_channel_logger const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit wseverity_channel_logger(ArgsT... const& args);
    /*!
     * Assignment operator
     */
    wseverity_channel_logger& operator= (wseverity_channel_logger const& that)
    /*!
     * Swaps two loggers
     */
    void swap(wseverity_channel_logger& that);
};

/*!
 * \brief Wide-char thread-safe logger. Functionally equivalent to \c basic_severity_logger and \c basic_channel_logger.
 * 
 * See \c basic_severity_logger and \c basic_channel_logger class templates for a more detailed description
 */
class wseverity_channel_logger_mt :
    public basic_severity_logger<
        basic_channel_logger<
            basic_logger< wchar_t, wseverity_channel_logger_mt, multi_thread_model >
        >
    >
{
public:
    /*!
     * Default constructor
     */
    wseverity_channel_logger_mt();
    /*!
     * Copy constructor
     */
    wseverity_channel_logger_mt(wseverity_channel_logger_mt const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit wseverity_channel_logger_mt(ArgsT... const& args);
    /*!
     * Assignment operator
     */
    wseverity_channel_logger_mt& operator= (wseverity_channel_logger_mt const& that)
    /*!
     * Swaps two loggers
     */
    void swap(wseverity_channel_logger_mt& that);
};

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace sources

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SOURCES_SEVERITY_CHANNEL_LOGGER_HPP_INCLUDED_
