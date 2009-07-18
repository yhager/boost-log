/*
 * (C) 2009 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This header is the Boost.Log library implementation, see the library documentation
 * at http://www.boost.org/libs/log/doc/log.html.
 */
/*!
 * \file   support/regex.hpp
 * \author Andrey Semashev
 * \date   18.07.2009
 *
 * This header enables Boost.Regex support for Boost.Log.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SUPPORT_REGEX_HPP_INCLUDED_
#define BOOST_LOG_SUPPORT_REGEX_HPP_INCLUDED_

#include <boost/regex.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/functional.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

struct boost_regex_expression_tag {};

template< typename T >
struct is_regex
{
private:
    typedef char yes_type;
    struct no_type { char dummy[2]; };

    template< typename CharT, typename TraitsT >
    static yes_type check(basic_regex< CharT, TraitsT > const&);
    static no_type check(...);
    static T& get_T();

public:
    enum { value = sizeof(check(get_T())) == sizeof(yes_type) };
    typedef mpl::bool_< value > type;
};

//! The function is used to determine the kind of regex expression
template< typename ExpressionT >
BOOST_LOG_FORCEINLINE typename enable_if<
    is_regex< ExpressionT >,
    boost_regex_expression_tag
>::type match_expression_tag_of(ExpressionT*)
{
    return boost_regex_expression_tag();
}

//! The regex matching functor implementation
template< >
struct matches_fun_impl< boost_regex_expression_tag >
{
    template< typename StringT, typename CharT, typename TraitsT >
    static bool matches(
        StringT const& str,
        basic_regex< CharT, TraitsT > const& expr,
        match_flag_type flags = match_default)
    {
        return regex_match(str.begin(), str.end(), expr, flags);
    }
};

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_SUPPORT_REGEX_HPP_INCLUDED_
