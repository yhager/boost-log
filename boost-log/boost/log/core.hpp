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
 * \file   core.hpp
 * \author Andrey Semashev
 * \date   19.04.2007
 *
 * This header contains logging core class definition.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_CORE_HPP_INCLUDED_
#define BOOST_LOG_CORE_HPP_INCLUDED_

#include <string>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function/function1.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/attribute_set.hpp>

#ifdef _MSC_VER
#pragma warning(push)
 // non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sinks {

template< typename CharT >
class sink;

} // namespace sinks

/*!
 * \brief Logging library core class
 *
 * The logging core is used to interconnect log sources and sinks. It also provides
 * a number of basic features, like global filtering and global and thread-specific attribute storage.
 *
 * The logging core is a singleton. Users can acquire the core instance by calling the static method <tt>get</tt>.
 */
template< typename CharT >
class BOOST_LOG_EXPORT basic_core : noncopyable
{
public:
    //! Character type
    typedef CharT char_type;
    //! String type to be used as a message text holder
    typedef std::basic_string< char_type > string_type;
    //! Attribute set type
    typedef basic_attribute_set< char_type > attribute_set_type;
    //! Attribute values view type
    typedef basic_attribute_values_view< char_type > values_view_type;
    //! Sink interface type
    typedef sinks::sink< char_type > sink_type;
    //! Filter function type
    typedef function1< bool, values_view_type const& > filter_type;

private:
    //! Implementation type
    struct implementation;
    friend struct implementation;

private:
    //! A pointer to the implementation
    implementation* pImpl;

private:
    //! \cond
    basic_core();
    //! \endcond

public:
    /*!
     * Destructor. Destroys the core, releases any sinks and attributes that were registered.
     */
    ~basic_core();

    /*!
     * \return The method returns a pointer to the logging core singleton instance.
     */
    static shared_ptr< basic_core > get();

    /*!
     * The method enables or disables logging.
     *
     * Setting this status to \c false allows you to completely wipe out any logging activity, including
     * filtering and generation of attribute values. It is useful if you want to completely disable logging
     * in a running application. The state of logging does not alter any other properties of the logging
     * library, such as filters or sinks, so you can enable logging with the very same settings that you had
     * when the logging was disabled.
     * This feature may also be useful if you want to perform major changes to logging configuration and
     * don't want your application to block on opening or pushing a log record.
     *
     * By default logging is enabled.
     *
     * \param enabled The actual flag of logging activity.
     * \return The previous value of enabled/disabled logging flag
     */
    bool set_logging_enabled(bool enabled = true);

    /*!
     * The method sets the global logging filter. The filter is applied to every log record that is processed.
     *
     * \param filter The filter function object to be installed.
     */
    void set_filter(filter_type const& filter);
    /*!
     * The method removes the global logging filter. All log records are passed to sinks without global filtering applied.
     */
    void reset_filter();

    /*!
     * The method adds a new sink. The sink is included into logging process immediately after being added and until being removed.
     * No sink can be added more than once at the same time. If the sink is already registered, the call is ignored.
     *
     * \param s The sink to be registered.
     */
    void add_sink(shared_ptr< sink_type > const& s);
    /*!
     * The method removes the sink from the output. The sink will not receive any log records after removal.
     * The call has no effect if the sink is not registered.
     *
     * \param s The sink to be unregistered.
     */
    void remove_sink(shared_ptr< sink_type > const& s);

    /*!
     * The method adds an attribute to the global attribute set. The attribute will be implicitly added to every log record.
     *
     * \param name The attribute name.
     * \param attr Pointer to the attribute. Must not be NULL.
     * \return A pair of values. If the second member is \c true, then the attribute is added and the first member points to the
     *         attribute. Otherwise the attribute was not added and the first member points to the attribute that prevents
     *         addition.
     */
    std::pair< typename attribute_set_type::iterator, bool > add_global_attribute(
        string_type const& name, shared_ptr< attribute > const& attr);
    /*!
     * The method removes an attribute from the global attribute set.
     *
     * \pre The attribute was added with the \c add_global_attribute call.
     * \post The attribute is no longer registered as a global attribute. The iterator is invalidated after removal.
     *
     * \param it Iterator to the previously added attribute.
     */
    void remove_global_attribute(typename attribute_set_type::iterator it);

    /*!
     * The method returns copy of the complete set of currently registered global attributes.
     */
    attribute_set_type get_global_attributes() const;
    /*!
     * The method replaces the complete set of currently registered global attributes with the provided set.
     *
     * \note The method invalidates all iterators that may have been returned
     *       from the \c add_global_attribute method.
     *
     * \param attrs The set of attributes to be installed.
     */
    void set_global_attributes(attribute_set_type const& attrs) const;

    /*!
     * The method adds an attribute to the thread-specific attribute set. The attribute will be implicitly added to
     * every log record made in the current thread.
     *
     * \note In single-threaded build the effect is the same as adding the attribute globally. This, however, does
     *       not imply that iterators to thread-specific and global attributes are interchangable.
     *
     * \param name The attribute name.
     * \param attr Pointer to the attribute. Must not be NULL.
     * \return A pair of values. If the second member is \c true, then the attribute is added and the first member points to the
     *         attribute. Otherwise the attribute was not added and the first member points to the attribute that prevents
     *         addition.
     */
    std::pair< typename attribute_set_type::iterator, bool > add_thread_attribute(
        string_type const& name, shared_ptr< attribute > const& attr);
    /*!
     * The method removes an attribute from the thread-specific attribute set.
     *
     * \pre The attribute was added with the \c add_thread_attribute call.
     * \post The attribute is no longer registered as a thread-specific attribute. The iterator is invalidated after removal.
     *
     * \param it Iterator to the previously added attribute.
     */
    void remove_thread_attribute(typename attribute_set_type::iterator it);

    /*!
     * The method returns copy of the complete set of currently registered thread-specific attributes.
     */
    attribute_set_type get_thread_attributes() const;
    /*!
     * The method replaces the complete set of currently registered thread-specific attributes with the provided set.
     *
     * \note The method invalidates all iterators that may have been returned
     *       from the \c add_thread_attribute method.
     *
     * \param attrs The set of attributes to be installed.
     */
    void set_thread_attributes(attribute_set_type const& attrs) const;

    /*!
     * The method attempts to open a new record to be written. While attempting to open a log record all filtering is applied.
     * A successfully opened record must be either cancelled by calling \c cancel_record or pushed further to sinks by calling
     * the \c push_record method.
     *
     * More than one open records are allowed, such records exist independently. All attribute values are acquired during opening
     * the record and do not interact between records. However, only the last-open record remains active all the time, and
     * consequent record cancellation or pushing will dispatch only the last-open record, making the previously open one active,
     * and so on.
     *
     * \param source_attributes The set of source-specific attributes to be attached to the record to be opened.
     * \return \c true if the record is opened, \c false if not (e.g. because it didn't pass filtering).
     *
     * \b Throws: Nothing.
     */
    bool open_record(attribute_set_type const& source_attributes);
    /*!
     * The method pushes the record to sinks and closes it. Results are undefined if called before opening a record.
     *
     * \param message_text The formatted log message text.
     *
     * \b Throws: Nothing.
     */
    void push_record(string_type const& message_text);
    /*!
     * The method cancels the currently opened record. The record is not passed to any sinks. Results are undefined if called
     * before opening a record.
     *
     * \b Throws: Nothing.
     */
    void cancel_record();
};

#ifdef BOOST_LOG_USE_CHAR
typedef basic_core< char > core;        //!< Convenience typedef for narrow-character logging
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_core< wchar_t > wcore;    //!< Convenience typedef for wide-character logging
#endif

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_CORE_HPP_INCLUDED_
