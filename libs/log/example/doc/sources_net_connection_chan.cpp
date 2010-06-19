/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstddef>
#include <string>
#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/log/core.hpp>
#include <boost/log/filters.hpp>
#include <boost/log/formatters.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/sources/channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/scoped_attribute.hpp>
#include <boost/log/utility/init/common_attributes.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace flt = boost::log::filters;
namespace fmt = boost::log::formatters;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

//[ example_sources_network_connection_channels
class network_connection
{
    src::channel_logger< > m_net, m_stat;
    src::channel_logger< >::attribute_set_type::iterator
        m_net_remote_addr, m_stat_remote_addr;

public:
    network_connection() :
        // We can dump network-related messages through this logger
        // and be able to filter them later
        m_net(keywords::channel = "net"),
        // We also can separate statistic records in a different channel
        // in order to route them to a different sink
        m_stat(keywords::channel = "stat")
    {
    }

    void on_connected(std::string const& remote_addr)
    {
        // Add the remote address to both channels
        boost::shared_ptr< attrs::constant< std::string > > addr =
            boost::make_shared< attrs::constant< std::string > >(remote_addr);
        m_net_remote_addr = m_net.add_attribute("RemoteAddress", addr).first;
        m_stat_remote_addr = m_stat.add_attribute("RemoteAddress", addr).first;

        // Put message to the "net" channel
        BOOST_LOG(m_net) << "Connection established";
    }
    void on_disconnected()
    {
        // Put message to the "net" channel
        BOOST_LOG(m_net) << "Connection shut down";

        // Remove the attribute with the remote address
        m_net.remove_attribute(m_net_remote_addr);
        m_stat.remove_attribute(m_stat_remote_addr);
    }
    void on_data_received(std::size_t size)
    {
        BOOST_LOG_SCOPED_LOGGER_TAG(m_stat, "ReceivedSize", std::size_t, size);
        BOOST_LOG(m_stat) << "Some data received";
    }
    void on_data_sent(std::size_t size)
    {
        BOOST_LOG_SCOPED_LOGGER_TAG(m_stat, "SentSize", std::size_t, size);
        BOOST_LOG(m_stat) << "Some data sent";
    }
};
//]

int main(int, char*[])
{
    // Construct the sink for the "net" channel
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
    boost::shared_ptr< text_sink > pSink = boost::make_shared< text_sink >();

    pSink->locked_backend()->add_stream(
        boost::make_shared< std::ofstream >("net.log"));

    pSink->locked_backend()->set_formatter
    (
        fmt::stream
            << fmt::attr< unsigned int >("LineID")
            << ": [" << fmt::attr< std::string >("RemoteAddress") << "] "
            << fmt::message()
    );

    pSink->set_filter(flt::attr< std::string >("Channel") == "net");

    logging::core::get()->add_sink(pSink);

    // Construct the sink for the "stat" channel
    pSink = boost::make_shared< text_sink >();

    pSink->locked_backend()->add_stream(
        boost::make_shared< std::ofstream >("stat.log"));

    pSink->locked_backend()->set_formatter
    (
        fmt::stream
            << fmt::attr< std::string >("RemoteAddress")
            << fmt::if_(flt::has_attr("ReceivedSize"))
               [
                    fmt::stream << " -> " << fmt::attr< std::size_t >("ReceivedSize") << " bytes: "
               ]
            << fmt::if_(flt::has_attr("SentSize"))
               [
                    fmt::stream << " <- " << fmt::attr< std::size_t >("SentSize") << " bytes: "
               ]
            << fmt::message()
    );

    pSink->set_filter(flt::attr< std::string >("Channel") == "stat");

    logging::core::get()->add_sink(pSink);

    // Register other common attributes, such as time stamp and record counter
    logging::add_common_attributes();

    // Emulate network activity
    network_connection conn;

    conn.on_connected("11.22.33.44");
    conn.on_data_received(123);
    conn.on_data_sent(321);
    conn.on_disconnected();

    return 0;
}