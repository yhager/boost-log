/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attr_attribute_values_view.cpp
 * \author Andrey Semashev
 * \date   24.01.2009
 *
 * \brief  This header contains tests for the attribute values view.
 */

#define BOOST_TEST_MODULE attr_attribute_values_view

#include <vector>
#include <string>
#include <utility>
#include <iterator>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/config.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/utility/type_dispatch/static_type_dispatcher.hpp>
#include "char_definitions.hpp"

namespace logging = boost::log;
namespace attrs = logging::attributes;

namespace {

    //! A simple attribute value receiver functional object
    template< typename T >
    struct receiver
    {
        typedef void result_type;
        receiver(T& val) : m_Val(val) {}
        result_type operator() (T const& val) const
        {
            m_Val = val;
        }

    private:
        T& m_Val;
    };

    //! The function extracts attribute value
    template< typename T >
    inline bool get_attr_value(logging::attribute_value const& val, T& res)
    {
        receiver< T > r(res);
        logging::static_type_dispatcher< T > disp(r);
        return val.dispatch(disp);
    }

} // namespace

// The test checks construction and assignment
BOOST_AUTO_TEST_CASE_TEMPLATE(construction, CharT, char_types)
{
    typedef logging::basic_attribute_set< CharT > attr_set;
    typedef logging::basic_attribute_values_view< CharT > values_view;
    typedef test_data< CharT > data;

    boost::shared_ptr< logging::attribute > attr1(new attrs::constant< int >(10));
    boost::shared_ptr< logging::attribute > attr2(new attrs::constant< double >(5.5));
    boost::shared_ptr< logging::attribute > attr3(new attrs::constant< std::string >("Hello, world!"));
    boost::shared_ptr< logging::attribute > attr4(new attrs::constant< char >('L'));

    {
        attr_set set1, set2, set3;
        set1[data::attr1()] = attr1;
        set1[data::attr2()] = attr2;
        set1[data::attr3()] = attr3;

        values_view view1(set1, set2, set3);
        view1.freeze();

        BOOST_CHECK(!view1.empty());
        BOOST_CHECK_EQUAL(view1.size(), 3UL);
    }
    {
        attr_set set1, set2, set3;
        set1[data::attr1()] = attr1;
        set2[data::attr2()] = attr2;
        set3[data::attr3()] = attr3;

        values_view view1(set1, set2, set3);
        view1.freeze();

        BOOST_CHECK(!view1.empty());
        BOOST_CHECK_EQUAL(view1.size(), 3UL);

        values_view view2 = view1;
        BOOST_CHECK(!view2.empty());
        BOOST_CHECK_EQUAL(view2.size(), 3UL);
    }

    // Check that the more prioritized attributes replace the less ones
    {
        boost::shared_ptr< logging::attribute > attr2_2(new attrs::constant< int >(20));
        boost::shared_ptr< logging::attribute > attr4_2(new attrs::constant< double >(10.3));
        boost::shared_ptr< logging::attribute > attr3_3(new attrs::constant< float >(static_cast< float >(-7.2)));
        boost::shared_ptr< logging::attribute > attr4_3(new attrs::constant< unsigned int >(5));

        attr_set set1, set2, set3;
        set3[data::attr1()] = attr1;
        set3[data::attr2()] = attr2;
        set3[data::attr3()] = attr3;
        set3[data::attr4()] = attr4;

        set2[data::attr2()] = attr2_2;
        set2[data::attr4()] = attr4_2;

        set1[data::attr3()] = attr3_3;
        set1[data::attr4()] = attr4_3;

        values_view view1(set1, set2, set3);
        view1.freeze();

        BOOST_CHECK(!view1.empty());
        BOOST_CHECK_EQUAL(view1.size(), 4UL);

        int n = 0;
        BOOST_CHECK(logging::visit< int >(data::attr1(), view1, receiver< int >(n)));
        BOOST_CHECK_EQUAL(n, 10);

        BOOST_CHECK(logging::visit< int >(data::attr2(), view1, receiver< int >(n)));
        BOOST_CHECK_EQUAL(n, 20);

        float f = static_cast< float >(0.0);
        BOOST_CHECK(logging::visit< float >(data::attr3(), view1, receiver< float >(f)));
        BOOST_CHECK_CLOSE(f, static_cast< float >(-7.2), static_cast< float >(0.001));

        unsigned int m = 0;
        BOOST_CHECK(logging::visit< unsigned int >(data::attr4(), view1, receiver< unsigned int >(m)));
        BOOST_CHECK_EQUAL(m, 5U);
    }
}

// The test checks lookup methods
BOOST_AUTO_TEST_CASE_TEMPLATE(lookup, CharT, char_types)
{
    typedef logging::basic_attribute_set< CharT > attr_set;
    typedef logging::basic_attribute_values_view< CharT > values_view;
    typedef test_data< CharT > data;
    typedef std::basic_string< CharT > string;

    boost::shared_ptr< logging::attribute > attr1(new attrs::constant< int >(10));
    boost::shared_ptr< logging::attribute > attr2(new attrs::constant< double >(5.5));
    boost::shared_ptr< logging::attribute > attr3(new attrs::constant< std::string >("Hello, world!"));

    attr_set set1, set2, set3;
    set1[data::attr1()] = attr1;
    set1[data::attr2()] = attr2;
    set1[data::attr3()] = attr3;

    values_view view1(set1, set2, set3);
    view1.freeze();

    // Traditional find methods
    typename values_view::const_iterator it = view1.find(data::attr1());
    BOOST_CHECK(it != view1.end());
    BOOST_CHECK(it->first == data::attr1());
    int val1 = 0;
    BOOST_CHECK(get_attr_value(it->second, val1));
    BOOST_CHECK_EQUAL(val1, 10);

    string s1 = data::attr2();
    it = view1.find(s1);
    BOOST_CHECK(it != view1.end());
    BOOST_CHECK(it->first == data::attr2());
    double val2 = 0;
    BOOST_CHECK(get_attr_value(it->second, val2));
    BOOST_CHECK_CLOSE(val2, 5.5, 0.001);

    it = view1.find(data::attr3());
    BOOST_CHECK(it != view1.end());
    BOOST_CHECK(it->first == data::attr3());
    std::string val3;
    BOOST_CHECK(get_attr_value(it->second, val3));
    BOOST_CHECK_EQUAL(val3, "Hello, world!");

    // make an additional check that the result is absent if the value type does not match the requested type
    BOOST_CHECK(!get_attr_value(it->second, val2));

    it = view1.find(data::attr4());
    BOOST_CHECK(it == view1.end());

    // Subscript operator
    logging::attribute_value p = view1[data::attr1()];
    BOOST_CHECK_EQUAL(view1.size(), 3UL);
    BOOST_CHECK(!!p);
    BOOST_CHECK(get_attr_value(p, val1));
    BOOST_CHECK_EQUAL(val1, 10);

    p = view1[s1];
    BOOST_CHECK_EQUAL(view1.size(), 3UL);
    BOOST_CHECK(!!p);
    BOOST_CHECK(get_attr_value(p, val2));
    BOOST_CHECK_CLOSE(val2, 5.5, 0.001);

    p = view1[data::attr3()];
    BOOST_CHECK_EQUAL(view1.size(), 3UL);
    BOOST_CHECK(!!p);
    BOOST_CHECK(get_attr_value(p, val3));
    BOOST_CHECK_EQUAL(val3, "Hello, world!");

    p = view1[data::attr4()];
    BOOST_CHECK(!p);
    BOOST_CHECK_EQUAL(view1.size(), 3UL);

    // Counting elements
    BOOST_CHECK_EQUAL(view1.count(data::attr1()), 1UL);
    BOOST_CHECK_EQUAL(view1.count(s1), 1UL);
    BOOST_CHECK_EQUAL(view1.count(data::attr3()), 1UL);
    BOOST_CHECK_EQUAL(view1.count(data::attr4()), 0UL);
}
