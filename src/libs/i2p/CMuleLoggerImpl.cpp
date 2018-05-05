#pragma GCC java_exceptions

#include <gcj/cni.h>
#include <CMuleLogger.h>

#ifdef FALSE
#undef FALSE
#define FALSE 0
#endif
#ifdef TRUE
#undef TRUE
#define TRUE 1
#endif

#include <wx/wx.h>

#include <Logger.h>
#include "OtherFunctions.h"

#include "JConversions.h"

void CMuleLogger::init( )
{
#ifdef __DEBUG__
        m_buffer = (gnu::gcj::RawData *) new char[DUMPSIZE];
        m_size = 0;
        memset( m_buffer, 0, DUMPSIZE);
#endif
}

void CMuleLogger::finalize()
{
#ifdef __DEBUG__
        delete [] (char *) m_buffer ;
#endif
}

void CMuleLogger::log ( jstring message )
{
        AddDebugLogLineN(( DebugType ) m_logId, ::toString ( message ) );
}

void CMuleLogger::logCritical ( jstring message )
{
        AddDebugLogLineC(( DebugType ) m_logId, ::toString ( message ) );
}

void CMuleLogger::dump()
{
#ifdef __DEBUG__
        if ( CLogger::IsEnabled ( ( DebugType ) m_logId ) ) {

                JvSynchronize lock ( this );

                DumpMem ( m_buffer, m_size );
        }
#endif
}

void CMuleLogger::store ( jbyteArray buffer, jint start, jint len )
{
#ifdef __DEBUG__

        if ( CLogger::IsEnabled ( ( DebugType ) m_logId ) ) {

                JvSynchronize lock ( this );

                while ( len > 0 ) {
                        int toDump = std::min ( len, DUMPSIZE - m_size );
                        memcpy ( ( ( char* ) m_buffer ) + m_size,
                                 ( ( char* ) elements ( buffer ) ) + start,
                                 toDump );
                        m_size += toDump;
                        start += toDump;
                        len -= toDump;

                        if ( m_size == DUMPSIZE ) {
                                DumpMem ( m_buffer , DUMPSIZE );
                                memset ( m_buffer, 0, DUMPSIZE );
                                m_size = 0 ;
                        }
                }
        }
#endif
}
