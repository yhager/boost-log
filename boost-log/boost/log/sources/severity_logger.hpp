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

#ifndef BOOST_LOG_SEVERITY_LOGGER_HPP_INCLUDED_
#define BOOST_LOG_SEVERITY_LOGGER_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/empty_deleter.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/attributes/mutable_constant.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
#endif // _MSC_VER

namespace boost {

namespace log {

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

} // namespace aux

//! Logger class
template< typename CharT >
class basic_severity_logger :
    public basic_logger< CharT >
{
    //! Base type
    typedef basic_logger< CharT > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! Attribute set type
    typedef typename base_type::attribute_set attribute_set;
    //! Output stream type
    typedef typename base_type::ostream_type ostream_type;

    //! Severity attribute type
    typedef attributes::mutable_constant< int > severity_attribute;

protected:
    //! Record pump type
    typedef typename base_type::record_pump_type::BOOST_NESTED_TEMPLATE rebind<
        basic_severity_logger
    >::other record_pump_type;

private:
    //! Severity attribute
    severity_attribute m_Severity;
    //! Default severity
    severity_attribute::held_type m_DefaultSeverity;

public:
    //! Constructor
    basic_severity_logger(severity_attribute::held_type default_svty = 0)
        : m_Severity(default_svty), m_DefaultSeverity(default_svty)
    {
        base_type::add_attribute(
            aux::severity_attribute_name< char_type >::get(),
            shared_ptr< attribute >(&m_Severity, empty_deleter()));
    }

    //! Assignment
    basic_severity_logger& operator= (basic_severity_logger const& that)
    {
        base_type::operator= (static_cast< base_type const& >(that));
        m_Severity = that.m_Severity;
        m_DefaultSeverity = that.m_DefaultSeverity;
        return *this;
    }

    using base_type::open_record;

    //! The method allows to assign a severity to the opening record
    bool open_record(severity_attribute::held_type svty)
    {
        m_Severity.set_value(svty);
        const bool Result = base_type::open_record();
        if (!Result)
            m_Severity.set_value(m_DefaultSeverity);

        return Result;
    }
    //! The method pushes the constructed message to the sinks and closes the record
    void push_record()
    {
        base_type::push_record();
        m_Severity.set_value(m_DefaultSeverity);
    }

    //! Logging stream getter
    record_pump_type strm()
    {
        return record_pump_type(this);
    }

protected:
    //! Severity attribute accessor
    severity_attribute& severity() { return m_Severity; }
    //! Severity attribute accessor
    severity_attribute const& severity() const { return m_Severity; }
    //! Default severity value getter
    severity_attribute::held_type default_severity() const { return m_DefaultSeverity; }
};

typedef basic_severity_logger< char > severity_logger;
typedef basic_severity_logger< wchar_t > wseverity_logger;

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#define BOOST_LOG_SEV(logger, svty)\
    if (!logger.open_record(svty))\
        ((void)0);\
    else\
        logger.strm()

#endif // BOOST_LOG_SEVERITY_LOGGER_HPP_INCLUDED_