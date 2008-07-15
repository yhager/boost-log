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
 * \file   attribute.hpp
 * \author Andrey Semashev
 * \date   15.04.2007
 * 
 * The header contains attribute and attribute_value interfaces definition.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTE_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTE_HPP_INCLUDED_

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/utility/type_dispatch/type_dispatcher.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief A base class for an attribute value
 * \sa basic_attribute_values_view
 * 
 * An attribute value is an object that contains a piece of data that represents an attribute state
 * at the point of the value acquision. All major operations with log records, such as filtering and
 * formatting, involve attribute values contained in a single view. Most likely an attribute value is
 * implemented as a simple holder of some typed value. This holder implements attribute_value interface
 * and provides type dispatching support in order to allow to extract the stored value.
 * 
 * Normally, attributes and their values shall be designed in order to exclude as much interference as
 * reasonable. Such approach allows to have more than one attribute value simultaneously, which improves
 * scalability and allows to implement generating attributes.
 * 
 * However, there are cases when this approach does not help to achieve the required level of independency
 * of attribute values and attribute itself from each other at a reasonable performance tradeoff.
 * For example, an attribute or its values may use thread-specific data, which is global and shared
 * between all the instances of the attribute/value. Passing such an attribute value to another thread
 * would be a disaster. To solve this the library defines an additional method for attribute values,
 * namely detach_from_thread. This method is called for all attribute values that are passed to
 * another thread. The method is called only once per attribute value, on the first thread change.
 * It is assumed that the value does not depend on any thread-specific data after this call.
 */
struct BOOST_LOG_NO_VTABLE attribute_value
{
private:
    //! \cond

    //! A simple type dispatcher to support the get method
    template< typename T >
    struct extractor :
        public type_dispatcher,
        public type_visitor< T >
    {
        //! Constructor
        explicit extractor(optional< T const& >& res) : res_(res) {}
        //! Returns itself if the value type matches the requested type
        void* get_visitor(std::type_info const& type)
        {
            BOOST_LOG_ASSUME(this != 0);
            if (type == typeid(T))
                return static_cast< type_visitor< T >* >(this);
            else
                return 0;
        }
        //! Extracts the value
        void visit(T const& value)
        {
            res_ = value;
        }

    private:
        //! The reference to the extracted value
        optional< T const& >& res_;
    };

    //! \endcond

public:
    /*!
     * Destructor. Destroys the value.
     */
    virtual ~attribute_value() {}

    /*!
     * The method dispatches the value to the given object.
     * 
     * \sa type_dispatcher 
     * \param dispatcher The object that attempts to dispatch the stored value.
     * \return true if \a dispatcher was capable to consume the real attribute value type and false otherwise.
     */
    virtual bool dispatch(type_dispatcher& dispatcher) = 0;

    /*!
     * The method is called when the attribute value is passed to another thread (e.g.
     * in case of asynchronous logging). The value should ensure it properly owns all thread-specific data.
     * 
     * \return An actual pointer to the attribute value. It may either point to this object or another.
     *         In the latter case the returned pointer replaces the pointer used by caller to invoke this
     *         method and is considered to be a functional equivalent to the previous pointer.
     */
    virtual shared_ptr< attribute_value > detach_from_thread() = 0;

    /*!
     * An alternative to type dispatching. This is a simpler way to get the stored value in case if one knows its exact type.
     * 
     * \return An optional constant reference to the stored value. The returned value is present if the
     *         requested type matches the stored type, otherwise the returned value is absent.
     */
    template< typename T >
    optional<
#ifndef BOOST_LOG_DOXYGEN_PASS
        typename add_reference<
            typename add_const<
                typename remove_cv<
                    typename remove_reference< T >::type
                >::type
            >::type
        >::type
#else
        T const&
#endif // BOOST_LOG_DOXYGEN_PASS
    > get()
    {
        typedef typename remove_cv<
            typename remove_reference< T >::type
        >::type requested_type;
        typedef optional<
            typename add_reference<
                typename add_const< requested_type >::type
            >::type
        > result_type;

        result_type res;
        extractor< requested_type > disp(res);
        this->dispatch(disp);
        return res;
    }
};

/*!
 * \brief A base class for an attribute
 * 
 * An attribute is basically a wrapper for some logic of values acquision. The sole purpose of
 * an attribute is to return an actual value when requested. A simpliest attribute
 * can always return the same value that it stores internally, but more complex species may
 * perform a considirable amount of work to return a value, and their values may differ.
 * 
 * A word about thread safety. An attribute should be prepared to be requested a value from
 * multiple threads concurrently.
 */
struct BOOST_LOG_NO_VTABLE attribute
{
    /*!
     * Destructor. Destroys the attribute.
     */
    virtual ~attribute() {}

    /*!
     * The method returns the actual attribute value. It must not return NULL (use excetions
     * to indicate errors).
     */
    virtual shared_ptr< attribute_value > get_value() = 0;
};

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTE_HPP_INCLUDED_
