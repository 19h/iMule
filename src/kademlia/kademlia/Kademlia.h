//								-*- C++ -*-
// This file is part of the iMule Project.
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

#include "Prefs.h"
#ifndef __KAD_KADEMLIA_H__
#define __KAD_KADEMLIA_H__

#include <map>
#include "kademlia/utils/UInt128.h"
#include "kademlia/routing/Maps.h"
#include "kademlia/net/KademliaUDPListener.h"
#include <common/Macros.h>

#include "i2p/CI2PAddress.h"

////////////////////////////////////////
namespace Kademlia
{
////////////////////////////////////////

class CRoutingZone;
class CIndexed;
class CKadUDPKey;
class CKadClientSearcher;

typedef std::map<CRoutingZone*, CRoutingZone*> EventMap;

class CKademlia
{
public:
        static void Start();
        static void Start(CPrefs *prefs);
        static void Stop();

        static CPrefs               *GetPrefs();
        static CRoutingZone         *GetRoutingZone();
        static CKademliaUDPListener *GetUDPListener();
        static CIndexed             *GetIndexed();
        static bool                 IsRunning() {return m_running;}
        static bool                 IsConnected();
        static bool                 IsFirewalled() { return false; }
        static uint32_t             GetKademliaUsers();
        static uint32_t             GetKademliaFiles();
        static uint32_t             GetTotalStoreKey();
        static uint32_t             GetTotalStoreSrc();
        static uint32_t             GetTotalStoreNotes();
        static uint32_t             GetTotalFile();
        static bool                 GetPublish();
        static CI2PAddress          GetIPAddress();
        static void                 Bootstrap(const CI2PAddress & dest);
        static void                 ProcessPacket(const uint8_t* data, uint32_t lenData, const CI2PAddress & dest, bool validReceiverKey, const CKadUDPKey& senderKey);

        static void ProcessPacket(const uint8_t* data, uint32_t lenData, uint32_t ip, uint16_t port, bool validReceiverKey, const CKadUDPKey& senderKey);

        static void AddEvent(CRoutingZone *zone) throw()		{ m_events[zone] = zone; }
        static void RemoveEvent(CRoutingZone *zone)			{ m_events.erase(zone); }
        static void Process();
        static void StatsAddClosestDistance(const CUInt128& distance);
        static bool FindNodeIDByIP(CKadClientSearcher& requester, uint32_t tcphash, const CI2PAddress & udpdest);
        static bool FindIPByNodeID(CKadClientSearcher& requester, const uint8_t *nodeID);
        static void CancelClientSearch(CKadClientSearcher& fromRequester);
        static bool IsRunningInLANMode();

        static ContactList	s_bootstrapList;

private:
        CKademlia() {}
        ~CKademlia();

        static uint32_t	CalculateKadUsersNew();

        static CKademlia *instance;
        static EventMap	m_events;
        static time_t	m_nextSearchJumpStart;
        static time_t	m_nextSelfLookup;
        static time_t	m_nextFirewallCheck;
        static time_t	m_nextFindBuddy;
        static time_t	m_statusUpdate;
        static time_t	m_bigTimer;
        static time_t	m_bootstrap;
        static time_t	m_consolidate;
        static time_t	m_externPortLookup;
        static time_t	m_lanModeCheck;
        static bool	m_running;
        static bool	m_lanMode;
        static std::list<uint32_t>	m_statsEstUsersProbes;

        CPrefs *		m_prefs;
        CRoutingZone *		m_routingZone;
        CKademliaUDPListener *	m_udpListener;
        CIndexed *		m_indexed;
};

} // End namespace

void KadGetKeywordHash(const wxString& rstrKeyword, Kademlia::CUInt128* pKadID);
wxString KadGetKeywordBytes(const wxString& rstrKeywordW);

#endif // __KAD_KADEMLIA_H__