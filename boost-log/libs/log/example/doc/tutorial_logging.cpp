/*
 *          Copyright Andrey Semashev 2007 - 2012.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/init/to_file.hpp>
#include <boost/log/utility/init/common_attributes.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace fmt = boost::log::formatters;
namespace keywords = boost::log::keywords;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger, src::logger_mt)

void logging_function1()
{
    src::logger lg;

//[ example_tutorial_logging_manual_logging
    logging::record rec = lg.open_record();
    if (rec)
    {
        rec.message() = "Hello, World!";
        lg.push_record(rec);
    }
//]
}

void logging_function2()
{
    src::logger_mt& lg = my_logger::get();
    BOOST_LOG(lg) << "Greetings from the global logger!";
}

int main(int, char*[])
{
    logging::init_log_to_file("sample.log");
    logging::add_common_attributes();

    logging_function1();
    logging_function2();

    return 0;
}
