/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   attr.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 * 
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FILTERS_ATTR_HPP_INCLUDED_
#define BOOST_LOG_FILTERS_ATTR_HPP_INCLUDED_

#include <string>
#include <utility>
#include <boost/regex.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_extent.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/functional.hpp>
#include <boost/log/filters/basic_filters.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>
#include <boost/log/attributes/extractors.hpp>

namespace boost {

namespace log {

namespace filters {

//! The filter checks that the attribute value satisfies the predicate FunT
template< typename CharT, typename FunT, typename AttributeValueTypesT >
class flt_attr :
    public basic_filter< CharT, flt_attr< CharT, FunT, AttributeValueTypesT > >
{
    //! Base type
    typedef basic_filter< CharT, flt_attr< CharT, FunT, AttributeValueTypesT > > base_type;
    //! Attribute value extractor type
    typedef attributes::attribute_value_extractor< CharT, AttributeValueTypesT > extractor;

public:
    //! String type
    typedef typename base_type::string_type string_type;
    //! Attribute values container type
    typedef typename base_type::attribute_values_view attribute_values_view;
    //! Predicate functor type
    typedef FunT checker_type;

private:
    //! Function adapter that saves the checker function result into a referenced variable
    struct checker_wrapper
    {
        typedef void result_type;

        checker_wrapper(checker_type const& fun, bool& res) : m_Fun(fun), m_Result(res) {}

        template< typename T >
        void operator() (T const& arg) const
        {
            m_Result = m_Fun(arg);
        }

    private:
        checker_type const& m_Fun;
        bool& m_Result;
    };

private:
    //! Attribute value extractor
    extractor m_Extractor;
    //! Attribute value checker
    checker_type m_Checker;

public:
    flt_attr(string_type const& name, checker_type const& checker)
        : m_Extractor(name), m_Checker(checker)
    {
    }

    bool operator() (attribute_values_view const& values) const
    {
        bool result = false;
        checker_wrapper receiver(m_Checker, result);

        m_Extractor(values, receiver);

        return result;
    }
};

namespace aux {

    template< typename > struct is_char : mpl::false_ {};
    template< > struct is_char< char > : mpl::true_ {};
    template< > struct is_char< wchar_t > : mpl::true_ {};

    //! An auxiliary type translator to store strings by value in function objects
    template< typename StringT, typename ArgT >
    struct make_embedded_type
    {
        // Make sure that string literals and C string are converted to STL strings
        typedef typename mpl::if_c<
            is_char<
                typename remove_cv<
                    typename mpl::eval_if<
                        is_array< ArgT >,
                        remove_extent< ArgT >,
                        mpl::eval_if<
                            is_pointer< ArgT >,
                            remove_pointer< ArgT >,
                            mpl::identity< void >
                        >
                    >::type
                >::type
            >::value,
            StringT,
            ArgT
        >::type type;
    };

    //! The base class for attr filter generator
    template< typename CharT, typename AttributeValueTypesT, bool >
    class flt_attr_gen_base;

    //! Specialization for non-string attribute value types
    template< typename CharT, typename AttributeValueTypesT >
    class flt_attr_gen_base< CharT, AttributeValueTypesT, false >
    {
    public:
        //! Char type
        typedef CharT char_type;
        //! String type
        typedef std::basic_string< char_type > string_type;
        //! Attribute values container type
        typedef basic_attribute_values_view< char_type > attribute_values_view;
        //! Size type
        typedef typename attribute_values_view::size_type size_type;
        //! Supported attribute value types
        typedef AttributeValueTypesT attribute_value_types;
    
    protected:
        //! Attribute name
        string_type m_AttributeName;
    
    public:
        explicit flt_attr_gen_base(string_type const& name) : m_AttributeName(name) {}
    
#define BOOST_LOG_FILTER_ATTR_MEMBER(member, fun)\
        template< typename T >\
        flt_attr<\
            char_type,\
            boost::log::aux::binder2nd< fun, typename make_embedded_type< string_type, T >::type >,\
            attribute_value_types\
        > member (T const& arg) const\
        {\
            typedef typename make_embedded_type< string_type, T >::type arg_type;\
            typedef boost::log::aux::binder2nd< fun, arg_type > binder_t;\
            typedef flt_attr< char_type, binder_t, attribute_value_types > flt_attr_t;\
            return flt_attr_t(this->m_AttributeName, binder_t(fun(), arg_type(arg)));\
        }
    
        BOOST_LOG_FILTER_ATTR_MEMBER(operator ==, boost::log::aux::equal_to)
        BOOST_LOG_FILTER_ATTR_MEMBER(operator !=, boost::log::aux::not_equal_to)
        BOOST_LOG_FILTER_ATTR_MEMBER(operator >, boost::log::aux::greater)
        BOOST_LOG_FILTER_ATTR_MEMBER(operator <, boost::log::aux::less)
        BOOST_LOG_FILTER_ATTR_MEMBER(operator >=, boost::log::aux::greater_equal)
        BOOST_LOG_FILTER_ATTR_MEMBER(operator <=, boost::log::aux::less_equal)

        //! Filter generator for checking whether the attribute value lies within a specific range
        template< typename T >
        flt_attr<
            char_type,
            boost::log::aux::binder2nd<
                boost::log::aux::in_range_fun,
                std::pair<
                    typename make_embedded_type< string_type, T >::type,
                    typename make_embedded_type< string_type, T >::type
                >
            >,
            attribute_value_types
        > is_in_range(T const& lower, T const& upper) const
        {
            typedef typename make_embedded_type< string_type, T >::type arg_type;
            typedef boost::log::aux::binder2nd< boost::log::aux::in_range_fun, std::pair< arg_type, arg_type > > binder_t;
            typedef flt_attr< char_type, binder_t, attribute_value_types > flt_attr_t;
            return flt_attr_t(
                this->m_AttributeName,
                binder_t(boost::log::aux::in_range_fun(), std::make_pair(arg_type(lower), arg_type(upper))));
        }

        //! Filter generator for user-provided predicate function
        template< typename UnaryFunT >
        flt_attr<
            char_type,
            UnaryFunT,
            attribute_value_types
        > satisfies(UnaryFunT const& fun) const
        {
            typedef flt_attr<
                char_type,
                UnaryFunT,
                attribute_value_types
            > flt_attr_t;
            return flt_attr_t(this->m_AttributeName, fun);
        }
    };

    //! Specialization for string attribute value types
    template< typename CharT, typename AttributeValueTypesT >
    class flt_attr_gen_base< CharT, AttributeValueTypesT, true > :
        public flt_attr_gen_base< CharT, AttributeValueTypesT, false >
    {
        typedef flt_attr_gen_base< CharT, AttributeValueTypesT, false > base_type;

    public:
        //! Character type
        typedef typename base_type::char_type char_type;
        //! String type
        typedef typename base_type::string_type string_type;
        //! Supported attribute value types
        typedef typename base_type::attribute_value_types attribute_value_types;

    public:
        explicit flt_attr_gen_base(string_type const& name) : base_type(name) {}

        BOOST_LOG_FILTER_ATTR_MEMBER(begins_with, boost::log::aux::begins_with_fun)
        BOOST_LOG_FILTER_ATTR_MEMBER(ends_with, boost::log::aux::ends_with_fun)
        BOOST_LOG_FILTER_ATTR_MEMBER(contains, boost::log::aux::contains_fun)

        //! Filter generator for checking whether the attribute value matches a regex
        template< typename RegexTraitsT >
        flt_attr<
            char_type,
            boost::log::aux::binder2nd< boost::log::aux::matches_fun, basic_regex< char_type, RegexTraitsT > >,
            attribute_value_types
        > matches(basic_regex< char_type, RegexTraitsT > const& expr, match_flag_type flags = match_default) const
        {
            typedef boost::log::aux::binder2nd<
                boost::log::aux::matches_fun,
                basic_regex< char_type, RegexTraitsT >
            > binder_t;
            typedef flt_attr< char_type, binder_t, attribute_value_types > flt_attr_t;
            return flt_attr_t(this->m_AttributeName, binder_t(boost::log::aux::matches_fun(flags), expr));
        }

        //! Filter generator for checking whether the attribute value matches a regex
        flt_attr<
            char_type,
            boost::log::aux::binder2nd< boost::log::aux::matches_fun, basic_regex< char_type, regex_traits< char_type > > >,
            attribute_value_types
        > matches(string_type const& expr, match_flag_type flags = match_default) const
        {
            basic_regex< char_type, regex_traits< char_type > > ex(expr.c_str());
            return this->matches(ex, flags);
        }
    };

#undef BOOST_LOG_FILTER_ATTR_MEMBER

    //! The MPL predicate detects if the type is STL string
    template< typename T >
    struct is_string : mpl::false_ {};
    template< typename CharT, typename TraitsT, typename AllocatorT >
    struct is_string< std::basic_string< CharT, TraitsT, AllocatorT > > : mpl::true_ {};

    //! The metafunction replaces 1-sized MPL sequences with a single equivalent type
    template< typename TypesT, bool = mpl::is_sequence< TypesT >::value >
    struct simplify_type_sequence;
    template< typename TypesT >
    struct simplify_type_sequence< TypesT, false >
    {
        typedef TypesT type;
        enum { is_only_string = is_string< TypesT >::value };
    };
    template< typename TypesT >
    struct simplify_type_sequence< TypesT, true >
    {
    private:
        typedef typename mpl::eval_if<
            mpl::equal_to< mpl::size< TypesT >, mpl::int_< 1 > >,
            mpl::at_c< TypesT, 0 >,
            mpl::identity< TypesT >
        >::type actual_types;

    public:
        typedef actual_types type;
        enum { is_only_string = is_string< actual_types >::value };
    };

    //! Attribute filter generator
    template< typename CharT, typename AttributeValueTypesT >
    class flt_attr_gen :
        public flt_attr_gen_base<
            CharT,
            typename simplify_type_sequence< AttributeValueTypesT >::type,
            simplify_type_sequence< AttributeValueTypesT >::is_only_string
        >
    {
        typedef flt_attr_gen_base<
            CharT,
            typename simplify_type_sequence< AttributeValueTypesT >::type,
            simplify_type_sequence< AttributeValueTypesT >::is_only_string
        > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        explicit flt_attr_gen(string_type const& name) : base_type(name) {}
    };

} // namespace aux

//! Filter generator
template< typename AttributeValueTypesT, typename CharT >
aux::flt_attr_gen< CharT, AttributeValueTypesT > attr(
    const CharT* name BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(AttributeValueTypesT))
{
    return aux::flt_attr_gen< CharT, AttributeValueTypesT >(name);
}

//! Filter generator
template< typename AttributeValueTypesT, typename CharT >
aux::flt_attr_gen< CharT, AttributeValueTypesT > attr(
    std::basic_string< CharT > const& name BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(AttributeValueTypesT))
{
    return aux::flt_attr_gen< CharT, AttributeValueTypesT >(name);
}

} // namespace filters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FILTERS_ATTR_HPP_INCLUDED_