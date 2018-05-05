//
// This file is part of the imule Project.
//
// Copyright (c) 2003-2011 imule Team ( mkvore@mail.i2p / http://www.imule.i2p )
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

//
// Handling incoming connections (up or downloadrequests)
//

#ifndef CLIENTTCPSOCKET_H
#define CLIENTTCPSOCKET_H

#include "EMSocket.h"		// Needed for CEMSocket
#include <wx/event.h>

//------------------------------------------------------------------------------
// CClientTCPSocket
//------------------------------------------------------------------------------

class CUpDownClient;
class CPacket;
class CTimerWnd;
class CI2PSocketEvent;

class CClientTCPSocket : public CEMSocket
{
public:
        CClientTCPSocket(CUpDownClient* in_client = NULL);
        virtual ~CClientTCPSocket();

        void		Disconnect(const wxString& strReason);

        bool		InitNetworkData();

        bool		CheckTimeOut();

        void		Safe_Delete();
        void		Safe_Delete_Client();

        bool		ForDeletion() const { return m_ForDeletion; }
        
        void            OnSocketEvent(CI2PSocketEvent&);

        void		OnConnect(int nErrorCode);
        void		OnSend(int nErrorCode);
        void		OnReceive(int nErrorCode);

        void		OnClose(int nErrorCode);
        void		OnError(int nErrorCode);

        const CI2PAddress& GetRemoteDest() const { return m_remotedest; }

        CUpDownClient* GetClient() { return m_client; }

        virtual void SendPacket(CPacket* packet, bool delpacket = true, bool controlpacket = true, uint32 actualPayloadSize = 0);
        virtual SocketSentBytes SendControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend);
        virtual SocketSentBytes SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend);

protected:
        virtual bool PacketReceived(CPacket* packet);

private:
        CUpDownClient*	m_client;

        bool	ProcessPacket(const byte* packet, uint32 size, uint8 opcode);
        bool	ProcessExtPacket(const byte* packet, uint32 size, uint8 opcode);
        bool	ProcessED2Kv2Packet(const byte* packet, uint32 size, uint8 opcode);
        void	ResetTimeOutTimer();
        void	SetClient(CUpDownClient* client);

        bool	m_ForDeletion; // 0.30c (Creteil), set as bool
        uint32	timeout_timer;
        CI2PAddress m_remotedest;
        wxString encodeDirName(const CPath & dirName);
};

#endif // CLIENTTCPSOCKET_H
// File_checked_for_headers