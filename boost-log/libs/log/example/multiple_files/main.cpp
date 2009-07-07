/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   main.cpp
 * \author Andrey Semashev
 * \date   26.04.2008
 * 
 * \brief  This example shows how to perform logging to several files simultaneously,
 *         with files being created on an attribute value basis - thread identifier in this case.
 *         In the example the application creates a number of threads and registers thread
 *         identifiers as attributes. Every thread performs logging, and the sink separates
 *         log records from different threads into different files.
 */

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif // _CRT_SECURE_NO_WARNINGS

// #define BOOST_LOG_DYN_LINK 1

#include <stdexcept>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <boost/log/core.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/counter.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sinks/sink.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/formatters/ostream.hpp>
#include <boost/log/formatters/format.hpp>
#include <boost/log/formatters/attr.hpp>
#include <boost/log/formatters/date_time.hpp>
#include <boost/log/formatters/message.hpp>
#include <boost/log/utility/scoped_attribute.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace fmt = boost::log::formatters;
namespace keywords = boost::log::keywords;

using boost::shared_ptr;

enum
{
    THREAD_COUNT = 5,
    LOG_RECORDS_TO_WRITE = 10
};

// Global logger declaration
BOOST_LOG_DECLARE_GLOBAL_LOGGER(my_logger, src::logger_mt)

// This function is executed in a separate thread
void thread_foo()
{
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::thread::id, boost::this_thread::get_id());
    for (unsigned int i = 0; i < LOG_RECORDS_TO_WRITE; ++i)
    {
        BOOST_LOG(my_logger::get()) << "Log record " << i;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        // Create a text file sink
        typedef sinks::synchronous_sink< sinks::text_multifile_backend > file_sink;
        shared_ptr< file_sink > sink(new file_sink);

        // Set up how the file names will be generated
        sink->locked_backend()->file_name_composer(
            fmt::ostrm << "logs/" << fmt::attr< boost::thread::id >("ThreadID") << ".log");

        // Set the log record formatter
        sink->locked_backend()->set_formatter(
            fmt::format("%1%: [%2%] - %3%")
                % fmt::attr< unsigned int >("Line #")
                % fmt::date_time< boost::posix_time::ptime >("TimeStamp")
                % fmt::message()
            );

        // Add it to the core
        logging::core::get()->add_sink(sink);

        // Add some attributes too
        shared_ptr< logging::attribute > attr(new attrs::local_clock);
        logging::core::get()->add_global_attribute("TimeStamp", attr);
        attr.reset(new attrs::counter< unsigned int >);
        logging::core::get()->add_global_attribute("Line #", attr);

        // Create threads and make some logs
        boost::thread_group threads;
        for (unsigned int i = 0; i < THREAD_COUNT; ++i)
            threads.create_thread(&thread_foo);

        threads.join_all();

        return 0;
    }
    catch (std::exception& e)
    {
        std::cout << "FAILURE: " << e.what() << std::endl;
        return 1;
    }
}