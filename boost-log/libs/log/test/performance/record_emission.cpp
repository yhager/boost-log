/*!
 * (C) 2009 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   record_emission.cpp
 * \author Andrey Semashev
 * \date   22.03.2009
 * 
 * \brief  This code measures performance of log record emission
 */

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif // _CRT_SECURE_NO_WARNINGS

// #define BOOST_LOG_USE_CHAR
// #define BOOST_ALL_DYN_LINK 1
// #define BOOST_LOG_DYN_LINK 1

#include <iomanip>
#include <iostream>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/microsec_time_clock.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>

#include <boost/log/core.hpp>
#include <boost/log/common.hpp>

#include <boost/log/sinks/sink.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>

#include <boost/log/attributes/counter.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/timer.hpp>

#include <boost/log/filters/attr.hpp>

enum
{
    RECORD_COUNT = 1000000,
    THREAD_COUNT = 2
};

namespace logging = boost::log;
namespace flt = boost::log::filters;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;

enum severity_level
{
    normal,
    warning,
    error
};

namespace {

    //! A fake sink backend that receives log records
    template< typename CharT >
    class fake_backend :
        public sinks::basic_sink_backend< CharT, sinks::frontend_synchronization_tag >
    {
        typedef sinks::basic_sink_backend< CharT, sinks::frontend_synchronization_tag > base_type;

    public:
        typedef typename base_type::record_type record_type;

        void consume(record_type const& record)
        {
        }
    };

} // namespace

void test(unsigned int record_count, boost::barrier& bar)
{
    bar.wait();

    src::severity_logger< severity_level > slg;
    for (unsigned int i = 0; i < record_count; ++i)
    {
        BOOST_LOG_SEV(slg, warning) << "Test record";
    }
}

int main(int argc, char* argv[])
{
    typedef sinks::synchronous_sink< fake_backend< char > > fake_sink;
    boost::shared_ptr< fake_sink > pSink = boost::make_shared< fake_sink >();

    logging::core::get()->add_sink(pSink);

    boost::shared_ptr< logging::attribute > pCounter(new attrs::counter< unsigned int >(1));
    logging::core::get()->add_global_attribute("LineID", pCounter);
    boost::shared_ptr< logging::attribute > pTimeStamp(new attrs::local_clock());
    logging::core::get()->add_global_attribute("TimeStamp", pTimeStamp);

    logging::core::get()->set_filter(flt::attr< severity_level >("Severity") > normal); // all records pass the filter
    // logging::core::get()->set_filter(flt::attr< severity_level >("Severity") > error); // all records don't pass the filter

    const unsigned int record_count = RECORD_COUNT / THREAD_COUNT;
    boost::barrier bar(THREAD_COUNT);
    boost::thread_group threads;

    for (unsigned int i = 1; i < THREAD_COUNT; ++i)
        threads.create_thread(boost::bind(&test, record_count, boost::ref(bar)));

    boost::posix_time::ptime start = boost::date_time::microsec_clock< boost::posix_time::ptime >::universal_time(), end;
    test(record_count, bar);
    if (THREAD_COUNT > 1)
        threads.join_all();
    end = boost::date_time::microsec_clock< boost::posix_time::ptime >::universal_time();

    unsigned long long duration = (end - start).total_microseconds();

    std::cout << "Test duration: " << duration << " us, " << static_cast< unsigned int >(THREAD_COUNT) << " threads ("
        << std::fixed << std::setprecision(3) << static_cast< double >(RECORD_COUNT) / (static_cast< double >(duration) / 1000000.0)
        << " records per second)" << std::endl;

    return 0;
}
