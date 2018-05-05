//
// C++ Interface: myMutexLocker
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

// a helper class which locks the mutex in the ctor and unlocks it in the dtor:
// this ensures that mutex is always unlocked, even if the function returns or
// throws an exception before it reaches the end

#ifndef _MYMUTEXLOCKER_H
#define _MYMUTEXLOCKER_H

#include "MuleThread.h"

#ifdef MULE_USE_THREADS
class wiMutex ;

class myMutexLocker
{
public:
        // lock the mutex in the ctor
        myMutexLocker ( wiMutex& mutex, const char * func ) ;

        myMutexLocker ( wiMutex& mutex, long sid, const char * func ) ;
        // returns true if mutex was successfully locked in ctor
        bool IsOk() const ;
        // unlock the mutex in dtor
        ~myMutexLocker();

        static bool lockedInThread( wiMutex& mutex );

private:
        // no assignment operator nor copy ctor
        myMutexLocker ( const myMutexLocker& );
        myMutexLocker& operator= ( const myMutexLocker& );

        void test();

        bool            m_isOk  ;
        wiMutex *       m_mutex ;
        const char *    m_func  ;
};


#define ASSERT_LOCK( locker, mutex, func ) myMutexLocker locker( mutex, func );

#define WRITE_BUFF_LOCK( wb, func ) myMutexLocker wbl( *((wb).mutex), (long) (wb).sid, func );

#define ASSERT_LOCKED( mutex, msg ) wxASSERT_MSG( myMutexLocker::lockedInThread(mutex), msg )

#else

#define ASSERT_LOCK( locker, mutex, func )
#define WRITE_BUFF_LOCK( wb, func )
#define ASSERT_LOCKED( mutex, msg )
typedef wiMutexLocker myMutexLocker;

#endif

#endif
