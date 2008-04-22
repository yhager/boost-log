/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   scoped_attribute.hpp
 * \author Andrey Semashev
 * \date   13.05.2007
 * 
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SCOPED_ATTRIBUTE_HPP_INCLUDED_
#define BOOST_LOG_SCOPED_ATTRIBUTE_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/empty_deleter.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/logging_core.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/attributes/attribute_set.hpp>

namespace boost {

namespace log {

namespace aux {

    //! A base class for all scoped attribute classes
    class attribute_scope_guard {};

} // namespace aux

typedef aux::attribute_scope_guard const& scoped_attribute;

namespace aux {

    //! A scoped logger attribute guard
    template< typename LoggerT >
    class scoped_logger_attribute :
        public attribute_scope_guard
    {
    private:
        //! Logger type
        typedef LoggerT logger_type;

    private:
        //! A reference to the logger
        mutable logger_type* m_pLogger;
        //! An iterator to the added attribute
        typename logger_type::attribute_set::iterator m_itAttribute;
        //! A saved attribute, if it was already registered
        mutable shared_ptr< attribute > m_pSavedAttribute;

    public:
        //! Constructor
        scoped_logger_attribute(
            logger_type& l,
            typename logger_type::string_type const& name,
            shared_ptr< attribute > const& attr
        ) :
            m_pLogger(boost::addressof(l))
        {
            std::pair<
                typename logger_type::attribute_set::iterator,
                bool
            > res = l.add_attribute(name, attr);
            m_itAttribute = res.first;
            if (!res.second)
            {
                m_pSavedAttribute = attr;
                m_pSavedAttribute.swap(m_itAttribute->second);
            }
        }
        //! Copy constructor (implemented as move)
        scoped_logger_attribute(scoped_logger_attribute const& that) :
            m_pLogger(that.m_pLogger),
            m_itAttribute(that.m_itAttribute)
        {
            that.m_pLogger = 0;
            m_pSavedAttribute.swap(that.m_pSavedAttribute);
        }

        //! Destructor
        ~scoped_logger_attribute()
        {
            if (m_pLogger)
            {
                if (!m_pSavedAttribute)
                    m_pLogger->remove_attribute(m_itAttribute);
                else
                    m_pSavedAttribute.swap(m_itAttribute->second);
            }
        }

    private:
        //! Assignment (closed)
        scoped_logger_attribute& operator= (scoped_logger_attribute const&);
    };

} // namespace aux

//  Generator helper functions
template< typename LoggerT >
inline aux::scoped_logger_attribute< LoggerT > add_scoped_logger_attribute(
    LoggerT& l, typename LoggerT::string_type const& name, shared_ptr< attribute > const& attr)
{
    return aux::scoped_logger_attribute< LoggerT >(l, name, attr);
}

template< typename LoggerT >
inline aux::scoped_logger_attribute< LoggerT > add_scoped_logger_attribute(
    LoggerT& l, typename LoggerT::char_type const* name, shared_ptr< attribute > const& attr)
{
    return aux::scoped_logger_attribute< LoggerT >(l, name, attr);
}

template< typename LoggerT, typename AttributeT >
inline aux::scoped_logger_attribute< LoggerT > add_scoped_logger_attribute(
    LoggerT& l, typename LoggerT::string_type const& name, AttributeT& attr)
{
    return aux::scoped_logger_attribute< LoggerT >(
        l, name, shared_ptr< attribute >(boost::addressof(attr), empty_deleter()));
}

template< typename LoggerT, typename AttributeT >
inline aux::scoped_logger_attribute< LoggerT > add_scoped_logger_attribute(
    LoggerT& l, typename LoggerT::char_type const* name, AttributeT& attr)
{
    return aux::scoped_logger_attribute< LoggerT >(
        l, name, shared_ptr< attribute >(boost::addressof(attr), empty_deleter()));
}

namespace aux {

    //! A scoped thread-specific attribute guard
    template< typename CharT >
    class scoped_thread_attribute :
        public attribute_scope_guard
    {
    private:
        //! Logging core type
        typedef basic_logging_core< CharT > logging_core_type;

    private:
        //! A pointer to the logging core
        mutable shared_ptr< logging_core_type > m_pCore;
        //! An iterator to the added attribute
        typename logging_core_type::attribute_set::iterator m_itAttribute;
        //! A saved attribute, if it was already registered
        mutable shared_ptr< attribute > m_pSavedAttribute;

    public:
        //! Constructor
        scoped_thread_attribute(
            typename logging_core_type::string_type const& name, shared_ptr< attribute > const& attr) :
            m_pCore(logging_core_type::get())
        {
            std::pair<
                typename logging_core_type::attribute_set::iterator,
                bool
            > res = m_pCore->add_thread_attribute(name, attr);
            m_itAttribute = res.first;
            if (!res.second)
            {
                m_pSavedAttribute = attr;
                m_pSavedAttribute.swap(m_itAttribute->second);
            }
        }
        //! Copy constructor (implemented as move)
        scoped_thread_attribute(scoped_thread_attribute const& that) : m_itAttribute(that.m_itAttribute)
        {
            m_pCore.swap(that.m_pCore);
            m_pSavedAttribute.swap(that.m_pSavedAttribute);
        }

        //! Destructor
        ~scoped_thread_attribute()
        {
            if (m_pCore)
            {
                if (!m_pSavedAttribute)
                    m_pCore->remove_thread_attribute(m_itAttribute);
                else
                    m_pSavedAttribute.swap(m_itAttribute->second);
            }
        }
    };

} // namespace aux

//  Generator helper functions
template< typename CharT >
inline aux::scoped_thread_attribute< CharT > add_scoped_thread_attribute(
    std::basic_string< CharT > const& name, shared_ptr< attribute > const& attr)
{
    return aux::scoped_thread_attribute< CharT >(name, attr);
}

template< typename CharT >
inline aux::scoped_thread_attribute< CharT > add_scoped_thread_attribute(
    const CharT* name, shared_ptr< attribute > const& attr)
{
    return aux::scoped_thread_attribute< CharT >(name, attr);
}

template< typename CharT, typename AttributeT >
inline aux::scoped_thread_attribute< CharT > add_scoped_thread_attribute(
    std::basic_string< CharT > const& name, AttributeT& attr)
{
    return aux::scoped_thread_attribute< CharT >(
        name, shared_ptr< attribute >(boost::addressof(attr), empty_deleter()));
}

template< typename CharT, typename AttributeT >
inline aux::scoped_thread_attribute< CharT > add_scoped_thread_attribute(
    const CharT* name, AttributeT& attr)
{
    return aux::scoped_thread_attribute< CharT >(
        name, shared_ptr< attribute >(boost::addressof(attr), empty_deleter()));
}

} // namespace log

} // namespace boost

#endif // BOOST_LOG_SCOPED_ATTRIBUTE_HPP_INCLUDED_