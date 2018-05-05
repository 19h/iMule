//
// This file is part of the imule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "ListenSocket.h"	// Interface declarations

#include <common/EventIDs.h>

#include "ClientTCPSocket.h"	// Needed for CClientRequestSocket
#include "Logger.h"			// Needed for AddLogLineM
#include "Statistics.h"		// Needed for theStats
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"		// Needed for theApp
#include "ServerConnect.h"	// Needed for CServerConnect
#include "common/Format.h"
#include "updownclient.h"

//-----------------------------------------------------------------------------
// CListenSocket
//-----------------------------------------------------------------------------
//
// This is the socket that listens to incomming connections in aMule's TCP port
// As soon as a connection is detected, it creates a new socket of type
// CClientTCPSocket to handle (accept) the connection.
//

CListenSocket::CListenSocket(const wxString & key)
        :
// wxSOCKET_NOWAIT      - means : trigger a WOULDBLOCK error if i/o sent/received less bytes than asked
// wxSOCKET_REUSEADDR - means we can reuse the socket imediately (wx-2.5.3)
        wxI2PSocketServer(key, wxSOCKET_NOWAIT | wxSOCKET_REUSEADDR )
{
        // 0.42e - vars not used by us
        bListening = false;
        shutdown = false;
        m_OpenSocketsInterval = 0;
        m_nPendingConnections = 0;
        totalconnectionchecks = 0;
        averageconnections = 0.0;
}


CListenSocket::~CListenSocket()
{
        shutdown = true;

#ifdef __DEBUG__
        // No new sockets should have been opened by now
        for (SocketSet::iterator it = socket_list.begin(); it != socket_list.end(); it++) {
                wxASSERT((*it)->OnDestroy());
        }
#endif

        AddDebugLogLineN(logListenSocket, CFormat(wxT("Deleted Listen Socket")) );
}

bool CListenSocket::Destroy()
{
        AddDebugLogLineN(logListenSocket, CFormat(wxT("Destroy called on CListenSocket")) );
        shutdown = true ;
        KillAllSockets();
        return wxI2PSocketServer::Destroy() ;
}


bool CListenSocket::StartListening()
{
        AddDebugLogLineN(logListenSocket, CFormat(wxT("StartListening")) );
        // 0.42e
        bListening = true;

        Notify(true);
        return true;
}

void CListenSocket::ReStartListening()
{
        AddDebugLogLineN(logListenSocket, CFormat(wxT("ReStartListening")) );
        // 0.42e
        bListening = true;
        Notify(true);
        if (m_nPendingConnections) {
                m_nPendingConnections--;
                OnAccept(0);
        }
}

void CListenSocket::StopListening()
{
        AddDebugLogLineN(logListenSocket, CFormat(wxT("StopListening")) );
        // 0.42e
        bListening = false;
        Notify(false);
        theStats::AddMaxConnectionLimitReached();
}

void CListenSocket::OnAccept(int nErrorCode)
{
        AddDebugLogLineN(logListenSocket, CFormat(wxT("OnAccept(%d)")) % nErrorCode );
        // 0.42e
        if (!nErrorCode) {
                m_nPendingConnections++;
                if (m_nPendingConnections < 1) {
                        wxFAIL;
                        m_nPendingConnections = 1;
                }
                if (TooManySockets(true) && !theApp->serverconnect->IsConnecting()) {
                        StopListening();
                        return;
                } else if (bListening == false) {
                        // If the client is still at maxconnections,
                        // this will allow it to go above it ...
                        // But if you don't, you will get a lowID on all servers.
                        ReStartListening();
                }
                // Deal with the pending connections, there might be more than one, due to
                // the StopListening() call above.
                while (m_nPendingConnections) {
                        m_nPendingConnections--;
                        // Create a new socket to deal with the connection
                        CClientTCPSocket* newclient = new CClientTCPSocket();
                        // Accept the connection and give it to the newly created socket
                        if (!AcceptWith(*newclient, false)) {
                                newclient->Safe_Delete();
                        } else {
                                wxASSERT(theApp->IsRunning());
                                if (!newclient->InitNetworkData()) {
                                        // IP or port were not returned correctly
                                        // from the accepted address, or filtered.
                                        AddDebugLogLineN(logListenSocket, CFormat(wxT("%x : Rejected connection from %x")) % (long) newclient % newclient->GetPeer().hashCode());
                                        newclient->Safe_Delete();
                                } else
                                        AddDebugLogLineN(logListenSocket, CFormat(wxT("%x : Accepted connection from %x")) % (long) newclient % newclient->GetPeer().hashCode());
                        }
                }
        }
}

void CListenSocket::AddConnection()
{
        m_OpenSocketsInterval++;
}

void CListenSocket::Process()
{
        // 042e + Kry changes for Destroy
        m_OpenSocketsInterval = 0;
        for (CClientTCPSocket* cur_socket: socket_list) {
                if (!cur_socket->OnDestroy()) {
                        if (cur_socket->ForDeletion()) {
                                cur_socket->Destroy();
                        } else {
                                cur_socket->CheckTimeOut();
                        }
                }
        }

        if ((GetOpenSockets()+5 < thePrefs::GetMaxConnections() || theApp->serverconnect->IsConnecting()) && !bListening) {
                ReStartListening();
        }
}

void CListenSocket::RecalculateStats()
{
        // 0.42e
        memset(m_ConnectionStates,0,6);
        for (CClientTCPSocket* cur_socket: socket_list) {
                switch (cur_socket->GetConState()) {
                case ES_DISCONNECTED:
                        m_ConnectionStates[0]++;
                        break;
                case ES_NOTCONNECTED:
                        m_ConnectionStates[1]++;
                        break;
                case ES_CONNECTED:
                        m_ConnectionStates[2]++;
                        break;
                }
        }
}

void CListenSocket::AddSocket(CClientTCPSocket* toadd)
{
        wxASSERT(toadd);
        socket_list.insert(toadd);
        theStats::AddActiveConnection();
}

void CListenSocket::RemoveSocket(CClientTCPSocket* todel)
{
        wxASSERT(todel);
        socket_list.erase(todel);
        theStats::RemoveActiveConnection();
}

void CListenSocket::KillAllSockets()
{
        // 0.42e reviewed - they use delete, but our safer is Destroy...
        // But I bet it would be better to call Safe_Delete on the socket.
        // Update: no... Safe_Delete MARKS for deletion. We need to delete it.
        for (CClientTCPSocket* cur_socket: socket_list) {
                if (cur_socket->GetClient()) {
                        cur_socket->GetClient()->Safe_Delete();
                } else {
                        cur_socket->Safe_Delete();
                        cur_socket->Destroy();
                }
        }
}

bool CListenSocket::TooManySockets(bool bIgnoreInterval)
{
        if (GetOpenSockets() > thePrefs::GetMaxConnections() || (m_OpenSocketsInterval > (thePrefs::GetMaxConperFive()*GetMaxConperFiveModifier()) && !bIgnoreInterval)) {
                return true;
        } else {
                return false;
        }
}

bool CListenSocket::IsValidSocket(CClientTCPSocket* totest)
{
        // 0.42e
        return socket_list.count(totest);
}


void CListenSocket::UpdateConnectionsStatus()
{
        // 0.42e xcept for the khaos stats
        if( theApp->IsConnected() ) {
                totalconnectionchecks++;
                float percent;
                percent = (float)(totalconnectionchecks-1)/(float)totalconnectionchecks;
                if( percent > .99f ) {
                        percent = .99f;
                }
                averageconnections = (averageconnections*percent) + (float)GetOpenSockets()*(1.0f-percent);
        }
}


float CListenSocket::GetMaxConperFiveModifier()
{
        float SpikeSize = GetOpenSockets() - averageconnections;
        if ( SpikeSize < 1 ) {
                return 1;
        }

        float SpikeTolerance = 2.5f*thePrefs::GetMaxConperFive();
        if ( SpikeSize > SpikeTolerance ) {
                return 0;
        }

        return 1.0f - (SpikeSize/SpikeTolerance);
}
// File_checked_for_headers