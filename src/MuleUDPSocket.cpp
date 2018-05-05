//
// This file is part of the imule Project.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/wx.h>
#include <algorithm>

#include "MuleUDPSocket.h"              // Interface declarations
#include "i2p/CSamDefines.h"

#include <protocol/ed2k/Constants.h>

#include "amule.h"                      // Needed for theApp
#include "GetTickCount.h"               // Needed for GetTickCount()
#include "Packet.h"                     // Needed for CPacket
#include <common/StringFunctions.h>     // Needed for unicode2char
#include "Logger.h"                     // Needed for AddDebugLogLine{C,N}
#include "UploadBandwidthThrottler.h"
#include "EncryptedDatagramSocket.h"
#include "OtherFunctions.h"
#include "kademlia/kademlia/Prefs.h"
#include "ClientList.h"
#include "common/Format.h"
#include "DataToText.h"
#include <wx/apptrait.h>
#include "MuleThread.h"

static uint16_t packetHash ( const byte * buf, int size )
{
        uint16_t h = 0;

        while ( size-- > 0 )
                h = (uint16_t) (h + * ( buf++ ));

        return h ;
}

CMuleUDPSocket::CMuleUDPSocket(const wxString& name/*, int id*/, wxString key, const CProxyData* ProxyData)
        :
        m_busy(false),
        m_name(name),
        //         m_id(id),
        m_privKey ( key ),
        m_proxy(ProxyData),
        m_socket(NULL)
{
}


CMuleUDPSocket::~CMuleUDPSocket()
{
        if (theApp->uploadBandwidthThrottler)
                theApp->uploadBandwidthThrottler->RemoveFromAllQueues(this);

        wiMutexLocker lock(m_mutex);
        DestroySocket();
}


void CMuleUDPSocket::CreateSocket()
{
        wxCHECK_RET(!m_socket, wxT("Socket already opened."));

        m_socket = new CEncryptedDatagramSocket(m_privKey, wxSOCKET_NOWAIT);
        SetClientData(m_socket_data);
        SetEventHandler(*m_socket_handler, m_socket_id);
        SetNotify( m_socket_flags);
        Notify(m_socket_notify);

        /* iMule : socket creation takes time. Do not destroy it too soon !
        if (!m_socket->Ok()) {
                AddDebugLogLineC(logMuleUDP, wxT("Failed to create valid ") + m_name);
                DestroySocket();
        } else {
                AddLogLineN(wxString(wxT("Created ")) << m_name << wxT(" at port ") << m_privKey);
        }
        */
}


wxString CMuleUDPSocket::GetPrivKey()
{
        if (! m_socket)
                return wxT("");
        else
                return m_socket->GetPrivKey() ;
}

void CMuleUDPSocket::SetClientData( void *data )
{
        m_socket_data = data ;
        if ( m_socket )
                m_socket->SetClientData ( data );
}

void CMuleUDPSocket::SetEventHandler( wxEvtHandler& handler, int id )
{
        m_socket_handler = &handler ;
        m_socket_id      = id ;
        if ( m_socket )
                m_socket->SetEventHandler ( handler, id );
}

void CMuleUDPSocket::SetNotify( wxSocketEventFlags flags )
{
        m_socket_flags = flags ;
        if ( m_socket )
                m_socket->SetNotify ( flags );
}

void CMuleUDPSocket::Notify( bool notify )
{
        m_socket_notify = notify ;
        if ( m_socket )
                m_socket->Notify ( notify );
}

bool CMuleUDPSocket::Destroy()
{
        DestroySocket();

        // schedule this object for deletion
#if wxCHECK_VERSION(3, 0, 0)
        if ( wxTheApp ) {
                // let the traits object decide what to do with us
                wxGetApp().ScheduleForDestruction(this);
        } else { // no app or no traits
                // in wxBase we might have no app object at all, don't leak memory
                delete this;
        }
#else
        wxAppTraits *traits = wxTheApp ? wxTheApp->GetTraits() : NULL;
        if ( traits ) {
                // let the traits object decide what to do with us
                traits->ScheduleForDestroy(this);
        } else { // no app or no traits
                // in wxBase we might have no app object at all, don't leak memory
                delete this;
	}
#endif
        return true;
}

void CMuleUDPSocket::DestroySocket()
{
        if (m_socket) {
                AddDebugLogLineN(logMuleUDP, wxT("Shutting down ") + m_name);
                m_socket->SetNotify(0);
                m_socket->Notify(false);
                m_socket->Close();
                m_socket->Destroy();
                m_socket = NULL;
        }
}


void CMuleUDPSocket::Open()
{
        wiMutexLocker lock(m_mutex);

        CreateSocket();
}


void CMuleUDPSocket::Close()
{
        wiMutexLocker lock(m_mutex);

        DestroySocket();
}


void CMuleUDPSocket::OnSend(int errorCode)
{
        if (errorCode) {
                return;
        }

        {
                wiMutexLocker lock(m_mutex);
                m_busy = false;
                if (m_queue.empty()) {
                        return;
                }
        }

        theApp->uploadBandwidthThrottler->QueueForSendingControlPacket(this);
}


const unsigned UDP_BUFFER_SIZE = SAM_DGRAM_PAYLOAD_MAX;


void CMuleUDPSocket::OnReceive(int errorCode)
{
        AddDebugLogLineN(logMuleUDP, CFormat(wxT("Got UDP callback for read: Error %i Socket state %i"))
                         % errorCode % Ok());

        char buffer[UDP_BUFFER_SIZE];
        CI2PAddress addr;
        unsigned length = 0;
        bool error = false;
        int lastError = 0;

        {
                wiMutexLocker lock(m_mutex);

                if (errorCode || (m_socket == NULL) || !m_socket->Ok()) {
                        DestroySocket();
                        CreateSocket();

                        return;
                }


                length = m_socket->RecvFrom(addr, buffer, UDP_BUFFER_SIZE).LastCount();
                error = m_socket->Error();
                lastError = m_socket->LastError();
        }

        if (error) {
                OnReceiveError(lastError, addr);
        } else if (length < 2) {
                // 2 bytes (protocol and opcode) is the smallets possible packet.
                AddDebugLogLineN(logMuleUDP, m_name + wxT(": Invalid Packet received"));
        } else if ( !addr ) {
                AddLogLineNS ( wxT("Unknown dest receiving on UDP packet! Ignoring: '") + addr.humanReadable() + wxT("'"));
        } else if (theApp->clientlist->IsBannedClient(addr)) {
                AddDebugLogLineN(logMuleUDP, m_name + wxT(": Dropped packet from banned dest ") + addr.humanReadable());
        } else {
                AddDebugLogLineN(logMuleUDP, (m_name + wxT(": Packet received ("))
                                 << addr.humanReadable() << wxT("): ")
                                 << length << wxT("b"));
                if ( length > UDP_BUFFER_SIZE )
                        AddDebugLogLineN( logMuleUDP, CFormat(_("Strange : length(%d) > UDP_BUFFER_SIZE(%d) in"
                                                                " CMuleUDPSocket::OnReceive. protocol was %s, opcode was %s."))
                                          % length % UDP_BUFFER_SIZE %opstr(buffer[0]) % opstr(buffer[0], buffer[1]) );
                OnPacketReceived ( addr, (byte*)buffer, length );
        }
}


void CMuleUDPSocket::OnReceiveError(int errorCode, const CI2PAddress& WXUNUSED(from))
{
        AddDebugLogLineN(logMuleUDP, (m_name + wxT(": Error while reading: ")) << errorCode);
}


void CMuleUDPSocket::OnDisconnected(int WXUNUSED(errorCode))
{
        /* Due to bugs in wxWidgets, UDP sockets will sometimes
         * be closed. This is caused by the fact that wx treats
         * zero-length datagrams as EOF, which is only the case
         * when dealing with streaming sockets.
         *
         * This has been reported as patch #1885472:
         * http://sourceforge.net/tracker/index.php?func=detail&aid=1885472&group_id=9863&atid=309863
         */
        AddDebugLogLineC(logMuleUDP, m_name + wxT("Socket died, recreating."));
        DestroySocket();
        CreateSocket();
}


void CMuleUDPSocket::SendPacket(std::unique_ptr<CPacket> packet, const CI2PAddress & dest, bool bEncrypt, const uint8* pachTargetClientHashORKadID, bool bKad, uint32 nReceiverVerifyKey)
{
        wxCHECK_RET(packet, wxT("Invalid packet."));
        /*wxCHECK_RET(port, wxT("Invalid port."));
        wxCHECK_RET(IP, wxT("Invalid IP."));
        */

        if (!dest.isValid()) {
                return;
        }
        // check packet size is not too big
        if ( packet->GetPacketSize() > UDP_BUFFER_SIZE ) {
                AddDebugLogLineC(logMuleUDP, wxT("Not sending packet. Reason: too large"));
        }
        wxCHECK_RET ( packet->GetPacketSize() <= UDP_BUFFER_SIZE , (wxString) (CFormat(wxT( "Packet too big : %" PRIu32 "b (max: %db"))
                        % packet->GetPacketSize() % UDP_BUFFER_SIZE) ) ;

        if (!Ok()) {
                AddDebugLogLineN(logMuleUDP, CFormat ( wxT ( "%s: Packet discarded (socket not Ok) from %x (size=%db)" ) )
                                 % m_name % dest.hashCode() % packet->GetPacketSize() );
                return;
        }

        AddDebugLogLineN(logMuleUDP, CFormat ( wxT ( "%s: Packet queued to %x (size=%db, hash=%x)" ) )
                         % m_name % dest.hashCode() % packet->GetPacketSize() %
                         packetHash ( packet->GetDataBuffer(), packet->GetPacketSize() ) );

        UDPPack newpending;
        newpending.dest = dest;
        newpending.packet = std::move(packet);
        newpending.time = GetTickCount();
        newpending.bEncrypt = bEncrypt && (pachTargetClientHashORKadID != NULL || (bKad && nReceiverVerifyKey != 0));
        newpending.bKad = bKad;
        newpending.nReceiverVerifyKey = nReceiverVerifyKey;
        if (newpending.bEncrypt && pachTargetClientHashORKadID != NULL) {
                md4cpy(newpending.pachTargetClientHashORKadID, pachTargetClientHashORKadID);
        } else {
                md4clr(newpending.pachTargetClientHashORKadID);
        }

        {
                wiMutexLocker lock(m_mutex);
                m_queue.push_back(std::move(newpending));
        }

        theApp->uploadBandwidthThrottler->QueueForSendingControlPacket(this);
}


bool CMuleUDPSocket::Ok()
{
        wiMutexLocker lock(m_mutex);

        return m_socket && m_socket->Ok();
}

const uint32_t & CMuleUDPSocket::maxPacketDataSize()
{
        static const uint32_t l = CPacket::UDPPacketMaxDataSize();
        return l ;
}

SocketSentBytes CMuleUDPSocket::SendControlData(uint32 maxNumberOfBytesToSend, uint32 WXUNUSED(minFragSize))
{
        wiMutexLocker lock(m_mutex);
        uint32 sentBytes = 0;
        while (!m_queue.empty() && !m_busy && (sentBytes < maxNumberOfBytesToSend)) {
                UDPPack & item = m_queue.front();
                std::unique_ptr<CPacket> & packet = item.packet;
                if (GetTickCount() - item.time < UDPMAXQUEUETIME) {
                        uint32_t len = packet->GetPacketSize() + 2;
                        std::unique_ptr<uint8_t[]> sendbuffer(new uint8_t [len]);
                        memcpy(sendbuffer.get(), packet->GetUDPHeader(), 2);
                        memcpy(sendbuffer.get() + 2, packet->GetDataBuffer(), packet->GetPacketSize());

                        if (item.bEncrypt && (theApp->GetPublicDest().isValid() || item.bKad)) {
                                len = CEncryptedDatagramSocket::EncryptSendClient(sendbuffer, len, item.pachTargetClientHashORKadID, item.bKad, item.nReceiverVerifyKey, (item.bKad ? Kademlia::CPrefs::GetUDPVerifyKey(item.dest) : 0));
                        }

                        if (SendTo(sendbuffer.get(), len, item.dest)) {
                                sentBytes += len;
                                m_queue.pop_front();
                        } else {
                                // TODO: Needs better error handling, see SentTo
                                break;
                        }
                } else {
                        m_queue.pop_front();
                }
        }
        if (!m_busy && !m_queue.empty()) {
                theApp->uploadBandwidthThrottler->QueueForSendingControlPacket(this);
        }
        SocketSentBytes returnVal = { true, 0, sentBytes };

        return returnVal;
}


bool CMuleUDPSocket::SendTo(uint8_t *buffer, uint32_t length, const CI2PAddress & addr)
{
        // Just pretend that we sent the packet in order to avoid infinite loops.
        if (!(m_socket && m_socket->Ok())) {
                return true;
        }


        // We better clear this flag here, status might have been changed
        // between the U.B.T. addition and the real sending happening later
        m_busy = false;
        bool sent = false;
        m_socket->SendTo(addr, buffer, length);
        if (m_socket->Error()) {
                wxSocketError error = m_socket->LastError();

                if (error == wxSOCKET_WOULDBLOCK) {
                        // Socket is busy and can't send this data right now,
                        // so we just return not sent and set the wouldblock
                        // flag so it gets resent when socket is ready.
                        m_busy = true;
                } else {
                        // An error which we can't handle happended, so we drop
                        // the packet rather than risk entering an infinite loop.
                        AddLogLineN((wxT("WARNING! ") + m_name + wxT(": Packet to "))
                                    << addr.humanReadable()
                                    << wxT(" discarded due to error (") << error << wxT(") while sending."));
                        sent = true;
                }
        } else {
                AddDebugLogLineN(logMuleUDP, (m_name + wxT(": Packet sent ("))
                                 << addr.humanReadable() << wxT("): ")
                                 << length << wxT("b"));
                sent = true;
        }

        return sent;
}

bool CMuleUDPSocket::GetLocal(CI2PAddress & ret)
{
        if (m_socket) {
                m_socket->GetLocal(ret);
                return true;
        }
        return false;
}

// File_checked_for_headers
