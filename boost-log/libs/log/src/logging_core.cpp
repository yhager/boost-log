/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   logging_core.cpp
 * \author Andrey Semashev
 * \date   19.04.2007
 * 
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#include <cstddef>
#include <memory>
#include <stack>
#include <vector>
#include <algorithm>
#include <boost/ref.hpp>
#include <boost/none.hpp>
#include <boost/thread/tss.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/log/logging_core.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>

namespace boost {

namespace log {

namespace {

    //! A simple exclusive ownership smart-pointer with moving copy semantics
    template< typename T >
    class exclusive_ptr
    {
    public:
        //! The pointed to type
        typedef T element_type;

    private:
        //! The stored pointer
        mutable element_type* m_p;

    public:
        //  Structors
        exclusive_ptr() : m_p(NULL) {}
        exclusive_ptr(exclusive_ptr const& that) : m_p(that.m_p) { that.m_p = NULL; }
        explicit exclusive_ptr(element_type* p) : m_p(p) {}
        ~exclusive_ptr() { this->reset(); }

        //! Moving assignment
        exclusive_ptr& operator= (exclusive_ptr const& that)
        {
            this->reset(that.m_p);
            that.m_p = NULL;
            return *this;
        }
        //! Indirection
        element_type* operator->() const { return this->m_p; }

        //! The method assigns the new value to the pointer
        void reset(element_type* p = NULL)
        {
            delete this->m_p;
            this->m_p = p;
        }
        //! The method releases the stored pointer
        element_type* release()
        {
            register element_type* p = this->m_p;
            this->m_p = NULL;
            return p;
        }
        //! Returns the value of the pointer
        element_type* get() const { return this->m_p; }
    };

} // namespace

//! Logging system implementation
template< typename CharT >
struct basic_logging_core< CharT >::implementation
{
public:
    //! Front-end class type
    typedef basic_logging_core< char_type > logging_core_type;
    //! Read lock type
    typedef shared_lock< shared_mutex > scoped_read_lock;
    //! Write lock type
    typedef unique_lock< shared_mutex > scoped_write_lock;

    //! Sinks container type
    typedef std::vector< shared_ptr< sink_type > > sink_list;

    //! Thread-specific data
    struct thread_data
    {
        //! A structure that holds a particular logging record data
        struct pending_record
        {
            //! A list of sinks that will accept the record
            sink_list AcceptingSinks;
            //! Attribute values view
            attribute_values_view AttributeValues;

            //! Constructor
            pending_record(
                attribute_set const& SourceAttrs,
                attribute_set const& ThreadAttrs,
                attribute_set const& GlobalAttrs
            ) : AttributeValues(SourceAttrs, ThreadAttrs, GlobalAttrs)
            {
            }
        };
        //! A stack of records being validated and pushed to the sinks
        std::stack< exclusive_ptr< pending_record >, std::vector< exclusive_ptr< pending_record > > > PendingRecords;

        //! Thread-specific attribute set
        attribute_set ThreadAttributes;
    };

public:
    //! Synchronization mutex
    shared_mutex Mutex;

    //! List of sinks involved into output
    sink_list Sinks;

    //! Global attribute set
    attribute_set GlobalAttributes;
    //! Thread-specific data
    thread_specific_ptr< thread_data > pThreadData;

    //! Global filter
    filter_type Filter;

public:
    //! Constructor
    implementation() : Mutex() {}

    //! The method initializes thread-specific data
    void init_thread_data()
    {
        scoped_write_lock lock(Mutex);

        if (!pThreadData.get())
        {
            std::auto_ptr< thread_data > p(new thread_data());
            pThreadData.reset(p.get());
            p.release();
        }
    }

    //! The function holds the reference to the logging system singleton
    static shared_ptr< logging_core_type >& get_instance()
    {
        static shared_ptr< logging_core_type > pInstance;
        return pInstance;
    }
    //! The function initializes the logging system
    static void init_logging_core()
    {
        get_instance().reset(new logging_core_type());
    }
};


//! Logging system constructor
template< typename CharT >
basic_logging_core< CharT >::basic_logging_core()
    : pImpl(new implementation())
{
}

//! Logging system destructor
template< typename CharT >
basic_logging_core< CharT >::~basic_logging_core()
{
    delete pImpl;
}

//! The method returns a pointer to the logging system instance
template< typename CharT >
shared_ptr< basic_logging_core< CharT > > basic_logging_core< CharT >::get()
{
    static once_flag flag = BOOST_ONCE_INIT;
    call_once(&implementation::init_logging_core, flag);
    return implementation::get_instance();
}

//! The method should be called in every non-boost thread on its finish to cleanup some thread-specific data
template< typename CharT >
void basic_logging_core< CharT >::thread_cleanup()
{
    typename implementation::scoped_write_lock lock(pImpl->Mutex);
    pImpl->pThreadData.reset();
}

//! The method adds a new sink
template< typename CharT >
void basic_logging_core< CharT >::add_sink(shared_ptr< sink_type > const& s)
{
    typename implementation::scoped_write_lock lock(pImpl->Mutex);
    typename implementation::sink_list::iterator it =
        std::find(pImpl->Sinks.begin(), pImpl->Sinks.end(), s);
    if (it == pImpl->Sinks.end())
        pImpl->Sinks.push_back(s);
}

//! The method removes the sink from the output
template< typename CharT >
void basic_logging_core< CharT >::remove_sink(shared_ptr< sink_type > const& s)
{
    typename implementation::scoped_write_lock lock(pImpl->Mutex);
    typename implementation::sink_list::iterator it =
        std::find(pImpl->Sinks.begin(), pImpl->Sinks.end(), s);
    if (it != pImpl->Sinks.end())
        pImpl->Sinks.erase(it);
}


//! The method adds an attribute to the global attribute set
template< typename CharT >
typename basic_logging_core< CharT >::attribute_set::iterator
basic_logging_core< CharT >::add_global_attribute(string_type const& name, shared_ptr< attribute > const& attr)
{
    typename implementation::scoped_write_lock lock(pImpl->Mutex);
    return pImpl->GlobalAttributes.insert(typename attribute_set::key_type(name), attr);
}

//! The method removes an attribute from the global attribute set
template< typename CharT >
void basic_logging_core< CharT >::remove_global_attribute(typename attribute_set::iterator it)
{
    typename implementation::scoped_write_lock lock(pImpl->Mutex);
    pImpl->GlobalAttributes.erase(it);
}

//! The method adds an attribute to the thread-specific attribute set
template< typename CharT >
typename basic_logging_core< CharT >::attribute_set::iterator
basic_logging_core< CharT >::add_thread_attribute(string_type const& name, shared_ptr< attribute > const& attr)
{
    if (!pImpl->pThreadData.get())
        pImpl->init_thread_data();

    return pImpl->pThreadData->ThreadAttributes.insert(typename attribute_set::key_type(name), attr);
}

//! The method removes an attribute from the thread-specific attribute set
template< typename CharT >
void basic_logging_core< CharT >::remove_thread_attribute(typename attribute_set::iterator it)
{
    typename implementation::thread_data* p = pImpl->pThreadData.get();
    if (p)
        p->ThreadAttributes.erase(it);
}

//! An internal method to set the global filter
template< typename CharT >
void basic_logging_core< CharT >::set_filter_impl(filter_type const& filter)
{
    typename implementation::scoped_write_lock lock(pImpl->Mutex);
    pImpl->Filter = filter;
}

//! The method removes the global logging filter
template< typename CharT >
void basic_logging_core< CharT >::reset_filter()
{
    typename implementation::scoped_write_lock lock(pImpl->Mutex);
    pImpl->Filter.clear();
}

//! The method opens a new record to be written and returns true if the record was opened
template< typename CharT >
bool basic_logging_core< CharT >::open_record(attribute_set const& source_attributes)
{
    typename implementation::thread_data* tsd = pImpl->pThreadData.get();
    if (!tsd) try
    {
        pImpl->init_thread_data();
        tsd = pImpl->pThreadData.get();
    }
    catch (std::exception&)
    {
        return false;
    }

    // Lock the core to be safe against any attribute or sink set modifications
    typename implementation::scoped_read_lock lock(pImpl->Mutex);

    if (!pImpl->Sinks.empty()) try
    {
        // Construct a record
        typedef typename implementation::thread_data::pending_record pending_record;
        exclusive_ptr< pending_record > record(new pending_record(
            source_attributes, tsd->ThreadAttributes, pImpl->GlobalAttributes));

        if ((pImpl->Filter.empty() || pImpl->Filter(record->AttributeValues)))
        {
            // The global filter passed, trying the sinks
            record->AcceptingSinks.reserve(pImpl->Sinks.size());
            typename implementation::sink_list::iterator it = pImpl->Sinks.begin(), end = pImpl->Sinks.end();
            for (; it != end; ++it)
            {
                try
                {
                    if (it->get()->will_write_message(record->AttributeValues))
                        record->AcceptingSinks.push_back(*it);
                }
                catch (...)
                {
                    // Assume that the sink is incapable to receive messages now
                }
            }

            if (!record->AcceptingSinks.empty())
            {
                // Some sinks are willing to process the record
                tsd->PendingRecords.push(record);
                return true;
            }
        }
    }
    catch (...)
    {
        // Something has gone wrong. As the library should impose minimum influence
        // on the user's code, we simply mimic here that the record is not needed.
    }

    return false;
}

//! The method pushes the record and closes it
template< typename CharT >
void basic_logging_core< CharT >::push_record(string_type const& message_text)
{
    typename implementation::thread_data* tsd = pImpl->pThreadData.get();
    if (!tsd)
    {
        pImpl->init_thread_data();
        tsd = pImpl->pThreadData.get();
    }

    try
    {
        if (tsd->PendingRecords.empty())
        {
            // If push_record was called with no prior call to open_record, we call it here
            attribute_set empty_source_attributes;
            if (!this->open_record(empty_source_attributes))
                return;
        }

        typename implementation::thread_data::pending_record* record =
            tsd->PendingRecords.top().get();

        typename implementation::sink_list::iterator it = record->AcceptingSinks.begin(),
            end = record->AcceptingSinks.end();
        for (; it != end; ++it)
        {
            try
            {
                (*it)->write_message(record->AttributeValues, message_text);
            }
            catch (...)
            {
            }
        }
    }
    catch (...)
    {
    }

    // Inevitably, close the record
    tsd->PendingRecords.pop();
}

//! The method cancels the record
template< typename CharT >
void basic_logging_core< CharT >::cancel_record()
{
    typename implementation::thread_data* tsd = pImpl->pThreadData.get();
    if (tsd && !tsd->PendingRecords.empty())
    {
        tsd->PendingRecords.pop();
    }
}

//! Explicitly instantiate logging_core implementation
template class BOOST_LOG_EXPORT basic_logging_core< char >;
template class BOOST_LOG_EXPORT basic_logging_core< wchar_t >;

} // namespace log

} // namespace boost