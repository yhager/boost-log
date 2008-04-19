/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   severity_logger.hpp
 * \author Andrey Semashev
 * \date   08.03.2007
 * 
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SOURCES_SEVERITY_LOGGER_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_SEVERITY_LOGGER_HPP_INCLUDED_

#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/empty_deleter.hpp>
#include <boost/thread/once.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/basic_attribute_value.hpp>
#include <boost/log/detail/thread_specific.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
#endif // _MSC_VER

namespace boost {

namespace log {

namespace sources {

namespace keywords {

    BOOST_PARAMETER_KEYWORD(tag, severity)

} // namespace keywords

namespace aux {

    //! A helper traits to get severity attribute name constant in the proper type
    template< typename >
    struct severity_attribute_name;
    template< >
    struct severity_attribute_name< char >
    {
        static const char* get() { return "Severity"; }
    };
    template< >
    struct severity_attribute_name< wchar_t >
    {
        static const wchar_t* get() { return L"Severity"; }
    };

    //! Severity level attribute implementation
    class severity_level :
        public attribute
    {
    public:
        typedef int held_type;

    public:
        //! Default constructor
        severity_level()
        {
            static once_flag flag = BOOST_ONCE_INIT;
            boost::call_once(flag, &severity_level::get_instance);
        }
        //! The method returns the actual attribute value. It must not return NULL.
        virtual shared_ptr< attribute_value > get_value()
        {
            return shared_ptr< attribute_value >(
                new attributes::basic_attribute_value< held_type >(get_instance().get()));
        }
        //! The method sets the actual level
        void set_value(held_type level)
        {
            get_instance().set(level);
        }

    private:
        static log::aux::thread_specific< held_type >& get_instance()
        {
            static log::aux::thread_specific< held_type > instance;
            return instance;
        }
    };

} // namespace aux

//! Logger class
template< typename BaseT >
class basic_severity_logger :
    public BaseT
{
    //! Base type
    typedef BaseT base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! Final type
    typedef typename base_type::final_type final_type;
    //! Attribute set type
    typedef typename base_type::attribute_set attribute_set;

    //! Severity attribute type
    typedef aux::severity_level severity_attribute;

private:
    //! Default severity
    severity_attribute::held_type m_DefaultSeverity;
    //! Severity attribute
    severity_attribute m_Severity;

public:
    //! Constructor
    basic_severity_logger() :
        base_type(),
        m_DefaultSeverity(0)
    {
        base_type::add_attribute_unlocked(
            aux::severity_attribute_name< char_type >::get(),
            shared_ptr< attribute >(&m_Severity, empty_deleter()));
    }
    //! Copy constructor
    basic_severity_logger(basic_severity_logger const& that) :
        base_type(static_cast< base_type const& >(that)),
        m_DefaultSeverity(that.m_DefaultSeverity)
    {
        base_type::add_attribute_unlocked(
            aux::severity_attribute_name< char_type >::get(),
            shared_ptr< attribute >(&m_Severity, empty_deleter()));
    }
    //! Constructor with arguments
    template< typename ArgsT >
    explicit basic_severity_logger(ArgsT const& args) :
        base_type(args),
        m_DefaultSeverity(args[keywords::severity | 0])
    {
        base_type::add_attribute_unlocked(
            aux::severity_attribute_name< char_type >::get(),
            shared_ptr< attribute >(&m_Severity, empty_deleter()));
    }

    //! The method opens a new logging record with the default severity
    bool open_record()
    {
        m_Severity.set_value(m_DefaultSeverity);
        return base_type::open_record();
    }

    //! The method allows to assign a severity to the opening record
    template< typename ArgsT >
    bool open_record(ArgsT const& args)
    {
        m_Severity.set_value(args[keywords::severity | m_DefaultSeverity]);
        return base_type::open_record();
    }

protected:
    //! Severity attribute accessor
    severity_attribute& severity() { return m_Severity; }
    //! Severity attribute accessor
    severity_attribute const& severity() const { return m_Severity; }
    //! Default severity value getter
    severity_attribute::held_type default_severity() const { return m_DefaultSeverity; }

    //! Unlocked swap
    void swap_unlocked(basic_severity_logger& that)
    {
        base_type::swap_unlocked(static_cast< base_type& >(that));
        std::swap(m_DefaultSeverity, that.m_DefaultSeverity);
    }

private:
    //! Assignment (should be implemented in the final type as copy and swap)
    basic_severity_logger& operator= (basic_severity_logger const&);
};

//! Narrow-char logger with severity level support
BOOST_LOG_DECLARE_LOGGER(severity_logger, (basic_severity_logger));

//! Wide-char logger with severity level support
BOOST_LOG_DECLARE_WLOGGER(wseverity_logger, (basic_severity_logger));

//! Narrow-char thread-safe logger with severity level support
BOOST_LOG_DECLARE_LOGGER_MT(severity_logger_mt, (basic_severity_logger));

//! Wide-char thraed-safe logger with severity level support
BOOST_LOG_DECLARE_WLOGGER_MT(wseverity_logger_mt, (basic_severity_logger));

} // namespace sources

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#define BOOST_LOG_SEV(logger, svty)\
    BOOST_LOG_WITH_PARAMS((logger), (::boost::log::sources::keywords::severity = (svty)))

#endif // BOOST_LOG_SOURCES_SEVERITY_LOGGER_HPP_INCLUDED_
