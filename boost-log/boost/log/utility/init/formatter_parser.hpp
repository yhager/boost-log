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
 * \file   formatter_parser.hpp
 * \author Andrey Semashev
 * \date   07.04.2008
 * 
 * The header contains definition of a formatter parser function, along with facilities to
 * add support for custom formattetrs.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_INIT_FORMATTER_PARSER_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_INIT_FORMATTER_PARSER_HPP_INCLUDED_

#include <iosfwd>
#include <map>
#include <string>
#include <boost/function/function2.hpp>
#include <boost/function/function3.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief Auxiliary formatter traits
 * 
 * The structure generates commonly used types related to formatters and formatter factories.
 */
template< typename CharT >
struct formatter_types
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Output stream type
    typedef std::basic_ostream< char_type > ostream_type;
    //! Map of attribute values
    typedef basic_attribute_values_view< char_type > values_view_type;
    //! The formatter function object
    typedef function3< void, ostream_type&, values_view_type const&, string_type const& > formatter_type;

    /*!
     * Type of the map of formatter factory arguments [argument name -> argument value].
     * This type of maps will be passed to formatter factories on attempt to create a formatter.
     */
    typedef std::map< string_type, string_type > formatter_factory_args;
    /*!
     * \brief The type of a function object that constructs formatter instance
     * \param name Attribute name
     * \param args Formatter arguments
     * \return The constructed formatter. The formatter must not be empty.
     * \throw An <tt>std::exception</tt>-based If an exception is thrown from the method,
     *        the exception is propagated to the parse_formatter caller
     */
    typedef function2< formatter_type, string_type const&, formatter_factory_args const& > formatter_factory;
    //! Map of formatter factory function objects
    typedef std::map< string_type, formatter_factory > factories_map;
};

/*!
 * \brief The function registers a user-defined formatter factory
 * 
 * The function registers a user-defined formatter factory. The registered factioy function will be
 * called when the formatter parser detects the specified attribute name in the formatter string.
 * 
 * \param attr_name Attribute name. Must point to a zero-terminated sequence of characters, must not be NULL.
 * \param factory Formatter factory function
 */
template< typename CharT >
BOOST_LOG_EXPORT void
register_formatter_factory(
    const CharT* attr_name,
    typename formatter_types< CharT >::formatter_factory const& factory);

/*!
 * \brief The function registers a user-defined formatter factory
 * 
 * The function registers a user-defined formatter factory. The registered factioy function will be
 * called when the formatter parser detects the specified attribute name in the formatter string.
 * 
 * \param attr_name Attribute name
 * \param factory Formatter factory function
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline void
register_formatter_factory(
    std::basic_string< CharT, TraitsT, AllocatorT > const& attr_name,
    typename formatter_types< CharT >::formatter_factory const& factory)
{
    register_formatter_factory(attr_name.c_str(), factory);
}

/*!
 * The function parses a formatter from the sequence of characters
 * 
 * \pre <tt>begin <= end</tt>
 * \param begin Pointer to the first character of the sequence. Must not be NULL.
 * \param end Pointer to the after-the-last character of the sequence. Must not be NULL.
 * \return A function object that can be used as a formatter.
 * \throw An <tt>std::exception</tt>-based exception, if a formatter cannot be recognized in the character sequence.
 */
template< typename CharT >
BOOST_LOG_EXPORT typename formatter_types< CharT >::formatter_type
parse_formatter(const CharT* begin, const CharT* end);

/*!
 * The function parses a formatter from the string
 * 
 * \param str A string that contains format description
 * \return A function object that can be used as a formatter.
 * \throw An <tt>std::exception</tt>-based exception, if a formatter cannot be recognized in the character sequence.
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline typename formatter_types< CharT >::formatter_type
parse_formatter(std::basic_string< CharT, TraitsT, AllocatorT > const& str)
{
    const CharT* p = str.c_str();
    return parse_formatter(p, p + str.size());
}

/*!
 * The function parses a formatter from the string
 * 
 * \param str A string that contains format description. Must point to a zero-terminated character sequence,
 *            must not be NULL. 
 * \return A function object that can be used as a formatter.
 * \throw An <tt>std::exception</tt>-based exception, if a formatter cannot be recognized in the character sequence.
 */
template< typename CharT >
inline typename formatter_types< CharT >::formatter_type
parse_formatter(const CharT* str)
{
    return parse_formatter(str, str + std::char_traits< CharT >::length(str));
}

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_UTILITY_INIT_FORMATTER_PARSER_HPP_INCLUDED_
