//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef MULETHREAD_H
#define MULETHREAD_H

#include <config.h>

#ifdef IMULE_USE_THREADS

#include <wx/thread.h>
#define wiThread wxThread
#define wiMutex  wxMutex
#define wiCondition wxCondition
#define wiThreadError wxTheadError

class CMuleThread : public wxThread
{
public:
        //! @see wxThread::wxThread
        CMuleThread(wxThreadKind kind = wxTHREAD_DETACHED)
        : wxThread(kind),
        m_stop(false) {}
        
        /**
         * Stops the thread.
         *
         * For detached threads, this function is equivalent
         * to Delete, but is also useable for joinable threads,
         * where Delete should not be used, due to crashes
         * experienced in that case. In the case of joinable
         * threads, Wait is called rather than Delete.
         *
         * @see wxThread::Delete
         */
        void Stop() {
                m_stop = true;
                if (IsDetached()) {
                        Delete();
                } else {
                        Wait();
                }
        }
        
        
        //! Returns true if Delete or Stop has been called.
        virtual bool TestDestroy() {
                // m_stop is checked last, because some functionality is
                // dependant upon wxThread::TestDestroy() being called,
                // for instance Pause().
                return wxThread::TestDestroy() || m_stop;
        }
private:
        //! Is set if Stop is called.
        bool    m_stop;
};

/**
 * Automatically unlocks a mutex on construction and locks it on destruction.
 *
 * This class is the complement of wxMutexLocker.  It is intended to be used
 * when a mutex, which is locked for a period of time, needs to be
 * temporarily unlocked for a bit.  For example:
 *
 *      wxMutexLocker lock(mutex);
 *
 *      // ... do stuff that requires that the mutex is locked ...
 *
 *      {
 *              CMutexUnlocker unlocker(mutex);
 *              // ... do stuff that requires that the mutex is unlocked ...
 *      }
 *
 *      // ... do more stuff that requires that the mutex is locked ...
 *
 */
class CMutexUnlocker
{
public:
        // unlock the mutex in the ctor
        CMutexUnlocker(wxMutex& mutex)
        : m_isOk(false), m_mutex(mutex)
        { m_isOk = ( m_mutex.Unlock() == wxMUTEX_NO_ERROR ); }
        
        // returns true if mutex was successfully unlocked in ctor
        bool IsOk() const
        { return m_isOk; }
        
        // lock the mutex in dtor
        ~CMutexUnlocker()
        { if ( IsOk() ) m_mutex.Lock(); }
        
private:
        // no assignment operator nor copy ctor
        CMutexUnlocker(const CMutexUnlocker&);
        CMutexUnlocker& operator=(const CMutexUnlocker&);
        
        bool     m_isOk;
        wxMutex& m_mutex;
};


#else //IMULE_USE_THREADS

#define wiMUTEX_RECURSIVE
#define wiThreadError int
#define wiTHREAD_NO_ERROR 0
#define wiTHREAD_NO_RESOURCE 1
#define wiTHREAD_RUNNING 2
#define wiTHREAD_NOT_RUNNING 3
#define wiTHREAD_KILLED 4
#define wiTHREAD_MISC_ERROR 6

struct wiThread
{
        static bool IsMain() { return true; }
};

struct CMuleThread : wiThread {};

struct wiMutex
{
        void Lock() const {}
        void Unlock() const {}
};

struct wiMutexLocker
{
        wiMutexLocker(const wiMutex &) {}
};

struct CMutexUnlocker
{
        CMutexUnlocker(const wiMutex&){}
};

struct wiCondition
{
        wiCondition(const wiMutex &) {}
        void Broadcast() const {}
        void Wait() const {}
};

#endif //IMULE_USE_THREADS

#endif
