/*
 *          Copyright Andrey Semashev 2007 - 2012.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attr_value_visitation.cpp
 * \author Andrey Semashev
 * \date   21.01.2009
 *
 * \brief  This header contains tests for the attribute value extraction helpers.
 */

#define BOOST_TEST_MODULE attr_value_visitation

#include <string>
#include <boost/mpl/vector.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>
#include "char_definitions.hpp"

namespace mpl = boost::mpl;
namespace logging = boost::log;
namespace attrs = logging::attributes;

namespace {

    // The receiver functional object that verifies the extracted attribute values
    struct my_receiver
    {
        typedef void result_type;

        enum type_expected
        {
            none_expected,
            int_expected,
            double_expected,
            string_expected
        };

        my_receiver() : m_Expected(none_expected), m_Int(0), m_Double(0.0) {}

        void set_expected()
        {
            m_Expected = none_expected;
        }
        void set_expected(int value)
        {
            m_Expected = int_expected;
            m_Int = value;
        }
        void set_expected(double value)
        {
            m_Expected = double_expected;
            m_Double = value;
        }
        void set_expected(std::string const& value)
        {
            m_Expected = string_expected;
            m_String = value;
        }

        // Implement visitation logic for all supported types
        void operator() (int const& value)
        {
            BOOST_CHECK_EQUAL(m_Expected, int_expected);
            BOOST_CHECK_EQUAL(m_Int, value);
        }
        void operator() (double const& value)
        {
            BOOST_CHECK_EQUAL(m_Expected, double_expected);
            BOOST_CHECK_CLOSE(m_Double, value, 0.001);
        }
        void operator() (std::string const& value)
        {
            BOOST_CHECK_EQUAL(m_Expected, string_expected);
            BOOST_CHECK_EQUAL(m_String, value);
        }
        void operator() (char value)
        {
            // This one should not be called
            BOOST_ERROR("The unexpected operator() has been called");
        }

    private:
        type_expected m_Expected;
        int m_Int;
        double m_Double;
        std::string m_String;
    };

} // namespace

// The test checks invokers specialized on a single attribute value type
BOOST_AUTO_TEST_CASE_TEMPLATE(single_type, CharT, char_types)
{
    typedef logging::basic_attribute_set< CharT > attr_set;
    typedef logging::basic_attribute_values_view< CharT > values_view;
    typedef test_data< CharT > data;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);
    attrs::constant< std::string > attr3("Hello, world!");

    attr_set set1, set2, set3;
    set1[data::attr1()] = attr1;
    set1[data::attr2()] = attr2;

    values_view view1(set1, set2, set3);
    view1.freeze();

    my_receiver recv;

    logging::value_visitor_invoker< CharT, int > invoker1(data::attr1());
    logging::value_visitor_invoker< CharT, double > invoker2(data::attr2());
    logging::value_visitor_invoker< CharT, std::string > invoker3(data::attr3());
    logging::value_visitor_invoker< CharT, char > invoker4(data::attr1());
    logging::value_visitor_invoker< CharT, int > invoker5(data::attr2());

    // These two extractors will find their values in the view
    recv.set_expected(10);
    BOOST_CHECK(invoker1(view1, recv));

    recv.set_expected(5.5);
    BOOST_CHECK(invoker2(view1, recv));

    // This one will not
    recv.set_expected();
    BOOST_CHECK(!invoker3(view1, recv));

    // But it will find it in this view
    set1[data::attr3()] = attr3;

    values_view view2(set1, set2, set3);
    view2.freeze();

    recv.set_expected("Hello, world!");
    BOOST_CHECK(invoker3(view2, recv));

    // This one will find the sought attribute value, but it will have an incorrect type
    recv.set_expected();
    BOOST_CHECK(!invoker4(view1, recv));

    // This one is the same, but there is a value of the requested type in the view
    BOOST_CHECK(!invoker5(view1, recv));
}


// The test checks invokers specialized with type lists
BOOST_AUTO_TEST_CASE_TEMPLATE(multiple_types, CharT, char_types)
{
    typedef logging::basic_attribute_set< CharT > attr_set;
    typedef logging::basic_attribute_values_view< CharT > values_view;
    typedef test_data< CharT > data;
    typedef mpl::vector< int, double, std::string, char >::type types;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);
    attrs::constant< std::string > attr3("Hello, world!");

    attr_set set1, set2, set3;
    set1[data::attr1()] = attr1;
    set1[data::attr2()] = attr2;

    values_view view1(set1, set2, set3);
    view1.freeze();

    my_receiver recv;

    logging::value_visitor_invoker< CharT, types > invoker1(data::attr1());
    logging::value_visitor_invoker< CharT, types > invoker2(data::attr2());
    logging::value_visitor_invoker< CharT, types > invoker3(data::attr3());

    // These two extractors will find their values in the view
    recv.set_expected(10);
    BOOST_CHECK(invoker1(view1, recv));

    recv.set_expected(5.5);
    BOOST_CHECK(invoker2(view1, recv));

    // This one will not
    recv.set_expected();
    BOOST_CHECK(!invoker3(view1, recv));

    // But it will find it in this view
    set1[data::attr3()] = attr3;

    values_view view2(set1, set2, set3);
    view2.freeze();

    recv.set_expected("Hello, world!");
    BOOST_CHECK(invoker3(view2, recv));
}

// The test verifies the visit function
BOOST_AUTO_TEST_CASE_TEMPLATE(visit_function, CharT, char_types)
{
    typedef logging::basic_attribute_set< CharT > attr_set;
    typedef logging::basic_attribute_values_view< CharT > values_view;
    typedef test_data< CharT > data;
    typedef mpl::vector< int, double, std::string, char >::type types;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);
    attrs::constant< std::string > attr3("Hello, world!");

    attr_set set1, set2, set3;
    set1[data::attr1()] = attr1;
    set1[data::attr2()] = attr2;

    values_view view1(set1, set2, set3);
    view1.freeze();

    my_receiver recv;

    // These two extractors will find their values in the view
    recv.set_expected(10);
    BOOST_CHECK(logging::visit< types >(data::attr1(), view1, recv));

    recv.set_expected(5.5);
    BOOST_CHECK(logging::visit< double >(data::attr2(), view1, recv));

    // These will not
    recv.set_expected();
    BOOST_CHECK(!logging::visit< types >(data::attr3(), view1, recv));
    BOOST_CHECK(!logging::visit< char >(data::attr1(), view1, recv));

    // But it will find it in this view
    set1[data::attr3()] = attr3;

    values_view view2(set1, set2, set3);
    view2.freeze();

    recv.set_expected("Hello, world!");
    BOOST_CHECK(logging::visit< std::string >(data::attr3(), view2, recv));
}
