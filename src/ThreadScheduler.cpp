//
// This file is part of the aMule Project.
//
// Copyright (c) 2006-2011 Mikkel Schubert ( xaignar@amule.org / http://www.amule.org )
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

#include "ThreadScheduler.h"	// Interface declarations
#include "Logger.h"				// Needed for Add(Debug)LogLine{C,N}
#include <common/Format.h>		// Needed for CFormat
#include <common/coroutine.h>
#include "MuleThread.h"

#include <algorithm>			// Needed for std::sort		// Do_not_auto_remove (mingw-gcc-3.4.5)

//! Global lock the scheduler and its thread.
static wiMutex s_lock;
//! Pointer to the global scheduler instance (automatically instantiated).
static CThreadScheduler* s_scheduler = NULL;
//! Specifies if the scheduler is running.
static bool	s_running = false;
//! Specifies if the gobal scheduler has been terminated.
static bool s_terminated = false;


void CThreadScheduler::Start()
{
        wiMutexLocker lock(s_lock);

        s_running = true;
        s_terminated = false;

        // Ensures that a thread is started if tasks are already waiting.
        if (s_scheduler) {
                AddDebugLogLineN(logThreads, wxT("Starting scheduler"));
                s_scheduler->CreateSchedulerThread();
        }
}


void CThreadScheduler::Terminate()
{
        AddDebugLogLineN(logThreads, wxT("Terminating scheduler"));
        CThreadScheduler* ptr = NULL;

        {
                wiMutexLocker lock(s_lock);

                // Safely unlink the scheduler, as to avoid race-conditions.
                ptr = s_scheduler;
                s_running = false;
                s_terminated = true;
                s_scheduler = NULL;
        }

        delete ptr;
        AddDebugLogLineN(logThreads, wxT("Scheduler terminated"));
}


bool CThreadScheduler::AddTask(CThreadTask* task, bool overwrite)
{
        wiMutexLocker lock(s_lock);

        // When terminated (on shutdown), all tasks are ignored.
        if (s_terminated) {
                AddDebugLogLineN(logThreads, wxT("Task discarded: ") + task->GetDesc());
                delete task;
                return false;
        } else if (s_scheduler == NULL) {
                s_scheduler = new CThreadScheduler();
                AddDebugLogLineN(logThreads, wxT("Scheduler created."));
        }

        return s_scheduler->DoAddTask(task, overwrite);
}


/** Returns string representation of error code. */
wxString GetErrMsg(wiThreadError err)
{
        switch (err) {
        case wiTHREAD_NO_ERROR:		return wxT("wxTHREAD_NO_ERROR");
        case wiTHREAD_NO_RESOURCE:	return wxT("wxTHREAD_NO_RESOURCE");
        case wiTHREAD_RUNNING:		return wxT("wxTHREAD_RUNNING");
        case wiTHREAD_NOT_RUNNING:	return wxT("wxTHREAD_NOT_RUNNING");
        case wiTHREAD_KILLED:		return wxT("wxTHREAD_KILLED");
        case wiTHREAD_MISC_ERROR:	return wxT("wxTHREAD_MISC_ERROR");
        default:
                return wxT("Unknown error");
        }
}


void CThreadScheduler::CreateSchedulerThread()
{
        if (IsAlive() || m_tasks.empty()) {
                return;
        }

        CMuleCoroutine::Start();
        AddDebugLogLineN(logThreads, wxT("Scheduler thread started"));
}


/** This is the sorter functor for the task-queue. */
struct CTaskSorter {
        bool operator()(const CThreadScheduler::CEntryPair& a, const CThreadScheduler::CEntryPair& b) {
                if (a.first->GetPriority() != b.first->GetPriority()) {
                        return a.first->GetPriority() > b.first->GetPriority();
                }

                // Compare tasks numbers.
                return a.second < b.second;
        }
};



CThreadScheduler::CThreadScheduler()
        : CMuleCoroutine(JOINABLE),
          m_tasksDirty(false),
          m_currentTask(NULL)
{

}


CThreadScheduler::~CThreadScheduler()
{
}


size_t CThreadScheduler::GetTaskCount() const
{
        wiMutexLocker lock(s_lock);

        return m_tasks.size();
}


bool CThreadScheduler::DoAddTask(CThreadTask* task, bool overwrite)
{
        wxASSERT(wiThread::IsMain());
        
        // GetTick is too lowres, so we just use a counter to ensure that
        // the sorted order will match the order in which the tasks were added.
        static unsigned taskAge = 0;

        // Get the map for this task type, implicitly creating it as needed.
        wxString stype = task->GetType().Clone();
        CDescMap& map = m_taskDescs[stype];

        CDescMap::value_type entry(task->GetDesc(), task);
        if (map.insert(entry).second) {
                AddDebugLogLineN(logThreads, wxT("Task scheduled: ") + task->GetType() + wxT(" - ") + task->GetDesc());
                m_tasks.push_back(CEntryPair(task, taskAge++));
                m_tasksDirty = true;
        } else if (overwrite) {
                AddDebugLogLineN(logThreads, wxT("Task overwritten: ") + task->GetType() + wxT(" - ") + task->GetDesc());

                CThreadTask* existingTask = map[task->GetDesc()];
                if (m_currentTask == existingTask) {
                        // The duplicate is already being executed, abort it.
                        m_currentTask->m_abort = true;
                } else {
                        // Task not yet started, simply remove and delete.
                        wxCHECK2(map.erase(existingTask->GetDesc()), /* Do nothing. */);
                        delete existingTask;
                }

                m_tasks.push_back(CEntryPair(task, taskAge++));
                map[task->GetDesc()] = task;
                m_tasksDirty = true;
        } else {
                AddDebugLogLineN(logThreads, wxT("Duplicate task, discarding: ") + task->GetType() + wxT(" - ") + task->GetDesc());
                delete task;
                return false;
        }

        if (s_running) {
                CreateSchedulerThread();
        }

        return true;
}

bool CThreadScheduler::Continue()
{
        CR_BEGIN;
        
        AddDebugLogLineN(logThreads, wxT("Entering scheduling loop"));

        while (!TestDestroy()) {

                // Resort tasks by priority/age if list has been modified.
                if (m_tasksDirty) {
                        AddDebugLogLineN(logThreads, wxT("Resorting tasks"));
                        std::sort(m_tasks.begin(), m_tasks.end(), CTaskSorter());
                        m_tasksDirty = false;
                } else if (m_tasks.empty()) {
                        AddDebugLogLineN(logThreads, wxT("No more tasks, stopping"));
                        break;
                }
                
                // Select the next task
                cont.task.reset(m_tasks.front().first);
                m_tasks.pop_front();
                m_currentTask = cont.task.get();
                
                AddDebugLogLineN(logThreads, wxT("Current task: ") + cont.task->GetType() + wxT(" - ") + cont.task->GetDesc());
                // Execute the task
                cont.task->m_owner = this;
                CR_RETURN;
                CR_CALL(cont.task->Continue());
                cont.task->OnExit();
                CR_RETURN;

                {
                        // Check if this was the last task of this type
                        bool isLastTask = false;
                        
                        // If the task has been aborted, the entry now refers to
                        // a different task, so dont remove it. That also means
                        // that it cant be the last task of this type.
                        if (!cont.task->m_abort) {
                                AddDebugLogLineN(logThreads,
                                                 CFormat(wxT("Completed task '%s%s', %u tasks remaining."))
                                                 % cont.task->GetType()
                                                 % (cont.task->GetDesc().IsEmpty() ? wxString() : (wxT(" - ") + cont.task->GetDesc()))
                                                 % m_tasks.size() );
                                
                                CDescMap& map = m_taskDescs[cont.task->GetType()];
                                if (!map.erase(cont.task->GetDesc())) {
                                        wxFAIL;
                                } else if (map.empty()) {
                                        m_taskDescs.erase(cont.task->GetType());
                                        isLastTask = true;
                                }
                        }
                        
                        m_currentTask = NULL;
                        
                        if (isLastTask) {
                                // Allow the task to signal that all sub-tasks have been completed
                                AddDebugLogLineN(logThreads, wxT("Last task, calling OnLastTask"));
                                cont.task->OnLastTask();
                        }
                }
        }
        AddDebugLogLineN(logThreads, wxT("Leaving scheduling loop"));

        CR_END;
        CR_EXIT;
}



CThreadTask::CThreadTask(const wxString& type, const wxString& desc, ETaskPriority priority)
        : CMuleCoroutine(CMuleCoroutine::JOINABLE, -1),
          m_type(type),
          m_desc(desc),
          m_priority(priority),
          m_owner(NULL),
          m_abort(CMuleCoroutine::m_stop)
{
}


CThreadTask::~CThreadTask()
{
}


void CThreadTask::OnLastTask()
{
        // Does nothing by default.
}


bool CThreadTask::TestDestroy() const
{
        wxCHECK(m_owner, m_abort);

        return CMuleCoroutine::TestDestroy() || m_owner->TestDestroy();
}


const wxString& CThreadTask::GetType() const
{
        return m_type;
}


const wxString& CThreadTask::GetDesc() const
{
        return m_desc;
}


ETaskPriority CThreadTask::GetPriority() const
{
        return m_priority;
}


// File_checked_for_headers
