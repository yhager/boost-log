; /* --------------------------------------------------------
; HEADER SECTION
;*/
SeverityNames=(Debug=0x0:BOOST_LOG_SEVERITY_DEBUG
               Info=0x1:BOOST_LOG_SEVERITY_INFO
               Warning=0x2:BOOST_LOG_SEVERITY_WARNING
               Error=0x3:BOOST_LOG_SEVERITY_ERROR
              )
;
;
;
;/* ------------------------------------------------------------------
; MESSAGE DEFINITION SECTION
;*/

MessageIdTypedef=DWORD

MessageId=0x100
Severity=Debug
Facility=Application
SymbolicName=BOOST_LOG_MSG_DEBUG
Language=English
%1 
.

MessageId=0x101
Severity=Info
Facility=Application
SymbolicName=BOOST_LOG_MSG_INFO
Language=English
%1 
.

MessageId=0x102
Severity=Warning
Facility=Application
SymbolicName=BOOST_LOG_MSG_WARNING
Language=English
%1 
.

MessageId=0x103
Severity=Error
Facility=Application
SymbolicName=BOOST_LOG_MSG_ERROR
Language=English
%1 
.
