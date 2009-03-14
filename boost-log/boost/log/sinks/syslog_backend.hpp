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
 * \file   syslog_backend.hpp
 * \author Andrey Semashev
 * \date   08.01.2008
 *
 * The header contains implementation of a Syslog sink backend along with its setup facilities.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_SYSLOG_BACKEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_SYSLOG_BACKEND_HPP_INCLUDED_

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/function/function1.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/asio_fwd.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/syslog_constants.hpp>
#include <boost/log/sinks/attribute_mapping.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>
#include <boost/log/keywords/facility.hpp>
#include <boost/log/keywords/use_impl.hpp>
#include <boost/log/keywords/ip_version.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sinks {

//! Supported IP protocol versions
enum ip_versions
{
    v4,
    v6
};

namespace syslog {

    //! The enumeration defined the possible implementation types for the syslog backend
    enum impl_types
    {
#ifdef BOOST_LOG_USE_NATIVE_SYSLOG
        native = 0,             //!< Use native syslog API
#endif
        udp_socket_based = 1    //!< Use UDP sockets, according to RFC3164
    };

    /*!
     * \brief Straightforward severity level mapping
     *
     * This type of mapping assumes that attribute with a particular name always
     * provides values that map directly onto the Syslog levels. The mapping
     * simply returns the extracted attribute value converted to the Syslog severity level.
     */
    template< typename CharT, typename AttributeValueT = int >
    class basic_direct_severity_mapping :
        public basic_direct_mapping< CharT, level_t, AttributeValueT >
    {
        //! Base type
        typedef basic_direct_mapping< CharT, level_t, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit basic_direct_severity_mapping(string_type const& name) :
            base_type(name, info)
        {
        }
    };

    /*!
     * \brief Customizable severity level mapping
     *
     * The class allows to setup a custom mapping between an attribute and Syslog severity levels.
     * The mapping should be initialized similarly to the standard \c map container, by using
     * indexing operator and assignment.
     */
    template< typename CharT, typename AttributeValueT = int >
    class basic_custom_severity_mapping :
        public basic_custom_mapping< CharT, level_t, AttributeValueT >
    {
        //! Base type
        typedef basic_custom_mapping< CharT, level_t, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit basic_custom_severity_mapping(string_type const& name) :
            base_type(name, info)
        {
        }
    };

#ifdef BOOST_LOG_USE_CHAR

    /*!
     * \brief Straightforward severity level mapping
     *
     * This is a convenience template typedef over \c basic_direct_severity_mapping
     * for narrow-character logging.
     */
    template< typename AttributeValueT = int >
    class direct_severity_mapping :
        public basic_direct_severity_mapping< char, AttributeValueT >
    {
        //! Base type
        typedef basic_direct_severity_mapping< char, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit direct_severity_mapping(string_type const& name) : base_type(name)
        {
        }
    };

    /*!
     * \brief Customizable severity level mapping
     *
     * This is a convenience template typedef over \c basic_custom_severity_mapping
     * for narrow-character logging.
     */
    template< typename AttributeValueT = int >
    class custom_severity_mapping :
        public basic_custom_severity_mapping< char, AttributeValueT >
    {
        //! Base type
        typedef basic_custom_severity_mapping< char, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit custom_severity_mapping(string_type const& name) : base_type(name)
        {
        }
    };

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

    /*!
     * \brief Straightforward severity level mapping
     *
     * This is a convenience template typedef over \c basic_direct_severity_mapping
     * for wide-character logging.
     */
    template< typename AttributeValueT = int >
    class wdirect_severity_mapping :
        public basic_direct_severity_mapping< wchar_t, AttributeValueT >
    {
        //! Base type
        typedef basic_direct_severity_mapping< wchar_t, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit wdirect_severity_mapping(string_type const& name) : base_type(name)
        {
        }
    };

    /*!
     * \brief Customizable severity level mapping
     *
     * This is a convenience template typedef over \c basic_custom_severity_mapping
     * for wide-character logging.
     */
    template< typename AttributeValueT = int >
    class wcustom_severity_mapping :
        public basic_custom_severity_mapping< wchar_t, AttributeValueT >
    {
        //! Base type
        typedef basic_custom_severity_mapping< wchar_t, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit wcustom_severity_mapping(string_type const& name) : base_type(name)
        {
        }
    };

#endif // BOOST_LOG_USE_WCHAR_T

} // namespace syslog

/*!
 * \brief An implementation of a syslog sink backend
 *
 * The backend provides support for the syslog protocol, defined in RFC3164.
 * The backend sends log records to a remote host via UDP. The host name can
 * be specified by calling the \c set_target_address method. By default log
 * records will be sent to localhost:514. The local address can be specified
 * as well, by calling the \c set_local_address method. By default syslog
 * packets will be sent from any local address available.
 *
 * It is safe to create several sink backends with the same local addresses -
 * the backends within the process will share the same socket. The same applies
 * to different processes that use the syslog backends to send records from
 * the same socket. However, it is not guaranteed to work if some third party
 * facility is using the socket.
 *
 * On systems with native syslog implementation it may be preferable to utilize
 * the POSIX syslog API instead of direct socket management in order to bypass
 * possible security limitations that may be in action. To do so one has to pass
 * the <tt>use_impl = native</tt> to the backend constructor. Note, however,
 * that in that case you will only have one chance to specify syslog facility - on
 * the first native syslog backend construction. Other native syslog backends will
 * ignore this parameter. Obviously, the \c set_local_address and \c set_target_address
 * methods have no effect for native backends. Using <tt>use_impl = native</tt>
 * on platforms with no native support for POSIX syslog API will have no effect.
 */
template< typename CharT >
class basic_syslog_backend :
    public basic_formatting_sink_backend< CharT, char >
{
    //! Base type
    typedef basic_formatting_sink_backend< CharT, char > base_type;
    //! Implementation type
    struct implementation;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type
    typedef typename base_type::string_type string_type;
    //! String type that is used to pass message test
    typedef typename base_type::target_string_type target_string_type;
    //! Attribute values view type
    typedef typename base_type::values_view_type values_view_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

    //! Syslog severity level mapper type
    typedef boost::function1<
        syslog::level_t,
        values_view_type const&
    > severity_mapper_type;

private:
    //! Pointer to the implementation
    implementation* m_pImpl;

public:
    /*!
     * Constructor. Creates a UDP socket-based backend with <tt>syslog::user</tt> facility code.
     * IPv4 protocol will be used.
     */
    explicit basic_syslog_backend() : m_pImpl(construct(syslog::user, syslog::udp_socket_based, v4))
    {
    }
    /*!
     * Constructor. Creates a sink backend with the specified named parameters.
     * The following named parameters are supported:
     *
     * \li \c facility - Specifies the facility code. If not specified, <tt>syslog::user</tt> will be used.
     * \li \c use_impl - Specifies the backend implementation. Can be one of:
     *                   \li \c native - Use the native syslog API, if available. If no native API
     *                                   is available, it is equivalent to \c udp_socket_based.
     *                   \li \c udp_socket_based - Use the UDP socket-based implementation, conforming to
     *                                             RFC3164 protocol specification. This is the default.
     * \li \c ip_version - Specifies IP protocol version to use, in case if socket-based implementation
     *                     is used. Can be either v4 (the default one) or v6.
     */
    template< typename ArgsT >
    explicit basic_syslog_backend(ArgsT const& args) :
        m_pImpl(construct(
            args[keywords::facility | syslog::user],
            args[keywords::use_impl | syslog::udp_socket_based],
            args[keywords::ip_version | v4]))
    {
    }
    /*!
     * Destructor
     */
    BOOST_LOG_EXPORT ~basic_syslog_backend();

    /*!
     * The method installs the function object that maps application severity levels to syslog levels
     */
    BOOST_LOG_EXPORT void set_severity_mapper(severity_mapper_type const& mapper);

    /*!
     * The method sets the local host name which log records will be sent from. The host name
     * is resolved to obtain the final IP address.
     *
     * \note Does not have effect if the backend was constructed to use native syslog API
     *
     * \param addr The local address
     * \param port The local port number
     */
    BOOST_LOG_EXPORT void set_local_address(std::string const& addr, unsigned short port = 514);
    /*!
     * The method sets the local address which log records will be sent from.
     *
     * \note Does not have effect if the backend was constructed to use native syslog API
     *
     * \param addr The local address
     * \param port The local port number
     */
    BOOST_LOG_EXPORT void set_local_address(boost::asio::ip::address const& addr, unsigned short port = 514);

    /*!
     * The method sets the remote host name where log records will be sent to. The host name
     * is resolved to obtain the final IP address.
     *
     * \note Does not have effect if the backend was constructed to use native syslog API
     *
     * \param addr The remote host address
     * \param port The port number on the remote host
     */
    BOOST_LOG_EXPORT void set_target_address(std::string const& addr, unsigned short port = 514);
    /*!
     * The method sets the address of the remote host where log records will be sent to.
     *
     * \note Does not have effect if the backend was constructed to use native syslog API
     *
     * \param addr The remote host address
     * \param port The port number on the remote host
     */
    BOOST_LOG_EXPORT void set_target_address(boost::asio::ip::address const& addr, unsigned short port = 514);

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! The method passes the formatted message to the Syslog API
    BOOST_LOG_EXPORT void do_consume(record_type const& record, target_string_type const& formatted_message);

    //! The method creates the backend implementation
    BOOST_LOG_EXPORT static implementation* construct(
        syslog::facility_t facility, syslog::impl_types use_impl, ip_versions ip_version);
#endif // BOOST_LOG_DOXYGEN_PASS
};

#ifdef BOOST_LOG_USE_CHAR
typedef basic_syslog_backend< char > syslog_backend;        //!< Convenience typedef for narrow-character logging
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_syslog_backend< wchar_t > wsyslog_backend;    //!< Convenience typedef for wide-character logging
#endif

} // namespace sinks

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SINKS_SYSLOG_BACKEND_HPP_INCLUDED_
