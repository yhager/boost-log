/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   standard_types.hpp
 * \author Andrey Semashev
 * \date   19.05.2007
 * 
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_STANDARD_TYPES_HPP_INCLUDED_
#define BOOST_LOG_STANDARD_TYPES_HPP_INCLUDED_

#include <string>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace log {

//! An MPL-sequence of integral types of attributes, supported by default
typedef mpl::vector<
    bool,
    signed char,
    unsigned char,
    short,
    unsigned short,
    int,
    unsigned int,
    long,
    unsigned long
#if defined(BOOST_HAS_LONG_LONG)
    , long long
    , unsigned long long
#endif // defined(BOOST_HAS_LONG_LONG)
>::type integral_types;

//! An MPL-sequence of FP types of attributes, supported by default
typedef mpl::vector<
    float,
    double,
    long double
>::type floating_point_types;

//! An MPL-sequence of all numeric types of attributes, supported by default
typedef mpl::joint_view<
    integral_types,
    floating_point_types
>::type numeric_types;

//! An MPL-sequence of string types of attributes, supported by default
template< typename CharT >
struct basic_string_types :
    public mpl::vector<
        CharT,
        std::basic_string< CharT >
    >::type
{
};

typedef basic_string_types< char > string_types;
typedef basic_string_types< wchar_t > wstring_types;

//! An auxiliary type sequence maker. The sequence contains all
//! attribute value types that are supported by the library by default.
template< typename CharT >
struct make_default_attribute_types
{
    typedef typename mpl::joint_view<
        numeric_types,
        basic_string_types< CharT >
    >::type type;
};

} // namespace log

} // namespace boost

#endif // BOOST_LOG_STANDARD_TYPES_HPP_INCLUDED_
