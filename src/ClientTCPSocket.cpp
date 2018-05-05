//
// This file is part of the imule Project.
//
// Copyright (c) 2003-201 imule Team ( mkvore@mail.i2p / http://www.imule.i2p )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include <cstdint>

#include "ClientTCPSocket.h"	// Interface declarations

#include <protocol/Protocols.h>
#include <protocol/ed2k/Client2Client/TCP.h>
#include <protocol/ed2k/Client2Client/UDP.h> // Sometimes we reply with UDP packets.
#include <protocol/ed2k/ClientSoftware.h>
#include <protocol/kad2/Client2Client/TCP.h>
#include <common/EventIDs.h>

#include "Preferences.h"	// Needed for thePrefs
#include "DataToText.h"		// Needed for CONNECTION_TIMEOUT
#include "Packet.h"		// Needed for CPacket
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"		// Neeed for logRemoteClient
#include "updownclient.h"	// Needed for CUpDownClient
#include <common/Format.h>	// Needed for CFormat
#include "amule.h"		// Needed for theApp
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "ClientList.h"		// Needed for CClientList
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "PartFile.h"		// Needed for CPartFile
#include "MemFile.h"		// Needed for CMemFile
#include "kademlia/kademlia/Kademlia.h" // Needed for CKademlia::Kademlia
#include "kademlia/kademlia/Prefs.h"	// Needed for CKademlia::CPrefs
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "Server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "IPFilter.h"		// Needed for CIPFilter
#include "ListenSocket.h"	// Needed for CListenSocket
#include "GuiEvents.h"		// Needed for Notify_*
#include "i2p/wxI2PEvents.h"


//#define __PACKET_RECV_DUMP__

void CClientTCPSocket::OnSocketEvent(CI2PSocketEvent& event)
{
        wxI2PSocketBase* baseSocket = event.GetSocket();
//	wxASSERT(baseSocket);	// Rather want a log message right now. Enough other wx problems. >:(
        if (!baseSocket) {		// WTF?
                AddDebugLogLineN(logClient, wxT("received bad wxSocketEvent"));
                return;
        }
        CClientTCPSocket *socket = dynamic_cast<CClientTCPSocket *>(baseSocket);
        wxASSERT(socket==this);

        //         if (!socket) {
        //                 return;
        //         }
        //         if (socket!=this) {
        //                 return;                
        //         }
        if (OnDestroy() || ForDeletion()) {
                return;
        }

        switch(event.GetSocketEvent()) {
        case wxSOCKET_LOST:
                OnError(0xFEFF /* SOCKET_LOST is not an error */);
                break;
        case wxSOCKET_INPUT:
                OnReceive(0);
                break;
        case wxSOCKET_OUTPUT:
                OnSend(0);
                break;
        case wxSOCKET_CONNECTION:
                // connection stablished, nothing to do about it?
                OnConnect(Error() ? LastError() : 0);
                break;
        default:
                // Nothing should arrive here...
                wxFAIL;
                break;
        }
}


//------------------------------------------------------------------------------
// CClientTCPSocket
//------------------------------------------------------------------------------
wxDEFINE_EVENT(ID_CLIENTTCPSOCKET_EVENT, CI2PSocketEvent);

CClientTCPSocket::CClientTCPSocket(CUpDownClient * in_client):CEMSocket()
{
        AddDebugLogLineN(logSocketList, CFormat("new CClientTCPSocket at address %p") % this);
        SetClient(in_client);
        if (in_client) {
		m_remotedest = in_client->GetUserIDHybrid();
        } else {
                m_remotedest = CI2PAddress::null;
        }
        ResetTimeOutTimer();
        m_ForDeletion = false;
        
        theApp->listensocket->AddSocket(this);
        theApp->listensocket->AddConnection();

        Bind(ID_CLIENTTCPSOCKET_EVENT, &CClientTCPSocket::OnSocketEvent, this);
        SetEventHandler(*this, ID_CLIENTTCPSOCKET_EVENT);
        SetNotify(
                wxSOCKET_CONNECTION_FLAG |
                wxSOCKET_INPUT_FLAG |
                wxSOCKET_OUTPUT_FLAG |
                wxSOCKET_LOST_FLAG);
        Notify(true);
}

CClientTCPSocket::~CClientTCPSocket()
{
        // remove event handler
        SetNotify(0);
        Notify(false);

        if (m_client) {
                m_client->SetSocket( NULL );
        }
        m_client = NULL;

        if (theApp->listensocket && !theApp->listensocket->OnShutdown()) {
                theApp->listensocket->RemoveSocket(this);
        }
        AddDebugLogLineN(logSocketList, CFormat("deleted CClientTCPSocket from address %p") % this);
}

bool CClientTCPSocket::InitNetworkData()
{
        wxASSERT(!m_remotedest.isValid());
        wxASSERT(!m_client);
        CI2PAddress addr;
        GetPeer(addr);
        m_remotedest = addr;

        MULE_CHECK(addr.isValid(), false);

        if (theApp->ipfilter->IsFiltered(m_remotedest)) {
                AddDebugLogLineN(logClient, wxT("Denied connection from ") + addr.humanReadable() + wxT("(Filtered IP)"));
                return false;
        } else if (theApp->clientlist->IsBannedClient(m_remotedest)) {
                AddDebugLogLineN(logClient, wxT("Denied connection from ") + addr.humanReadable() + wxT("(Banned IP)"));
                return false;
        } else {
                AddDebugLogLineN(logClient, wxT("Accepted connection from ") + addr.humanReadable());
                return true;
        }
}

void CClientTCPSocket::ResetTimeOutTimer()
{
        timeout_timer = ::GetTickCount();
}


bool CClientTCPSocket::CheckTimeOut()
{
        // 0.42x
        uint32 uTimeout = GetTimeOut();
        if (m_client) {

                /*NOTE: I2P = no buddy
                if (m_client->GetKadState() == KS_CONNECTED_BUDDY) {
                        //We originally ignored the timeout here for buddies.
                        //This was a stupid idea on my part. There is now a ping/pong system
                        //for buddies. This ping/pong system now prevents timeouts.
                        //This release will allow lowID clients with KadVersion 0 to remain connected.
                        //But a soon future version needs to allow these older clients to time out to prevent dead connections from continuing.
                        //JOHNTODO: Don't forget to remove backward support in a future release.
                        if ( m_client->GetKadVersion() == 0 ) {
                                return false;
                        }

                        uTimeout += MIN2MS(15);
                }
                */
                if (m_client->GetChatState() != MS_NONE) {
                        uTimeout += CONNECTION_TIMEOUT;
                }
        }

        if (::GetTickCount() - timeout_timer > uTimeout) {
                timeout_timer = ::GetTickCount();
                Disconnect(wxT("Timeout"));
                return true;
        }

        return false;
}


void CClientTCPSocket::SetClient(CUpDownClient* pClient)
{
        m_client = pClient;
        if (m_client) {
                m_client->SetSocket( this );
        }
}


void CClientTCPSocket::OnClose(int nErrorCode)
{
        // 0.42x
        wxASSERT(theApp->listensocket->IsValidSocket(this));
        CEMSocket::OnClose(nErrorCode);
        if (nErrorCode) {
                Disconnect(CFormat(wxT("Closed: %d")) % nErrorCode);
        } else {
                Disconnect(wxT("Close"));
        }
}


void CClientTCPSocket::Disconnect(const wxString& strReason)
{
        byConnected = ES_DISCONNECTED;
        if (m_client) {
                if (m_client->Disconnected(strReason, true)) {
                        // Somehow, Safe_Delete() is beeing called by Disconnected(),
                        // or any other function that sets m_client to NULL,
                        // so we must check m_client first.
                        if (m_client) {
                                m_client->SetSocket( NULL );
                                m_client->Safe_Delete();
                        }
                }
                m_client = NULL;
        }

        Safe_Delete();
}


void CClientTCPSocket::Safe_Delete()
{
        // More paranoia - make sure client is unlinked in any case
        if (m_client) {
                m_client->SetSocket( NULL );
                m_client = NULL;
        }

        if ( !ForDeletion() && !OnDestroy() ) {
                // Paranoia is back.
                SetNotify(0);
                Notify(false);
                // lfroen: first of all - stop handler
                m_ForDeletion = true;

                byConnected = ES_DISCONNECTED;
                Close(); // Destroy is suposed to call Close(), but.. it doesn't hurt.
                Destroy();
                AddDebugLogLineN(logSocketList, CFormat("Safe_Delete CClientTCPSocket at address %p") % this);
        }
}


void CClientTCPSocket::Safe_Delete_Client()
{
        if (m_client) {
                m_client->Safe_Delete();
                m_client = NULL;
        }
}


bool CClientTCPSocket::ProcessPacket(const byte* buffer, uint32 size, uint8 opcode)
{
#ifdef __PACKET_RECV_DUMP__
        //printf("Rec: OPCODE %x \n",opcode);
        DumpMem(buffer, size);
#endif
        if (!m_client && opcode != OP_HELLO) {
                throw wxString(CFormat(wxT("%x : Asks for something without saying hello")) % (long) this);
        } else if (m_client && opcode != OP_HELLO && opcode != OP_HELLOANSWER) {
                m_client->CheckHandshakeFinished();
        }

        switch(opcode) {
        case OP_HELLOANSWER: {	// 0.43b
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_HELLOANSWER from ") + m_client->GetTCPDest().humanReadable());
                theStats::AddDownOverheadOther(size);
                m_client->ProcessHelloAnswer(buffer, size);

                // start secure identification, if
                //  - we have received OP_EMULEINFO and OP_HELLOANSWER (old eMule)
                //	- we have received eMule-OP_HELLOANSWER (new eMule)
                if (m_client->GetInfoPacketsReceived() == IP_BOTH) {
                        m_client->InfoPacketsReceived();
                }

                // Socket might die because of sending in InfoPacketsReceived, so check
                if (m_client) {
                        m_client->ConnectionEstablished();
                }

                // Socket might die on ConnectionEstablished somehow. Check it.
                if (m_client) {
                        Notify_SharedCtrlRefreshClient( m_client->ECID() , AVAILABLE_SOURCE);
                }

                break;
        }
        case OP_HELLO: {	// 0.43b

                theStats::AddDownOverheadOther(size);
                bool bNewClient = !m_client;
                if (bNewClient) {
                        // create new client to save standart informations
                        m_client = new CUpDownClient(this);
                }

                // Do not move up!
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_HELLO from ") + m_client->GetTCPDest().humanReadable() );

                bool bIsMuleHello = false;

                try {
                        bIsMuleHello = m_client->ProcessHelloPacket(buffer, size);
                } catch(...) {
                        if (bNewClient && m_client) {
                                // Don't let CUpDownClient::Disconnected be processed for a client which is not in the list of clients.
                                m_client->Safe_Delete();
                                m_client = NULL;
                        }
                        throw;
                }

                if (thePrefs::ParanoidFilter() && !IsLowID(m_client->GetUserIDHybrid()) && (GetRemoteDest() != m_client->GetUserIDHybrid())) {
                        wxString reason = wxT("Client claims a different Dest from the one we received the hello packet from: ");
                        reason += m_client->GetUserIDHybrid().humanReadable() + wxT(" / ") + GetRemoteDest().humanReadable();
                        AddDebugLogLineN(logClient, reason);
                        if (bNewClient) {
                                m_client->Safe_Delete();
                                m_client = NULL;
                        }
                        Disconnect(wxT("Paranoid disconecting: ") + reason);
                        return false;
                }

                // if IP is filtered, dont reply but disconnect...
                if (theApp->ipfilter->IsFiltered(m_client->GetTCPDest())) {
                        if (bNewClient) {
                                m_client->Safe_Delete();
                                m_client = NULL;
                        }
                        Disconnect(wxT("IPFilter"));
                        return false;
                }

                wxASSERT(m_client);

                // now we check if we know this client already. if yes this socket will
                // be attached to the known client, the new client will be deleted
                // and the var. "client" will point to the known client.
                // if not we keep our new-constructed client ;)
                if (theApp->clientlist->AttachToAlreadyKnown(&m_client,this)) {
                        // update the old client informations
                        bIsMuleHello = m_client->ProcessHelloPacket(buffer, size);
                } else {
                        theApp->clientlist->AddClient(m_client);
                        m_client->SetCommentDirty();
                }
                Notify_SharedCtrlRefreshClient( m_client->ECID(), AVAILABLE_SOURCE );
                // send a response packet with standart informations
                if ((m_client->GetHashType() == SO_EMULE) && !bIsMuleHello) {
                        m_client->SendMuleInfoPacket(false);
                }

                // Client might die from Sending in SendMuleInfoPacket, so check
                if ( m_client ) {
                        m_client->SendHelloAnswer();
                }

                // Kry - If the other side supports it, send OS_INFO
                // Client might die from Sending in SendHelloAnswer, so check
                if (m_client && m_client->GetOSInfoSupport()) {
                        m_client->SendMuleInfoPacket(false,true); // Send the OS Info tag on the recycled Mule Info
                }

                // Client might die from Sending in SendMuleInfoPacket, so check
                if ( m_client ) {
                        m_client->ConnectionEstablished();
                }

                // start secure identification, if
                //	- we have received eMule-OP_HELLO (new eMule)
                if (m_client && m_client->GetInfoPacketsReceived() == IP_BOTH) {
                        m_client->InfoPacketsReceived();
                }

                break;
        }
        case OP_REQUESTFILENAME: {	// 0.43b
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_REQUESTFILENAME from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);
                // IP banned, no answer for this request
                if (m_client->IsBanned()) {
                        break;
                }
                if (size >= 16) {
                        if (!m_client->GetWaitStartTime()) {
                                m_client->SetWaitStartTime();
                        }
                        CMemFile data_in(buffer, size);
                        CMD4Hash reqfilehash = data_in.ReadHash();
                        CKnownFile *reqfile = theApp->sharedfiles->GetFileByID(reqfilehash);
                        if ( reqfile == NULL ) {
                                reqfile = theApp->downloadqueue->GetFileByID(reqfilehash);
                                if ( !( reqfile != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
                                        break;
                                }
                        }
                        // if we are downloading this file, this could be a new source
                        // no passive adding of files with only one part
                        if (reqfile->IsPartFile() && reqfile->GetFileSize() > PARTSIZE) {
                                if (thePrefs::GetMaxSourcePerFile() >
                                                ((CPartFile*)reqfile)->GetSourceCount()) {
                                        theApp->downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, m_client);
                                }
                        }

                        // check to see if this is a new file they are asking for
                        if (m_client->GetUploadFileID() != reqfilehash) {
                                m_client->SetCommentDirty();
                        }

                        m_client->SetUploadFileID(reqfile);
                        m_client->ProcessExtendedInfo(&data_in, reqfile);

                        // send filename etc
                        CMemFile data_out(128);
                        data_out.WriteHash(reqfile->GetFileHash());

                        // Since it's for somebody else to see, we need to send the prettified
                        // filename, rather than the (possibly) mangled actual filename.
                        data_out.WriteString(reqfile->GetFileName().GetPrintable(), m_client->GetUnicodeSupport());

                        CPacket* packet = new CPacket(data_out, OP_EDONKEYPROT, OP_REQFILENAMEANSWER);
                        theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
                        AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_REQFILENAMEANSWER to ") + m_client->GetTCPDest().humanReadable() );
                        SendPacket(packet, true);

                        // SendPacket might kill the socket, so check
                        if (m_client)
                                m_client->SendCommentInfo(reqfile);

                        break;
                }
                throw wxString(wxT("Invalid OP_REQUESTFILENAME packet size"));
                break;
        }
        case OP_SETREQFILEID: {	// 0.43b EXCEPT track of bad clients
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_SETREQFILEID from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);

                if (m_client->IsBanned()) {
                        break;
                }

                // DbT:FileRequest
                if (size == 16) {
                        if (!m_client->GetWaitStartTime()) {
                                m_client->SetWaitStartTime();
                        }

                        const CMD4Hash fileID(buffer);
                        CKnownFile *reqfile = theApp->sharedfiles->GetFileByID(fileID);
                        if ( reqfile == NULL ) {
                                reqfile = theApp->downloadqueue->GetFileByID(fileID);
                                if ( !( reqfile  != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
                                        CPacket* replypacket = new CPacket(OP_FILEREQANSNOFIL, 16, OP_EDONKEYPROT);
                                        replypacket->Copy16ToDataBuffer(fileID.GetHash());
                                        theStats::AddUpOverheadFileRequest(replypacket->GetPacketSize());
                                        AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_FILERE to ") + m_client->GetTCPDest().humanReadable() );
                                        SendPacket(replypacket, true);
                                        break;
                                }
                        }

                        // check to see if this is a new file they are asking for
                        if (m_client->GetUploadFileID() != fileID) {
                                m_client->SetCommentDirty();
                        }

                        m_client->SetUploadFileID(reqfile);
                        // send filestatus
                        CMemFile data(16+16);
                        data.WriteHash(reqfile->GetFileHash());
                        if (reqfile->IsPartFile()) {
                                ((CPartFile*)reqfile)->WritePartStatus(&data);
                        } else {
                                data.WriteUInt64(0);
                        }
                        CPacket* packet = new CPacket(data, OP_EDONKEYPROT, OP_FILESTATUS);
                        theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
                        AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_FILESTATUS to ") + m_client->GetTCPDest().humanReadable() );
                        SendPacket(packet, true);
                        break;
                }
                throw wxString(wxT("Invalid OP_FILEREQUEST packet size"));
                break;
                // DbT:End
        }

        case OP_FILEREQANSNOFIL: {	// 0.43b protocol, lacks ZZ's download manager on swap
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_FILEREQANSNOFIL from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);
                if (size == 16) {
                        // if that client does not have my file maybe has another different
                        CPartFile* reqfile = theApp->downloadqueue->GetFileByID(CMD4Hash(buffer));
                        if ( reqfile) {
                                reqfile->AddDeadSource( m_client );
                        } else {
                                break;
                        }

                        // we try to swap to another file ignoring no needed parts files
                        switch (m_client->GetDownloadState()) {
                        case DS_CONNECTED:
                        case DS_ONQUEUE:
                        case DS_NONEEDEDPARTS:
                                if (!m_client->SwapToAnotherFile(true, true, true, NULL)) {
                                        theApp->downloadqueue->RemoveSource(m_client);
                                }
                                break;
                        }
                        break;
                }
                throw wxString(wxT("Invalid OP_FILEREQUEST packet size"));
                break;
        }

        case OP_REQFILENAMEANSWER: {	// 0.43b except check for bad clients
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_REQFILENAMEANSWER from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);
                CMemFile data(buffer, size);
                CMD4Hash hash = data.ReadHash();
                const CPartFile* file = theApp->downloadqueue->GetFileByID(hash);
                m_client->ProcessFileInfo(&data, file);
                break;
        }

        case OP_FILESTATUS: {		// 0.43b except check for bad clients
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_FILESTATUS from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);
                CMemFile data(buffer, size);
                CMD4Hash hash = data.ReadHash();
                const CPartFile* file = theApp->downloadqueue->GetFileByID(hash);
                m_client->ProcessFileStatus(false, &data, file);
                break;
        }

        case OP_STARTUPLOADREQ: {
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_STARTUPLOADREQ from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);

                if (!m_client->CheckHandshakeFinished()) {
			AddDebugLogLineN( logRemoteClient, CFormat(wxT("But %s did not pass CheckHandshakeFinished")) % m_client->GetTCPDest().humanReadable() );
                        break;
                }

                m_client->CheckForAggressive();
                if ( m_client->IsBanned() ) {
			AddDebugLogLineN( logRemoteClient, CFormat(wxT("But %s did not pass IsBanned")) % m_client->GetTCPDest().humanReadable() );
                        break;
                }

                if (size == 16) {
                        const CMD4Hash fileID(buffer);
                        CKnownFile* reqfile = theApp->sharedfiles->GetFileByID(fileID);
                        if (reqfile) {
                                if (m_client->GetUploadFileID() != fileID) {
                                        m_client->SetCommentDirty();
                                }
                                m_client->SetUploadFileID(reqfile);
                                m_client->SendCommentInfo(reqfile);

                                // Socket might die because of SendCommentInfo, so check
                                if (m_client) {
					AddDebugLogLineN( logRemoteClient, CFormat(wxT("add %s to queue")) % m_client->GetTCPDest().humanReadable() );
					theApp->uploadqueue->AddClientToQueue(m_client);
				}
                        }
                }
                break;
        }

        case OP_QUEUERANK: {	// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_QUEUERANK from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);
                CMemFile data(buffer, size);
                uint32 rank = data.ReadUInt32();

                m_client->SetRemoteQueueRank(rank);
                break;
        }

        case OP_ACCEPTUPLOADREQ: {	// 0.42e (xcept khaos stats)
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_ACCEPTUPLOADREQ from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);
                if (m_client->GetRequestFile() && !m_client->GetRequestFile()->IsStopped() && (m_client->GetRequestFile()->GetStatus()==PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {
                        if (m_client->GetDownloadState() == DS_ONQUEUE ) {
                                m_client->SetDownloadState(DS_DOWNLOADING);
                                m_client->SetLastPartAsked(0xffff); // Reset current downloaded Chunk // Maella -Enhanced Chunk Selection- (based on jicxicmic)
                                m_client->SendBlockRequests();
                        }
                } else {
                        if (!m_client->GetSentCancelTransfer()) {
                                CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
                                theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
                                AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + m_client->GetTCPDest().humanReadable() );
                                m_client->SendPacket(packet,true,true);

                                // SendPacket can cause the socket to die, so check
                                if (m_client)
                                        m_client->SetSentCancelTransfer(1);
                        }

                        if (m_client)
                                m_client->SetDownloadState((m_client->GetRequestFile()==NULL || m_client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
                }
                break;
        }

        case OP_REQUESTPARTS: {		// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_REQUESTPARTS from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);

                m_client->ProcessRequestPartsPacket(buffer, size, false);

                break;
        }

        case OP_CANCELTRANSFER: {		// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_CANCELTRANSFER from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);
                theApp->uploadqueue->RemoveFromUploadQueue(m_client);
                AddDebugLogLineN( logClient, m_client->GetUserName() + wxT(": Upload session ended due canceled transfer."));
                break;
        }

        case OP_END_OF_DOWNLOAD: { // 0.43b except check for bad clients
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_END_OF_DOWNLOAD from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);
                if (size>=16 && m_client->GetUploadFileID() == CMD4Hash(buffer)) {
                        theApp->uploadqueue->RemoveFromUploadQueue(m_client);
                        AddDebugLogLineN( logClient, m_client->GetUserName() + wxT(": Upload session ended due ended transfer."));
                }
                break;
        }

        case OP_HASHSETREQUEST: {		// 0.43b except check for bad clients
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_HASHSETREQUEST from ") + m_client->GetTCPDest().humanReadable() );


                theStats::AddDownOverheadFileRequest(size);
                if (size != 16) {
                        throw wxString(wxT("Invalid OP_HASHSETREQUEST packet size"));
                }
                m_client->SendHashsetPacket(CMD4Hash(buffer));
                break;
        }

        case OP_HASHSETANSWER: {		// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_HASHSETANSWER from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);
                m_client->ProcessHashSet(buffer, size);
                break;
        }

        case OP_SENDINGPART: {		// 0.47a
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_SENDINGPART from ") + m_client->GetTCPDest().humanReadable() );

                if (	 m_client->GetRequestFile() &&
                                !m_client->GetRequestFile()->IsStopped() &&
                                (m_client->GetRequestFile()->GetStatus() == PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {

                        m_client->ProcessBlockPacket(buffer, size, false, true); // true because imule only use 64bits for blocks

                        if ( 	m_client &&
                                        ( m_client->GetRequestFile()->IsStopped() ||
                                          m_client->GetRequestFile()->GetStatus() == PS_PAUSED ||
                                          m_client->GetRequestFile()->GetStatus() == PS_ERROR) ) {
                                if (!m_client->GetSentCancelTransfer()) {
                                        CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
                                        theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
                                        AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + m_client->GetTCPDest().humanReadable() );
                                        m_client->SendPacket(packet,true,true);

                                        // Socket might die because of SendPacket, so check
                                        if (m_client)
                                                m_client->SetSentCancelTransfer(1);
                                }

                                if (m_client)
                                        m_client->SetDownloadState(m_client->GetRequestFile()->IsStopped() ? DS_NONE : DS_ONQUEUE);
                        }
                } else {
                        if (!m_client->GetSentCancelTransfer()) {
                                CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
                                theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
                                AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + m_client->GetTCPDest().humanReadable() );
                                m_client->SendPacket(packet,true,true);

                                // Socket might die because of SendPacket, so check
                                m_client->SetSentCancelTransfer(1);
                        }
                        m_client->SetDownloadState((m_client->GetRequestFile()==NULL || m_client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
                }
                break;
        }

        case OP_OUTOFPARTREQS: {	// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_OUTOFPARTREQS from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);
                if (m_client->GetDownloadState() == DS_DOWNLOADING) {
                        m_client->SetDownloadState(DS_ONQUEUE);
                }
                break;
        }

        case OP_CHANGE_CLIENT_ID: { 	// Kad reviewed
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_CHANGE_CLIENT_ID from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadOther(size);
                CMemFile data(buffer, size);
                CI2PAddress nNewUserID = data.ReadAddress();
                CI2PAddress nNewServerIP = data.ReadAddress();

			if (IsLowID(nNewUserID)) { // client changed server and gots a LowID
				CServer* pNewServer = theApp->serverlist->GetServerByDest(nNewServerIP);

                        if (pNewServer != NULL) {
					m_client->SetUserIDHybrid(nNewUserID); // update UserID only if we know the server
					m_client->SetServerIP(nNewServerIP);
				}
			} else if (nNewUserID == m_client->GetTCPDest()) { // client changed server and gots a HighID(IP)
				m_client->SetUserIDHybrid(nNewUserID);
				CServer* pNewServer = theApp->serverlist->GetServerByDest(nNewServerIP);
				if (pNewServer != NULL){
					m_client->SetServerIP(nNewServerIP);
                        }
                }

                break;
        }

        case OP_CHANGE_SLOT: {	// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_CHANGE_SLOT from ") + m_client->GetTCPDest().humanReadable() );

                // sometimes sent by Hybrid
                theStats::AddDownOverheadOther(size);
                break;
        }

        case OP_MESSAGE: {		// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_MESSAGE from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadOther(size);

                if (size < 2) {
                        throw wxString(wxT("invalid message packet"));
                }
                CMemFile message_file(buffer, size);
                uint16 length = message_file.ReadUInt16();
                if (length + 2u != size) {
                        throw wxString(wxT("invalid message packet"));
                }

                // limit message length
                static const uint16 MAX_CLIENT_MSG_LEN = 450;

                if (length > MAX_CLIENT_MSG_LEN) {
                        AddDebugLogLineN(logRemoteClient, CFormat(wxT("Message from '%s' (IP:%s) exceeds limit by %u chars, truncated."))
                                         % m_client->GetUserName() % m_client->GetFullIP() % (length - MAX_CLIENT_MSG_LEN));
                        length = MAX_CLIENT_MSG_LEN;
                }

                wxString message = message_file.ReadOnlyString((m_client->GetUnicodeSupport() != utf8strNone), length);
                m_client->ProcessChatMessage(message);

                break;
        }

        case OP_ASKSHAREDFILES:	{	// 0.43b (well, er, it does the same, but in our own way)
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILES from ") + m_client->GetTCPDest().humanReadable() );

                // client wants to know what we have in share, let's see if we allow him to know that
                theStats::AddDownOverheadOther(size);
                // IP banned, no answer for this request
                if (m_client->IsBanned()) {
                        break;
                }

                if (thePrefs::CanSeeShares() == vsfaEverybody || (thePrefs::CanSeeShares() == vsfaFriends && m_client->IsFriend())) {
                        AddLogLineC(CFormat( _("User %s (%x) requested your sharedfiles-list -> Accepted"))
                                    % m_client->GetUserName() % m_client->GetUserIDHybrid().hashCode());

                        std::vector<CKnownFile*> list;
                        theApp->sharedfiles->CopyFileList(list);

                        CMemFile tempfile(80);
                        tempfile.WriteUInt32(list.size());
                        for (unsigned i = 0; i < list.size(); ++i) {
                                if (!list[i]->IsLargeFile() || m_client->SupportsLargeFiles()) {
                                        list[i]->CreateOfferedFilePacket(&tempfile, NULL, m_client);
                                }
                        }

                        // create a packet and send it
                        CPacket* replypacket = new CPacket(tempfile, OP_EDONKEYPROT, OP_ASKSHAREDFILESANSWER);
                        AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ASKSHAREDFILESANSWER to ") + m_client->GetTCPDest().humanReadable() );
                        theStats::AddUpOverheadOther(replypacket->GetPacketSize());
                        SendPacket(replypacket, true, true);
                } else {
                        AddLogLineC(CFormat(_("User %s (%x) requested your sharedfiles-list -> Denied"))
                                    % m_client->GetUserName()
                                    % m_client->GetUserIDHybrid().hashCode());

                        CPacket* replypacket = new CPacket(OP_ASKSHAREDDENIEDANS, 0, OP_EDONKEYPROT);
                        theStats::AddUpOverheadOther(replypacket->GetPacketSize());
                        AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ASKSHAREDDENIEDANS to ") + m_client->GetFullIP() );
                        SendPacket(replypacket, true, true);
                }

                break;
        }

        case OP_ASKSHAREDFILESANSWER: {	// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILESANSWER from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadOther(size);
                wxString EmptyStr;
                m_client->ProcessSharedFileList(buffer, size, EmptyStr);
                break;
        }

        case OP_ASKSHAREDDIRS: { 		// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_ASKSHAREDDIRS from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadOther(size);
                wxASSERT( size == 0 );
                // IP banned, no answer for this request
                if (m_client->IsBanned()) {
                        break;
                }
                if ((thePrefs::CanSeeShares()==vsfaEverybody) || ((thePrefs::CanSeeShares()==vsfaFriends) && m_client->IsFriend())) {
                        AddLogLineC(CFormat( _("User %s (%x) requested your shareddirectories-list -> Accepted") )
                                    % m_client->GetUserName()
                                    % m_client->GetUserIDHybrid().hashCode() );
                        // send the list of shared directories
                        m_client->SendSharedDirectories();
                } else {
                        AddLogLineC(CFormat(_("User %s (%x) requested your shareddirectories-list -> Denied"))
                                    % m_client->GetUserName() % m_client->GetUserIDHybrid().hashCode());

                        CPacket* replypacket = new CPacket(OP_ASKSHAREDDENIEDANS, 0, OP_EDONKEYPROT);
                        theStats::AddUpOverheadOther(replypacket->GetPacketSize());
                        AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ASKSHAREDDENIEDANS to ") + m_client->GetTCPDest().humanReadable() );
                        SendPacket(replypacket, true, true);
                }

                break;
        }

        case OP_ASKSHAREDFILESDIR: {	// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILESDIR from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadOther(size);
                // IP banned, no answer for this request
                if (m_client->IsBanned()) {
                        break;
                }
                CMemFile data(buffer, size);

                wxString strReqDir = data.ReadString((m_client->GetUnicodeSupport() != utf8strNone));
                if (thePrefs::CanSeeShares()==vsfaEverybody || (thePrefs::CanSeeShares()==vsfaFriends && m_client->IsFriend())) {
                        AddLogLineC(CFormat(_("User %s (%x) requested your sharedfiles-list for directory '%s' -> accepted")) % m_client->GetUserName() % m_client->GetUserIDHybrid().hashCode() % strReqDir);
                        wxASSERT( data.GetPosition() == data.GetLength() );
                        // send the list of shared files for the requested directory
                        m_client->SendSharedFilesOfDirectory(strReqDir);
                } else {
                        AddLogLineC(CFormat(_("User %s (%x) requested your sharedfiles-list for directory %s -> denied"))
                                    % m_client->GetUserName() % m_client->GetTCPDest().hashCode() % strReqDir);

                        CPacket* replypacket = new CPacket(OP_ASKSHAREDDENIEDANS, 0, OP_EDONKEYPROT);
                        theStats::AddUpOverheadOther(replypacket->GetPacketSize());
                        AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_ASKSHAREDDENIEDANS to ")
                                         + m_client->GetTCPDest().humanReadable());
                        SendPacket(replypacket, true, true);
                }
                break;
        }

        case OP_ASKSHAREDDIRSANS: {		// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_ASKSHAREDDIRSANS from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadOther(size);
                if (m_client->GetFileListRequested() == 1) {
                        CMemFile data(buffer, size);
                        uint32 uDirs = data.ReadUInt32();
                        AddLogLineC(CFormat(_("User %s (%x) shares %u directories")) % m_client->GetUserName() % m_client->GetUserIDHybrid().hashCode() % uDirs); 
                        for (uint32 i = 0; i < uDirs; i++) {
                                wxString strDir = data.ReadString((m_client->GetUnicodeSupport() != utf8strNone));
                                AddDebugLogLineN(logRemoteClient, CFormat(_("User %s (%x) shares directory %s")) % m_client->GetUserName() % m_client->GetUserIDHybrid().hashCode() % strDir); 
                                CMemFile tempfile(80);
                                tempfile.WriteString(strDir, m_client->GetUnicodeSupport());
                                CPacket* replypacket = new CPacket(tempfile, OP_EDONKEYPROT, OP_ASKSHAREDFILESDIR);
                                theStats::AddUpOverheadOther(replypacket->GetPacketSize());
                                AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ASKSHAREDFILESD to ") + m_client->GetTCPDest().humanReadable() );
                                SendPacket(replypacket, true, true);
                        }
                        if (data.GetPosition() < data.GetLength()) {
                                wxString strDir = data.ReadString((m_client->GetUnicodeSupport() != utf8strNone));
                                wxASSERT(strDir == OP_INCOMPLETE_SHARED_FILES);
                        }
                        wxASSERT( data.GetPosition() == data.GetLength() );
                        m_client->SetFileListRequested(uDirs);
                } else {
                        AddLogLineC(CFormat(_("User %s (%x) sent unrequested shared dirs."))
                                    % m_client->GetUserName() % m_client->GetUserIDHybrid().hashCode());
                }
                break;
        }

        case OP_ASKSHAREDFILESDIRANS: {		// 0.43b
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_ASKSHAREDFILESDIRANS from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadOther(size);
                CMemFile data(buffer, size);
                wxString strDir = data.ReadString((m_client->GetUnicodeSupport() != utf8strNone));

                if (m_client->GetFileListRequested() > 0) {
                        AddLogLineC(CFormat(_("User %s (%x) sent sharedfiles-list for directory %s"))
				% m_client->GetUserName() 
				% m_client->GetUserIDHybrid().hashCode() 
				% strDir);

                        m_client->ProcessSharedFileList(buffer + data.GetPosition(), size - data.GetPosition(), strDir);
                        if (m_client->GetFileListRequested() == 0) {
                                AddLogLineC(CFormat(_("User %s (%x) finished sending sharedfiles-list"))
                                            % m_client->GetUserName() % m_client->GetUserIDHybrid().hashCode());
                        }
                } else {
                        AddLogLineC(CFormat(_("User %s (%x) sent unwanted sharedfiles-list"))
                                    % m_client->GetUserName() % m_client->GetUserIDHybrid().hashCode());
                }
                break;
        }

        case OP_ASKSHAREDDENIEDANS:
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_ASKSHAREDDENIEDANS from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadOther(size);
                wxASSERT(size == 0);
                AddLogLineC(CFormat(_("User %s (%x) denied access to shared directories/files list"))
                            % m_client->GetUserName() % m_client->GetUserIDHybrid().hashCode());

                m_client->SetFileListRequested(0);
                break;

        default:
                theStats::AddDownOverheadOther(size);
                AddDebugLogLineN( logRemoteClient, wxString::Format(wxT("Edonkey packet: unknown opcode: %i %x from "), opcode, opcode) + m_client->GetTCPDest().humanReadable());
                return false;
        }

        return true;
}

#include "SHA.h"		// Needed for CSHA

wxString CClientTCPSocket::encodeDirName(const CPath & dir)
{
        static unsigned char *key = NULL;	// key for encoding

        wxString dirName = dir.GetRaw();
        if (key == NULL) {
                key = new uint8_t[512];
                srand(time(NULL));
                for (int i = 0; i < 512; i++)
                        key[i] = (uint8_t) (rand() % 512);
        }
        unsigned int len = 512 + strlen((const char *) dirName.mb_str());
        char buffer[len];
        memcpy(buffer, key, 512);
        memcpy(buffer + 512, dirName.mb_str(), strlen((const char *) dirName.mb_str()));

        CSHA hasher;
        hasher.Add(buffer, len);
        CAICHHash hash;
        hasher.Finish(hash);
        wxString ret = hash.GetString();

        return ret;
}




bool CClientTCPSocket::ProcessExtPacket(const byte* buffer, uint32 size, uint8 opcode)
{
#ifdef __PACKET_RECV_DUMP__
        //printf("Rec: OPCODE %x \n",opcode);
        DumpMem(buffer,size);
#endif

        // 0.42e - except the catchs on mem exception and file exception
        if (!m_client) {
                throw wxString(wxT("Unknown clients sends extended protocol packet"));
        }
        /*
        if (!client->CheckHandshakeFinished()) {
        	// Here comes an extended packet without finishing the handshake.
        	// IMHO, we should disconnect the client.
        	throw wxString(wxT("Client send extended packet before finishing handshake"));
        }
        */
        switch(opcode) {
        case OP_MULTIPACKET_EXT:
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_MULTIPACKET_EXT from ") + m_client->GetTCPDest().humanReadable());
        case OP_MULTIPACKET: {
                if (opcode == OP_MULTIPACKET) AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_MULTIPACKET from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);

                if (m_client->IsBanned()) {
                        break;
                }

                if (!m_client->CheckHandshakeFinished()) {
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_MULTIPACKET before finishing handshake"));
                }

                CMemFile data_in(buffer, size);
                CMD4Hash reqfilehash = data_in.ReadHash();
                uint64 nSize = (opcode == OP_MULTIPACKET_EXT) ? data_in.ReadUInt64() : 0;

                bool file_not_found = false;
                CKnownFile* reqfile = theApp->sharedfiles->GetFileByID(reqfilehash);
                if ( reqfile == NULL ) {
                        reqfile = theApp->downloadqueue->GetFileByID(reqfilehash);
                        if ( !( reqfile != NULL && reqfile->GetFileSize() > PARTSIZE ) ) {
                                AddDebugLogLineN(logRemoteClient, wxT("Remote client asked for a non-shared file"));
                                file_not_found = true;
                        }
                }

                if (!file_not_found && reqfile->IsLargeFile() && !m_client->SupportsLargeFiles()) {
                        AddDebugLogLineN(logRemoteClient, wxT("Remote client asked for a large file but doesn't support them"));
                        file_not_found = true;
                }

                if (!file_not_found && nSize && (reqfile->GetFileSize() != nSize)) {
                        AddDebugLogLineN(logRemoteClient, wxT("Remote client asked for a file but specified wrong size"));
                        file_not_found = true;
                }

                if (file_not_found) {
                        CPacket* replypacket = new CPacket(OP_FILEREQANSNOFIL, 16, OP_EDONKEYPROT);
                        replypacket->Copy16ToDataBuffer(reqfilehash.GetHash());
                        theStats::AddUpOverheadFileRequest(replypacket->GetPacketSize());
                        AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_FILEREQANSNOFIL to ") + m_client->GetTCPDest().humanReadable() );
                        SendPacket(replypacket, true);
                        break;
                }

                if (!m_client->GetWaitStartTime()) {
                        m_client->SetWaitStartTime();
                }
                // if we are downloading this file, this could be a new source
                // no passive adding of files with only one part
                if (reqfile->IsPartFile() && reqfile->GetFileSize() > PARTSIZE) {
                        if (thePrefs::GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount()) {
                                theApp->downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, m_client);
                        }
                }
                // check to see if this is a new file they are asking for
                if (m_client->GetUploadFileID() != reqfilehash) {
                        m_client->SetCommentDirty();
                }
                m_client->SetUploadFileID(reqfile);
                CMemFile data_out(128);
                data_out.WriteHash(reqfile->GetFileHash());
                while(data_in.GetLength()-data_in.GetPosition()) {
                        if (!m_client) {
                                throw wxString(wxT("Client suddenly disconnected"));
                        }
                        uint8 opcode_in = data_in.ReadUInt8();
                        switch(opcode_in) {
                        case OP_REQUESTFILENAME: {
                                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_MULTIPACKET has OP_REQUESTFILENAME") );
                                m_client->ProcessExtendedInfo(&data_in, reqfile);
                                data_out.WriteUInt8(OP_REQFILENAMEANSWER);

                                // Since it's for somebody else to see, we need to send the prettified
                                // filename, rather than the (possibly) mangled actual filename
                                data_out.WriteString(reqfile->GetFileName().GetPrintable(), m_client->GetUnicodeSupport());
                                break;
                        }
                        case OP_AICHFILEHASHREQ: {
                                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_MULTIPACKET has OP_AICHFILEHASHANS") );
                                if (m_client->IsSupportingAICH() && reqfile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE
                                                && reqfile->GetAICHHashset()->HasValidMasterHash()) 
				{
                                        data_out.WriteUInt8(OP_AICHFILEHASHANS);
                                        reqfile->GetAICHHashset()->GetMasterHash().Write(&data_out);
                                }
                                break;
                        }
                        case OP_SETREQFILEID: {
                                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_MULTIPACKET has OP_SETREQFILEID") );
                                data_out.WriteUInt8(OP_FILESTATUS);
                                if (reqfile->IsPartFile()) {
                                        ((CPartFile*)reqfile)->WritePartStatus(&data_out);
                                } else {
                                        data_out.WriteUInt64(0);
                                }
                                break;
                        }
                        //We still send the source packet separately..
                        //We could send it within this packet.. If agreeded, I will fix it..
                        case OP_REQUESTSOURCES2:
                        case OP_REQUESTSOURCES: {
                                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_MULTIPACKET has OP_REQUESTSOURCES(2)"));
                                uint8 byRequestedVersion = 0;
                                uint16 byRequestedOptions = 0;
                                if (opcode_in == OP_REQUESTSOURCES2) { // SX2 requests contains additional data
                                        byRequestedVersion = data_in.ReadUInt8();
                                        byRequestedOptions = data_in.ReadUInt16();
                                }

                                //Although this shouldn't happen, it's a just in case to any Mods that mess with version numbers.

                                if (byRequestedVersion > 0 || m_client->GetSourceExchange1Version() > 1) {
                                        uint32 dwTimePassed = ::GetTickCount() - m_client->GetLastSrcReqTime() + CONNECTION_LATENCY;
                                        bool bNeverAskedBefore = m_client->GetLastSrcReqTime() == 0;
                                        if(
                                                //if not complete and file is rare
                                                (    reqfile->IsPartFile()
                                                     && (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS)
                                                     && ((CPartFile*)reqfile)->GetSourceCount() <= RARE_FILE
                                                ) ||
                                                //OR if file is not rare or if file is complete
                                                ( (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS * MINCOMMONPENALTY) )
                                        	) 
					{
                                                m_client->SetLastSrcReqTime();
                                                CPacket* tosend = reqfile->CreateSrcInfoPacket(m_client, byRequestedVersion, byRequestedOptions);
                                                if(tosend) {
                                                        theStats::AddUpOverheadSourceExchange(tosend->GetPacketSize());
                                                        AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_ANSWERSOURCES to ")
                                                                         + m_client->GetTCPDest().humanReadable());
                                                        SendPacket(tosend, true);
                                                }
                                        }
                                }
                                break;
                        }
                        }

                }
                if( data_out.GetLength() > 16 ) {
                        CPacket* reply = new CPacket(data_out, OP_EMULEPROT, OP_MULTIPACKETANSWER);
                        theStats::AddUpOverheadFileRequest(reply->GetPacketSize());
                        AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_MULTIPACKETANSWER to ") + m_client->GetTCPDest().humanReadable());
                        SendPacket(reply, true);
                }
                break;
        }

        case OP_MULTIPACKETANSWER: {	// 0.43b
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_MULTIPACKETANSWER from ") + m_client->GetTCPDest().humanReadable() );

                theStats::AddDownOverheadFileRequest(size);

                if (m_client->IsBanned()) {
                        AddDebugLogLineN(logRemoteClient, CFormat(wxT("Remote Client: OP_MULTIPACKETANSWER from %s : IGNORED cause=client is banned"))
                                         % m_client->GetTCPDest().humanReadable() );
                        break;
                }

                if( m_client->GetUDPDest().isValid() ) {
                        Kademlia::CKademlia::Bootstrap(m_client->GetUDPDest());
                }

                if (!m_client->CheckHandshakeFinished()) {
                        AddDebugLogLineN(logRemoteClient, CFormat(wxT("Remote Client: OP_MULTIPACKETANSWER from %s : IGNORED cause=handshake not finished"))
                                         % m_client->GetTCPDest().humanReadable() );
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_MULTIPACKETANSWER before finishing handshake"));
                }

                CMemFile data_in(buffer, size);
                CMD4Hash reqfilehash = data_in.ReadHash();
                const CPartFile *reqfile = theApp->downloadqueue->GetFileByID(reqfilehash);
                //Make sure we are downloading this file.
                if ( !reqfile ) {
                        throw wxString(wxT(" Wrong File ID: (OP_MULTIPACKETANSWER; reqfile==NULL)"));
                }
                if ( !m_client->GetRequestFile() ) {

                        throw wxString(wxT(" Wrong File ID: OP_MULTIPACKETANSWER; client->reqfile==NULL)"));
                }
                if (reqfile != m_client->GetRequestFile()) {
                        throw wxString(wxT(" Wrong File ID: OP_MULTIPACKETANSWER; reqfile!=client->reqfile)"));
                }
                while (data_in.GetLength()-data_in.GetPosition()) {
                        // Some of the cases down there can actually send a packet and lose the client
                        if (!m_client) {
                                throw wxString(wxT("Client suddenly disconnected"));
                        }
                        uint8 opcode_in = data_in.ReadUInt8();
                        AddDebugLogLineN(logRemoteClient, CFormat(wxT("Remote Client: OP_MULTIPACKETANSWER from %s : internal opcode=%x"))
                                         % m_client->GetTCPDest().humanReadable() % opcode_in );

                        switch(opcode_in) {
                        case OP_REQFILENAMEANSWER: {
                                if (!m_client) {
                                        throw wxString(wxT("Client suddenly disconnected"));
                                } else {
                                        m_client->ProcessFileInfo(&data_in, reqfile);
                                }
                                break;
                        }
                        case OP_FILESTATUS: {
                                if (!m_client) {
                                        throw wxString(wxT("Client suddenly disconnected"));
                                } else {
                                        m_client->ProcessFileStatus(false, &data_in, reqfile);
                                }
                                break;
                        }
                        case OP_AICHFILEHASHANS: {
                                if (!m_client) {
                                        throw wxString(wxT("Client suddenly disconnected"));
                                } else {
                                        m_client->ProcessAICHFileHash(&data_in, reqfile);
                                }
                                break;
                        }
                        }
                }

                break;
        }

        case OP_EMULEINFO: {	// 0.43b
                theStats::AddDownOverheadOther(size);

                if (!m_client->ProcessMuleInfoPacket(buffer, size)) {
                        AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_EMULEINFO from ") + m_client->GetTCPDest().humanReadable() );

                        // If it's not a OS Info packet, is an old client
                        // start secure identification, if
                        //  - we have received eD2K and eMule info (old eMule)
                        if (m_client->GetInfoPacketsReceived() == IP_BOTH) {
                                m_client->InfoPacketsReceived();
                        }
                        m_client->SendMuleInfoPacket(true);
                } else {
                        AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_EMULEINFO is an OS_INFO") );
                }
                break;
        }
        case OP_EMULEINFOANSWER: {	// 0.43b
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_EMULEINFOANSWER from ") + m_client->GetTCPDest().humanReadable() );
                theStats::AddDownOverheadOther(size);

                m_client->ProcessMuleInfoPacket(buffer, size);
                // start secure identification, if
                //  - we have received eD2K and eMule info (old eMule)

                if (m_client->GetInfoPacketsReceived() == IP_BOTH) {
                        m_client->InfoPacketsReceived();
                }

                break;
        }

        case OP_SECIDENTSTATE: {		// 0.43b
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_SECIDENTSTATE from ") + m_client->GetTCPDest().humanReadable() );

                if (!m_client->CheckHandshakeFinished()) {
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_SECIDENTSTATE before finishing handshake"));
                }
                m_client->ProcessSecIdentStatePacket(buffer, size);
                // ProcessSecIdentStatePacket() might cause the socket to die, so check
                if (m_client) {
                        int SecureIdentState = m_client->GetSecureIdentState();
                        if (SecureIdentState == IS_SIGNATURENEEDED) {
                                m_client->SendSignaturePacket();
                        } else if (SecureIdentState == IS_KEYANDSIGNEEDED) {
                                m_client->SendPublicKeyPacket();
                                // SendPublicKeyPacket() might cause the socket to die, so check
                                if ( m_client ) {
                                        m_client->SendSignaturePacket();
                                }
                        }
                }
                break;
        }

        case OP_PUBLICKEY: {		// 0.43b
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_PUBLICKEY from ") + m_client->GetTCPDest().humanReadable() );

                if (m_client->IsBanned() ) {
                        break;
                }

                if (!m_client->CheckHandshakeFinished()) {
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_PUBLICKEY before finishing handshake"));
                }

                m_client->ProcessPublicKeyPacket(buffer, size);
                break;
        }
        case OP_SIGNATURE: {			// 0.43b
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_SIGNATURE from ") + m_client->GetTCPDest().humanReadable() );

                if (!m_client->CheckHandshakeFinished()) {
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_COMPRESSEDPART before finishing handshake"));
                }

                m_client->ProcessSignaturePacket(buffer, size);
                break;
        }
        case OP_SENDINGPART:
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_SENDINGPART from ") + m_client->GetTCPDest().humanReadable() );
#ifdef sixty_four_ok_from_the_beginning_of_imule
        case OP_COMPRESSEDPART_I64:
                if (opcode == OP_COMPRESSEDPART_I64) AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_COMPRESSEDPART_I64 from ") + m_client->GetTCPDest().humanReadable() );
#endif
        case OP_COMPRESSEDPART: {	// 0.47a
                if (opcode == OP_COMPRESSEDPART) AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_COMPRESSEDPART from ") + m_client->GetTCPDest().humanReadable() );

                if (!m_client->CheckHandshakeFinished()) {
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_COMPRESSEDPART before finishing handshake"));
                }

                if (m_client->GetRequestFile() && !m_client->GetRequestFile()->IsStopped() && (m_client->GetRequestFile()->GetStatus()==PS_READY || m_client->GetRequestFile()->GetStatus()==PS_EMPTY)) {

                        m_client->ProcessBlockPacket(buffer, size, (opcode != OP_SENDINGPART), true);

                        if (m_client && (
                                                m_client->GetRequestFile()->IsStopped() ||
                                                m_client->GetRequestFile()->GetStatus() == PS_PAUSED ||
                                                m_client->GetRequestFile()->GetStatus() == PS_ERROR)) {
                                if (!m_client->GetSentCancelTransfer()) {
                                        CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
                                        theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
                                        AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + m_client->GetTCPDest().humanReadable() );
                                        m_client->SendPacket(packet,true,true);

                                        if (m_client) {
                                                m_client->SetSentCancelTransfer(1);
                                        }
                                }

                                if ( m_client ) {
                                        m_client->SetDownloadState(m_client->GetRequestFile()->IsStopped() ? DS_NONE : DS_ONQUEUE);
                                }
                        }
                } else {
                        if (!m_client->GetSentCancelTransfer()) {
                                CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
                                theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
                                AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + m_client->GetTCPDest().humanReadable() );
                                m_client->SendPacket(packet,true,true);

                                if ( m_client ) {
                                        m_client->SetSentCancelTransfer(1);
                                }
                        }

                        if ( m_client ) {
                                m_client->SetDownloadState((m_client->GetRequestFile()==NULL || m_client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
                        }
                }
                break;
        }

        case OP_REQUESTPARTS: {
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_REQUESTPARTS from ") + m_client->GetTCPDest().humanReadable()  );

                theStats::AddDownOverheadFileRequest(size);

                m_client->ProcessRequestPartsPacket(buffer, size, true);

                break;
        }
        case OP_QUEUERANKING: {		// 0.43b
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_QUEUERANKING from ") + m_client->GetTCPDest().humanReadable()  );

                theStats::AddDownOverheadOther(size);

                if (!m_client->CheckHandshakeFinished()) {
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_QUEUERANKING before finishing handshake"));
                }

                if (size != 12) {
                        throw wxString(wxT("Invalid size (OP_QUEUERANKING)"));
                }

                uint16 newrank = PeekUInt16(buffer);
                m_client->SetRemoteQueueFull(false);
                m_client->SetRemoteQueueRank(newrank);
                break;
        }
        case OP_REQUESTSOURCES2:
        case OP_REQUESTSOURCES: {
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_REQUESTSOURCES from ") + m_client->GetTCPDest().humanReadable()  );

                theStats::AddDownOverheadSourceExchange(size);

                if (!m_client->CheckHandshakeFinished()) {
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_REQUESTSOURCES before finishing handshake"));
                }

                uint8 byRequestedVersion = 0;
                uint16 byRequestedOptions = 0;
                CMemFile data_in(buffer, size);
                if (opcode == OP_REQUESTSOURCES2) { // SX2 requests contains additional data
                        byRequestedVersion = data_in.ReadUInt8();
                        byRequestedOptions = data_in.ReadUInt16();
                }

                if (byRequestedVersion > 0 || m_client->GetSourceExchange1Version() >= 1) {
                        if(size != 16) {
                                throw wxString(wxT("Invalid size (OP_QUEUERANKING)"));
                        }
                        //first check shared file list, then download list
                        const CMD4Hash fileID(buffer);
                        CKnownFile* file = theApp->sharedfiles->GetFileByID(fileID);
                        if(!file) {
                                file = theApp->downloadqueue->GetFileByID(fileID);
                        }
                        if(file) {
                                // There are some clients which do not follow the correct protocol procedure of sending
                                // the sequence OP_REQUESTFILENAME, OP_SETREQFILEID, OP_REQUESTSOURCES. If those clients
                                // are doing this, they will not get the optimal set of sources which we could offer if
                                // they would follow the above noted protocol sequence. They better do it the right way
                                // or they will get just a random set of sources because we do not know their download
                                // part status which may get cleared with the call of 'SetUploadFileID'.
                                m_client->SetUploadFileID(file);

                                uint32 dwTimePassed = ::GetTickCount() - m_client->GetLastSrcReqTime() + CONNECTION_LATENCY;
                                bool bNeverAskedBefore = m_client->GetLastSrcReqTime() == 0;
                                if(
                                        //if not complete and file is rare, allow once every 40 minutes
                                        ( file->IsPartFile() &&
                                          ((CPartFile*)file)->GetSourceCount() <= RARE_FILE &&
                                          (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS)
                                        ) ||
                                        //OR if file is not rare or if file is complete, allow every 90 minutes
                                        ( (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS * MINCOMMONPENALTY) )
                                	)
				{
                                        m_client->SetLastSrcReqTime();
                                        CPacket* tosend = file->CreateSrcInfoPacket(m_client, byRequestedVersion, byRequestedOptions);
                                        if(tosend) {
                                                theStats::AddUpOverheadSourceExchange(tosend->GetPacketSize());
                                                AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ANSWERSOURCES to ") + m_client->GetTCPDest().humanReadable());
                                                SendPacket(tosend, true, true);
                                        }
                                }
                        }
                }
                break;
        }
        case OP_ANSWERSOURCES: {
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_ANSWERSOURCES from ") + m_client->GetTCPDest().humanReadable()  );

                theStats::AddDownOverheadSourceExchange(size);

                if (!m_client->CheckHandshakeFinished()) {
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_ANSWERSOURCES before finishing handshake"));
                }

                CMemFile data(buffer, size);
                CMD4Hash hash = data.ReadHash();
                const CKnownFile* file = theApp->downloadqueue->GetFileByID(hash);
                if(file) {
                        if (file->IsPartFile()) {
                                //set the client's answer time
                                m_client->SetLastSrcAnswerTime();
                                //and set the file's last answer time
                                ((CPartFile*)file)->SetLastAnsweredTime();

                                ((CPartFile*)file)->AddClientSources(&data, SF_SOURCE_EXCHANGE, m_client->GetSourceExchange1Version(), false, m_client);
                        }
                }
                break;
        }
        case OP_ANSWERSOURCES2: {
                //printf("Received OP_ANSWERSOURCES2\n");
                theStats::AddDownOverheadSourceExchange(size);

                if (!m_client->CheckHandshakeFinished()) {
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_ANSWERSOURCES2 before finishing handshake"));
                }

                CMemFile data(buffer, size);
                uint8 byVersion = data.ReadUInt8();
                CMD4Hash hash = data.ReadHash();
                const CKnownFile* file = theApp->downloadqueue->GetFileByID(hash);
                if (file) {
                        if (file->IsPartFile()) {
                                //set the client's answer time
                                m_client->SetLastSrcAnswerTime();
                                //and set the file's last answer time
                                ((CPartFile*)file)->SetLastAnsweredTime();
                                ((CPartFile*)file)->AddClientSources(&data, SF_SOURCE_EXCHANGE, byVersion, true, m_client);
                        }
                }
                break;
        }
        case OP_FILEDESC: {		// 0.43b
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_FILEDESC from ") + m_client->GetTCPDest().humanReadable()  );

                theStats::AddDownOverheadFileRequest(size);

                if (!m_client->CheckHandshakeFinished()) {
                        // Here comes an extended packet without finishing the handshake.
                        // IMHO, we should disconnect the client.
                        throw wxString(wxT("Client send OP_FILEDESC before finishing handshake"));
                }

                m_client->ProcessMuleCommentPacket(buffer, size);
                break;
        }

        // Unsupported
        case OP_REQUESTPREVIEW: {
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_REQUESTPREVIEW  from ") + m_client->GetTCPDest().humanReadable()  );
                break;
        }
        // Unsupported
        case OP_PREVIEWANSWER: {
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_PREVIEWANSWER from ") + m_client->GetTCPDest().humanReadable()  );
                break;
        }

        case OP_PUBLICIP_ANSWER: {
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_PUBLICIP_ANSWER from ") + m_client->GetTCPDest().humanReadable()  );
                theStats::AddDownOverheadOther(size);
                m_client->ProcessPublicIPAnswer(buffer, size);
                break;
        }
        case OP_PUBLICIP_REQ: {
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_PUBLICIP_REQ from ") + m_client->GetTCPDest().humanReadable()  );
                theStats::AddDownOverheadOther(size);
                byte payload[m_client->GetTCPDest().size()];
                m_client->GetTCPDest().writeBytes(payload);
                CPacket *pPacket = new CPacket(OP_PUBLICIP_ANSWER,
                                               sizeof(payload), OP_EMULEPROT);
                pPacket->CopyToDataBuffer(0, payload, sizeof(payload));
                theStats::AddUpOverheadOther(pPacket->GetPacketSize());
                AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_PUBLICIP_ANSWER to") + m_client->GetTCPDest().humanReadable());
                SendPacket(pPacket);
                break;
        }
        case OP_AICHANSWER: {
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_AICHANSWER from ") + m_client->GetTCPDest().humanReadable()  );
                theStats::AddDownOverheadOther(size);
                m_client->ProcessAICHAnswer(buffer, size);
                break;
        }
        case OP_AICHREQUEST: {
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_AICHREQUEST from ") + m_client->GetTCPDest().humanReadable()  );
                theStats::AddDownOverheadOther(size);
                m_client->ProcessAICHRequest(buffer, size);
                break;
        }
        case OP_AICHFILEHASHANS: {
                // those should not be received normally, since we should only get those in MULTIPACKET
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_AICHFILEHASHANS from ") + m_client->GetTCPDest().humanReadable()  );
                theStats::AddDownOverheadOther(size);
                CMemFile data(buffer, size);
                m_client->ProcessAICHFileHash(&data, NULL);
                break;
        }
        case OP_AICHFILEHASHREQ: {
                // those should not be received normally, since we should only get those in MULTIPACKET
                AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_AICHFILEHASHREQ from ") + m_client->GetTCPDest().humanReadable()  );
                CMemFile data(buffer, size);
                CMD4Hash hash = data.ReadHash();
                CKnownFile* pPartFile = theApp->sharedfiles->GetFileByID(hash);
                if (pPartFile == NULL) {
                        break;
                }

                if (m_client->IsSupportingAICH() && pPartFile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE
                                && pPartFile->GetAICHHashset()->HasValidMasterHash()) {
                        CMemFile data_out;
                        data_out.WriteHash(hash);
                        pPartFile->GetAICHHashset()->GetMasterHash().Write(&data_out);
                        CPacket* packet = new CPacket(data_out, OP_EMULEPROT, OP_AICHFILEHASHANS);
                        theStats::AddUpOverheadOther(packet->GetPacketSize());
                        AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_AICHFILEHASHANS to") + m_client->GetTCPDest().humanReadable());
                        SendPacket(packet);
                }
                break;
        }
        case OP_CALLBACK: {
                AddDebugLogLineN( logRemoteClient, wxT("Remote Client: OP_CALLBACK from ") + m_client->GetTCPDest().humanReadable() );
                theStats::AddDownOverheadFileRequest(size);
                if(!Kademlia::CKademlia::IsRunning()) {
                        break;
                }
                CMemFile data(buffer, size);
                CUInt128 check = data.ReadUInt128();
                check.XOR(Kademlia::CUInt128(true));
                if( check.CompareTo(Kademlia::CKademlia::GetPrefs()->GetKadID())) {
                        break;
                }
                CUInt128 fileid = data.ReadUInt128();
                byte fileid2[16];
                fileid.ToByteArray(fileid2);
                const CMD4Hash fileHash(fileid2);
                if (theApp->sharedfiles->GetFileByID(fileHash) == NULL) {
                        if (theApp->downloadqueue->GetFileByID(fileHash) == NULL) {
                                break;
                        }
                }

                CI2PAddress dest = data.ReadAddress();
                CUpDownClient *callback;
                callback = theApp->clientlist->FindClientByIP(dest);
                if (callback == NULL) {
                        //#warning Do we actually have to check friend status here?
                        callback = new CUpDownClient(dest, CI2PAddress::null, NULL, false, false);
                        theApp->clientlist->AddClient(callback);
                }
                callback->TryToConnect(true);
                break;
        }
	case OP_CHATCAPTCHAREQ:
		{
			AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_CHATCAPTCHAREQ from ") + m_client->GetTCPDest().humanReadable());
			theStats::AddDownOverheadOther(size);
			CMemFile data_in(buffer, size);
			m_client->ProcessCaptchaRequest(&data_in);
			break;
		}
	case OP_CHATCAPTCHARES:
		{
			AddDebugLogLineN(logRemoteClient, wxT("Remote Client: OP_CHATCAPTCHARES from ") + m_client->GetTCPDest().humanReadable());
			theStats::AddDownOverheadOther(size);
			if (size) {
				m_client->ProcessCaptchaReqRes(buffer[0]);
			}
			break;
		}
        default:
                theStats::AddDownOverheadOther(size);
                AddDebugLogLineN( logRemoteClient, wxString::Format(wxT("eMule packet : unknown opcode: %i %x from "),opcode,opcode) + m_client->GetTCPDest().humanReadable());
                break;
        }

        return true;
}

bool CClientTCPSocket::ProcessED2Kv2Packet(const byte* buffer, uint32 size, uint8 opcode)
{
#ifdef __PACKET_RECV_DUMP__
        //printf("Rec: OPCODE %x ED2Kv2\n",opcode);
        DumpMem(buffer,size);
#endif

        if (!m_client) {
                throw wxString(wxT("Unknown clients sends extended ED2Kv2 protocol packet"));
        }

        CMemFile data(buffer, size);
        try {
                switch(opcode) {
                case OP_QUEUERANK: {
                        AddDebugLogLineN(logRemoteClient, wxT("Remote Client: ED2Kv2 OP_QUEUERANK from ") + m_client->GetTCPDest().humanReadable() );

                        uint8 numtags = data.ReadUInt8();
                        wxASSERT(numtags == 1);
                        if(numtags) {}	// prevent GCC warning

                        m_client->SetRemoteQueueRank(CTag(data).GetInt());

                        theStats::AddDownOverheadFileRequest(size);
                        break;
                }

                case OP_REQUESTPARTS: {
                        AddDebugLogLineN(logRemoteClient, wxT("Remote Client: ED2Kv2 OP_REQUESTPARTS from ") + m_client->GetTCPDest().humanReadable() );

                        m_client->ProcessRequestPartsPacketv2(data);

                        theStats::AddDownOverheadFileRequest(size);
                        break;
                }

                default:
                        theStats::AddDownOverheadOther(size);
                        AddDebugLogLineN(logRemoteClient, wxString::Format(wxT("ED2Kv2 packet : unknown opcode: %i %x from "), opcode, opcode) + m_client->GetTCPDest().humanReadable());
                }
        } catch (...) {
                AddDebugLogLineN(logRemoteClient, wxString::Format(wxT("ED2Kv2 packet is corrupt at pos %u! opcode: %i %x from "),data.GetPosition(), opcode, opcode) + m_client->GetTCPDest().humanReadable());
                throw;
        }

        return true;
}

void CClientTCPSocket::OnConnect(int nErrorCode)
{
        AddDebugLogLineN(logRemoteClient, CFormat(wxT("%x (%s --> %s): Client: %x : Processing connection event"))
                         % (long) this % GetLocal().humanReadable() % GetPeer().humanReadable() % (long) m_client);

        if (nErrorCode) {
                AddDebugLogLineN(logRemoteClient, CFormat(wxT("%x (%s --> %s): Client: %x : Processing connection event : error"))
                                 % (long) this % GetLocal().humanReadable() % GetPeer().humanReadable() % (long) m_client);
                OnError(nErrorCode);
        } else if (!m_client) {
                // and now? Disconnect? not?
                AddDebugLogLineN( logClient, wxT("Couldn't send hello packet (Client deleted!)") );
        } else if (!m_client->SendHelloPacket()) {
                // and now? Disconnect? not?
                AddDebugLogLineN( logClient, wxT("Couldn't send hello packet (Client deleted by SendHelloPacket!)") );
        } else {
                ResetTimeOutTimer();
        }
}


void CClientTCPSocket::OnSend(int nErrorCode)
{
        ResetTimeOutTimer();
        CEMSocket::OnSend(nErrorCode);
}


void CClientTCPSocket::OnReceive(int nErrorCode)
{
        ResetTimeOutTimer();
        // We might have updated ipfilter
        wxASSERT(m_remotedest.isValid());

        if (theApp->ipfilter->IsFiltered(m_remotedest)) {
                if (m_client) {
                        m_client->Safe_Delete();
                }
                Safe_Delete();
                AddDebugLogLineN( logIPFilter, wxT("A connected client was dropped by IPFilter on new packet received"));
        } else {
                CEMSocket::OnReceive(nErrorCode);
        }
}


void CClientTCPSocket::OnError(int nErrorCode)
{
        //printf("* Called OnError for %p\n",this);
        // 0.42e + Kry changes for handling of socket lost events
        wxString strError;

        if ((nErrorCode == 0) || (nErrorCode == 7) || (nErrorCode == 0xFEFF)) {
                if (m_client) {
                        if (!m_client->GetUserName().IsEmpty()) {
                                strError = wxT("Client '") + m_client->GetUserName() + wxT("'");
                        } else {
                                strError = wxT("An unnamed client");
                        }
                        strError += CFormat(wxT(" (hashDest: %x)")) % m_client->GetTCPDest().hashCode();
                } else {
                        strError = wxT("A client ");
                }
                if (nErrorCode == 0) {
                        strError += wxT("closed connection.");
                } else if (nErrorCode == 0xFEFF) {
                        strError += wxT(" caused a wxSOCKET_LOST event.");
                }	else {
                        strError += wxT("caused a socket blocking error.");
                }
        } else {
                if (theLogger.IsEnabled(logClient) && nErrorCode != 107) {
                        // 0    -> No Error / Disconect
                        // 107  -> Transport endpoint is not connected
                        if (m_client) {
                                if (!m_client->GetUserName().IsEmpty()) {
                                        strError = CFormat(wxT("OnError: Client '%s' (IP:%s) caused an error: %d. Disconnecting client!"))
                                                   % m_client->GetUserName() % m_client->GetTCPDest().humanReadable() % nErrorCode;
                                } else {
                                        strError = CFormat(wxT("OnError: Unknown client (IP:%s) caused an error: %d. Disconnecting client!"))
                                                   % m_client->GetFullIP() % nErrorCode;
                                }
                        } else {
                                strError = CFormat(wxT("OnError: A client caused an error or did something bad (error %d). Disconnecting client !"))
                                           % nErrorCode;
                        }
                } else {
                        strError = wxT("Error 107 (Transport endpoint is not connected)");
                }
        }

        Disconnect(strError);
}


bool CClientTCPSocket::PacketReceived(CPacket* packet)
{
#ifdef __PACKET_RECV_DUMP__

        if (CLogger::IsEnabled(logRemoteClient)) {

                printf("%p (%s <-- %s): Recv: OPCODE %x SIZE %llu\n",
                       this,
                       (const char *) unicode2char(GetLocal().humanReadable().c_str()),
                       (const char *) unicode2char(GetPeer().humanReadable().c_str()), 
		       (unsigned long long) packet->GetOpCode(), packet->GetPacketSize());

                DumpMem(packet->GetPacket(), packet->GetPacketSize() + sizeof(Header_Struct));
        }
#endif
        AddDebugLogLineN(logRemoteClient, CFormat(wxT("%p : Packet (%s, %s), size %u, received from %s"))
                         % this % opstr(packet->GetProtocol()) %
                         opstr(packet->GetOpCode()) % packet->GetPacketSize() %
                         (m_client ? m_client->GetTCPDest().humanReadable() : wxString(wxT("Unknown Client"))));

        // 0.42e
        bool bResult = false;
        uint32 uRawSize = packet->GetPacketSize();

        AddDebugLogLineN( logRemoteClient,
                          CFormat(wxT("Packet with protocol %x, opcode %x, size %u received from %s"))
                          % packet->GetProtocol()
                          % packet->GetOpCode()
                          % packet->GetPacketSize()
                          % ( m_client ? m_client->GetTCPDest().humanReadable() : wxT("Unknown Client") )
                        );

        wxString exceptionString;

        try {
                bool process = true;

                if ((packet->GetProtocol() == OP_PACKEDPROT) ||
                                (packet->GetProtocol() == OP_ED2KV2PACKEDPROT)) {

                        if (!packet->UnPackPacket()) {
                                AddDebugLogLineN(logZLib, wxT("Failed to decompress client TCP packet."));
                                bResult = false;
                                process = false;
                        } else {
                                AddDebugLogLineN(logRemoteClient, CFormat(wxT("Packet unpacked, new protocol %x, opcode %x, size %u"))
                                                 % packet->GetProtocol() % packet->GetOpCode() % packet->GetPacketSize());
                        }
                }

                if (process) {
                        switch (packet->GetProtocol()) {
                        case OP_EDONKEYPROT:
                                bResult = ProcessPacket(packet->GetDataBuffer(),uRawSize,packet->GetOpCode());
                                break;
                        case OP_EMULEPROT:
                                bResult = ProcessExtPacket(packet->GetDataBuffer(), packet->GetPacketSize(), packet->GetOpCode());
                                break;
                        case OP_ED2KV2HEADER:
                                bResult = ProcessED2Kv2Packet(packet->GetDataBuffer(), packet->GetPacketSize(), packet->GetOpCode());
                                break;
                        case OP_ED2KV2PACKEDPROT:
                        case OP_PACKEDPROT:
                                // Packed inside packed?
                                wxFAIL;
                                break;
                        default: {
                                theStats::AddDownOverheadOther(uRawSize);
                                if (m_client) {
                                        m_client->SetDownloadState(DS_ERROR);
                                }
                                Disconnect(wxT("Unknown protocol"));
                                bResult = false;
                        }
                        }
                }
        } catch (const CEOFException& err) {
                exceptionString = wxT("EOF exception: ") + err.what();
        } catch (const CInvalidPacket& err) {
                exceptionString = wxT("InvalidPacket exception: ") + err.what();
        } catch (const wxString& error) {
                exceptionString = wxT("error: ") + (error.IsEmpty() ? wxString(wxT("Unknown error")) : error);
        }

        if (!exceptionString.IsEmpty()) {
                AddDebugLogLineN(logPacketErrors, CFormat(wxT("%p (%s <-- %s): Remote client: %p : "))
                                 % this % GetLocal().humanReadable() %
                                 GetPeer().humanReadable() % m_client +
                                 CFormat(wxT
                                         ("Caught %s\n" "On packet (%s, %s), size %u"
                                          "\tClientData: %s\n")) % exceptionString %
                                 opstr(packet->GetProtocol()) %
                                 opstr(packet->GetOpCode()) % packet->GetPacketSize() % (m_client ? m_client->GetClientFullInfo() : wxT("Unknown")));

                if (m_client) {
                        m_client->SetDownloadState(DS_ERROR);
                }

                AddDebugLogLineN(logRemoteClient, CFormat(wxT("%p (%s <-- %s): Remote client: %p : "))
                                 % this % GetLocal().humanReadable() %
                                 GetPeer().humanReadable() % m_client +
                                 CFormat(wxT("Client '%s' (Dest: %s) caused an error (%s). Disconnecting client!"))
                                 %
                                 (m_client ? m_client->GetUserName() : wxString(wxT("Unknown"))) %
                                 (m_client ? m_client->GetTCPDest().humanReadable() : wxString(wxT("Unknown"))) % exceptionString);

                Disconnect(wxT("Caught exception on CClientTCPSocket::ProcessPacket\n"));
        }

        return bResult;
}


SocketSentBytes CClientTCPSocket::SendControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend)
{
        SocketSentBytes returnStatus = CEMSocket::SendControlData(maxNumberOfBytesToSend, overchargeMaxBytesToSend);

        if(returnStatus.success && (returnStatus.sentBytesControlPackets > 0 || returnStatus.sentBytesStandardPackets > 0)) {
                ResetTimeOutTimer();
        }

        return returnStatus;
}


SocketSentBytes CClientTCPSocket::SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend)
{
        SocketSentBytes returnStatus = CEMSocket::SendFileAndControlData(maxNumberOfBytesToSend, overchargeMaxBytesToSend);

        if(returnStatus.success && (returnStatus.sentBytesControlPackets > 0 || returnStatus.sentBytesStandardPackets > 0)) {
                ResetTimeOutTimer();
        }

        return returnStatus;
}


void CClientTCPSocket::SendPacket(CPacket* packet, bool delpacket, bool controlpacket, uint32 actualPayloadSize)
{
        AddDebugLogLineN(logLocalClient,
                         CFormat(wxT
                                 ("%x : Packet (%s, %s), size %u queued to %s")) %
                         (long) this % opstr(packet->GetProtocol()) %
                         opstr(packet->GetOpCode()) % packet->GetPacketSize() % (m_client ? m_client->GetTCPDest().humanReadable() : wxT("Unknown Client")));

#ifdef __TCPPACKET_SEND_DUMP__
        if (theLogger.isEnabled(logLocalClient)) {
                DumpMem(packet->GetPacket(), packet->GetPacketSize() + sizeof(Header_Struct));
        }
#endif
        ResetTimeOutTimer();
        CEMSocket::SendPacket(packet,delpacket,controlpacket, actualPayloadSize);
}
// File_checked_for_headers
