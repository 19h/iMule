#include "myMutexLocker.h"

#include <common/MuleDebug.h>
#include <Logger.h>
#include <map>

#include "MuleThread.h"

//#define _DEBUG_MUTEX_

#ifdef MULE_USE_THREADS

typedef std::pair<wiMutex * , unsigned long> mutexThread ;

std::map< mutexThread, const char * > s_locked ;
wiMutex s_lockedMutex;

myMutexLocker::myMutexLocker ( wiMutex & mutex, const char * 
#ifdef _DEBUG_MUTEX_
        func
#endif
)
        : m_isOk ( false ), m_mutex ( &mutex )
{
#ifdef _DEBUG_MUTEX_
        m_func = func ;
        AddDebugLogLineN(logSamSession, wxString ( wxT ( "mutex >>>>>>> " ) )
                           << ( long long unsigned ) m_mutex
                           << wxT ( " thread " ) << wiThread::GetCurrentId()
                           <<  wxT ( " " ) << wxString::FromAscii ( m_func ) );
        {
                wiMutexLocker lock(s_lockedMutex) ;
                test() ;
                s_locked[ mutexThread ( m_mutex, wiThread::GetCurrentId() ) ] = m_func ;
        }
#endif
        m_isOk = ( m_mutex->Lock() == wiMutex_NO_ERROR );
#ifdef _DEBUG_MUTEX_
        wxASSERT_MSG ( IsOk() , wxString ( wxT ( "Can not lock the mutex" ) ) );

        AddDebugLogLineN(logSamSession, wxString ( wxT ( "mutex ======= " ) )
                           << ( long long unsigned ) m_mutex
                           << wxT ( " thread " ) << wiThread::GetCurrentId()
                           <<  wxT ( " " ) << wxString::FromAscii ( m_func ) );
#endif
}

myMutexLocker::myMutexLocker ( wiMutex& mutex, 
#ifdef _DEBUG_MUTEX_
                                               long sid, const char * func
#else
                                               long, const char * 
#endif
)
        : m_isOk ( false ), m_mutex ( &mutex )
{
#ifdef _DEBUG_MUTEX_
        m_func = func  ;
        AddDebugLogLineN(logSamSession, wxString ( wxT ( "mutex >>>>>>> " ) )
                           << ( long long unsigned ) m_mutex
                           << wxT ( " thread " ) << wiThread::GetCurrentId()
                           <<  wxT ( " " ) << wxString::FromAscii ( m_func )
                           << wxT ( " (STREAM " )
                           << sid << wxT ( ")" ) );

        {
                wiMutexLocker lock(s_lockedMutex) ;
                test() ;
                s_locked[mutexThread ( m_mutex, wiThread::GetCurrentId() ) ] = m_func ;
        }
#endif
        m_isOk = ( m_mutex->Lock() == wiMutex_NO_ERROR );
#ifdef _DEBUG_MUTEX_
        wxASSERT_MSG ( IsOk() , wxString ( wxT ( "Can not lock the mutex" ) ) );

        AddDebugLogLineN(logSamSession, wxString ( wxT ( "mutex ======= " ) )
                           << ( long long unsigned ) m_mutex
                           << wxT ( " thread " ) << wiThread::GetCurrentId()
                           <<  wxT ( " " ) << wxString::FromAscii ( m_func )
                           << wxT ( " (STREAM " )
                           << sid << wxT ( ")" ) );
#endif
}

void myMutexLocker::test()
{
        if ( s_locked.count ( mutexThread ( m_mutex, wiThread::GetCurrentId() ) ) != 0 ) {
                wxString list ;

                for ( std::map<mutexThread, const char *>::const_iterator it = s_locked.begin(); it != s_locked.end(); it++ )

                        list << ( long long unsigned ) it->first.first << wxT ( " -- " )
                             << wxString::FromAscii ( it->second ) << wxT ( "\n" );

                AddDebugLogLineC(logSamSession, wxString ( wxT ( "mutex " ) )
                                   << ( long long unsigned ) m_mutex
                                   <<  wxT ( " ALREADY LOCKED. List of locked mutexes : \n " )
                                   << list );

        }

}

bool myMutexLocker::lockedInThread( wiMutex& 
#ifdef _DEBUG_MUTEX_
                                             mutex
#endif
)
{
#ifdef _DEBUG_MUTEX_
        return (s_locked.count ( mutexThread ( &mutex, wiThread::GetCurrentId() ) ) != 0 ) ;
#else
        return true;
#endif
}


// returns true if mutex was successfully locked in ctor
bool myMutexLocker::IsOk() const
{
        return m_isOk;
}

// unlock the mutex in dtor
myMutexLocker::~myMutexLocker()
{
#ifdef _DEBUG_MUTEX_
        AddDebugLogLineN(logSamSession, wxString ( wxT ( "<<<<<<< mutex " ) )
                           << ( long long unsigned ) m_mutex
                           << wxT ( " thread " ) << wiThread::GetCurrentId()
                           <<  wxT ( " " )  << wxString::FromAscii ( m_func ) );
#endif
        if ( IsOk() ) {
#ifdef _DEBUG_MUTEX_
                {
                        wiMutexLocker lock(s_lockedMutex) ;
                        s_locked.erase ( mutexThread ( m_mutex, wiThread::GetCurrentId() ) ) ;
                }
#endif
                m_mutex->Unlock();
        }

}

#endif
