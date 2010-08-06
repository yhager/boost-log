/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   current_thread_id.hpp
 * \author Andrey Semashev
 * \date   12.09.2009
 *
 * The header contains implementation of a current thread id attribute
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTES_CURRENT_THREAD_ID_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_CURRENT_THREAD_ID_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

#if defined(BOOST_LOG_NO_THREADS)
#error Boost.Log: The current_thread_id attribute is only available in multithreaded builds
#endif

#include <boost/intrusive_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/log/attributes/attribute_factory.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/basic_attribute_value.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace attributes {

/*!
 * \brief A class of an attribute that always returns the current thread identifier
 *
 * \note This attribute can be registered globally, it will still return the correct
 *       thread identifier, no matter which thread emits the log record.
 */
class current_thread_id :
    public attribute_factory
{
public:
    //! A held attribute value type
    typedef thread::id value_type;

protected:
    //! Factory implementation
    class BOOST_LOG_VISIBLE impl :
        public attribute_factory::impl,
        public attribute_value::impl
    {
    public:
        bool dispatch(type_dispatcher& dispatcher)
        {
            type_dispatcher::callback< value_type > callback =
                dispatcher.get_callback< value_type >();
            if (callback)
            {
                callback(this_thread::get_id());
                return true;
            }
            else
                return false;
        }

        attribute_value get_value()
        {
            BOOST_LOG_ASSUME(this != NULL);
            return attribute_value(static_cast< attribute_value::impl* >(this));
        }

        intrusive_ptr< attribute_value::impl > detach_from_thread()
        {
            typedef basic_attribute_value< value_type > detached_value;
            return new detached_value(this_thread::get_id());
        }
    };

public:
    /*!
     * Default constructor
     */
    current_thread_id() : attribute_factory(new impl())
    {
    }
    /*!
     * Constructor for casting support
     */
    explicit current_thread_id(cast_source const& source) :
        attribute_factory(source.as< impl >())
    {
    }
};

} // namespace attributes

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTES_CURRENT_THREAD_ID_HPP_INCLUDED_
