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
 * \file
 * \author Andrey Semashev
 * \date   26.11.2007
 * 
 * The header contains implementation of a named scope formatter.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_NAMED_SCOPE_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_NAMED_SCOPE_HPP_INCLUDED_

#include <string>
#include <iterator>
#include <algorithm>
#include <boost/limits.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/formatters/basic_formatters.hpp>
#include <boost/log/utility/type_dispatch/type_dispatcher.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

namespace keywords {

#ifndef BOOST_LOG_DOXYGEN_PASS

    BOOST_PARAMETER_KEYWORD(tag, scope_delimiter)
    BOOST_PARAMETER_KEYWORD(tag, scope_depth)
    BOOST_PARAMETER_KEYWORD(tag, scope_iteration)

#else // BOOST_LOG_DOXYGEN_PASS

    //! The keyword for passing scopes delimiter to the \c named_scope formatter
    implementation_defined scope_delimiter;
    //! The keyword for passing maximum scopes depth to the \c named_scope formatter
    implementation_defined scope_depth;
    //! The keyword for passing scope iteration direction to the \c named_scope formatter
    implementation_defined scope_iteration;

#endif // BOOST_LOG_DOXYGEN_PASS

    //! Scope iteration directions
    enum scope_iteration_direction
    {
        forward,    //!< Iterate through scopes from outermost to innermost
        reverse     //!< Iterate through scopes from innermost to outermost
    };

} // namespace keywords

/*!
 * \brief Named scope attribute formatter
 * 
 * The formatter iterates through the list of scopes and puts each one into the resulting stream.
 * The formatter supports customizing the iteration direction, depth and delimiter between the scopes.
 */
template< typename CharT >
class fmt_named_scope :
    public basic_formatter< CharT, fmt_named_scope< CharT > >
{
private:
    //! Base type
    typedef basic_formatter< CharT, fmt_named_scope< CharT > > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type
    typedef typename base_type::string_type string_type;
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Attribute values set type
    typedef typename base_type::values_view_type values_view_type;

private:
    //! Scope stack container type
    typedef typename attributes::basic_named_scope< char_type >::scope_stack scope_stack;

private:
    //! Attribute name
    const string_type m_AttributeName;
    //! Scope delimiter
    const string_type m_ScopeDelimiter;
    //! Number of scopes to output
    const typename scope_stack::size_type m_MaxScopes;
    //! Scope iteration direction
    const keywords::scope_iteration_direction m_IterationDirection;

public:
    /*!
     * Constructor
     * 
     * \param name Attribute name
     * \param delimiter Scope delimiter string
     * \param max_scopes Maximum scope iteration depth
     * \param direction Scope iteration direction
     */
    template< typename T1, typename T2 >
    fmt_named_scope(
        T1 const& name,
        T2 const& delimiter,
        typename scope_stack::size_type max_scopes,
        keywords::scope_iteration_direction direction
    ) :
        m_AttributeName(name),
        m_ScopeDelimiter(delimiter),
        m_MaxScopes(max_scopes),
        m_IterationDirection(direction)
    {
    }

    /*!
     * Formatting operator. Acquires the scope list attribute with the specified on construction name from
     * \a attrs and puts its contents into the \a strm stream.
     * 
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param attrs A set of attribute values that are associated with the logging record
     * \param msg The logging record message
     */
    void operator() (ostream_type& strm, values_view_type const& attrs, string_type const& msg) const
    {
        typename values_view_type::const_iterator it = attrs.find(m_AttributeName);
        if (it != attrs.end())
        {
            optional< scope_stack const& > maybe_scopes = it->second->get< scope_stack >();
            if (!!maybe_scopes)
            {
                // Found the attribute value
                scope_stack const& scopes = maybe_scopes.get();
                typename scope_stack::size_type const scopes_to_iterate = (std::min)(m_MaxScopes, scopes.size());
                if (m_IterationDirection == keywords::forward)
                {
                    // Iterating through scopes in forward direction
                    typename scope_stack::const_iterator it = scopes.end(), end = it;
                    std::advance(it, -static_cast< typename scope_stack::difference_type >(scopes_to_iterate));

                    if (it != end)
                    {
                        if (it != scopes.begin())
                            strm << "..." << m_ScopeDelimiter;

                        strm << it->scope_name;
                        for (++it; it != end; ++it)
                            strm << m_ScopeDelimiter << it->scope_name;
                    }
                }
                else
                {
                    // Iterating through scopes in reverse direction
                    typename scope_stack::const_reverse_iterator it = scopes.rbegin(), end = it;
                    std::advance(end, static_cast< typename scope_stack::difference_type >(scopes_to_iterate));

                    if (it != end)
                    {
                        strm << it->scope_name;
                        for (++it; it != end; ++it)
                            strm << m_ScopeDelimiter << it->scope_name;

                        if (it != scopes.rend())
                            strm << m_ScopeDelimiter << "...";
                    }
                }
            }
        }
    }
};

namespace aux {

    //! Auxiliary traits to acquire correct default delimiter depending on the character type
    template< typename CharT >
    struct default_scope_delimiter;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct default_scope_delimiter< char >
    {
        static const char* forward() { return "->"; }
        static const char* reverse() { return "<-"; }
    };
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct default_scope_delimiter< wchar_t >
    {
        static const wchar_t* forward() { return L"->"; }
        static const wchar_t* reverse() { return L"<-"; }
    };
#endif

    //! Auxiliary function to construct formatter from the complete set of arguments
    template< typename CharT, typename ArgsT >
    fmt_named_scope< CharT > named_scope(const CharT* name, ArgsT const& args)
    {
        typedef fmt_named_scope< CharT > fmt_named_scope_t;

        keywords::scope_iteration_direction direction = args[keywords::scope_iteration | keywords::forward];
        const CharT* default_delimiter =
            (direction == keywords::forward ? default_scope_delimiter< CharT >::forward() : default_scope_delimiter< CharT >::reverse());

        return fmt_named_scope_t(
            name,
            args[keywords::scope_delimiter | default_delimiter],
            args[keywords::scope_depth | (std::numeric_limits< std::size_t >::max)()],
            direction);
    }
    //! Auxiliary function to construct formatter from the complete set of arguments
    template< typename CharT, typename ArgsT >
    fmt_named_scope< CharT > named_scope(std::basic_string< CharT > const& name, ArgsT const& args)
    {
        typedef fmt_named_scope< CharT > fmt_named_scope_t;

        keywords::scope_iteration_direction direction = args[keywords::scope_iteration | keywords::forward];
        const CharT* default_delimiter =
            (direction == keywords::forward ? default_scope_delimiter< CharT >::forward() : default_scope_delimiter< CharT >::reverse());

        return fmt_named_scope_t(
            name,
            args[keywords::scope_delimiter | default_delimiter],
            args[keywords::scope_depth | (std::numeric_limits< std::size_t >::max)()],
            direction);
    }

} // namespace aux

#ifndef BOOST_LOG_DOXYGEN_PASS

#ifdef BOOST_LOG_USE_CHAR

//! Formatter generator
inline fmt_named_scope< char > named_scope(const char* name)
{
    return fmt_named_scope< char >(name, "->", (std::numeric_limits< std::size_t >::max)(), keywords::forward);
}
//! Formatter generator
inline fmt_named_scope< char > named_scope(std::basic_string< char > const& name)
{
    return fmt_named_scope< char >(name, "->", (std::numeric_limits< std::size_t >::max)(), keywords::forward);
}

//! Formatter generator
template< typename T1 >
inline fmt_named_scope< char > named_scope(const char* name, T1 const& arg1)
{
    return aux::named_scope(name, arg1);
}
//! Formatter generator
template< typename T1 >
inline fmt_named_scope< char > named_scope(std::basic_string< char > const& name, T1 const& arg1)
{
    return aux::named_scope(name, arg1);
}

//! Formatter generator
template< typename T1, typename T2 >
inline fmt_named_scope< char > named_scope(const char* name, T1 const& arg1, T2 const& arg2)
{
    return aux::named_scope(name, (arg1, arg2));
}
//! Formatter generator
template< typename T1, typename T2 >
inline fmt_named_scope< char > named_scope(std::basic_string< char > const& name, T1 const& arg1, T2 const& arg2)
{
    return aux::named_scope(name, (arg1, arg2));
}

//! Formatter generator
template< typename T1, typename T2, typename T3 >
inline fmt_named_scope< char > named_scope(const char* name, T1 const& arg1, T2 const& arg2, T3 const& arg3)
{
    return aux::named_scope(name, (arg1, arg2, arg3));
}
//! Formatter generator
template< typename T1, typename T2, typename T3 >
inline fmt_named_scope< char > named_scope(std::basic_string< char > const& name, T1 const& arg1, T2 const& arg2, T3 const& arg3)
{
    return aux::named_scope(name, (arg1, arg2, arg3));
}

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

//! Formatter generator
inline fmt_named_scope< wchar_t > named_scope(const wchar_t* name)
{
    return fmt_named_scope< wchar_t >(name, L"->", (std::numeric_limits< std::size_t >::max)(), keywords::forward);
}
//! Formatter generator
inline fmt_named_scope< wchar_t > named_scope(std::basic_string< wchar_t > const& name)
{
    return fmt_named_scope< wchar_t >(name, L"->", (std::numeric_limits< std::size_t >::max)(), keywords::forward);
}

//! Formatter generator
template< typename T1 >
inline fmt_named_scope< wchar_t > named_scope(const wchar_t* name, T1 const& arg1)
{
    return aux::named_scope(name, arg1);
}
//! Formatter generator
template< typename T1 >
inline fmt_named_scope< wchar_t > named_scope(std::basic_string< wchar_t > const& name, T1 const& arg1)
{
    return aux::named_scope(name, arg1);
}

//! Formatter generator
template< typename T1, typename T2 >
inline fmt_named_scope< wchar_t > named_scope(const wchar_t* name, T1 const& arg1, T2 const& arg2)
{
    return aux::named_scope(name, (arg1, arg2));
}
//! Formatter generator
template< typename T1, typename T2 >
inline fmt_named_scope< wchar_t > named_scope(std::basic_string< wchar_t > const& name, T1 const& arg1, T2 const& arg2)
{
    return aux::named_scope(name, (arg1, arg2));
}

//! Formatter generator
template< typename T1, typename T2, typename T3 >
inline fmt_named_scope< wchar_t > named_scope(const wchar_t* name, T1 const& arg1, T2 const& arg2, T3 const& arg3)
{
    return aux::named_scope(name, (arg1, arg2, arg3));
}
//! Formatter generator
template< typename T1, typename T2, typename T3 >
inline fmt_named_scope< wchar_t > named_scope(std::basic_string< wchar_t > const& name, T1 const& arg1, T2 const& arg2, T3 const& arg3)
{
    return aux::named_scope(name, (arg1, arg2, arg3));
}

#endif // BOOST_LOG_USE_WCHAR_T

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * Formatter generator. Construct the named scope formatter with the specified formatting parameters.
 * 
 * \param name Attribute name
 * \param args An optional set of named parameters. Supported parameters:
 *             \li \c scope_delimiter - a string that is used to delimit the formatted scope names. Default: "->" or "<-", depending on the iteration direction.
 *             \li \c scope_iteration - iteration direction. Default: forward.
 *             \li \c scope_depth - iteration depth. Default: unlimited.
 */
template< typename CharT, typename... ArgsT >
fmt_named_scope< CharT > named_scope(std::basic_string< CharT > const& name, ArgsT... const& args);

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_NAMED_SCOPE_HPP_INCLUDED_
