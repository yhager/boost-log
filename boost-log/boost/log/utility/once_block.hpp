/*
 *          Copyright Andrey Semashev 2007 - 2012.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   once_block.hpp
 * \author Andrey Semashev
 * \date   23.06.2010
 *
 * \brief  The header defines classes and macros for once-blocks.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_ONCE_BLOCK_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_ONCE_BLOCK_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>
#include <boost/log/utility/unique_identifier_name.hpp>

#ifndef BOOST_LOG_NO_THREADS

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief A flag to detect if a code block has already been executed.
 *
 * This structure should be used in conjunction with the \c BOOST_LOG_ONCE_BLOCK_FLAG
 * macro. Usage example:
 *
 * <code>
 * void foo()
 * {
 *     static once_block_flag flag = BOOST_LOG_ONCE_BLOCK_INIT;
 *     BOOST_LOG_ONCE_BLOCK_FLAG(flag)
 *     {
 *         puts("Hello, world once!");
 *     }
 * }
 * </code>
 */
struct once_block_flag
{
#ifndef BOOST_LOG_DOXYGEN_PASS
    // Do not use, implementation detail
    enum
    {
        uninitialized = 0,
        being_initialized,
        initialized
    }
    status;
#endif // BOOST_LOG_DOXYGEN_PASS
};

/*!
 * \def BOOST_LOG_ONCE_BLOCK_INIT
 *
 * The static initializer for \c once_block_flag.
 */
#define BOOST_LOG_ONCE_BLOCK_INIT { boost::log::once_block_flag::uninitialized }

namespace aux {

class once_block_sentry
{
private:
    once_block_flag& m_Flag;

public:
    explicit once_block_sentry(once_block_flag& f) : m_Flag(f)
    {
    }

    ~once_block_sentry()
    {
        if (m_Flag.status != once_block_flag::initialized)
            rollback();
    }

    bool executed() const
    {
        return (m_Flag.status == once_block_flag::initialized || enter_once_block());
    }

    BOOST_LOG_EXPORT void commit();

private:
    //  Non-copyable, non-assignable
    once_block_sentry(once_block_sentry const&);
    once_block_sentry& operator= (once_block_sentry const&);

    BOOST_LOG_EXPORT bool enter_once_block() const;
    BOOST_LOG_EXPORT void rollback();
};

} // namespace aux

} // namespace log

} // namespace boost

#else // BOOST_LOG_NO_THREADS

namespace boost {

namespace BOOST_LOG_NAMESPACE {

struct once_block_flag
{
    bool status;
};

#define BOOST_LOG_ONCE_BLOCK_INIT { false }

namespace aux {

class once_block_sentry
{
private:
    once_block_flag& m_Flag;

public:
    explicit once_block_sentry(once_block_flag& f) : m_Flag(f)
    {
    }

    bool executed() const
    {
        return m_Flag.status;
    }

    void commit()
    {
        m_Flag.status = true;
    }

private:
    //  Non-copyable, non-assignable
    once_block_sentry(once_block_sentry const&);
    once_block_sentry& operator= (once_block_sentry const&);
};

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_NO_THREADS

#define BOOST_LOG_ONCE_BLOCK_FLAG_INTERNAL(flag_var, sentry_var)\
    for (boost::log::aux::once_block_sentry sentry_var((flag_var));\
        !sentry_var.executed(); sentry_var.commit())

#define BOOST_LOG_ONCE_BLOCK_INTERNAL(flag_var, sentry_var)\
    static boost::log::once_block_flag flag_var = BOOST_LOG_ONCE_BLOCK_INIT;\
    BOOST_LOG_ONCE_BLOCK_FLAG_INTERNAL(flag_var, sentry_var)

/*!
 * \def BOOST_LOG_ONCE_BLOCK_FLAG(flag_var)
 *
 * Begins a code block to be executed only once, with protection against thread concurrency.
 * User has to provide the flag variable that controls whether the block has already
 * been executed.
 */
#define BOOST_LOG_ONCE_BLOCK_FLAG(flag_var)\
    BOOST_LOG_ONCE_BLOCK_INTERNAL(\
        flag_var,\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_once_block_sentry_))

/*!
 * \def BOOST_LOG_ONCE_BLOCK()
 *
 * Begins a code block to be executed only once, with protection against thread concurrency.
 */
#define BOOST_LOG_ONCE_BLOCK()\
    BOOST_LOG_ONCE_BLOCK_INTERNAL(\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_once_block_flag_),\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_once_block_sentry_))

#endif // BOOST_LOG_UTILITY_ONCE_BLOCK_HPP_INCLUDED_
