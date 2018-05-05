//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne (http://www.emule-project.net)
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

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#ifndef __KAD_UDP_LISTENER_H__
#define __KAD_UDP_LISTENER_H__

#include "../utils/UInt128.h"
#include <Tag.h>
#include "PacketTracking.h"

#include <i2p/CI2PAddress.h>


class CMemFile;
struct SSearchTerm;

////////////////////////////////////////
namespace Kademlia
{
////////////////////////////////////////

class CContact;
class CKadUDPKey;
class CKadClientSearcher;

struct FetchNodeID_Struct {
        uint32         tcpDest;
        uint32_t expire;
        CKadClientSearcher* requester;
};

#ifdef __DEBUG__
#   define DebugSendF(what, dest)   AddDebugLogLineN(logClientKadUDP, what+wxString(wxT(" to "))+dest.humanReadable())
#   define DebugRecvF(what, dest)   AddDebugLogLineN(logClientKadUDP, what+wxString(wxT(" from "))+dest.humanReadable())
#else
#   define DebugSendF(what, dest)
#   define DebugRecvF(what, dest)
#endif

#define DebugSend(what, dest)   DebugSendF(wxSTRINGIZE_T(what), dest)
#define DebugRecv(what, dest)   DebugRecvF(wxSTRINGIZE_T(what), dest)


class CKademliaUDPListener : public CPacketTracking
{
public:
        ~CKademliaUDPListener();
        void Bootstrap(const CI2PAddress & dest, bool kad2, uint8_t kadVersion = 0, const CUInt128* cryptTargetID = NULL);
        void SendMyDetails(uint8_t opcode, const CI2PAddress & dest, uint8_t kadVersion, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID, bool requestAckPacket);
        void SendNullPacket(uint8_t opcode, const CI2PAddress & dest, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID);
        void SendPublishSourcePacket(const CContact& contact, const CUInt128& targetID, const CUInt128& contactID, const TagList& tags);
        virtual void ProcessPacket(const uint8_t* data, uint32_t lenData, const CI2PAddress & dest, bool validReceiverKey, const CKadUDPKey& senderKey);
        void SendPacket(const CMemFile& data, uint8_t opcode, const CI2PAddress & dest, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID);

        bool FindNodeIDByIP(CKadClientSearcher *requester, uint32_t tcphash, const CI2PAddress & udpdest);
        void ExpireClientSearch(CKadClientSearcher *expireImmediately = NULL);
private:
        static SSearchTerm* CreateSearchExpressionTree(CMemFile& bio, int iLevel);
        static void Free(SSearchTerm* pSearchTerms);

        // Kad1.0
        bool AddContact (CMemFile & data, const CI2PAddress & udpdest, const CI2PAddress & tcpdest, const CKadUDPKey& key, bool& destVerified, bool update, CUInt128* outContactID);
        void AddContacts(CMemFile & data, uint16_t numContacts, bool update, uint8_t kadversion);
        void SendLegacyChallenge(const CI2PAddress & dest, const CUInt128& contactID, bool kad2);

        void ProcessBootstrapRequest		(CMemFile & packetData, const CI2PAddress & dest);
        void ProcessBootstrapResponse		(CMemFile & packetData, const CI2PAddress & dest);
        void ProcessHelloRequest			(CMemFile & packetData, const CI2PAddress & dest);
        void ProcessHelloResponse			(CMemFile & packetData, const CI2PAddress & dest);
        void ProcessKademliaRequest			(CMemFile & packetData, const CI2PAddress & dest);
        void ProcessKademliaResponse		(CMemFile & packetData, const CI2PAddress & dest);
        void ProcessSearchRequest			(CMemFile & packetData, const CI2PAddress & dest);
        void ProcessSearchResponse			(CMemFile & packetData, const CI2PAddress & dest);
        void ProcessPublishRequest          (CMemFile & packetData, const CI2PAddress & dest, bool self=false);
        void ProcessPublishResponse         (CMemFile & packetData, const CI2PAddress & dest);
        void ProcessSearchNotesRequest      (CMemFile & packetData, const CI2PAddress & dest);
//     void ProcessSearchNotesResponse     (CMemFile & packetData, const CI2PAddress & dest);
        void ProcessPublishNotesRequest     (CMemFile & packetData, const CI2PAddress & dest, bool self=false);
        void ProcessPublishNotesResponse	(CMemFile & packetData, const CI2PAddress & dest);

        // Kad2.0
        bool AddContact2(CMemFile & data, const CI2PAddress & dest, uint8_t* outVersion, const CKadUDPKey& key, bool& ipVerified, bool update, bool fromHelloReq, bool* outRequestsACK, CUInt128* outContactID);

        void Process2BootstrapRequest       (const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2BootstrapResponse      (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey, bool validReceiverKey);
        void Process2HelloRequest           (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey, bool validReceiverKey);
        void Process2HelloResponse          (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey, bool validReceiverKey);
        void Process2HelloResponseAck       (CMemFile & packet, const CI2PAddress & dest, bool validReceiverKey);
        void ProcessKademlia2Request        (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void ProcessKademlia2Response       (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2SearchNotesRequest     (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2SearchKeyRequest       (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2SearchSourceRequest    (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2SearchResponse         (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2PublishNotesRequest    (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2PublishKeyRequest      (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2PublishSourceRequest   (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2PublishResponse        (CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2Ping                   (                   const CI2PAddress & dest, const CKadUDPKey& senderKey);
        void Process2Pong                   (CMemFile & packet, const CI2PAddress & dest);

        void DebugClientOutput(const wxString& place, const CI2PAddress & dest);

        typedef std::list<FetchNodeID_Struct>	FetchNodeIDList;
        FetchNodeIDList m_fetchNodeIDRequests;
};

} // End namespace

#endif //__KAD_UDP_LISTENER_H__
// File_checked_for_headers