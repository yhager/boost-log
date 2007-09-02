/*!
 * (C) 2007 Luca Regini
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   nt_eventlog_sink.cpp
 * \author Luca Regini
 * \date   02.09.2007
 * 
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#include <algorithm>
#include <boost/log/sinks/nt_eventlog_sink.hpp>
#include <boost/log/formatters/basic_formatters.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <windows.h>

namespace boost {

namespace log {

namespace {

    //! A helper struct that holds line feed char constant
    template< typename >
    struct endl_literal;

    template< >
    struct endl_literal< char >
    {
        static const char value = '\n';
    };
    const char endl_literal< char >::value;

    template< >
    struct endl_literal< wchar_t >
    {
        static const wchar_t value = L'\n';
    };
    const wchar_t endl_literal< wchar_t >::value;

} // namespace


template< typename CharT >
    nt_eventlog_sink< CharT >::nt_eventlog_sink() :
    m_StreamBuf(m_FormattedRecord),
    m_FormattingStream(&m_StreamBuf),
    m_Formatter(formatters::fmt_message< char_type >() << endl_literal< char_type >::value)
    {
    }

template< typename CharT >
 nt_eventlog_sink< CharT >::~nt_eventlog_sink() 
{
    for(size_t i=0;i<m_source_handlers.size();i++)
        DeregisterEventSource(m_source_handlers[i]);
}

 template< typename CharT >
 bool nt_eventlog_sink< CharT >::add_source(const char* source,const char* server)
 {
    HANDLE h;
    scoped_write_lock lock(this->mutex());
    h= RegisterEventSource(TEXT(server),TEXT(source));
    if(h==NULL)
        return false;
    m_source_handlers.push_back(h);
    return true;
 }

//! The method sets the locale used during formatting
template< typename CharT >
std::locale nt_eventlog_sink< CharT >::imbue(std::locale const& loc)
{
    return m_FormattingStream.imbue(loc);
}

template< typename CharT >
bool nt_eventlog_sink< CharT >::will_write_message(attribute_values_view const& attributes)
{
    scoped_read_lock lock(this->mutex());
    if (m_FormattingStream.good() && m_source_handlers.size()>0 )
        return this->will_write_message_unlocked(attributes);
    else
        return false;
}

namespace {

    //! Scope guard to automatically clear the storage
    template< typename T >
    class clear_invoker
    {
        T& m_T;

    public:
        explicit clear_invoker(T& t) : m_T(t) {}
        ~clear_invoker() { m_T.clear(); }
    };

} // namespace

template< >
void nt_eventlog_sink< char >::
report_event(size_t event_handler,typename string_type::const_pointer const message,WORD information,WORD category)
{
	        ReportEventA(m_source_handlers[event_handler],
                    EVENTLOG_SUCCESS, // information event
                    0, // No custom category
                    0, // No eventID
                    NULL, //No sid
                    1, //Number of messages in the message array
                    0, //No binary data to log
                    (LPCTSTR*) &message, //Pointer to string array LPCWSTR if UNICODE is defined LPCSTR otherwhise
                    NULL //Pointer to data
                    );
}

template< >
void nt_eventlog_sink< wchar_t >::
report_event(size_t event_handler,typename string_type::const_pointer const message,WORD information,WORD category)
{
            ReportEventW(m_source_handlers[event_handler],
                    EVENTLOG_SUCCESS, // information event
                    0, // No custom category
                    0, // No eventID
                    NULL, //No sid
                    1, //Number of messages in the message array
                    0, //No binary data to log
                    (LPCWSTR*) &message, //Pointer to string array LPCWSTR if UNICODE is defined LPCSTR otherwhise
                    NULL //Pointer to data
                    );
}


//! The method writes the message to the sink
template< typename CharT >
void nt_eventlog_sink< CharT >::write_message(
    attribute_values_view const& attributes, string_type const& message)
{
    scoped_write_lock lock(this->mutex());
    clear_invoker< string_type > storage_cleanup(m_FormattedRecord);

    m_Formatter(m_FormattingStream, attributes, message);
    m_FormattingStream.flush();

	typename string_type::const_pointer const p = m_FormattedRecord.data();
    typename string_type::size_type const s = m_FormattedRecord.size();

    for(size_t i=0;i<m_source_handlers.size();i++)
    {
        report_event(i,p,EVENTLOG_SUCCESS,0);
    }
}


//! Explicitly instantiate sink implementation
template class BOOST_LOG_EXPORT  nt_eventlog_sink< char >;
template class BOOST_LOG_EXPORT  nt_eventlog_sink< wchar_t >;

} //namespace log

} //namespace boost