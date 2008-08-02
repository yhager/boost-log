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
 * \file   type_info_wrapper.hpp
 * \author Andrey Semashev
 * \date   15.04.2007
 * 
 * The header contains implementation of a type information wrapper.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_TYPE_INFO_WRAPPER_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_TYPE_INFO_WRAPPER_HPP_INCLUDED_

#ifdef __GNUC__
#include <cxxabi.h>
#include <memory.h>
#endif // __GNUC__
#include <typeinfo>
#include <string>
#include <algorithm>
#include <boost/operators.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief A simple <tt>std::type_info</tt> wrapper that implements value semantic for type information objects
 * 
 * The type info wrapper is very useful for storing type information objects in containers,
 * as a key or value. It also provides a number of useful features, such as default construction
 * and assignment support, an empty state and extended support for human-friendly type names.
 */
class type_info_wrapper
    //! \cond
    : public partially_ordered< type_info_wrapper,
        equality_comparable< type_info_wrapper >
    >
    //! \endcond
{
private:
#ifndef BOOST_LOG_DOXYGEN_PASS

    //! An inaccessible type to indicate an uninitialized state of the wrapper
    enum uninitialized {};

#ifndef BOOST_NO_UNSPECIFIED_BOOL
    struct dummy
    {
        int data1;
        int data2;
    };

    typedef int (dummy::*unspecified_bool);
#endif // BOOST_NO_UNSPECIFIED_BOOL

#ifdef __GNUC__
    //! A simple scope guard for automatic memory free
    struct auto_free
    {
        explicit auto_free(void* p) : p_(p) {}
        ~auto_free() { free(p_); }
    private:    
        void* p_;
    };
#endif // __GNUC__

#endif // BOOST_LOG_DOXYGEN_PASS

private:
    //! A pointer to the actual type info
    std::type_info const* info;

public:
    /*!
     * Default constructor
     * 
     * \post <tt>!*this == true</tt>
     */
    type_info_wrapper() : info(&typeid(uninitialized)) {}
    /*!
     * Copy constructor
     * 
     * \post <tt>*this == that</tt>
     * \param that Source type info wrapper to copy from
     */
    type_info_wrapper(type_info_wrapper const& that) : info(that.info) {}
    /*!
     * Conversion constructor
     * 
     * \post <tt>*this == that && !!*this</tt>
     * \param that Type info object to be wrapped
     */
    type_info_wrapper(std::type_info const& that) : info(&that) {}

    /*!
     * \return \c true if the type info wrapper was initialized with a particular type,
     *         \c false if the wrapper was default-constructed and not yet initialized
     */
#ifdef BOOST_NO_UNSPECIFIED_BOOL
    operator bool () const { return (*info != typeid(uninitialized)); }
#else
    operator unspecified_bool() const
    {
        if (*info != typeid(uninitialized))
            return &dummy::data2;
        else
            return 0;
    }
#endif // BOOST_NO_UNSPECIFIED_BOOL

    /*!
     * Stored type info getter
     * 
     * \pre <tt>!!*this</tt>
     * \return Constant reference to the wrapped type info object
     */
    std::type_info const& get() const { return *info; }

    /*!
     * Swaps two instances of the wrapper
     */
    void swap(type_info_wrapper& that)
    {
        std::swap(info, that.info);
    }

    /*!
     * The method returns the contained type name string in a possibly more readable format than <tt>get().name()</tt>
     * 
     * \pre <tt>!!*this</tt>
     * \return Type name string
     */
    std::string pretty_name() const
    {
        if (*info != typeid(uninitialized))
        {
#ifdef __GNUC__
            // GCC returns decorated type name, will need to demangle it using ABI
            int status = 0;
            size_t size = 0;
            const char* name = info->name();
            char* undecorated = abi::__cxa_demangle(name, NULL, &size, &status);
            auto_free _(undecorated);

            if (undecorated)
                return undecorated;
            else
                return name;
#else
            return info->name();
#endif
        }
        else
            return "[uninitialized]";
    }

#ifdef _MSC_VER
#pragma warning(push)
// 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4800)
#endif

    /*!
     * \return \c false if the type info wrapper was initialized with a particular type,
     *         \c true if the wrapper was default-constructed and not yet initialized
     */
    bool operator! () const { return (*info == typeid(uninitialized)); }

    /*!
     * Equality comparison
     * 
     * \param that Comparand
     * \return If either this object or comparand is in empty state and the other is not, the result is \c false.
     *         If both arguments are empty, the result is \c true. If both arguments are not empty, the result
     *         is \c true if this object wraps the same type as the comparand and \c false otherwise.
     */
    bool operator== (type_info_wrapper const& that) const
    {
        return (*info == *that.info);
    }
    /*!
     * Ordering operator
     * 
     * \pre <tt>!!*this && !!that</tt>
     * \param that Comparand
     * \return \c true if this object wraps type info object that is ordered before
     *         the type info object in the comparand, \c false otherwise
     * \note The results of this operator are only consistent within a single run of application.
     *       The result may change for the same types after rebuilding or even restarting the application.
     */
    bool operator< (type_info_wrapper const& that) const
    {
        return static_cast< bool >(info->before(*that.info));
    }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

};

//! Free swap for type info wrapper
inline void swap(type_info_wrapper& left, type_info_wrapper& right)
{
    left.swap(right);
}

} // namespace log

} // namespace boost

#endif // BOOST_LOG_UTILITY_TYPE_INFO_WRAPPER_HPP_INCLUDED_
