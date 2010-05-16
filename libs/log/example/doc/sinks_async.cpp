/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <string>
#include <fstream>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/log/core.hpp>
#include <boost/log/filters.hpp>
#include <boost/log/formatters.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/empty_deleter.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace flt = boost::log::filters;
namespace fmt = boost::log::formatters;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//[ example_sinks_async_init
// Complete sink type
typedef sinks::asynchronous_sink< sinks::text_ostream_backend > sink_t;

boost::shared_ptr< sink_t > init_logging()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    // Create a backend and initialize it with a stream
    boost::shared_ptr< sinks::text_ostream_backend > backend =
        boost::make_shared< sinks::text_ostream_backend >();
    backend->add_stream(
        boost::shared_ptr< std::ostream >(&std::clog, logging::empty_deleter()));

    // Wrap it into the frontend and register in the core
    boost::shared_ptr< sink_t > sink(new sink_t(backend));
    core->add_sink(sink);

    // You can manage filtering through the sink interface
    sink->set_filter(flt::attr< int >("Severity") >= 2);

    // You can also manage backend in a thread-safe manner
    {
        sink_t::locked_backend_ptr p = sink->locked_backend();
        p->add_stream(boost::make_shared< std::ofstream >("sample.log"));
        p->set_formatter
        (
            fmt::stream
                << "Level: " << fmt::attr< int >("Severity")
                << " Message: " << fmt::message()
        );
    } // the backend gets released here

    return sink;
}
//]

//[ example_sinks_async_stop
void stop_logging(boost::shared_ptr< sink_t >& sink)
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    // Remove the sink from the core, so that no records are passed to it
    core->remove_sink(sink);

    // Break the feeding loop
    sink->stop();

    // Flush all log records that may have left buffered
    sink->feed_records();

    sink.reset();
}
//]

int main(int, char*[])
{
    boost::shared_ptr< sink_t > sink = init_logging();

    src::severity_channel_logger< > lg(keywords::channel = "net");
    BOOST_LOG_SEV(lg, 3) << "Hello world!";

    stop_logging(sink);

    return 0;
}
