//
// This file is part of the imule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne (http://www.emule-project.net)
// Copyright (c) 2004-2011 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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

#include "Search.h"

#include <protocol/Protocols.h>
#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad/Constants.h>
#include <protocol/kad2/Client2Client/UDP.h>
#include <tags/FileTags.h>

#include "Defines.h"
#include "Kademlia.h"
#include "../routing/RoutingZone.h"
#include "../routing/Contact.h"
#include "../net/KademliaUDPListener.h"
#include "../utils/KadClientSearcher.h"
#include "../../amule.h"
#include "../../SharedFileList.h"
#include "../../DownloadQueue.h"
#include "../../PartFile.h"
#include "../../SearchList.h"
#include "../../MemFile.h"
#include "../../ClientList.h"
#include "../../updownclient.h"
#include "../../Logger.h"
#include "../../Preferences.h"
#include "../../GuiEvents.h"
#include "../../Packet.h"

#ifndef AMULE_DAEMON
#include "../../amuleDlg.h"           // Needed for CamuleDlg
#include "../../SearchDlg.h"          // Needed for CSearchDlg
#include "../../SearchListCtrl.h"
#include <wx/wupdlock.h>
#endif

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CSearchContact::CSearchContact() : CContact()
{
        m_results_count = 0;
}

CSearchContact::CSearchContact ( const CContact& other) : CContact(other)
{
        m_results_count = 0;
}

// void CSearchContact::SetExecutionDelay ( time_t e )
// {
//         m_execution_date = e + std::time(NULL);
// }
// bool CSearchContact::DeathSentence() const
// {
//         return m_execution_date < std::time(NULL);
// }







CSearch::CSearch(const CUInt128 & target) :
m_ask_for_contacts(target),
m_ask_for_data(target),
m_best_complete(target),
m_charged_contacts(target)
{
        m_target = target;
        m_type = (uint32_t)-1;
        m_completedPeers = 0;
        m_searchResults = 0;
        m_searchID = (uint32_t)-1;
        m_stopping = false;
        m_totalLoad = 0;
        m_totalLoadResponses = 0;
        m_nodeSpecialSearchRequester = NULL;
        m_lastActivity = time ( NULL );
        m_closestDistantFound = 0;
        m_requestedMoreNodesContact = CSearchContact();
}

CSearch::~CSearch()
{
        AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i:  deleted search" ) ) % m_searchID ) ;

        // remember the closest node we found and tried to contact (if any) during this search
        // for statistical caluclations, but only if its a certain type
        switch (m_type) {
        case NODECOMPLETE:
        case FILE:
        case KEYWORD:
        case NOTES:
        case STOREFILE:
        case STOREKEYWORD:
        case STORENOTES:
                if (m_closestDistantFound != 0) {
                        CKademlia::StatsAddClosestDistance(m_closestDistantFound);
                }
                break;
        default: // NODE, NODESPECIAL, NODEFWCHECKUDP, FINDBUDDY
                break;
        }
        if (m_nodeSpecialSearchRequester != NULL) {
                // inform requester that our search failed
                m_nodeSpecialSearchRequester->KadSearchIPByNodeIDResult(KCSR_NOTFOUND, CI2PAddress::null);
        }

        // Check if a source search is currently being done.
        CPartFile* temp = theApp->downloadqueue->GetFileByKadFileSearchID(GetSearchID());

        // Reset the searchID if a source search is currently being done.
        if (temp) {
                temp->SetKadFileSearchID(0);
        }

        // Check if this search was containing an overload node and adjust time of next time we use that node.
        if ( CKademlia::IsRunning() && GetNodeLoad() > 20 ) {
                switch(GetSearchType()) {
                case CSearch::STOREKEYWORD:
                        Kademlia::CKademlia::GetIndexed()->AddLoad(GetTarget(), ((uint32_t)(DAY2S(7)*((double)GetNodeLoad()/100.0))+(uint32_t)time(NULL)));
                        break;
                }
        }

        // add keywords if a file has been published
        // and updates the item in the sharedfileslist : new source counts can have been received
        if ( GetSearchType() == CSearch::STOREFILE
                        && GetCompletedPeers() > 0 ) {
                CKnownFile* file = NULL ;
                uint8_t fileid[16];
                m_target.ToByteArray ( fileid );
                file = theApp->sharedfiles->GetFileByID ( CMD4Hash ( fileid ) );
                if (file ) {
                        theApp->sharedfiles->AddKeywords( file ) ;
                        Notify_SharedFilesUpdateItem(file);
                }
        }

        switch (m_type) {
        case KEYWORD:
                Notify_KadSearchEnd(m_searchID);
                break;
        }
}

void CSearch::Go()
{
        wxString debugStr;
#ifdef __DEBUG__
#define DEBSTR(X) debugStr << (X) << wxT(" ")
#else
#define DEBSTR(X)
#endif
        // Send some requests to myself
        // Useful for very small networks
        switch (m_type) {
        case FILE: case KEYWORD: case NOTES: case STOREFILE: case STOREKEYWORD: case STORENOTES:
		wxASSERT( m_ask_for_data.empty() );
		m_known.add(CSearchContact::Self());
                m_ask_for_data.add(CSearchContact::Self());
                chargeclosest(m_ask_for_data, [this](CSearchContact c){SendRequestTo(c);});
        }

        // Start with a lot of possible contacts, this is a fallback in case search stalls due to dead contacts

        wxASSERT( m_ask_for_contacts.empty() && m_ask_for_data.empty() );

        DEBSTR ( 1 );
        CUInt128 distance = CKademlia::GetPrefs()->GetKadID() ^ m_target;
        CKademlia::GetRoutingZone()->GetClosestToTarget ( 3, distance, 50, m_ask_for_contacts, true );
        // fall back to type 4 contacts if looking for node and still empty
        if (m_ask_for_contacts.size()<50) {
                CKademlia::GetRoutingZone()->GetClosestToTarget ( 4, distance, 50, m_ask_for_contacts, false );
        }

        DEBSTR ( 2 );
	// add contacts to known set
        m_ask_for_contacts.apply([this](CSearchContact&c){m_known.add(c);});
	
        AddDebugLogLineN(logKadSearch,
                         CFormat ( wxT ( "#%i: Go. %s ! known: %d, CONTACTS askable: %d" ) )
                         % m_searchID
                         % typestr()
                         % m_known.size()
                         % m_ask_for_contacts.size()
	);
}

//If we allow about a 15 sec delay before deleting, we won't miss a lot of delayed returning packets.
// stop looking for peers, but gives time for waiting for responses

void CSearch::StopLookingForPeers() throw ()
{
        if ( m_stopping ) {
                return;
        }
        m_stopping = true;
}

/**
 * JumpStart: collect responses and act accordingly
 * return: false if the search is to be killed
 */

bool CSearch::JumpStart()
{
        time_t now = time(NULL);
        // If we had a response within the last 3 seconds, no jumpstart.
        if ((time_t)(m_lastActivity + SEC(1)) > now) {
		return true;
        }

        AddDebugLogLineN(logKadSearch,
                         CFormat(wxT("#%i: JumpStart %s. known: %d, charged: %d, CONTACTS askable: %d, DATA askable: %d, completed: %d, trash: %d, searchAnswers: %d (maxAnswers: %d)"))
                         % m_searchID
                         % typestr()
                         % m_known.size()
                         % m_charged_contacts.size()
                         % m_ask_for_contacts.size()
                         % m_ask_for_data.size()
                         % m_best_complete.size()
                         % m_trash.size()
                         % GetSearchAnswers() % GetSearchMaxAnswers());


        
	// kill this search if max answers have been received and no known contact is better than the worst complete
        // or if every possible peers have already answered with results
	bool kill_this_search = false;
        CUInt128 worst_distance(true);
	if (GetSearchAnswers()>=GetSearchMaxAnswers() || GetCompletedPeers()>=GetCompletedListMaxSize()) {
		kill_this_search = true;
		worst_distance = m_best_complete.getFurthestDistance();
		if (m_type!=NODE &&
			(  (!m_charged_contacts.empty() && m_charged_contacts.getClosestDistance() < worst_distance)
			|| (!m_ask_for_contacts.empty() && m_ask_for_contacts.getClosestDistance() < worst_distance)
			|| (!m_ask_for_data.empty() && m_ask_for_data.getClosestDistance() < worst_distance)))
			kill_this_search = false;
		
		if (!kill_this_search) {
			if (!m_charged_contacts.empty() && m_charged_contacts.getClosestDistance() < worst_distance)
				AddDebugLogLineN(logKadSearch,
						 CFormat(wxT("#%i: JumpStart %s. worst_complete=%s, best charged=%s"))
						 % m_searchID
						 % typestr()
						 % worst_distance.ToBinaryString().Mid(0,12)
						 % m_charged_contacts.getClosestDistance().ToBinaryString().Mid(0,12));
			else if (!m_ask_for_contacts.empty() && m_ask_for_contacts.getClosestDistance() < worst_distance)
				AddDebugLogLineN(logKadSearch,
						 CFormat(wxT("#%i: JumpStart %s. worst_complete=%s, best ask_for_contacts=%s"))
						 % m_searchID
						 % typestr()
						 % worst_distance.ToBinaryString().Mid(0,12)
						 % m_ask_for_contacts.getClosestDistance().ToBinaryString().Mid(0,12));
			else if (!m_ask_for_data.empty() && m_ask_for_data.getClosestDistance() < worst_distance)
				AddDebugLogLineN(logKadSearch,
						 CFormat(wxT("#%i: JumpStart %s. worst_complete=%s, best ask_for_data=%s"))
						 % m_searchID
						 % typestr()
						 % worst_distance.ToBinaryString().Mid(0,12)
						 % m_ask_for_data.getClosestDistance().ToBinaryString().Mid(0,12));
		}
        }
        if (m_ask_for_contacts.empty() and m_ask_for_data.empty() and (m_charged_contacts.empty() or now > m_lastActivity + GetSearchLifetime()))
                kill_this_search = true;

        if (kill_this_search) {
		if ( m_type == NODECOMPLETE && GetSearchAnswers() != 0 ) CKademlia::GetPrefs()->SetPublish ( true );
		return false ;
	}	

	enum { NONE, CONTACT, DATA } todo = NONE ;
        if (!m_ask_for_contacts.empty()) {
                todo = CONTACT;
        }
        if (!m_ask_for_data.empty()) {
                if (todo==NONE || m_ask_for_contacts.getClosestDistance() > m_ask_for_data.getClosestDistance())
                        todo = DATA;
        }
        if (todo!=NONE && GetCompletedPeers()>=GetCompletedListMaxSize()) {
                worst_distance = m_best_complete.getFurthestDistance();
                if (todo==CONTACT && (m_ask_for_contacts.getClosestDistance() >= worst_distance))
                {
                        todo = NONE ;
                        AddDebugLogLineN(logKadSearch,
                                         CFormat(wxT("#%i: JumpStart %s. worst_complete=%s < best ask_for_contacts=%s - nothing to do"))
                                         % m_searchID
                                         % typestr()
                                         % worst_distance.ToBinaryString().Mid(0,12)
                                         % m_ask_for_contacts.getClosestDistance().ToBinaryString().Mid(0,12));
                        m_ask_for_contacts.moveFurthestTo(&m_trash);
                }
                
                if (todo==DATA && (m_ask_for_data.getClosestDistance() >= worst_distance))
                {
                        todo = NONE ;
                        AddDebugLogLineN(logKadSearch,
                                         CFormat(wxT("#%i: JumpStart %s. worst_complete=%s < best ask_for_data=%s - nothing to do"))
                                         % m_searchID
                                         % typestr()
                                         % worst_distance.ToBinaryString().Mid(0,12)
                                         % m_ask_for_data.getClosestDistance().ToBinaryString().Mid(0,12));
                        m_ask_for_data.moveFurthestTo(&m_trash);
                }
        }	
	switch (todo) {
		case CONTACT: {
                        chargeclosest(m_ask_for_contacts, [this](CSearchContact&c){SendFindPeersForTarget(c);});
			break;
		}
		case DATA: {
                        chargeclosest(m_ask_for_data, [this](CSearchContact&c){SendRequestTo(c);});
			break;
		}
		default:
			break;
	}

        return true ;
}

/**
 * Processes a contact list received for this search
 * @param fromDest
 * @param results
 */
void CSearch::ProcessResponse ( const CI2PAddress & fromIP, const ContactList & results )
{
        AddDebugLogLineN(logKadSearch, wxT("Processing search response from ") + fromIP.humanReadable());

        m_lastActivity = time(NULL);

        // Find contact that is responding.

        CSearchContact fromContact = getChargedContact(fromIP);
	if (fromContact.IsInvalid()) {
		AddDebugLogLineC(logKadSearch, wxT("Node ") + fromIP.humanReadable() + wxT(" sent contacts without being asked"));
		return;
        }

	
        // Make sure the node is not sending more results than we requested, which is not only a protocol violation
        // but most likely a malicious answer
        if (results.size() > GetRequestContactCount() && !(m_requestedMoreNodesContact.IsValid() && m_requestedMoreNodesContact == fromContact && results.size() <= KADEMLIA_FIND_VALUE_MORE)) {
                AddDebugLogLineN(logKadSearch, wxT("Node ") + fromIP.humanReadable() + wxT(" sent more contacts than requested on a routing query, ignoring response"));
                killMaliciousContact(fromContact);
                return;
        }

        // Not interested in responses for FIND_NODE, will be added to contacts by udp listener
        if (m_type == NODE) {
                AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: Node type search result, discarding." ) ) % m_searchID );
                m_searchResults++;
                m_completedPeers ++ ;
		addToCompletedPeers(fromContact);
                return;
        }

	std::map<uint32_t, unsigned> receivedIPs;
	// A node is not allowed to answer with contacts to itself
	receivedIPs[fromIP] = 1;

	AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: %s added to the list of people who responded, searching %s" ) )
				% m_searchID % fromIP.humanReadable() % m_target.ToHexString() );

	// Loop through their responses
	for ( ContactList::const_iterator it = results.begin(); it != results.end(); ++it ) {
		// Get next result
		CContact c = *it;
		// Ignore this contact if already known
		if (m_known.contains(c)) {
// 			AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: %s already known searching %s" ) )
// 						% m_searchID % c.GetUDPDest().humanReadable() % m_target.ToHexString() );
			continue;
		}

		// We only accept unique IPs in the answer, having multiple IDs pointing to one IP in the routing tables
		// is no longer allowed since eMule0.49a, aMule-2.2.1 anyway
		if (receivedIPs.count(c.GetIPAddress()) > 0) {
			AddDebugLogLineN(logKadSearch, wxT("Multiple KadIDs pointing to same IP (") + c.GetIPAddress().humanReadable() + wxT(") in Kad2Res answer - ignored, sent by ") + fromContact.GetIPAddress().humanReadable());
			continue;
		} else {
			receivedIPs[c.GetIPAddress()] = 1;
		}

		// Add to known and ask_for_contacts list
		AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: %s added to the list of known clients searching %s" ) )
					% m_searchID % c.GetUDPDest().humanReadable() % m_target.ToHexString() );
		m_known.add(c);
		m_ask_for_contacts.add(c);
        }

	// Complete node search, just increment the counter.
	if ( m_type == NODECOMPLETE || m_type == NODESPECIAL ) {
		AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: Search result type: Node complete" ) ) % m_searchID );
		m_searchResults++;
		m_completedPeers++;
		addToCompletedPeers(fromContact);
	} else {
		// Add to list of people to ask for data
		unchargeContactTo(fromContact, &m_ask_for_data);
		AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: Adding %s to m_ask_for_data" ) ) % m_searchID % fromContact.GetUDPDest().humanReadable());
	}
}

// sends the effective request to peers that have responded to peer request
// called CSearch::StorePacket() in amule
void CSearch::SendRequestTo(CContact To)
{
        CUInt128                    ToDistance    = To.GetClientID() ^ m_target;
        //         bool requestSent = false ;
        if (ToDistance < m_closestDistantFound || m_closestDistantFound == 0) {
                m_closestDistantFound = ToDistance;
        }

        // What kind of search are we doing?
        switch (m_type) {
        case FILE: {
                AddDebugLogLineN(logKadSearch, wxT("Search request type: File"));
                CMemFile searchTerms;
                searchTerms.WriteUInt128(m_target);
                if (To.GetVersion() >= 3) {
                        // Find file we are storing info about.
                        uint8_t fileid[16];
                        m_target.ToByteArray(fileid);
                        CKnownFile *file = theApp->downloadqueue->GetFileByID(CMD4Hash(fileid));
                        if (file) {
                                // Start position range (0x0 to 0x7FFF)
                                searchTerms.WriteUInt16(0);
                                searchTerms.WriteUInt64(file->GetFileSize());
                                DebugSend(Kad2SearchSourceReq, To.GetUDPDest());
                                if (To.GetVersion() >= 6) {
                                        CUInt128 clientID = To.GetClientID();
                                        CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_SOURCE_REQ, To.GetUDPDest(), To.GetUDPKey(), &clientID);
                                } else {
                                        CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_SOURCE_REQ, To.GetUDPDest(), 0, NULL);
                                        wxASSERT(To.GetUDPKey() == CKadUDPKey(0));
                                }
                        } else {
                                StopLookingForPeers();
                                break;
                        }
                } else {
                        searchTerms.WriteUInt8(1);
                        DebugSendF(wxT("KadSearchReq(File)"), To.GetUDPDest());
                        CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA_SEARCH_REQ, To.GetUDPDest(), 0, NULL);
                }
                //                 requestSent = true;
                break;
        }
        case KEYWORD: {
                AddDebugLogLineN(logKadSearch, wxT("Search request type: Keyword"));
                CMemFile searchTerms;
                searchTerms.WriteUInt128(m_target);
                if (To.GetVersion() >= 3) {
                        if (m_searchTerms.GetLength() == 0) {
                                // Start position range (0x0 to 0x7FFF)
                                searchTerms.WriteUInt16(0);
                        } else {
                                // Start position range (0x8000 to 0xFFFF)
                                searchTerms.WriteUInt16(0x8000);
                                searchTerms.Write(m_searchTerms.GetRawBuffer(), m_searchTerms.GetLength());
                        }
                        DebugSend(Kad2SearchKeyReq, To.GetUDPDest());
                } else {
                        if (m_searchTerms.GetLength() == 0) {
                                searchTerms.WriteUInt8(0); // iMule v1 seems to need this
                        } else {
                                searchTerms.WriteUInt8(1); // iMule v1 seems to need this
                                searchTerms.Write(m_searchTerms.GetRawBuffer(), m_searchTerms.GetLength());
                        }
                        DebugSendF(wxT("KadSearchReq(Keyword)"), To.GetUDPDest());
                }
                if (To.GetVersion() >= 6) {
                        CUInt128 clientID = To.GetClientID();
                        CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_KEY_REQ, To.GetUDPDest(), To.GetUDPKey(), &clientID);
                } else if (To.GetVersion() >= 3) {
                        CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_KEY_REQ, To.GetUDPDest(), 0, NULL);
                        wxASSERT(To.GetUDPKey() == CKadUDPKey(0));
                } else {
                        CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA_SEARCH_REQ, To.GetUDPDest(), 0, NULL);
                }
                //                 requestSent = true;
                break;
        }
        case NOTES: {
                AddDebugLogLineN(logKadSearch, wxT("Search request type: Notes"));
                // Write complete packet.
                CMemFile searchTerms;
                searchTerms.WriteUInt128(m_target);
                if (To.GetVersion() >= 3) {
                        // Find file we are storing info about.
                        uint8_t fileid[16];
                        m_target.ToByteArray(fileid);
                        CKnownFile *file = theApp->sharedfiles->GetFileByID(CMD4Hash(fileid));
                        if (file) {
                                // Start position range (0x0 to 0x7FFF)
                                searchTerms.WriteUInt64(file->GetFileSize());
                                DebugSend(Kad2SearchNotesReq, To.GetUDPDest());
                                if (To.GetVersion() >= 6) {
                                        CUInt128 clientID = To.GetClientID();
                                        CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_NOTES_REQ, To.GetUDPDest(),
                                                                                To.GetUDPKey(), &clientID);
                                } else {
                                        CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA2_SEARCH_NOTES_REQ, To.GetUDPDest(), 0, NULL);
                                        wxASSERT(To.GetUDPKey() == CKadUDPKey(0));
                                }
                        } else {
                                StopLookingForPeers();
                                break;
                        }
                } else {
                        searchTerms.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
                        DebugSend(KadSearchNotesReq, To.GetUDPDest());
                        CKademlia::GetUDPListener()->SendPacket(searchTerms, KADEMLIA_SEARCH_NOTES_REQ, To.GetUDPDest(), 0, NULL);
                }
                //                 requestSent = true;
                break;
        }
        case STOREFILE: {
                AddDebugLogLineN(logKadSearch, wxT("Search request type: StoreFile"));
                // Try to store myself as a source to a Node.

                // Find the file I'm trying to store myself as a source to.
                uint8_t fileid[16];
                m_target.ToByteArray(fileid);
                CKnownFile* file = theApp->sharedfiles->GetFileByID(CMD4Hash(fileid));
                if (file) {
                        // We store this mostly for GUI reasons.
                        m_fileName = file->GetFileName().GetPrintable();

                        // Get our clientID for the packet.
                        CUInt128 id(CKademlia::GetPrefs()->GetClientHash());
                        TagList taglist;

                        // We're not firewalled..
                        taglist.push_back ( CTag ( TAG_SOURCETYPE, 1 ) );
                        taglist.push_back ( CTag ( TAG_SOURCEDEST, theApp->GetTcpDest() ) );
                        taglist.push_back ( CTag ( TAG_FLAGS, file->IsPartFile() ? 0 : TAG_FLAGS_COMPLETEFILE) );
                        if (To.GetVersion() >= 2) {
                                taglist.push_back(CTag(TAG_FILESIZE, file->GetFileSize()));
                        }
                        taglist.push_back(CTag(TAG_ENCRYPTION, (uint64_t) CPrefs::GetMyConnectOptions(true, true)));

                        // Send packet
                        CKademlia::GetUDPListener()->SendPublishSourcePacket(To, m_target, id, taglist);
			AddDebugLogLineN(logKadPublish, CFormat(wxT("#%d: SendPublishSourcePacket: file %s To %s (distance %s)"))
				% this->GetSearchID()
				% m_target.ToBinaryString().Mid(0,12)
				% To.GetClientID().ToBinaryString().Mid(0,12)
				% (m_target ^ To.GetClientID()).ToBinaryString().Mid(0,12)
			);

                        //                         requestSent = true;
                } else {
                        StopLookingForPeers();
                }
                break;
        }
        case STOREKEYWORD: {
                AddDebugLogLineN(logKadSearch, wxT("Search request type: StoreKeyword"));
                // Try to store keywords to a Node.

                uint16_t count = m_fileIDs.size();
                if (count == 0) {
                        StopLookingForPeers();
                        break;
                } else if (count > 150) {
                        count = 150;
                }

                UIntList::const_iterator itLastSentFileID, itListFileID = m_fileIDs.begin();
                uint8_t fileid[16];
                uint64_t posNresults = 0;
                bool initPacket = true ;
                uint16_t packetCount = 0;
                CMemFile packetdata(2 * CPacket::UDPPacketMaxDataSize()); // Allocate a good amount of space.
                size_t posLastResult = 0 ;

                while (count && (itListFileID != m_fileIDs.end())) {
                        // packet initialization
                        if (initPacket) {
                                packetdata.Reset();
                                packetdata.WriteUInt128(m_target);
                                posNresults = packetdata.GetPosition() ;
                                packetdata.WriteUInt16 ( 0 );           // number of results. Will be overriden by "count"
                                packetCount = 0;
                                initPacket = false ;                    // unset initPacket flag
                                posLastResult = (uint32_t)packetdata.GetPosition() ; // packet length before writing the entry
                        }
                        CUInt128 id(*itListFileID);
                        id.ToByteArray(fileid);
                        CKnownFile *pFile = theApp->sharedfiles->GetFileByID(CMD4Hash(fileid));
                        if (pFile) {
                                packetdata.WriteUInt128(id);
                                PreparePacketForTags(&packetdata, pFile);
                                // test : if packet size is too big, revert last write
                                //        and reset initPacket flag
                                if (packetdata.GetPosition()>CPacket::UDPPacketMaxDataSize()) {
                                        if ( count != 0 ) {
                                                initPacket = true ;
                                        } else { // forget this entry if it cannot fit alone in a packet
                                                count-- ;
                                                itListFileID++ ;
                                        }
                                } else {
                                        packetCount++;
                                        posLastResult = (uint32_t)packetdata.GetPosition() ; // packet length after writing the entry
                                        count--;
                                        itListFileID++;
                                }

                        }
                        // test : send packet if it is full or if we reached its max size
                        if ( initPacket or (itListFileID==m_fileIDs.end()) ) {
                                // Correct file count.
                                packetdata.Seek ( posNresults );
                                packetdata.WriteUInt16(packetCount);
                                packetdata.Reset();
                                packetdata.SetLength( posLastResult );

                                // Send packet
                                if (To.GetVersion() >= 6) {
                                        DebugSend(Kad2PublishKeyReq, To.GetUDPDest());
                                        CUInt128 clientID = To.GetClientID();
                                        CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_PUBLISH_KEY_REQ, To.GetUDPDest(), To.GetUDPKey(), &clientID);
                                } else if (To.GetVersion() >= 2) {
                                        DebugSend(Kad2PublishKeyReq, To.GetUDPDest());
                                        CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_PUBLISH_KEY_REQ, To.GetUDPDest(), 0, NULL);
                                        wxASSERT(To.GetUDPKey() == CKadUDPKey(0));
                                } else {
                                        DebugSend(KadPublishReq, To.GetUDPDest());
                                        CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA_PUBLISH_REQ, To.GetUDPDest(), 0, NULL);
                                }
                        }
                }
                //                 requestSent = true;
                break;
        }
        case STORENOTES: {
                AddDebugLogLineN(logKadSearch, wxT("Search request type: StoreNotes"));
                // Find file we are storing info about.
                uint8_t fileid[16];
                m_target.ToByteArray(fileid);
                CKnownFile* file = theApp->sharedfiles->GetFileByID(CMD4Hash(fileid));

                if (file) {
                        CMemFile packetdata(1024*2);
                        // Send the hash of the file we're storing info about.
                        packetdata.WriteUInt128(m_target);
                        // Send our ID with the info.
                        packetdata.WriteUInt128(CKademlia::GetPrefs()->GetKadID());

                        // Create our taglist.
                        TagList taglist;
                        taglist.push_back(CTag(TAG_FILENAME, file->GetFileName().GetPrintable()));
                        if (file->GetFileRating() != 0) {
                                taglist.push_back(CTag(TAG_FILERATING, (uint64_t) file->GetFileRating()));
                        }
                        if (!file->GetFileComment().IsEmpty()) {
                                taglist.push_back(CTag(TAG_DESCRIPTION, file->GetFileComment()));
                        }
                        if (To.GetVersion() >= 2) {
                                taglist.push_back(CTag(TAG_FILESIZE, file->GetFileSize()));
                        }
                        taglist.WriteTo(&packetdata);

                        // Send packet
                        if (To.GetVersion() >= 6) {
                                DebugSend(Kad2PublishNotesReq, To.GetUDPDest());
                                CUInt128 clientID = To.GetClientID();
                                CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_PUBLISH_NOTES_REQ, To.GetUDPDest(), To.GetUDPKey(), &clientID);
                        } else if (To.GetVersion() >= 2) {
                                DebugSend(Kad2PublishNotesReq, To.GetUDPDest());
                                CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_PUBLISH_NOTES_REQ, To.GetUDPDest(), 0, NULL);
                                wxASSERT(To.GetUDPKey() == CKadUDPKey(0));
                        } else {
                                DebugSend(KadPublishNotesReq, To.GetUDPDest());
                                CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA_PUBLISH_NOTES_REQ_DEPRECATED, To.GetUDPDest(), 0, NULL);
                        }
                        //                         requestSent = true;
                } else {
                        StopLookingForPeers();
                }
                break;
        }
        case NODESPECIAL: {
                // we are looking for the IP of a given NodeID, so we just check if we 0 distance and if so, report the
                // tip to the requester
                if (ToDistance == 0) {
                        m_nodeSpecialSearchRequester-> KadSearchIPByNodeIDResult(KCSR_SUCCEEDED, To.GetTCPDest());
                        m_nodeSpecialSearchRequester = NULL;
                        StopLookingForPeers();
                }
                break;
        }
        case NODECOMPLETE:
                AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: Search result type : NodeComplete" ) ) % m_searchID );
                break;
        case NODE:
                AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: Search result type : Node" ) ) % m_searchID );
                break;
        default:
                AddDebugLogLineN(logKadSearch, wxString::Format ( wxT ( "#%" PRIu32 ":Search result type: Unknown (%" PRIu32 ")" ), m_searchID, m_type ) );
                break;
        }
}

void CSearch::ProcessSearchResultList ( const CMemFile & bio, const CI2PAddress & fromIP )
{
        m_lastActivity = time(NULL);

        CSearchContact contact = getChargedContact(fromIP);
        if (contact.IsInvalid()) {
                AddDebugLogLineC(logKadSearch, wxT("Node ") + fromIP.humanReadable() + wxT(" sent search results without being asked"));
                return;
        }
        CSearchContact & fromContact = m_charged_contacts.ref(contact);
        uint16_t count = bio.ReadUInt16();
        fromContact.add_result(count);
        AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: processSearchResultList (%d results -> %d from %s)" ) )
                         % m_searchID % count % fromContact.get_results_count() % fromContact.GetClientIDString());
#ifndef AMULE_DAEMON
        std::unique_ptr<wxWindowUpdateLocker> lockwindow;
        if (theApp->amuledlg->m_searchwnd->GetSearchList(GetSearchID())) 
                lockwindow.reset(new wxWindowUpdateLocker(theApp->amuledlg->m_searchwnd->GetSearchList(GetSearchID())));
#endif
        while ( count > 0 ) {
                // What is the answer
                CUInt128 answer = bio.ReadUInt128();

                // Get info about answer

                try {
                        AddDebugLogLineN(logClientKadUDP,
                                         CFormat ( wxT ( "processSearchResultList : %d response(s) remaining (rem. filesize=%llu)" ) )
                                         % count %  bio.GetAvailable() );
                        TagList tags ( bio, true );
                        ProcessSearchResult (answer, tags);
                        count--;
                } catch ( ... ) {
                        AddDebugLogLineN(logKadSearch, wxT ( "CSearch::processSearchResultList : error while trying to add a result" ) );
                        throw;
                }
        }

        m_completedPeers ++ ;
	addToCompletedPeers(fromContact);
}


void CSearch::ProcessSearchResult ( const CUInt128 &answer, TagList & info )
{
        wxString type = wxT("Unknown");
        switch (m_type) {
        case FILE:
                type = wxT("File");
                ProcessResultFile(answer, info);
                break;
        case KEYWORD:
                type = wxT("Keyword");
                ProcessResultKeyword(answer, info);
                break;
        case NOTES:
                type = wxT("Notes");
                ProcessResultNotes(answer, info);
                break;
        }
        AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: type=%s processSearchResult : answer = %s" ) ) % m_searchID % type  % answer.ToHexString() );
}




void CSearch::ProcessResultFile ( const CUInt128 &answer, TagList & info )
{
        uint8_t type = 0;
        CI2PAddress tcpdest;
        CI2PAddress udpdest;
        CI2PAddress serverdest;
        uint8	    byCryptOptions = 0; // 0 = not supported

        for ( TagList::const_iterator it = info.begin(); it != info.end(); ++it ) {
                const CTag & tag = *it;

                if ( tag . GetID() == TAG_SOURCETYPE ) {
                        type = (uint8_t) tag . GetInt();
                } else if ( tag . GetID() == TAG_SOURCEDEST ) {
                        tcpdest = tag . GetAddr();
                } else if ( tag . GetID() == TAG_SOURCEUDEST ) {
                        udpdest = tag . GetAddr();
                } else if ( tag . GetID() == TAG_SERVERDEST ) {
                        serverdest = tag . GetAddr();
                } else if ( tag . GetID() == TAG_ENCRYPTION ) {
                        byCryptOptions = tag . GetInt();
                }
        }

        AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "#%i: source type : %d" ) ) % m_searchID % type );

        // Process source based on its type. Currently only one method is needed to process all types.
        switch( type ) {
        case 1:
        case 3: {
                if (theApp->downloadqueue->KademliaSearchFile ( m_searchID, &answer, type, tcpdest, udpdest, serverdest, byCryptOptions ))
			m_searchResults++;
                break;
        }

        case 2:
                //Don't use this type, some clients will process it wrong..
        default:
                break;
        }
}

void CSearch::ProcessResultNotes ( const CUInt128 &answer, TagList & info )
{
        // Process a received Note to a file.
        // Create a Note and set the IDs.
        CEntry entry;
        entry.m_uKeyID.SetValue ( m_target );
        entry.m_uSourceID.SetValue ( answer );

        bool bFilterComment = false;

        // Loop through tags and pull wanted into. Currently we only keep Filename, Rating, Comment.
        for ( TagList::const_iterator it = info.begin(); it != info.end(); it++ ) {
                const CTag & tag = *it;

                switch ( tag.GetID() ) {
                case TAG_SOURCEDEST :
                        entry.m_tcpdest = tag . GetAddr();
                        break ;
                case TAG_FILENAME :
                        entry.SetFileName (tag . GetStr());
                        break ;
                case TAG_DESCRIPTION :
                        //wxString strComment ( tag . GetStr() );
                        bFilterComment = thePrefs::IsMessageFiltered ( tag . GetStr() );
                        entry.GetTagList().push_front ( tag );
                        break ;
                case TAG_FILERATING :
                        entry.GetTagList().push_front ( tag );
                        break ;
                default :
                        break ;
                }
        }

        if (bFilterComment) {
                return;
        }

        uint8_t fileid[16];
        m_target.ToByteArray(fileid);
        const CMD4Hash fileHash(fileid);

        //Check if this hash is in our shared files..
        CKnownFile* file = theApp->sharedfiles->GetFileByID(fileHash);

        if (!file) {
                // If we didn't find anything check if it's in our download queue.
                file = (CKnownFile*)theApp->downloadqueue->GetFileByID(fileHash);
        }

        // If we found a file try to add the note to the file.
        if (file) {
                file->AddNote ( entry ) ;
                m_searchResults++;
        } else {
                AddDebugLogLineN(logKadSearch, wxT("Comment received for unknown file"));
        }
}

void CSearch::ProcessResultKeyword ( const CUInt128 &answer, TagList & info )
{
        // Process a keyword that we received.
        // Set of data we can use for a keyword result.
        wxString name;
        uint64_t size = 0;
        wxString type;
        uint32_t publishInfo = 0;
        // Flag that is set if we want this keyword
        bool bFileName = false;
        bool bFileSize = false;

        TagList taglist;

        for ( TagList::const_iterator it = info.begin(); it != info.end(); ++it ) {
                const CTag& tag = *it;

                switch ( tag.GetID() ) {
                case TAG_FILENAME :
                        name = tag . GetStr();
                        bFileName = !name.IsEmpty();
                        break ;

                case TAG_FILESIZE :
                        size = tag . GetInt();
                        bFileSize = true;
                        break ;

                case  TAG_FILETYPE :
                        type = tag . GetStr();
                        break ;

                case TAG_PUBLISHINFO : {
                        // we don't keep this as tag, but as a member property of the searchfile, as we only need its informations
                        // in the search list and don't want to carry the tag over when downloading the file (and maybe even wrongly publishing it)
                        publishInfo = tag.GetInt();
#ifdef __DEBUG__
                        uint32_t differentNames = (publishInfo & 0xFF000000) >> 24;
                        uint32_t publishersKnown = (publishInfo & 0x00FF0000) >> 16;
                        uint32_t trustValue = publishInfo & 0x0000FFFF;
                        AddDebugLogLineN(logKadSearch, CFormat(wxT("Received PublishInfo Tag: %u different names, %u publishers, %.2f trustvalue")) % differentNames % publishersKnown % ((double)trustValue/ 100.0));
#endif
                }
                break;
                default :
                        taglist. push_back ( tag );
                }
        }

        // If we don't have a valid filename and filesize, drop this keyword.
        if (!bFileName || !bFileSize) {
                AddDebugLogLineN(logKadSearch, wxString(wxT("No ")) + (!bFileName ? wxT("filename") : wxT("filesize")) + wxT(" on search result, ignoring"));
                return;
        }

        // the file name of the current search response is stored in "name"
        // the list of words the user entered is stored in "m_words"
        // so the file name now gets parsed for all the words entered by the user (even repetitive ones):

        // Step 1: Get the words of the response file name
        WordList listFileNameWords;
        CSearchManager::GetWords(name, &listFileNameWords, true);

        // Step 2: Look for each entered search word in those present in the filename
        bool bFileNameMatchesSearch = true;  // this will be set to "false", if not all search words are found in the file name

        for (WordList::const_iterator itSearchWords = m_words.begin(); itSearchWords != m_words.end(); ++itSearchWords) {
                bool bSearchWordPresent = false;
                for (WordList::iterator itFileNameWords = listFileNameWords.begin(); itFileNameWords != listFileNameWords.end(); ++itFileNameWords) {
                        if (!itFileNameWords->CmpNoCase(*itSearchWords)) {
                                listFileNameWords.erase(itFileNameWords);  // remove not to find same word twice
                                bSearchWordPresent = true;
                                break;  // found word, go on using the next searched word
                        }
                }
                if (!bSearchWordPresent) {
                        bFileNameMatchesSearch = false;  // not all search words were found in the file name
                        break;
                }
        }

        // Step 3: Accept result only if all(!) words are found
        if (bFileNameMatchesSearch) {
                if (theApp->searchlist->KademliaSearchKeyword ( m_searchID, &answer, name, (uint64_t) size, type, publishInfo, taglist ))
			m_searchResults++;
        }
}

//void CSearch::SendFindValue(CContact *contact, bool reaskMore)
void CSearch::SendFindPeersForTarget ( CContact contact, bool reaskMore )
{
        // Found a node that we think has contacts closer to our target.
        try {
                if (m_stopping) {
                        return;
                }

                CMemFile packetdata( 1/*number of contacts requested*/ + 16 /*m_target*/ + 16 /*dest ID (check)*/ + 16 /*my ID (proposed for booking)*/);
                // The number of returned contacts is based on the type of search.
                uint8_t contactCount = GetRequestContactCount();

                if (reaskMore) {
                        if (m_requestedMoreNodesContact.IsInvalid()) {
                                m_requestedMoreNodesContact = contact;
                                wxASSERT(contactCount == KADEMLIA_FIND_VALUE);
                                contactCount = KADEMLIA_FIND_VALUE_MORE;
                        } else {
                                wxFAIL;
                        }
                }

                if (contactCount > 0) {
                        packetdata.WriteUInt8(contactCount);
                } else {
                        return;
                }
                // Put the target we want into the packet.
                packetdata.WriteUInt128 ( m_target );
                // Add the ID of the contact we're contacting for sanity checks on the other end.
                packetdata.WriteUInt128(contact.GetClientID());
                // Add my ID so that the contact can ask for my details if he is interested.
                packetdata.WriteUInt128(CContact::Self().GetClientID());

                // send the request, but first signal the routing zone that the contact will be sent something
                // so that the type of contact will increase if it does not respond
                //         contact.CheckingType() ;

                if (contact.GetVersion() >= 2) {
                        if (contact.GetVersion() >= 6) {
                                CUInt128 clientID = contact.GetClientID();
                                CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_REQ, contact.GetUDPDest(), contact.GetUDPKey(), &clientID);
                        } else {
                                CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_REQ, contact.GetUDPDest(), 0, NULL);
                                wxASSERT(contact.GetUDPKey() == CKadUDPKey(0));
                        }
#ifdef __DEBUG__
                        switch (m_type) {
                        case NODE:
                                DebugSendF(wxT("Kad2Req(Node)"), contact.GetUDPDest());
                                break;
                        case NODECOMPLETE:
                                DebugSendF(wxT("Kad2Req(NodeComplete)"), contact.GetUDPDest());
                                break;
                        case NODESPECIAL:
                                DebugSendF(wxT("Kad2Req(NodeSpecial)"), contact.GetUDPDest());
                                break;
                        case FILE:
                                DebugSendF(wxT("Kad2Req(File)"), contact.GetUDPDest());
                                break;
                        case KEYWORD:
                                DebugSendF(wxT("Kad2Req(Keyword)"), contact.GetUDPDest());
                                break;
                        case STOREFILE:
                                DebugSendF(wxT("Kad2Req(StoreFile)"), contact.GetUDPDest());
                                break;
                        case STOREKEYWORD:
                                DebugSendF(wxT("Kad2Req(StoreKeyword)"), contact.GetUDPDest());
                                break;
                        case STORENOTES:
                                DebugSendF(wxT("Kad2Req(StoreNotes)"), contact.GetUDPDest());
                                break;
                        case NOTES:
                                DebugSendF(wxT("Kad2Req(Notes)"), contact.GetUDPDest());
                                break;
                        default:
                                DebugSend(Kad2Req, contact.GetUDPDest());
                                break;
                        }
#endif
                } else {
                        CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA_REQ_DEPRECATED, contact.GetUDPDest(), 0, NULL);
#ifdef __DEBUG__
                        switch (m_type) {
                        case NODE:
                                DebugSendF(wxT("KadReq(Node)"), contact.GetUDPDest());
                                break;
                        case NODECOMPLETE:
                                DebugSendF(wxT("KadReq(NodeComplete)"), contact.GetUDPDest());
                                break;
                        case NODESPECIAL:
                                DebugSendF(wxT("KadReq(NodeSpecial)"), contact.GetUDPDest());
                                break;
                        case FILE:
                                DebugSendF(wxT("KadReq(File)"), contact.GetUDPDest());
                                break;
                        case KEYWORD:
                                DebugSendF(wxT("KadReq(Keyword)"), contact.GetUDPDest());
                                break;
                        case STOREFILE:
                                DebugSendF(wxT("KadReq(StoreFile)"), contact.GetUDPDest());
                                break;
                        case STOREKEYWORD:
                                DebugSendF(wxT("KadReq(StoreKeyword)"), contact.GetUDPDest());
                                break;
                        case STORENOTES:
                                DebugSendF(wxT("KadReq(StoreNotes)"), contact.GetUDPDest());
                                break;
                        case NOTES:
                                DebugSendF(wxT("KadReq(Notes)"), contact.GetUDPDest());
                                break;
                        default:
                                DebugSend(KadReq, contact.GetUDPDest());
                                break;
                        }
#endif
                }
        } catch (const CEOFException& err) {
                AddDebugLogLineC(logKadSearch, wxT("CEOFException in CSearch::SendFindPeersForTarget: ") + err.what());
        } catch (const CInvalidPacket& err) {
                AddDebugLogLineC(logKadSearch, wxT("CInvalidPacket Exception in CSearch::SendFindPeersForTarget: ") + err.what());
        } catch (const wxString& e) {
                AddDebugLogLineC(logKadSearch, wxT("Exception in CSearch::SendFindPeersForTarget: ") + e);
        } catch ( const CSafeIOException & ioe ) {
                AddDebugLogLineN(logKadSearch, CFormat ( wxT ( "Exception in CSearch::SendFindPeersForTarget (IO error(%s))" ) ) % ioe.what() );
        }
}

void CSearch::ProcessPublishResult ( const CMemFile & bio, const CI2PAddress & fromIP )
{
        m_lastActivity = time(NULL);

	AddDebugLogLineN(logKadPublish, CFormat ( wxT ( "#%i: processPublishResult" ) )
                         % m_searchID
                        );
        CSearchContact fromContact = getChargedContact(fromIP);
        if (fromContact.IsInvalid()) {
                AddDebugLogLineC(logKadSearch, wxT("Node ") + fromIP.humanReadable() + wxT(" sent publish result without being asked"));
                return;
        }
        // Verify packet is expected size
        uint8_t minExpectedSize = 0 ;

        if ( m_type == STOREFILE ) minExpectedSize = 8 ;

        if ( bio.GetAvailable() < minExpectedSize ) {
                throw ( wxString ) ( CFormat ( wxT ( "***NOTE: Received wrong size (%llu, at least %u expected) packet in %s" ) ) %
                                     bio.GetAvailable() % minExpectedSize % wxString::FromAscii ( __func__ ) );
        }


        if ( m_type == STOREFILE ) {
                // update source counts in file entry
                uint32_t total = bio.ReadUInt32();
                uint32_t complete = bio.ReadUInt32();

                uint8_t fileid[16];
                m_target.ToByteArray ( fileid );
                CKnownFile * file =	theApp->sharedfiles->GetFileByID ( CMD4Hash ( fileid ) ) ;

                if ( file != NULL ) {
                        file->m_nCompleteSourcesCount = std::max((uint32) file->m_nCompleteSourcesCount, complete);
                        file->m_nSourcesCount = std::max((uint32_t) file->m_nSourcesCount, total);
			AddDebugLogLineN(logKadPublish, CFormat ( wxT ( "#%i: processPublishResult: known file %s published (sources ~ %u/%u)" ) )
					% m_searchID
					% file->GetFileName() % complete % total
                        );                }
        }
        if ( bio.GetLength() > bio.GetPosition() ) {
                uint8_t load = bio.ReadUInt8();
                UpdateNodeLoad ( load );
        }

        m_searchResults  ++ ;
        m_completedPeers ++ ;
	addToCompletedPeers(fromContact);
}

void CSearch::PreparePacketForTags(CMemFile *bio, CKnownFile *file)
/*! Prepares a CSearch for keyword publishing.
 * Puts infos (taglist) in a packet :
 * - file name
 * - file size
 * - number of complete sources
 * - total number of sources
 * - OPT: file type
 * - OPT: file format
 * - OPT: TAG_MEDIA_ARTIST
 * - OPT: TAG_MEDIA_ALBUM
 * - OPT: TAG_MEDIA_TITLE
 * - OPT: TAG_MEDIA_LENGTH
 * - OPT: TAG_MEDIA_BITRATE
 * - OPT: TAG_MEDIA_CODEC
 */
{
        // We're going to publish a keyword, set up the tag list
        TagList taglist;

        try {
                if (file && bio) {
                        // Name, Size
                        taglist.push_back ( CTag ( TAG_FILENAME, file->GetFileName().GetPrintable() ) );
                        taglist.push_back ( CTag ( TAG_FILESIZE, file->GetFileSize() ) );
                        taglist.push_back ( CTag ( TAG_COMPLETE_SOURCES, file->m_nCompleteSourcesCount ) );
                        taglist.push_back ( CTag ( TAG_SOURCES, file->m_nSourcesCount ) );

                        AddDebugLogLineN(logKadPublish, CFormat ( wxT ( "PreparePacketForTags. Filename : %s, Printable : %s, TAG_SOURCES : %d/%d" ) )
                                         % file->GetFileName().GetRaw() % file->GetFileName().GetPrintable() % file->m_nCompleteSourcesCount % file->m_nSourcesCount );

                        // eD2K file type (Audio, Video, ...)
                        // NOTE: Archives and CD-Images are published with file type "Pro"
                        wxString strED2KFileType(GetED2KFileTypeSearchTerm(GetED2KFileTypeID(file->GetFileName())));
                        if (!strED2KFileType.IsEmpty()) {
                                taglist.push_back ( CTag ( TAG_FILETYPE, strED2KFileType ) );
                        }


                        // additional meta data (Artist, Album, Codec, Length, ...)
                        // only send verified meta data to nodes
                        if (file->GetMetaDataVer() > 0) {
                                static const struct {
                                        uint8_t	nName;
                                        uint8_t	nType;
                                } _aMetaTags[] = {
                                        { TAG_MEDIA_ARTIST,  2 },
                                        { TAG_MEDIA_ALBUM,   2 },
                                        { TAG_MEDIA_TITLE,   2 },
                                        { TAG_MEDIA_LENGTH,  3 },
                                        { TAG_MEDIA_BITRATE, 3 },
                                        { TAG_MEDIA_CODEC,   2 }
                                };
                                for ( size_t i = 0; i < itemsof ( _aMetaTags ); i++ ) {
                                        const ::CTag& pTag = file->GetTag ( _aMetaTags[i].nName, _aMetaTags[i].nType );

                                        if ( pTag.isValid() ) {
                                                // skip string tags with empty string values
                                                if ( pTag . IsStr() && pTag . GetStr().IsEmpty() ) {
                                                        continue;
                                                }
                                                // skip integer tags with '0' values
                                                if ( pTag . IsInt() && pTag . GetInt() == 0 ) {
                                                        continue;
                                                }
                                                taglist.push_back ( pTag ) ;
                                        }
                                }
                        }
                        taglist.WriteTo ( bio );

                        taglist.deleteTags();
                } else if ( bio ) {
                        // empty tag list
                        taglist.WriteTo ( bio );
                } else {
                        //If we get here.. Bad things happen.. Will fix this later if it is a real issue.
                        wxASSERT ( 0 );
                }
        } catch (const CEOFException& err) {
                AddDebugLogLineC(logKadSearch, wxT("CEOFException in CSearch::PreparePacketForTags: ") + err.what());
        } catch (const CInvalidPacket& err) {
                AddDebugLogLineC(logKadSearch, wxT("CInvalidPacket Exception in CSearch::PreparePacketForTags: ") + err.what());
        } catch (const wxString& e) {
                AddDebugLogLineC(logKadSearch, wxT("Exception in CSearch::PreparePacketForTags: ") + e);
        }
}


void CSearch::SetSearchTermData( CMemFile * searchTermsData)
{
        m_searchTerms.Reset();
        m_searchTerms.Write( searchTermsData->GetRawBuffer(), searchTermsData->GetLength() ) ;
}


time_t CSearch::GetSearchLifetime() const throw()

{
        switch ( GetSearchType() ) {
        case FILE               : return SEARCHFILE_LIFETIME ;
        case KEYWORD            : return SEARCHKEYWORD_LIFETIME ;
        case NOTES              : return SEARCHNOTES_LIFETIME ;
        case NODE               : return SEARCHNODE_LIFETIME ;
        case NODECOMPLETE       : return SEARCHNODECOMP_LIFETIME ;
        case STOREFILE          : return SEARCHSTOREFILE_LIFETIME ;
        case STOREKEYWORD       : return SEARCHSTOREKEYWORD_LIFETIME ;
        case STORENOTES	        : return SEARCHSTORENOTES_LIFETIME ;
        default                 : return SEARCH_LIFETIME ;
        }
}

uint8_t CSearch::GetRequestContactCount() const throw()
{
        // Returns the amount of contacts we request on routing queries based on the search type
        switch (m_type) {
        case NODE:
        case NODECOMPLETE:
        case NODESPECIAL:
                return KADEMLIA_FIND_NODE;
        case FILE:
        case KEYWORD:
        case NOTES:
                return KADEMLIA_FIND_VALUE;
        case STOREFILE:
        case STOREKEYWORD:
        case STORENOTES:
                return KADEMLIA_STORE;
        default:
                AddDebugLogLineN(logKadSearch, wxT("Invalid search type. (CSearch::GetRequestContactCount())"));
                wxFAIL;
                return 0;
        }
}

uint32_t CSearch::GetSearchMaxAnswers() const throw()
{
        switch ( GetSearchType() ) {
        case FILE                           : return SEARCHFILE_TOTAL ;
        case KEYWORD                        : return SEARCHKEYWORD_TOTAL ;
        case NOTES                          : return SEARCHNOTES_TOTAL ;
        case NODE                           : return 1 ;
        case NODECOMPLETE                   : return SEARCHNODECOMP_TOTAL ;
        case STOREFILE                      : return SEARCHSTOREFILE_TOTAL ;
        case STOREKEYWORD                   : return SEARCHSTOREKEYWORD_TOTAL ;
        case STORENOTES	                    : return SEARCHSTORENOTES_TOTAL ;
        default                             : return 1 ;
        }
}

uint8_t CSearch::GetCompletedListMaxSize() const throw()
{
	switch ( GetSearchType() ) {
	case NODE                           : return 1;
	case NODECOMPLETE                   : return SEARCHNODECOMP_TOTAL;
	case FILE                           :
	case STOREFILE                      : return SEARCHSTOREFILE_TOTAL ;
	case KEYWORD                        : 
	case STOREKEYWORD                   : return SEARCHSTOREKEYWORD_TOTAL ;
	case NOTES                          :
	case STORENOTES                     : return SEARCHSTORENOTES_TOTAL ;
	default                             : return 1;
	}
}


wxString CSearch::typestr()
{
        switch(m_type) {
        case NODE: return wxString(wxT("NODE"));
        case NODECOMPLETE: return wxString(wxT("NODECOMPLETE"));
        case NODESPECIAL: return wxString(wxT("NODESPECIAL"));
        case FILE: return wxString(wxT("FILE"));
        case KEYWORD: return wxString(wxT("KEYWORD"));
        case NOTES: return wxString(wxT("NOTES"));
        case STOREFILE: return wxString(wxT("STOREFILE"));
        case STOREKEYWORD: return wxString(wxT("STOREKEYWORD"));
        case STORENOTES: return wxString(wxT("STORENOTES"));
        default: return wxString(wxT("syntax error"));
        }
}



void CSearch::chargeclosest ( TargetContactMap& _map, function< void(CSearchContact&) >f )
{
        CSearchContact c = _map.popClosest();
        m_charged_contacts.add(c);
        f(c);
        m_lastActivity = time(NULL);
}

CSearchContact CSearch::getChargedContact(const CI2PAddress& dest)
{
        CSearchContact contact = m_charged_contacts.get_if([&dest](const CSearchContact&c){return c.GetUDPDest()==dest;});
        return contact;
}

void CSearch::unchargeContactTo(CSearchContact contact, ContactMap * _to)
{
        wxASSERT(m_charged_contacts.contains(contact));
        m_charged_contacts.moveContactTo(contact, _to);
}

void CSearch::addToCompletedPeers(CSearchContact contact)
{
        if (m_type & (KEYWORD | FILE | NOTES)) {
                if (contact.get_results_count()>=GetSearchMaxAnswers())
                        unchargeContactTo(contact, &m_best_complete);
                else
                        m_best_complete.add(contact);
        } else
                unchargeContactTo(contact, &m_best_complete);
        
        if (m_best_complete.size() > GetCompletedListMaxSize()) {
                m_best_complete.moveFurthestTo(&m_trash);
        }
}

void CSearch::killMaliciousContact ( CSearchContact contact )
{
        unchargeContactTo(contact, &m_trash);
}

