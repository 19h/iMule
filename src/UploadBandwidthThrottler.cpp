//
// This file is part of the imule Project.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "UploadBandwidthThrottler.h"

#include "protocol/ed2k/Constants.h"
#include "common/Macros.h"
#include "common/Constants.h"

#include <cmath>
#include <limits> // Do_not_auto_remove (NetBSD)
#include "OtherFunctions.h"
#include "ThrottledSocket.h"
#include "Logger.h"
#include "Preferences.h"
#include "Statistics.h"

#include <wx/timer.h>
#include <functional>

#ifndef _MSC_VER

#ifdef _UI64_MAX
#undef _UI64_MAX
#endif

#ifdef _I64_MAX
#undef _I64_MAX
#endif

#ifdef _UI32_MAX // added by mkvore for imule
#undef _UI32_MAX
#endif

#ifdef _I32_MAX // added by mkvore for imule
#undef _I32_MAX
#endif
const uint32 _UI32_MAX = std::numeric_limits<uint32>::max();
const sint32 _I32_MAX = std::numeric_limits<sint32>::max();
const uint64 _UI64_MAX = std::numeric_limits<uint64>::max();
const sint64 _I64_MAX = std::numeric_limits<sint64>::max();

#endif

/////////////////////////////////////


/**
 * The constructor starts the thread.
 */
UploadBandwidthThrottler::UploadBandwidthThrottler()
{
        m_SentBytesSinceLastCall = 0;
        m_SentBytesSinceLastCallOverhead = 0;

        m_doRun = true;
        Entry();
}


/**
 * The destructor stops the thread. If the thread has already stoppped, destructor does nothing.
 */
UploadBandwidthThrottler::~UploadBandwidthThrottler()
{
        EndThread();
}


/**
 * Find out how many bytes that has been put on the sockets since the last call to this
 * method. Includes overhead of control packets.
 *
 * @return the number of bytes that has been put on the sockets since the last call
 */
uint64 UploadBandwidthThrottler::GetNumberOfSentBytesSinceLastCallAndReset()
{
        uint64 numberOfSentBytesSinceLastCall = m_SentBytesSinceLastCall;
        m_SentBytesSinceLastCall = 0;

        return numberOfSentBytesSinceLastCall;
}

/**
 * Find out how many bytes that has been put on the sockets since the last call to this
 * method. Excludes overhead of control packets.
 *
 * @return the number of bytes that has been put on the sockets since the last call
 */
uint64 UploadBandwidthThrottler::GetNumberOfSentBytesOverheadSinceLastCallAndReset()
{
        uint64 numberOfSentBytesSinceLastCall = m_SentBytesSinceLastCallOverhead;
        m_SentBytesSinceLastCallOverhead = 0;

        return numberOfSentBytesSinceLastCall;
}


/**
 * Add a socket to the list of sockets that have upload slots. The main thread will
 * continously call send on these sockets, to give them chance to work off their queues.
 * The sockets are called in the order they exist in the list, so the top socket (index 0)
 * will be given a chance first to use bandwidth, and then the next socket (index 1) etc.
 *
 * It is possible to add a socket several times to the list without removing it inbetween,
 * but that should be avoided.
 *
 * @param index insert the socket at this place in the list. An index that is higher than the
 *              current number of sockets in the list will mean that the socket should be inserted
 *              last in the list.
 *
 * @param socket the address to the socket that should be added to the list. If the address is NULL,
 *               this method will do nothing.
 */
void UploadBandwidthThrottler::AddToStandardList(uint32 index, ThrottledFileSocket* socket)
{
        if ( socket ) {
                RemoveFromStandardListNoLock(socket);
                if (index > (uint32)m_StandardOrder_list.size()) {
                        index = m_StandardOrder_list.size();
                }

                m_StandardOrder_list.insert(m_StandardOrder_list.begin() + index, socket);
        }
}


/**
 * Remove a socket from the list of sockets that have upload slots.
 *
 * If the socket has mistakenly been added several times to the list, this method
 * will return all of the entries for the socket.
 *
 * @param socket the address of the socket that should be removed from the list. If this socket
 *               does not exist in the list, this method will do nothing.
 */
bool UploadBandwidthThrottler::RemoveFromStandardList(ThrottledFileSocket* socket)
{
        return RemoveFromStandardListNoLock(socket);
}


/**
 * Remove a socket from the list of sockets that have upload slots. NOT THREADSAFE!
 * This is an internal method that doesn't take the necessary lock before it removes
 * the socket. This method should only be called when the current thread already owns
 * the m_sendLocker lock!
 *
 * @param socket address of the socket that should be removed from the list. If this socket
 *               does not exist in the list, this method will do nothing.
 */
bool UploadBandwidthThrottler::RemoveFromStandardListNoLock(ThrottledFileSocket* socket)
{
        return (EraseFirstValue( m_StandardOrder_list, socket ) > 0);
}


/**
* Notifies the send thread that it should try to call controlpacket send
* for the supplied socket. It is allowed to call this method several times
* for the same socket, without having controlpacket send called for the socket
* first. The doublette entries are never filtered, since it is incurs less cpu
* overhead to simply call Send() in the socket for each double. Send() will
* already have done its work when the second Send() is called, and will just
* return with little cpu overhead.
*
* @param socket address to the socket that requests to have controlpacket send
*               to be called on it
*/
void UploadBandwidthThrottler::QueueForSendingControlPacket(ThrottledControlSocket* socket, bool hasSent)
{
        if ( m_doRun ) {
                if( hasSent ) {
                        m_TempControlQueueFirst_list.push_back(socket);
                } else {
                        m_TempControlQueue_list.push_back(socket);
                }
        }
}



/**
 * Remove the socket from all lists and queues. This will make it safe to
 * erase/delete the socket. It will also cause the main thread to stop calling
 * send() for the socket.
 *
 * @param socket address to the socket that should be removed
 */
void UploadBandwidthThrottler::DoRemoveFromAllQueues(ThrottledControlSocket* socket)
{
        if ( m_doRun ) {
                // Remove this socket from control packet queue
                EraseValue( m_ControlQueue_list, socket );
                EraseValue( m_ControlQueueFirst_list, socket );
                EraseValue( m_TempControlQueue_list, socket );
                EraseValue( m_TempControlQueueFirst_list, socket );
        }
}


void UploadBandwidthThrottler::RemoveFromAllQueues(ThrottledControlSocket* socket)
{
        DoRemoveFromAllQueues( socket );
}


void UploadBandwidthThrottler::RemoveFromAllQueues(ThrottledFileSocket* socket)
{
        if (m_doRun) {
                DoRemoveFromAllQueues(socket);

                // And remove it from upload slots
                RemoveFromStandardListNoLock(socket);
        }
}


/**
 * Make the thread exit. This method will not return until the thread has stopped
 * looping. This guarantees that the thread will not access the CEMSockets after this
 * call has exited.
 */
void UploadBandwidthThrottler::EndThread()
{
        if (m_doRun) {	// do it only once
                // signal the thread to stop looping and exit.
                m_doRun = false;
        }
}


/**
 * The thread method that handles calling send for the individual sockets.
 *
 * Control packets will always be tried to be sent first. If there is any bandwidth leftover
 * after that, send() for the upload slot sockets will be called in priority order until we have run
 * out of available bandwidth for this loop. Upload slots will not be allowed to go without having sent
 * called for more than a defined amount of time (i.e. two seconds).
 *
 * @return always returns 0.
 */

void UploadBandwidthThrottler::Entry()
{
        const uint32 TIME_BETWEEN_UPLOAD_LOOPS = 1;

        cr_begin(entryCtx);
        
        m_sleep.Bind(wxEVT_TIMER, [this](wxTimerEvent &){this->Entry();});
        entryCtx.lastLoopTick = GetTickCountFullRes();
        // Bytes to spend in current cycle. If we spend more this becomes negative and causes a wait next time.
        entryCtx.bytesToSpend = 0;
        entryCtx.allowedDataRate = 0;
        entryCtx.rememberedSlotCounter = 0;
        entryCtx.extraSleepTime = TIME_BETWEEN_UPLOAD_LOOPS;
        
        while (m_doRun) {
                entryCtx.timeSinceLastLoop = GetTickCountFullRes() - entryCtx.lastLoopTick;
                
                // Calculate data rate
                if (thePrefs::GetMaxUpload() == UNLIMITED) {
                        // Try to increase the upload rate from UploadSpeedSense
                        entryCtx.allowedDataRate = (uint32)theStats::GetUploadRate() + 5 * 1024;
                } else {
                        entryCtx.allowedDataRate = thePrefs::GetMaxUpload() * 1024;
                }
                
                entryCtx.minFragSize = 1300;
                entryCtx.doubleSendSize = entryCtx.minFragSize*2; // send two packages at a time so they can share an ACK
                if (entryCtx.allowedDataRate < 6*1024) {
                        entryCtx.minFragSize = 536;
                        entryCtx.doubleSendSize = entryCtx.minFragSize; // don't send two packages at a time at very low speeds to give them a smoother load
                }
                
                
                if (entryCtx.bytesToSpend < 1) {
                        // We have sent more than allowed in last cycle so we have to wait now
                        // until we can send at least 1 byte.
                        entryCtx.sleepTime = std::max((-entryCtx.bytesToSpend + 1) * 1000 / entryCtx.allowedDataRate + 2, // add 2 ms to allow for rounding inaccuracies
                                             entryCtx.extraSleepTime);
                } else {
                        // We could send at once, but sleep a while to not suck up all cpu
                        entryCtx.sleepTime = entryCtx.extraSleepTime;
                }
                
                if(entryCtx.timeSinceLastLoop < entryCtx.sleepTime) {
                        m_sleep.Start(entryCtx.sleepTime-entryCtx.timeSinceLastLoop, true);
                        cr_return(,entryCtx);
                }
                
                // Check after sleep in case the thread has been signaled to end
                if (!m_doRun) {
                        break;
                }
                
                entryCtx.thisLoopTick = GetTickCountFullRes();
                entryCtx.timeSinceLastLoop = entryCtx.thisLoopTick - entryCtx.lastLoopTick;
                entryCtx.lastLoopTick = entryCtx.thisLoopTick;
                
                if(entryCtx.timeSinceLastLoop > entryCtx.sleepTime + 2000) {
                        AddDebugLogLineN(logGeneral, CFormat(wxT("UploadBandwidthThrottler: Time since last loop too long. time: %ims wanted: %ims Max: %ims"))
                        % entryCtx.timeSinceLastLoop % entryCtx.sleepTime % (entryCtx.sleepTime + 2000));
                        
                        entryCtx.timeSinceLastLoop = entryCtx.sleepTime + 2000;
                }
                
                // Calculate how many bytes we can spend
                
                entryCtx.bytesToSpend += (sint32) (entryCtx.allowedDataRate / 1000.0 * entryCtx.timeSinceLastLoop);
                
                if(entryCtx.bytesToSpend >= 1) {
                        sint32 spentBytes = 0;
                        sint32 spentOverhead = 0;
                        
                        // are there any sockets in m_TempControlQueue_list? Move them to normal m_ControlQueue_list;
                        m_ControlQueueFirst_list.insert(	m_ControlQueueFirst_list.end(),
                                                                m_TempControlQueueFirst_list.begin(),
                                                                m_TempControlQueueFirst_list.end() );
                        
                        m_ControlQueue_list.insert( m_ControlQueue_list.end(),
                                                    m_TempControlQueue_list.begin(),
                                                    m_TempControlQueue_list.end() );
                        
                        m_TempControlQueue_list.clear();
                        m_TempControlQueueFirst_list.clear();
                        
                        // Send any queued up control packets first
                        while (spentBytes < entryCtx.bytesToSpend && (!m_ControlQueueFirst_list.empty() || !m_ControlQueue_list.empty())) {
                                ThrottledControlSocket* socket = NULL;
                                
                                if(!m_ControlQueueFirst_list.empty()) {
                                        socket = m_ControlQueueFirst_list.front();
                                        m_ControlQueueFirst_list.pop_front();
                                } else if(!m_ControlQueue_list.empty()) {
                                        socket = m_ControlQueue_list.front();
                                        m_ControlQueue_list.pop_front();
                                }
                                
                                if(socket != NULL) {
                                        SocketSentBytes socketSentBytes = socket->SendControlData(entryCtx.bytesToSpend-spentBytes, entryCtx.minFragSize);
                                        spentBytes += socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                                        spentOverhead += socketSentBytes.sentBytesControlPackets;
                                }
                        }
                        
                        // Check if any sockets haven't gotten data for a long time. Then trickle them a package.
                        uint32 slots = m_StandardOrder_list.size();
                        for (uint32 slotCounter = 0; slotCounter < slots; slotCounter++) {
                                ThrottledFileSocket* socket = m_StandardOrder_list[ slotCounter ];
                                
                                if(socket != NULL) {
                                        if(entryCtx.thisLoopTick-socket->GetLastCalledSend() > SEC2MS(1)) {
                                                // trickle
                                                uint32 neededBytes = socket->GetNeededBytes();
                                                
                                                if(neededBytes > 0) {
                                                        SocketSentBytes socketSentBytes = socket->SendFileAndControlData(neededBytes, entryCtx.minFragSize);
                                                        spentBytes += socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                                                        spentOverhead += socketSentBytes.sentBytesControlPackets;
                                                }
                                        }
                                } else {
                                        AddDebugLogLineN(logGeneral, CFormat( wxT("There was a NULL socket in the UploadBandwidthThrottler Standard list (trickle)! Prevented usage. Index: %i Size: %i"))
                                        % slotCounter % m_StandardOrder_list.size());
                                }
                        }
                        
                        // Give available bandwidth to slots, starting with the one we ended with last time.
                        // There are two passes. First pass gives packets of doubleSendSize, second pass
                        // gives as much as possible.
                        // Second pass starts with the last slot of the first pass actually.
                        for (uint32 slotCounter = 0; (slotCounter < slots * 2) && spentBytes < entryCtx.bytesToSpend; slotCounter++) {
                                if (entryCtx.rememberedSlotCounter >= slots) {	// wrap around pointer
                                        entryCtx.rememberedSlotCounter = 0;
                                }
                                
                                uint32 data = (slotCounter < slots - 1)	? entryCtx.doubleSendSize				// pass 1
                                : (entryCtx.bytesToSpend - spentBytes);	// pass 2
                                ThrottledFileSocket* socket = m_StandardOrder_list[ entryCtx.rememberedSlotCounter ];
                                
                                if(socket != NULL) {
                                        SocketSentBytes socketSentBytes = socket->SendFileAndControlData(data, entryCtx.doubleSendSize);
                                        spentBytes += socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                                        spentOverhead += socketSentBytes.sentBytesControlPackets;
                                } else {
                                        AddDebugLogLineN(logGeneral, CFormat(wxT("There was a NULL socket in the UploadBandwidthThrottler Standard list (equal-for-all)! Prevented usage. Index: %i Size: %i"))
                                        % entryCtx.rememberedSlotCounter % m_StandardOrder_list.size());
                                }
                                
                                entryCtx.rememberedSlotCounter++;
                        }
                        
                        // Do some limiting of what we keep for the next loop.
                        entryCtx.bytesToSpend -= spentBytes;
                        sint32 minBytesToSpend = (slots + 1) * entryCtx.minFragSize;
                        
                        if (entryCtx.bytesToSpend < - minBytesToSpend) {
                                entryCtx.bytesToSpend = - minBytesToSpend;
                        } else {
                                sint32 bandwidthSavedTolerance = slots * 512 + 1;
                                if (entryCtx.bytesToSpend > bandwidthSavedTolerance) {
                                        entryCtx.bytesToSpend = bandwidthSavedTolerance;
                                }
                        }
                        
                        m_SentBytesSinceLastCall += spentBytes;
                        m_SentBytesSinceLastCallOverhead += spentOverhead;
                        
                        if (spentBytes == 0) {	// spentBytes includes the overhead
                                entryCtx.extraSleepTime = std::min<uint32>(entryCtx.extraSleepTime * 5, 1000); // 1s at most
                        } else {
                                entryCtx.extraSleepTime = TIME_BETWEEN_UPLOAD_LOOPS;
                        }
                }
        }
        
        m_TempControlQueue_list.clear();
        m_TempControlQueueFirst_list.clear();
        m_ControlQueue_list.clear();
        m_StandardOrder_list.clear();
        
        cr_end(entryCtx);
}
// File_checked_for_headers