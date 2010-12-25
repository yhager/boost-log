/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   mutable_constant.hpp
 * \author Andrey Semashev
 * \date   06.11.2007
 *
 * The header contains implementation of a mutable constant attribute.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTES_MUTABLE_CONSTANT_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_MUTABLE_CONSTANT_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/locks.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/basic_attribute_value.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace attributes {

/*!
 * \brief A class of an attribute that holds a single constant value with ability to change it
 *
 * The mutable_constant attribute stores a single value of type, specified as the first template argument.
 * A copy of this value is returned on each attribute value acquisition.
 *
 * The attribute also allows to modify the stored value, even if the attibute is registered in an attribute set.
 * In order to ensure thread safety of such modifications the \c mutable_constant class is also parametrized
 * with three additional template arguments: mutex type, scoped write and scoped read lock types. The implementation
 * may avoid using these types to actually create and use the mutex, if a more efficient synchronization method is
 * available (such as atomic operations on the value type). By default no synchronization is done.
 */
template<
    typename T,
    typename MutexT = void,
    typename ScopedWriteLockT =
#ifndef BOOST_LOG_NO_THREADS
        typename mpl::if_c<
            boost::log::aux::is_exclusively_lockable< MutexT >::value,
            boost::log::aux::exclusive_lock_guard< MutexT >,
            void
        >::type,
#else
        void,
#endif
    typename ScopedReadLockT =
#ifndef BOOST_LOG_NO_THREADS
        typename mpl::if_c<
            boost::log::aux::is_shared_lockable< MutexT >::value,
            boost::log::aux::shared_lock_guard< MutexT >,
            ScopedWriteLockT
        >::type
#else
        ScopedWriteLockT
#endif
>
class mutable_constant :
    public attribute
{
public:
    //! The attribute value type
    typedef T value_type;

protected:
    //! Factory implementation
    class BOOST_LOG_VISIBLE impl :
        public attribute::impl
    {
    private:
        //! Mutex type
        typedef MutexT mutex_type;
        //! Shared lock type
        typedef ScopedReadLockT scoped_read_lock;
        //! Exclusive lock type
        typedef ScopedWriteLockT scoped_write_lock;
        BOOST_STATIC_ASSERT(!(is_void< mutex_type >::value || is_void< scoped_read_lock >::value || is_void< scoped_write_lock >::value));

    private:
        //! The actual value
        value_type m_Value;
        //! Thread protection mutex
        mutex_type m_Mutex;

    public:
        /*!
         * Initializing constructor
         */
        impl(value_type const& value) : m_Value(value)
        {
        }

        attribute_value get_value()
        {
            typedef basic_attribute_value< value_type > attr_value;
            return attribute_value(new attr_value(get()));
        }

        void set(value_type const& value)
        {
            scoped_write_lock lock(m_Mutex);
            m_Value = value;
        }

        value_type get() const
        {
            scoped_read_lock lock(m_Mutex);
            return m_Value;
        }
    };

public:
    /*!
     * Constructor with the stored value initialization
     */
    explicit mutable_constant(value_type const& value) : attribute(new impl(value))
    {
    }
    /*!
     * Constructor for casting support
     */
    explicit mutable_constant(cast_source const& source) : attribute(source.as< impl >())
    {
    }

    /*!
     * The method sets a new attribute value. The implementation exclusively locks the mutex in order
     * to protect the value assignment.
     */
    void set(value_type const& value)
    {
        get_impl()->set(value);
    }

    /*!
     * The method acquires the current attribute value. The implementation non-exclusively locks the mutex in order
     * to protect the value acquisition.
     */
    value_type get() const
    {
        return get_impl()->get();
    }

protected:
    /*!
     * \returns Pointer to the factory implementation
     */
    impl* get_impl() const
    {
        return static_cast< impl* >(attribute::get_impl());
    }
};


/*!
 * \brief Specialization for unlocked case
 *
 * This version of attribute does not perform thread synchronization to access the stored value.
 */
template< typename T >
class mutable_constant< T, void, void, void > :
    public attribute
{
public:
    //! The attribute value type
    typedef T value_type;

protected:
    //! Factory implementation
    class BOOST_LOG_VISIBLE impl :
        public attribute::impl
    {
    private:
        //! The actual value
        value_type m_Value;

    public:
        /*!
         * Initializing constructor
         */
        impl(value_type const& value) : m_Value(value)
        {
        }

        attribute_value get_value()
        {
            typedef basic_attribute_value< value_type > attr_value;
            return attribute_value(new attr_value(m_Value));
        }

        void set(value_type const& value)
        {
            m_Value = value;
        }

        value_type get() const
        {
            return m_Value;
        }
    };

public:
    /*!
     * Constructor with the stored value initialization
     */
    explicit mutable_constant(value_type const& value) : attribute(new impl(value))
    {
    }
    /*!
     * Constructor for casting support
     */
    explicit mutable_constant(cast_source const& source) : attribute(source.as< impl >())
    {
    }

    /*!
     * The method sets a new attribute value.
     */
    void set(value_type const& value)
    {
        get_impl()->set(value);
    }

    /*!
     * The method acquires the current attribute value.
     */
    value_type get() const
    {
        return get_impl()->get();
    }

protected:
    /*!
     * \returns Pointer to the factory implementation
     */
    impl* get_impl() const
    {
        return static_cast< impl* >(attribute::get_impl());
    }
};

} // namespace attributes

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTES_MUTABLE_CONSTANT_HPP_INCLUDED_
