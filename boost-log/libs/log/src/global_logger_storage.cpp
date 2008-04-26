/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   global_logger_storage.cpp
 * \author Andrey Semashev
 * \date   21.04.2008
 * 
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#include <map>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/log/detail/singleton.hpp>
#include <boost/log/detail/type_info_wrapper.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

namespace boost {

namespace log {

namespace sources {

namespace aux {

namespace {

//! The loggers repository singleton
template< typename CharT >
struct loggers_repository :
    public log::aux::lazy_singleton< loggers_repository< CharT > >
{
    //! Repository map type
    typedef std::map< log::aux::type_info_wrapper, shared_ptr< logger_holder_base > > loggers_map_t;

    //! Synchronization primitive
    mutable mutex m_Mutex;
    //! Map of logger holders
    loggers_map_t m_Loggers;
};

} // namespace

//! Finds or creates the logger and returns its holder
template< typename CharT >
shared_ptr< logger_holder_base > global_storage< CharT >::get_or_init(
    std::type_info const& key,
    function0< shared_ptr< logger_holder_base > > const& initializer)
{
    typedef loggers_repository< CharT > repository_t;
    typedef typename repository_t::loggers_map_t loggers_map_t;
    repository_t& repo = repository_t::get();
    log::aux::type_info_wrapper wrapped_key = key;

    lock_guard< mutex > _(repo.m_Mutex);
    typename loggers_map_t::iterator it = repo.m_Loggers.find(wrapped_key);
    if (it != repo.m_Loggers.end())
    {
        // There is an instance
        return it->second;
    }
    else
    {
        // We have to create a logger instance
        shared_ptr< logger_holder_base > inst = initializer();
        repo.m_Loggers[wrapped_key] = inst;
        return inst;
    }
}

//! Explicitly instantiate global_storage implementation
template struct global_storage< char >;
template struct global_storage< wchar_t >;

} // namespace aux

} // namespace sources

} // namespace log

} // namespace boost