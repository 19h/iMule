//
// This file is part of imule Project
//
// Copyright (c) 2004-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2006 imule Project ( http://www.imule-project.net )
// Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

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

#include <wx/wx.h>

#include "KademliaUDPListener.h"

#include <protocol/Protocols.h>
#include <protocol/kad/Constants.h>
#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad2/Constants.h>
#include <protocol/kad2/Client2Client/UDP.h>
#include <protocol/ed2k/Client2Client/TCP.h> // OP_CALLBACK is sent in some cases.
#include <common/Macros.h>
#include <common/Format.h>
#include <tags/FileTags.h>

#include "../routing/Contact.h"
#include "../routing/RoutingZone.h"
#include "../kademlia/Indexed.h"
#include <tags/FileTags.h>
#include "../kademlia/Defines.h"
#include "../kademlia/UDPFirewallTester.h"
#include "../utils/KadUDPKey.h"
#include "../utils/KadClientSearcher.h"
#include "../../amule.h"
#include "../../ClientUDPSocket.h"
#include "../../Packet.h"
#include "../../ClientList.h"
#include "../../Statistics.h"
#include "../../MemFile.h"
#include "../../updownclient.h"
#include "../../ClientTCPSocket.h"
#include "../../Logger.h"
#include <common/Format.h>
#include "../../Preferences.h"
#include "../../IPFilter.h"
#include "../../RandomFunctions.h"		// Needed for GetRandomUint128()
#include "kademlia/kademlia/Search.h"

#include <wx/tokenzr.h>
#include <memory>

#define THIS_DEBUG_IS_JUST_FOR_KRY_DONT_TOUCH_IT_KTHX 0

#if defined(__SUNPRO_CC)
#define __FUNCTION__ __FILE__+__LINE__
#endif

#define CHECK_PACKET_SIZE(OP, OPS, SIZE) \
                 if (packet.GetAvailable() OP (uint32_t)(SIZE)) \
                 throw wxString::Format(wxT("***NOTE: Received wrong size (%" PRIu64 "" OPS "%" PRIu64 ") packet in "), packet.GetAvailable(), SIZE) + wxString::FromAscii(__FUNCTION__)

#define CHECK_PACKET_MIN_SIZE(SIZE) CHECK_PACKET_SIZE(<, "<", SIZE)
#define CHECK_PACKET_EXACT_SIZE(SIZE)   CHECK_PACKET_SIZE(!=, "!=", SIZE)

#define CHECK_TRACKED_PACKET(OPCODE) \
                 if (!IsOnOutTrackList(dest, OPCODE)) \
                 throw wxString::Format(wxT("***NOTE: Received unrequested response packet, size (%" PRIu64 ") in "), packet.GetAvailable()) + wxString::FromAscii(__FUNCTION__)


const unsigned int & detailslen()
{
        static const unsigned int l = 16/*ID*/ + 2 * CI2PAddress::validLength() /*TCP+UDP*/ + 1/*type de contact*/;

        return l;
}

const unsigned int & kad2detailslen()
{
        static const unsigned int l = 16/*ID*/ + 1 * CI2PAddress::validLength() /*UDP*/ + 1/*kad version*/;

        return l;
}

const uint32_t & maxMessageSize()
{
        static const uint32_t l = CPacket::UDPPacketMaxDataSize() ;
        return l;
}

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CKademliaUDPListener::~CKademliaUDPListener()
{
        // report timeout to all pending FetchNodeIDRequests
        for (FetchNodeIDList::iterator it = m_fetchNodeIDRequests.begin(); it != m_fetchNodeIDRequests.end(); ++it) {
                it->requester->KadSearchNodeIDByIPResult(KCSR_TIMEOUT, NULL);
        }
}

void CKademliaUDPListener::Bootstrap ( const CI2PAddress & dest, bool kad2, uint8_t kadVersion, const CUInt128* cryptTargetID )
{
        wxASSERT ( dest.isValid() );

        if (kad2) {
                DebugSend(Kad2BootstrapReq, dest);
                CMemFile bio(0);
                if (kadVersion >= 6) {
                        SendPacket(bio, KADEMLIA2_BOOTSTRAP_REQ, dest, 0, cryptTargetID);
                } else {
                        SendPacket(bio, KADEMLIA2_BOOTSTRAP_REQ, dest, 0, NULL);
                }
        } else {
                DebugSend(KadBootstrapReq, dest);
                SendMyDetails ( KADEMLIA_BOOTSTRAP_REQ_DEPRECATED, dest, 0, 0, NULL, false );
        }
}

// Used by Kad1.0 and Kad2.0
void CKademliaUDPListener::SendMyDetails ( uint8_t opcode, const CI2PAddress & dest, uint8_t kadVersion,
                const CKadUDPKey& targetKey, const CUInt128* cryptTargetID, bool requestAckPacket )
{
        AddDebugLogLineN(logClientKadUDP, CFormat ( wxT ( "CKademliaUDPListener::sendMyDetails(%d, %s)" ) )
                         % opcode % dest.humanReadable() );

        CMemFile packetdata;
        if (kadVersion > 0) {
                CContact::Self().WriteToKad2Contact(packetdata);
                TagList taglist ;
#ifdef We_skip_aMule_tags_about_firewalls
                if (!CKademlia::GetPrefs()->GetUseExternKadPort()) {
                        tagCount++;
                }
                if (kadVersion >= 8 && (requestAckPacket || CKademlia::GetPrefs()->GetFirewalled() || CUDPFirewallTester::IsFirewalledUDP(true))) {
                        tagCount++;
                }
                packetdata.WriteUInt8(tagCount);
                if (!CKademlia::GetPrefs()->GetUseExternKadPort()) {
                        packetdata.WriteTag(CTagVarInt(TAG_SOURCEUPORT, CKademlia::GetPrefs()->GetInternKadPort()));
                }
#endif
                if (kadVersion >= 8 && requestAckPacket) {
                        // if we're firewalled we send this tag, so the other client doesn't add us to his routing table (if UDP firewalled) and for statistics reasons (TCP firewalled)
                        // 5 - reserved (!)
                        // 1 - requesting HELLO_RES_ACK
                        // 1 - TCP firewalled
                        // 1 - UDP firewalled
                        taglist.push_back( CTag(TAG_KADMISCOPTIONS, (uint8_t) ( (requestAckPacket ? 1 : 0) << 2 )));
                }
                // packetdata.WriteTag(CKadTagUInt(TAG_USER_COUNT, CKademlia::GetPrefs()->GetKademliaUsers()));
                // packetdata.WriteTag(CKadTagUInt(TAG_FILE_COUNT, CKademlia::GetPrefs()->GetKademliaFiles()));
                taglist.WriteTo(&packetdata);
                if (kadVersion >= 6) {
                        if (cryptTargetID == NULL || *cryptTargetID == 0) {
                                AddDebugLogLineN(logClientKadUDP, wxT("Sending hello response to crypt enabled Kad Node which provided "
                                                                      "an empty NodeID: ") + dest.humanReadable() + wxString::Format(wxT(" (%" PRIu8 ")"), kadVersion));
                                SendPacket(packetdata, opcode, dest, targetKey, NULL);
                        } else {
                                SendPacket(packetdata, opcode, dest, targetKey, cryptTargetID);
                        }
                } else {
                        SendPacket(packetdata, opcode, dest, 0, NULL);
                        wxASSERT(targetKey.IsEmpty());
                }
        } else {
                wxASSERT(!requestAckPacket);
                CContact::Self().WriteToKad1Contact(packetdata);
                SendPacket(packetdata, opcode, dest, 0, NULL);
        }
}

void CKademliaUDPListener::SendNullPacket ( uint8_t opcode, const CI2PAddress & dest, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID )

{
        CMemFile packetdata ( 0 );
        AddDebugLogLineN(logClientKadUDP, CFormat ( wxT ( "KadNullPacket sent to %x" ) ) % dest.hashCode() );
        SendPacket ( packetdata, opcode, dest, targetKey, cryptTargetID );
}

void CKademliaUDPListener::SendPublishSourcePacket(const CContact& contact, const CUInt128 &targetID, const CUInt128 &contactID, const TagList& tags)
{
        uint8_t opcode;
        CMemFile packetdata;
        packetdata.WriteUInt128(targetID);
        if (contact.GetVersion() >= 4/*47c*/) {
                opcode = KADEMLIA2_PUBLISH_SOURCE_REQ;
                packetdata.WriteUInt128(contactID);
                tags.WriteTo(&packetdata);
                DebugSend(Kad2PublishSrcReq, contact.GetUDPDest());
        } else {
                opcode = KADEMLIA_PUBLISH_REQ;
                //We only use this for publishing sources now.. So we always send one here..
                packetdata.WriteUInt16(1);
                packetdata.WriteUInt128(contactID);
                tags.WriteTo(&packetdata);
                DebugSend(KadPublishReq, contact.GetUDPDest());
        }
        if (contact.GetVersion() >= 6) {    // obfuscated ?
                CUInt128 clientID = contact.GetClientID();
                SendPacket(packetdata, opcode, contact.GetUDPDest(), contact.GetUDPKey(), &clientID);
        } else {
                SendPacket(packetdata, opcode, contact.GetUDPDest(), 0, NULL);
        }
}

void CKademliaUDPListener::ProcessPacket ( const uint8_t* data, uint32_t lenData, const CI2PAddress & from, bool validReceiverKey, const CKadUDPKey& senderKey )
{
#ifdef __PACKET_RECV_DUMP__
//	printf("processPacket, taille %d :\n", lenPacket);
//	DumpMem(packetData, 100);
#endif
        //Update connection state only when it changes.
        bool curCon = CKademlia::GetPrefs()->HasHadContact();
        CKademlia::GetPrefs()->SetLastContact();

        if ( curCon != CKademlia::GetPrefs()->HasHadContact() ) {
                theApp->ShowConnectionState();
        }

        if (CRoutingZone::GetTotalContactsNumber()==0) {
                DebugSend(KadHelloReq, from);
                CKademlia::GetUDPListener()->SendMyDetails(KADEMLIA_HELLO_REQ_DEPRECATED, from, 0, 0, NULL, false);
        }
        CMemFile bio ( data, lenData );
        bio.ReadUInt8();
        uint8_t opcode = bio.ReadUInt8();//data[1];

        if (!InTrackListIsAllowedPacket(from, opcode, validReceiverKey)) {
                return;
        }

        switch ( opcode ) {
        case KADEMLIA_BOOTSTRAP_REQ_DEPRECATED:
                DebugRecv(KadBootstrapReq, from);
                ProcessBootstrapRequest ( bio, from );
                break;
        case KADEMLIA2_BOOTSTRAP_REQ:
                DebugRecv(Kad2BootstrapReq, from);
                Process2BootstrapRequest(from, senderKey);
                break;
        case KADEMLIA_BOOTSTRAP_RES_DEPRECATED:
                DebugRecv(KadBootstrapRes, from);
                ProcessBootstrapResponse ( bio, from );
                break;
        case KADEMLIA2_BOOTSTRAP_RES:
                DebugRecv(Kad2BootstrapRes, from);
                Process2BootstrapResponse(bio, from, senderKey, validReceiverKey);
                break;
        case KADEMLIA_HELLO_REQ_DEPRECATED:
                DebugRecv(KadHelloReq, from);
                ProcessHelloRequest ( bio, from );
                break;
        case KADEMLIA2_HELLO_REQ:
                DebugRecv(Kad2HelloReq, from);
                Process2HelloRequest(bio, from, senderKey, validReceiverKey);
                break;
        case KADEMLIA_HELLO_RES_DEPRECATED:
                DebugRecv(KadHelloRes, from);
                ProcessHelloResponse(bio, from);
                break;
        case KADEMLIA2_HELLO_RES:
                DebugRecv(Kad2HelloRes, from);
                Process2HelloResponse(bio, from, senderKey, validReceiverKey);
                break;
        case KADEMLIA2_HELLO_RES_ACK:
                DebugRecv(Kad2HelloResAck, from);
                Process2HelloResponseAck(bio, from, validReceiverKey);
                break;
        case KADEMLIA_REQ_DEPRECATED:
                DebugRecv(KadReq, from);
                ProcessKademliaRequest(bio, from);
                break;
        case KADEMLIA2_REQ:
                DebugRecv(Kad2Req, from);
                ProcessKademlia2Request(bio, from, senderKey);
                break;
        case KADEMLIA_RES_DEPRECATED:
                DebugRecv(KadRes, from);
                ProcessKademliaResponse(bio, from);
                break;
        case KADEMLIA2_RES:
                DebugRecv(Kad2Res, from);
                ProcessKademlia2Response(bio, from, senderKey);
                break;
        case KADEMLIA_SEARCH_REQ:
                DebugRecv(KadSearchReq, from);
                ProcessSearchRequest(bio, from);
                break;
        case KADEMLIA_SEARCH_NOTES_REQ:
                DebugRecv(KadSearchNotesReq, from);
                ProcessSearchNotesRequest(bio, from);
                break;
        case KADEMLIA2_SEARCH_NOTES_REQ:
                DebugRecv(Kad2SearchNotesReq, from);
                Process2SearchNotesRequest(bio, from, senderKey);
                break;
        case KADEMLIA2_SEARCH_KEY_REQ:
                DebugRecv(Kad2SearchKeyReq, from);
                Process2SearchKeyRequest(bio, from, senderKey);
                break;
        case KADEMLIA2_SEARCH_SOURCE_REQ:
                DebugRecv(Kad2SearchSourceReq, from);
                Process2SearchSourceRequest(bio, from, senderKey);
                break;
        case KADEMLIA_SEARCH_RES:
                DebugRecv(KadSearchRes, from);
                ProcessSearchResponse(bio, from);
                break;
        case KADEMLIA2_SEARCH_RES:
                DebugRecv(Kad2SearchRes, from);
                Process2SearchResponse(bio, from, senderKey);
                break;
        case KADEMLIA_PUBLISH_REQ:
                DebugRecv(KadPublishReq, from);
                ProcessPublishRequest(bio, from);
                break;
        case KADEMLIA_PUBLISH_NOTES_REQ_DEPRECATED:
                DebugRecv(KadPublishNotesReq, from);
                ProcessPublishNotesRequest(bio, from);
                break;
        case KADEMLIA2_PUBLISH_NOTES_REQ:
                DebugRecv(Kad2PublishNotesReq, from);
                Process2PublishNotesRequest(bio, from, senderKey);
                break;
        case KADEMLIA2_PUBLISH_KEY_REQ:
                DebugRecv(Kad2PublishKeyReq, from);
                Process2PublishKeyRequest(bio, from, senderKey);
                break;
        case KADEMLIA2_PUBLISH_SOURCE_REQ:
                DebugRecv(Kad2PublishSourceReq, from);
                Process2PublishSourceRequest(bio, from, senderKey);
                break;
        case KADEMLIA_PUBLISH_RES:
                DebugRecv(KadPublishRes, from);
                ProcessPublishResponse(bio, from);
                break;
        case KADEMLIA2_PUBLISH_RES:
                DebugRecv(Kad2PublishRes, from);
                Process2PublishResponse(bio, from, senderKey);
                break;
        case KADEMLIA2_PING:
                DebugRecv(Kad2Ping, from);
                Process2Ping(from, senderKey);
                break;
        case KADEMLIA2_PONG:
                DebugRecv(Kad2Pong, from);
                Process2Pong(bio, from);
                break;
        default:
                throw wxString::Format(wxT("Unknown opcode %02" PRIx8 " on CKademliaUDPListener::ProcessPacket()"), opcode);
        }

        // signals the routing zone that the contact is alive
// 	CKademlia::GetRoutingZone()->setAlive( from );

        // bootstrap if necessary
// 	if ( CKademlia::GetRoutingZone()->GetNumContacts() == 0 )
// 		Bootstrap ( from, false );
}



// Used only for Kad1.0
bool CKademliaUDPListener::AddContact ( CMemFile & data, const CI2PAddress & udpdest, const CI2PAddress & tcpdest, const CKadUDPKey& key, bool& destVerified, bool update, CUInt128* outContactID )
{
        AddDebugLogLineN(logClientKadUDP, CFormat ( wxT ( "addContact : called with (udpdesy=%s)" ) )
                         % udpdest.humanReadable() );

        // read the contact from bio
        CContact new_contact(data, 0, -1);
        new_contact.SetUDPKey( key );
        if (outContactID != NULL) {
                *outContactID = new_contact.GetClientID();
        }
        if (tcpdest.isValid()) {
                new_contact.SetTCPDest(tcpdest);
        }
        // adjust its destinations if it is lying (?)
        if ( udpdest.isValid() ) {
                new_contact.SetUDPDest( udpdest );
        }
        return CKademlia::GetRoutingZone()->Add( new_contact, destVerified, update, false, true);
}




// Used only for Kad2.0
bool CKademliaUDPListener::AddContact2(CMemFile & bio, const CI2PAddress & WXUNUSED(dest), uint8_t* outVersion, const CKadUDPKey& udpkey,
                                       bool& destVerified, bool update, bool WXUNUSED(fromHelloReq), bool* outRequestsACK, CUInt128* outContactID)
{
        if (outRequestsACK != 0) {
                *outRequestsACK = false;
        }

        //     CMemFile bio(data, lenData);
        CContact new_contact(bio, true, -1);
        new_contact.SetUDPKey(udpkey);
        //     CUInt128 id = bio.ReadUInt128();
        if (outContactID != NULL) {
                *outContactID = new_contact.GetClientID();
        }
        if (new_contact.GetVersion() == 0) {
                throw wxString::Format(wxT("***NOTE: Received invalid Kademlia2 version (%" PRIu8 ") in "), new_contact.GetVersion()) +
                wxString::FromAscii(__FUNCTION__);
        }
        if (outVersion != NULL) {
                *outVersion = new_contact.GetVersion();
        }
        bool udpFirewalled = false;
        TagList taglist(bio);
        for (TagList::const_iterator it = taglist.begin(); it != taglist.end(); it++) {
                TagList::const_iterator tag = it;
#ifdef no_interesting_tags_there_for_imule_but_we_keep_it_here_to_keep_things_aligned
                if (tag->GetID() == TAG_SOURCEUDEST) {
                        if (tag->IsAddr() && tag->GetAddr().isValid()) {
                                new_contact.SetUDPDest(tag->GetAddr());
                        }
                } else
#endif
                        if (tag->GetID() == TAG_KADMISCOPTIONS) {
                                if (tag->IsInt() && tag->GetInt() > 0) {
                                        //                 udpFirewalled = (tag->GetInt() & 0x01) > 0;
                                        //                 tcpFirewalled = (tag->GetInt() & 0x02) > 0;
                                        if ((tag->GetInt() & 0x04) > 0) {
                                                if (outRequestsACK != NULL) {
                                                        if (new_contact.GetVersion() >= 8) {
                                                                *outRequestsACK = true;
                                                        }
                                                } else {
                                                        wxFAIL;
                                                }
                                        }
                                }
                        }
        }

        // check if we are waiting for informations (nodeid) about this client and if so inform the requester
        for (FetchNodeIDList::iterator it = m_fetchNodeIDRequests.begin(); it != m_fetchNodeIDRequests.end(); ++it) {
                if (it->tcpDest == new_contact.GetTCPDest()) {
                        //AddDebugLogLineN(logKadMain, wxT("Result Addcontact: ") + id.ToHexString());
                        uint8_t uchID[16];
                        new_contact.GetClientID().ToByteArray(uchID);
                        it->requester->KadSearchNodeIDByIPResult(KCSR_SUCCEEDED, uchID);
                        m_fetchNodeIDRequests.erase(it);
                        break;
                }
        }
#ifdef this_was_for_amule
        if (fromHelloReq && new_contact.GetVersion() >= 8) {
                // this is just for statistic calculations. We try to determine the ratio of (UDP) firewalled users,
                // by counting how many of all nodes which have us in their routing table (our own routing table is supposed
                // to have no UDP firewalled nodes at all) and support the firewalled tag are firewalled themself.
                // Obviously this only works if we are not firewalled ourself
                //         CKademlia::GetPrefs()->StatsIncUDPFirewalledNodes(udpFirewalled);
                //         CKademlia::GetPrefs()->StatsIncTCPFirewalledNodes(tcpFirewalled);
        }
#endif
        if (!udpFirewalled) {   // do not add (or update) UDP firewalled sources to our routing table
                return CKademlia::GetRoutingZone()->Add(new_contact, destVerified, update, false, true);
        } else {
                //         AddDebugLogLineN(logKadRouting, wxT("Not adding firewalled client to routing table (") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(")"));
                return false;
        }
}

// Used only for Kad1.0
/**
 * <b>Add  a list of contacts</b> to Kademlia RoutingZone
 * @param bio
 * @param numContacts
 */
void CKademliaUDPListener::AddContacts ( CMemFile & bio, uint16_t numContacts, bool update, uint8_t kadversion )
{
        AddDebugLogLineN(logClientKadUDP, CFormat ( wxT ( "addContacts : called with (numContacts=%d)" ) )
                         % numContacts );

        CRoutingZone *routingZone = CKademlia::GetRoutingZone();

        if (kadversion<2) {

                for ( uint16_t i = 0; i < numContacts; i++ ) {
                        CContact new_contact( bio, false, -1 );
                        bool verified = false;

                        if ( new_contact.GetUDPDest().isValid() ) {
                                new_contact.SetUDPKey(0);
                                routingZone->Add ( new_contact, verified, update, false, false);
                        }
                }
        } else {
                bool assumeVerified = CKademlia::GetRoutingZone()->GetNumContacts() == 0;
                for (; numContacts>0; numContacts--) {
                        CContact new_contact( bio, true, -1 );
                        bool verified = assumeVerified;
                        routingZone->Add(new_contact, verified, false, false, false);
                }

        }
}

//KADEMLIA_BOOTSTRAP_REQ

// Used only for Kad1.0
void CKademliaUDPListener::ProcessBootstrapRequest ( CMemFile & bio, const CI2PAddress & dest )
{
        AddDebugLogLineN(logClientKadUDP, CFormat ( wxT ( "processBootstrapRequest(bio, %s)" ) ) % dest.humanReadable() );

        // Verify packet is expected size

        if ( bio.GetAvailable() != detailslen() ) {
                throw ( wxString ) ( CFormat ( wxT ( "***NOTE: Received wrong size (%llu, expected %u) packet in %s" ) )
                                     % bio.GetAvailable() % detailslen() % wxString::FromAscii ( __func__ ) );
        }


        // Add the sender to the list of contacts
        bool verified = false;
        AddContact ( bio, dest, CI2PAddress::null, 0, verified, true, NULL );

        // Get some contacts to return
        ContactList contacts;
        uint32_t max_contacts = (CPacket::UDPPacketMaxDataSize()-2/*number of contacts*/)
                                / detailslen();

        uint16_t numContacts = (uint16_t) (1 + CKademlia::GetRoutingZone()->GetBootstrapContacts ( contacts, max_contacts - 1 /* -1 : my own node will be added at the end of the list*/ ));

        // Create response packet
        //We only collect a max of numContacts contacts here.. Max size is 527.

        CMemFile res ( 2 + detailslen() * (numContacts+1) );

        // Write packet info
        res.WriteUInt16 ( numContacts ); // size : 2

        CContact contact;

        ContactList::const_iterator it;

        // write contacts taken from our routing zone
        for ( it = contacts.begin(); it != contacts.end(); it++ ) {
                contact = *it;
                contact.WriteToKad1Contact(res); // size : detailslen() ( * numContacts)
        }

        // write ourself
        CContact::Self().WriteToKad1Contact(res); // size : detailslen()

        // Send response
        AddDebugLogLineN(logClientKadUDP, CFormat ( wxT ( "KadBootstrapRes %x" ) ) % dest.hashCode() );

        SendPacket ( res, KADEMLIA_BOOTSTRAP_RES_DEPRECATED, dest, 0, NULL );
}

// KADEMLIA2_BOOTSTRAP_REQ
// Used only for Kad2.0
void CKademliaUDPListener::Process2BootstrapRequest(const CI2PAddress & dest, const CKadUDPKey& senderKey)
{
        // Get some contacts to return
        ContactList contacts;
        uint32_t max_contacts = (CPacket::UDPPacketMaxDataSize()-16/*id*/-1/*version*/-CI2PAddress::validLength()/*tcp*/ - 2/*number of contacts*/)
                                / kad2detailslen();

        uint16_t numContacts = (uint16_t) CKademlia::GetRoutingZone()->GetBootstrapContacts ( contacts, max_contacts );

        // Create response packet
        //We only collect a max of numContacts contacts here.. Max size is 527.

        CMemFile packetdata ( 16 + 1 + CI2PAddress::validLength() + 2 + kad2detailslen() * (numContacts+1) );

        packetdata.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
        packetdata.WriteUInt8(KADEMLIA_VERSION);
        packetdata.WriteAddress(theApp->GetTcpDest());

        // Write packet info
        packetdata.WriteUInt16(numContacts);
        for (ContactList::const_iterator it = contacts.begin(); it != contacts.end(); ++it) {
                ContactList::const_iterator contact = it;
                contact->WriteToKad2Contact(packetdata);
        }

        // Send response
        DebugSend(Kad2BootstrapRes, dest);
        SendPacket(packetdata, KADEMLIA2_BOOTSTRAP_RES, dest, senderKey, NULL);
}

//KADEMLIA_BOOTSTRAP_RES
// Used only for Kad1.0
void CKademliaUDPListener::ProcessBootstrapResponse ( CMemFile & bio, const CI2PAddress & WXUNUSED(dest) )
{
        // Verify packet is expected size

        if ( bio.GetAvailable() < 2 ) {
                throw ( wxString ) ( CFormat ( wxT ( "***NOTE: Received wrong size (%llu) packet in %s" ) ) %
                                     bio.GetAvailable() % wxString::FromAscii ( __func__ ) );
        }

        // How many contacts were given

        uint16_t numContacts = bio.ReadUInt16();

        // Verify packet is expected size
        if ( bio.GetAvailable() != ( uint32_t ) ( detailslen() *numContacts ) ) {
                return;
        }

        // Add these contacts to the list.
        AddContacts ( bio, numContacts, false, 1 );
}

// KADEMLIA2_BOOTSTRAP_RES
// Used only for Kad2.0
void CKademliaUDPListener::Process2BootstrapResponse(CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey, bool validReceiverKey)
{
        CHECK_TRACKED_PACKET(KADEMLIA2_BOOTSTRAP_REQ);

        CRoutingZone *routingZone = CKademlia::GetRoutingZone();

        // How many contacts were given
        CMemFile & bio = packet;
        CUInt128 contactID = bio.ReadUInt128();
        uint8_t version = bio.ReadUInt8();
        CI2PAddress tdest = bio.ReadAddress();
        // if we don't know any Contacts yet and try to Bootstrap, we assume that all contacts are verified,
        // in order to speed up the connecting process. The attackvectors to exploit this are very small with no
        // major effects, so that's a good trade
        bool assumeVerified = CKademlia::GetRoutingZone()->GetNumContacts() == 0;

        if (CKademlia::s_bootstrapList.empty()) {
                CContact envoyeur( contactID, dest, tdest, version, senderKey, validReceiverKey, CKademlia::GetPrefs()->GetKadID() );
                routingZone->Add( envoyeur, validReceiverKey, true, false, false);
        }

        uint16_t numContacts = bio.ReadUInt16();
        while (numContacts) {
                CContact nouveau( bio, true, -1 );
                bool verified = assumeVerified;
                routingZone->Add(nouveau, verified, false, false, false);
                numContacts--;
        }
}

//KADEMLIA_HELLO_REQ
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessHelloRequest ( CMemFile & packet, const CI2PAddress & dest )
{
        // Verify packet is expected size
        CHECK_PACKET_EXACT_SIZE(detailslen());

        // Add the sender to the list of contacts
        bool validReceiverKey = false;
        CUInt128 contactID;
        bool addedOrUpdated = AddContact(packet, dest, CI2PAddress::null, 0, validReceiverKey, true, &contactID);


        // Send response
        DebugSend(KadHelloRes, dest);
        SendMyDetails(KADEMLIA_HELLO_RES_DEPRECATED, dest, 0, 0, NULL, false);

        if (addedOrUpdated && !validReceiverKey) {
                // we need to verify this contact but it doesn't support HELLO_RES_ACK nor keys, do a little workaround
                SendLegacyChallenge(dest, contactID, false);
        }
}


// KADEMLIA2_HELLO_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2HelloRequest(CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey, bool validReceiverKey)
{
        CI2PAddress dbgOldUDPDest = dest;
        uint8_t contactVersion = 0;
        CUInt128 contactID;
        bool addedOrUpdated = AddContact2(packet, dest, &contactVersion, senderKey, validReceiverKey, true, true, NULL, &contactID); // might change (udp)dest, validReceiverKey
        wxASSERT(contactVersion >= 1);
#ifdef __DEBUG__
        if (dbgOldUDPDest != dest) {
                AddDebugLogLineN(logClientKadUDP, wxT("KadContact ") + dbgOldUDPDest.humanReadable() + wxString::Format(wxT(" uses his internal (%s) instead external (%s) UDP Dest"), dest.humanReadable().c_str(), dbgOldUDPDest.humanReadable().c_str()));
        }
#endif

        DebugSend(Kad2HelloRes, dest);
        // if this contact was added or updated (so with other words not filtered or invalid) to our routing table and did not already send a valid
        // receiver key or is already verified in the routing table, we request an additional ACK package to complete a three-way-handshake and
        // verify the remote IP
        SendMyDetails(KADEMLIA2_HELLO_RES, dest, contactVersion, senderKey, &contactID, addedOrUpdated && !validReceiverKey);

        if (addedOrUpdated && !validReceiverKey && contactVersion == 7 && !HasActiveLegacyChallenge(dest)) {
                // Kad Version 7 doesn't support HELLO_RES_ACK but sender/receiver keys, so send a ping to validate
                DebugSend(Kad2Ping, dest);
                SendNullPacket(KADEMLIA2_PING, dest, senderKey, NULL);
#ifdef __DEBUG__
                CContact contact = CKademlia::GetRoutingZone()->GetContact(contactID);
                if ( !contact.IsInvalid() ) {
                        if (contact.GetType() < 2) {
                                AddDebugLogLineN(logClientKadUDP, wxT("Sending (ping) challenge to a long known contact (should be verified already) - ") + dest.humanReadable());
                        }
                } else {
                        wxFAIL;
                }
#endif
                /*    } else if (CKademlia::GetPrefs()->FindExternKadDest(false) && contactVersion > 5) { // do we need to find out our extern port?
                        // never happens with I2P
                        DebugSend(Kad2Ping, dest);
                        SendNullPacket(KADEMLIA2_PING, dest, senderKey, NULL);*/
        }

        if (addedOrUpdated && !validReceiverKey && contactVersion < 7 && !HasActiveLegacyChallenge(dest)) {
                // we need to verify this contact but it doesn't support HELLO_RES_ACK nor keys, do a little workaround
                SendLegacyChallenge(dest, contactID, true);
        }
}

//KADEMLIA_HELLO_RES
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessHelloResponse ( CMemFile & packet, const CI2PAddress & dest )
{
        CHECK_TRACKED_PACKET(KADEMLIA_HELLO_REQ_DEPRECATED);

        // Verify packet is expected size
        CHECK_PACKET_EXACT_SIZE(detailslen());

        // Add or Update contact.
        bool validReceiverKey = false;
        CUInt128 contactID;
        bool addedOrUpdated = AddContact(packet, dest, CI2PAddress::null, 0, validReceiverKey, true, &contactID);
        if (addedOrUpdated && !validReceiverKey) {
                // even though this is supposably an answer to a request from us, there are still possibilities to spoof
                // it, as long as the attacker knows that we would send a HELLO_REQ (which in this case is quite often),
                // so for old Kad Version which doesn't support keys, we need
                SendLegacyChallenge(dest, contactID, false);
        }
}

// KADEMLIA2_HELLO_RES_ACK
// Used in Kad2.0 only
void CKademliaUDPListener::Process2HelloResponseAck( CMemFile & packet, const CI2PAddress & dest, bool validReceiverKey)
{
        CHECK_PACKET_MIN_SIZE(17);
        CHECK_TRACKED_PACKET(KADEMLIA2_HELLO_RES);

        if (!validReceiverKey) {
                AddDebugLogLineN(logClientKadUDP, wxT("Receiver key is invalid! (sender: ") + dest.humanReadable() + wxT(")"));
                return;
        }

        // Additional packet to complete a three-way-handshake, making sure the remote contact is not using a spoofed ip.
        CUInt128 remoteID = packet.ReadUInt128();
        if (!CKademlia::GetRoutingZone()->VerifyContact(remoteID, dest)) {
                AddDebugLogLineN(logKadRouting, wxT("Unable to find valid sender in routing table (sender: ") + dest.humanReadable() + wxT(")"));
        }
#ifdef __DEBUG__
        else {
                AddDebugLogLineN(logKadRouting, wxT("Verified contact (") + dest.humanReadable() + wxT(") by HELLO_RES_ACK"));
        }
#endif
}

// KADEMLIA2_HELLO_RES
// Used in Kad2.0 only
void CKademliaUDPListener::Process2HelloResponse(CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey, bool validReceiverKey)
{
        CHECK_TRACKED_PACKET(KADEMLIA2_HELLO_REQ);

        // Add or Update contact.
        uint8_t contactVersion;
        CUInt128 contactID;
        bool sendACK = false;
        bool addedOrUpdated = AddContact2(packet, dest, &contactVersion, senderKey, validReceiverKey, true, false, &sendACK, &contactID);

        if (sendACK) {
                // the client requested us to send an ACK packet, which proves that we're not a spoofed fake contact
                // fulfill his wish
                if (senderKey.IsEmpty()) {
                        // but we don't have a valid sender key - there is no point to reply in this case
                        // most likely a bug in the remote client
                        AddDebugLogLineN(logClientKadUDP, wxT("Remote client demands ACK, but didn't send any sender key! (sender: ") + dest.humanReadable() + wxT(")"));
                } else {
                        CMemFile paquet(17);
                        paquet.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
                        paquet.WriteUInt8(0);   // no tags at this time
                        DebugSend(Kad2HelloResAck, dest);
                        SendPacket(paquet, KADEMLIA2_HELLO_RES_ACK, dest, senderKey, NULL);
                }
        } else if (addedOrUpdated && !validReceiverKey && contactVersion < 7) {
                // even though this is supposably an answer to a request from us, there are still possibilities to spoof
                // it, as long as the attacker knows that we would send a HELLO_REQ (which in this case is quite often),
                // so for old Kad Version which doesn't support keys, we need
                SendLegacyChallenge(dest, contactID, true);
        }

        // do we need to find out our extern port?
        if (CKademlia::GetPrefs()->FindExternKadPort(false) && contactVersion > 5) {
                DebugSend(Kad2Ping, dest);
                SendNullPacket(KADEMLIA2_PING, dest, senderKey, NULL);
        }
}

//KADEMLIA_REQ
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessKademliaRequest ( CMemFile & packet, const CI2PAddress & dest )
{
        // Verify packet is expected size
        CHECK_PACKET_EXACT_SIZE(1/*m_type*/ + 16 /*m_target*/ + 16 /*my ID*/ + 16 /*peer ID*/);


// Get type
        uint8_t type = packet.ReadUInt8();
        type &= 0x1F;
        if( type == 0 ) {
                throw wxString::Format(wxT("***NOTE: Received wrong type (0x%02" PRIu8 ") in "), type) + wxString::FromAscii(__FUNCTION__);
        }


        // This is the target node trying to be found.
        CUInt128 target = packet.ReadUInt128();
        CUInt128 distance ( CKademlia::GetPrefs()->GetKadID() );
        distance.XOR ( target );

        // This makes sure we are not mistaken identify. Some client may have fresh installed and have a new hash.
        CUInt128 check = packet.ReadUInt128();
        if (CKademlia::GetPrefs()->GetKadID().CompareTo(check)) {
                return;
        }

        // This is the ID of the requesting peer
        /*CUInt128 destId = */packet.ReadUInt128();

        // Get required number closest to target
        TargetContactMap results(target);
        uint32_t max_results = std::min<uint32_t>( type, (maxMessageSize() - 16 - 1) / detailslen() ) ;
        CKademlia::GetRoutingZone()->GetClosestToTarget ( 2, distance, ( int ) max_results, results );

        uint16_t count = ( uint16_t ) results.size();

        // Write response
        // Max count is 32. size 817..
        // 16 + 1 + 817(32)
        CMemFile res ( 16 + 1 + detailslen() * count );
        res.WriteUInt128 ( target );
        res.WriteUInt8 ( ( uint8_t ) count );
        results.apply([&res](CSearchContact&c){c.WriteToKad1Contact(res);});

        DebugSendF(wxString::Format(wxT("KadRes (count=%" PRIu16 ")"), count), dest);
        SendPacket ( res, KADEMLIA_RES_DEPRECATED, dest, 0, NULL );
}


// KADEMLIA2_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::ProcessKademlia2Request(CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey)
{
        // Get target and type
        uint8_t type = packet.ReadUInt8();
        type &= 0x1F;
        if (type == 0) {
                throw wxString::Format(wxT("***NOTE: Received wrong type (0x%02" PRIx8 ") in "), type) + wxString::FromAscii(__FUNCTION__);
        }

        // This is the target node trying to be found.
        CUInt128 target = packet.ReadUInt128();
        // Convert Target to Distance as this is how we store contacts.
        CUInt128 distance = target ^ CKademlia::GetPrefs()->GetKadID();

        // This makes sure we are not mistaken identify. Some client may have fresh installed and have a new KadID.
        CUInt128 check = packet.ReadUInt128();
        if (CKademlia::GetPrefs()->GetKadID() == check) {
                // Get required number closest to target
                TargetContactMap results(target);
                uint32_t max_results = std::min<uint32_t>( type, (maxMessageSize() - 16 - 1) / kad2detailslen() ) ;
                CKademlia::GetRoutingZone()->GetClosestToTarget(2, distance, ( int ) max_results, results );
                uint8_t count = (uint8_t)results.size();

                // Write response
                // Max count is 32. size 817..
                // 16 + 1 + 25(32)
                CMemFile packetdata(817);
                packetdata.WriteUInt128(target);
                packetdata.WriteUInt8(count);
                results.apply([&packetdata](CContact&c){c.WriteToKad2Contact(packetdata);});
                
                DebugSendF(wxString::Format(wxT("Kad2Res (count=%" PRIu8 ")"), count), dest);
                SendPacket(packetdata, KADEMLIA2_RES, dest, senderKey, NULL);
        }
}

//KADEMLIA_RES
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessKademliaResponse ( CMemFile & packet, const CI2PAddress & dest )
{
        AddDebugLogLineN(logClientKadUDP, CFormat ( wxT ( "processKademliaResponse(bio, %s)" ) ) % dest.humanReadable() );
        // Verify packet is expected size
        CHECK_PACKET_MIN_SIZE(17);
        CHECK_TRACKED_PACKET(KADEMLIA_REQ_DEPRECATED);

        //Used Pointers
        CRoutingZone *routingZone = CKademlia::GetRoutingZone();

//     if (CKademlia::GetPrefs()->GetRecheckIP()) {
//         FirewalledCheck(ip, port, 0, 0);
//         DebugSend(KadHelloReq, ip, port);
//         SendMyDetails(KADEMLIA_HELLO_REQ, ip, port, 0, 0, NULL, false);
//     }

        // What search does this relate to
        CUInt128 target = packet.ReadUInt128();

        uint16_t numContacts = packet.ReadUInt8();

        // Is this one of our legacy challenge packets?
        CUInt128 contactID;
        if (IsLegacyChallenge(target, dest, KADEMLIA_REQ_DEPRECATED, contactID)) {
                // yup it is, set the contact as verified
                if (!routingZone->VerifyContact(contactID, dest)) {
                        AddDebugLogLineN(logKadRouting, wxT("Unable to find valid sender in routing table (sender: ") + dest.humanReadable());
                }
#ifdef __DEBUG__
                else {
                        AddDebugLogLineN(logClientKadUDP, wxT("Verified contact with legacy challenge (KadReq) - ") + dest.humanReadable());
                }
#endif
                return; // we do not actually care for its other content
        }

        // Verify packet is expected size
        CHECK_PACKET_EXACT_SIZE( detailslen() * numContacts );

        ContactList results;
        uint32_t ignoredCount = 0;

        try {
                CContact myself = CContact::Self() ;
                for ( uint16_t i = 0; i < numContacts; i++ ) {
                        CContact new_contact( packet, false, -1 );

                        if ( ::IsGoodDest ( new_contact.GetUDPDest() ) ) {
                                if (!theApp->ipfilter->IsFiltered(new_contact.GetUDPDest())) {
                                        bool verified = false;
                                        // we are now setting all version for received contact to "2" which means we assume full Kad2 when adding
                                        // the contact to the routing table. If this should be an old Kad1 contact, we won't be able to keep it, but
                                        // we avoid having to send double hello packets to the 90% Kad2 nodes
                                        // This is the first step of dropping Kad1 support
//                     new_contact.SetVersion(2) ; // TODO: remove the comment, but what precedes is not (yet) true with iMule
                                        bool wasAdded = routingZone->AddUnfiltered(new_contact, verified, false, false, false);
                                        if (wasAdded || routingZone->IsAcceptableContact(new_contact)) {
                                                results. push_back(new_contact);
                                        } else {
                                                ignoredCount++;
                                        }
                                }
                        }
                }
        } catch ( ... ) {
                throw;
        }

        if (ignoredCount > 0) {
                AddDebugLogLineN(logClientKadUDP, wxString::Format(wxT("Ignored %" PRIu32 " contact(s) (%" PRIu16 " reveived) in routing answer from "), ignoredCount, numContacts) + dest.humanReadable());
        }

        CSearchManager::ProcessResponse ( target, dest, results );
}

// KADEMLIA2_RES
// Used in Kad2.0 only
void CKademliaUDPListener::ProcessKademlia2Response(CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey&
                WXUNUSED(senderKey))
{
        CHECK_TRACKED_PACKET(KADEMLIA2_REQ);

        // Used Pointers
        CRoutingZone *routingZone = CKademlia::GetRoutingZone();

        // don't do firewallchecks on this opcode anymore, since we need the contacts kad version - hello opcodes are good enough
//  if (CKademlia::GetPrefs()->GetRecheckIP()) {
//      FirewalledCheck(ip, port, senderKey);
//      DebugSend(Kad2HelloReq, ip, port);
//      SendMyDetails(KADEMLIA2_HELLO_REQ, ip, port, true, senderKey, NULL);
//  }

        // What search does this relate to
        CMemFile & bio = packet;
        CUInt128 target = bio.ReadUInt128();
        uint8_t numContacts = bio.ReadUInt8();

        // Is this one of our legacy challenge packets?
        CUInt128 contactID;
        if (IsLegacyChallenge(target, dest, KADEMLIA2_REQ, contactID)) {
                // yup it is, set the contact as verified
                if (!routingZone->VerifyContact(contactID, dest)) {
                        AddDebugLogLineN(logKadRouting, wxT("Unable to find valid sender in routing table (sender: ") + dest.humanReadable() + wxT(")"));
                }
#ifdef __DEBUG__
                else {
                        AddDebugLogLineN(logClientKadUDP, wxT("Verified contact with legacy challenge (Kad2Req) - ") + dest.humanReadable());
                }
#endif
                return; // we do not actually care for its other content
        }
        // Verify packet is expected size
        CHECK_PACKET_EXACT_SIZE(kad2detailslen()*numContacts);

        // is this a search for firewallcheck ips?
//     bool isFirewallUDPCheckSearch = false;
//     if (CUDPFirewallTester::IsFWCheckUDPRunning() && CSearchManager::IsFWCheckUDPSearch(target)) {
//         isFirewallUDPCheckSearch = true;
//     }

        uint32_t ignoredCount = 0;
        ContactList results;
        for (uint8_t i = 0; i < numContacts; i++) {
                CContact readContact(bio, true, -1);

                if (::IsGoodDest(readContact.GetUDPDest())) {
                        if (!theApp->ipfilter->IsFiltered(readContact.GetUDPDest()) and readContact.GetVersion() <= 5) { /*No DNS Port without encryption*/
                                /*if (isFirewallUDPCheckSearch) {
                                    // UDP FirewallCheck searches are special. The point is we need an IP which we didn't sent an UDP message yet
                                    // (or in the near future), so we do not try to add those contacts to our routingzone and we also don't
                                    // deliver them back to the searchmanager (because he would UDP-ask them for further results), but only report
                                    // them to FirewallChecker - this will of course cripple the search but thats not the point, since we only
                                    // care for IPs and not the random set target
                                    CUDPFirewallTester::AddPossibleTestContact(id, contactIP, contactPort, tport, target, version, 0, false);
                                } else*/ {
                                        bool verified = false ;
                                        bool wasAdded = routingZone->AddUnfiltered(readContact, verified, false, false, false);
//                     CContact *temp = new CContact(id, contactIP, contactPort, tport, version, 0, false, target);
                                        if (wasAdded || routingZone->IsAcceptableContact(readContact)) {
                                                results.push_back(readContact);
                                        } else {
                                                ignoredCount++;
//                         delete temp;
                                        }
                                }
                        }
                }
        }

        if (ignoredCount > 0) {
                AddDebugLogLineN(logClientKadUDP, wxString::Format(wxT("Ignored %" PRIu32 " contact(s) (%" PRIu8 " received) in routing answer from "), ignoredCount, numContacts) + dest.humanReadable());
        }

        CSearchManager::ProcessResponse(target, dest, results);
}


void CKademliaUDPListener::Free ( SSearchTerm* pSearchTerms )

{
        if ( !pSearchTerms ) {
                return;
        }

        Free ( pSearchTerms->left );

        Free ( pSearchTerms->right );
        delete pSearchTerms;
}



SSearchTerm* CKademliaUDPListener::CreateSearchExpressionTree(CMemFile& bio, int iLevel)
{
        // the max. depth has to match our own limit for creating the search expression
        // (see also 'ParsedSearchExpression' and 'GetSearchPacket')
        if (iLevel >= 24) {
                AddDebugLogLineN(logKadSearch, wxT("***NOTE: Search expression tree exceeds depth limit!"));
                return NULL;
        }
        iLevel++;

        uint8_t op = bio.ReadUInt8();
        if (op == 0x00) {
                uint8_t boolop = bio.ReadUInt8();
                if (boolop == 0x00) { // AND
                        SSearchTerm* pSearchTerm = new SSearchTerm;
                        pSearchTerm->type = SSearchTerm::AND;
                        //TRACE(" AND");
                        if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL) {
                                wxFAIL;
                                delete pSearchTerm;
                                return NULL;
                        }
                        if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL) {
                                wxFAIL;
                                Free(pSearchTerm->left);
                                delete pSearchTerm;
                                return NULL;
                        }
                        return pSearchTerm;
                } else if (boolop == 0x01) { // OR
                        SSearchTerm* pSearchTerm = new SSearchTerm;
                        pSearchTerm->type = SSearchTerm::OR;
                        //TRACE(" OR");
                        if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL) {
                                wxFAIL;
                                delete pSearchTerm;
                                return NULL;
                        }
                        if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL) {
                                wxFAIL;
                                Free(pSearchTerm->left);
                                delete pSearchTerm;
                                return NULL;
                        }
                        return pSearchTerm;
                } else if (boolop == 0x02) { // NOT
                        SSearchTerm* pSearchTerm = new SSearchTerm;
                        pSearchTerm->type = SSearchTerm::NOT;
                        //TRACE(" NOT");
                        if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL) {
                                wxFAIL;
                                delete pSearchTerm;
                                return NULL;
                        }
                        if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL) {
                                wxFAIL;
                                Free(pSearchTerm->left);
                                delete pSearchTerm;
                                return NULL;
                        }
                        return pSearchTerm;
                } else {
                        AddDebugLogLineN(logKadSearch, wxString::Format(wxT("*** Unknown boolean search operator 0x%02" PRIx8 " (CreateSearchExpressionTree)"), boolop));
                        return NULL;
                }
        } else if (op == 0x01) { // String
                wxString str(bio.ReadString(true));

                // Make lowercase, the search code expects lower case strings!
                str.MakeLower();

                SSearchTerm* pSearchTerm = new SSearchTerm;
                pSearchTerm->type = SSearchTerm::String;
                pSearchTerm->astr = new wxArrayString;

                // pre-tokenize the string term
                //#warning TODO: TokenizeOptQuotedSearchTerm
                wxStringTokenizer token(str, CSearchManager::GetInvalidKeywordChars(),wxTOKEN_DEFAULT );
                while (token.HasMoreTokens()) {
                        wxString strTok(token.GetNextToken());
                        if (!strTok.IsEmpty()) {
                                pSearchTerm->astr->Add(strTok);
                        }
                }

                return pSearchTerm;
        } else if (op == 0x02) { // Meta tag
                // read tag value
                wxString strValue(bio.ReadString(true));
                // Make lowercase, the search code expects lower case strings!
                strValue.MakeLower();

                // read tag name
                file_tags tagID =  (file_tags) bio.ReadUInt8();

                SSearchTerm* pSearchTerm = new SSearchTerm;
                pSearchTerm->type = SSearchTerm::MetaTag;
                pSearchTerm->tag = new CTag(tagID, strValue);
                return pSearchTerm;
        } else if (op == 0x03) { // Numeric relation
                static const SSearchTerm::ESearchTermType _aOps[] = {
                        SSearchTerm::OpEqual,        // mmop=0x00
                        SSearchTerm::OpGreater,      // mmop=0x01
                        SSearchTerm::OpLess,         // mmop=0x02
                        SSearchTerm::OpGreaterEqual, // mmop=0x03
                        SSearchTerm::OpLessEqual,    // mmop=0x04
                        SSearchTerm::OpNotEqual      // mmop=0x05
                };

                // read integer operator
                uint8_t mmop = bio.ReadUInt8();
                if (mmop >= itemsof(_aOps)) {
                        AddDebugLogLineN(logKadSearch, wxString::Format(wxT("*** Unknown integer search op=0x%02u (CreateSearchExpressionTree)"), mmop));
                        return NULL;
                }
                // read tag value
                CTag tag(bio);
                uint64_t ullValue = tag.GetInt();
                file_tags tagID = (file_tags) tag.GetID();

                SSearchTerm* pSearchTerm = new SSearchTerm;
                pSearchTerm->type = _aOps[mmop];
                pSearchTerm->tag = new CTag(tagID, uint64_t(ullValue));

                return pSearchTerm;
        } else {
                AddDebugLogLineN(logKadSearch, wxString::Format(wxT("*** Unknown search op=0x%02x (CreateSearchExpressionTree)"), op));
                return NULL;
        }
}

//KADEMLIA_SEARCH_REQ
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessSearchRequest ( CMemFile & packet, const CI2PAddress & dest )
{
        // Verify packet is expected size
        CHECK_PACKET_MIN_SIZE(17);

        CUInt128 target = packet.ReadUInt128();
        uint8_t restrictive = packet.ReadUInt8();

        if ( packet.GetAvailable() == 0 ) {
                if ( restrictive ) {
                        //Source request
                        CKademlia::GetIndexed()->SendValidSourceResult ( target, dest, false, 0, 0, 0 );
                        //DEBUG_ONLY( Debug("SendValidSourceResult: Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
                } else {
                        //Single keyword request
                        CKademlia::GetIndexed()->SendValidKeywordResult ( target, NULL, dest, false, 0, 0 );
                        //DEBUG_ONLY( Debug("SendValidKeywordResult (Single): Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
                }
        } else {
                SSearchTerm* pSearchTerms = NULL;
                //                 bool oldClient = true;
                if (restrictive) {
                        pSearchTerms = CreateSearchExpressionTree(packet, 0);
                        if (pSearchTerms == NULL) {
                                throw wxString(wxT("Invalid search expression"));
                        }
                        if (restrictive > 1) {
                                //                                 oldClient = false;
                        }
                } else {
                        //                         oldClient = false;
                }

                // Keyword request with added options.
                CKademlia::GetIndexed()->SendValidKeywordResult(target, pSearchTerms, dest, false, 0, 0);
                Free(pSearchTerms);
        }
}

// KADEMLIA2_SEARCH_KEY_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2SearchKeyRequest(CMemFile & bio, const CI2PAddress & dest, const CKadUDPKey& senderKey)
{
//     CMemFile bio(packetData, lenPacket);
        CUInt128 target = bio.ReadUInt128();
        uint16_t startPosition = bio.ReadUInt16();
        bool restrictive = ((startPosition & 0x8000) == 0x8000);
        startPosition &= 0x7FFF;
        SSearchTerm* pSearchTerms = NULL;
        if (restrictive) {
                pSearchTerms = CreateSearchExpressionTree(bio, 0);
                if (pSearchTerms == NULL) {
                        throw wxString(wxT("Invalid search expression"));
                }
        }
        CKademlia::GetIndexed()->SendValidKeywordResult(target, pSearchTerms, dest, true, startPosition, senderKey);
        if (pSearchTerms) {
                Free(pSearchTerms);
        }
}

// KADEMLIA2_SEARCH_SOURCE_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2SearchSourceRequest(CMemFile & bio, const CI2PAddress & dest, const CKadUDPKey& senderKey)
{
//     CMemFile bio(packetData, lenPacket);
        CUInt128 target = bio.ReadUInt128();
        uint16_t startPosition = (bio.ReadUInt16() & 0x7FFF);
        uint64_t fileSize = bio.ReadUInt64();
        CKademlia::GetIndexed()->SendValidSourceResult(target, dest, true, startPosition, fileSize, senderKey);
}

//KADEMLIA_SEARCH_RES
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessSearchResponse ( CMemFile & packet, const CI2PAddress & dest )
{
        // Verify packet is expected size
        CHECK_PACKET_MIN_SIZE(18); //16 (uint128=search id) + 2 (uint16=number of responses)

        CSearchManager::ProcessResult( packet, dest );

}

// KADEMLIA2_SEARCH_RES
// Used in Kad2.0 only
void CKademliaUDPListener::Process2SearchResponse(CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& WXUNUSED(senderKey))
{
        CMemFile & bio = packet;

        // Who sent this packet.
        /*CUInt128 source =*/ bio.ReadUInt128();

//     // What search does this relate to
//     CUInt128 target = bio.ReadUInt128();
//
//     // Total results.
//     uint16_t count = bio.ReadUInt16();
//     while (count > 0) {
//         // What is the answer
//         CUInt128 answer = bio.ReadUInt128();
//
//         // Get info about answer
//         // NOTE: this is the one and only place in Kad where we allow string conversion to local code page in
//         // case we did not receive an UTF8 string. this is for backward compatibility for search results which are
//         // supposed to be 'viewed' by user only and not feed into the Kad engine again!
//         // If that tag list is once used for something else than for viewing, special care has to be taken for any
//         // string conversion!
//         TagPtrList* tags = new TagPtrList;
//         try {
//             bio.ReadTagPtrList(tags, true);
//         } catch(...) {
//             deleteTagPtrListEntries(tags);
//             delete tags;
//             tags = NULL;
//             throw;
//         }
//         CSearchManager::ProcessResult(target, answer, tags);
//         count--;
//     }
        CSearchManager::ProcessResult( packet, dest );
}

//KADEMLIA_PUBLISH_REQ
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessPublishRequest ( CMemFile & packet, const CI2PAddress & dest, bool self )
{
        //There are different types of publishing..
        //Keyword and File are Stored..

        // Verify packet is expected size
        CHECK_PACKET_MIN_SIZE(37);

        //Used Pointers
        CIndexed *indexed = CKademlia::GetIndexed();

        CMemFile & bio = packet ;
        CUInt128 file = bio.ReadUInt128();    CUInt128 & key = file ;
        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "processPublishRequest : dest=%s, key=%s" ) ) % dest.humanReadable() % file.ToHexString() );

        CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
        distance.XOR(file);

//     if (thePrefs::FilterLanIPs() && distance.Get32BitChunk(0) > SEARCHTOLERANCE) {
//         return;
//     }

        wxString strInfo;
        uint16_t count = bio.ReadUInt16();
        //     bool flag = false;
        uint8_t load = 0;
        while ( count > 0 ) {
                strInfo.Clear();

                CUInt128 target = bio.ReadUInt128();

                Kademlia::CKeyEntry* entry = new Kademlia::CKeyEntry();
                try {
                        entry->m_udpdest = dest;
                        entry->m_uKeyID.SetValue(file);
                        entry->m_uSourceID.SetValue(target);
                        TagList taglist ( bio ) ;
                        while ( !taglist.empty() ) {
                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "processPublishRequest : #tags remaining=%d" ) ) % taglist.size() );
                                CTag tag = taglist.back() ;
                                taglist.pop_back();

                                if ( tag.isValid() ) {
                                        if ( tag . GetID() == TAG_SOURCETYPE ) {
                                                if ( entry->m_bSource == false ) {
                                                        /* record the type, which is checked in CSearch::processResultFile
                                                        even if it is of no use (no firewalled source with i2p) */
                                                        entry->AddTag( tag );
                                                        entry->AddTag( CTag ( TAG_SOURCEUDEST, entry->m_udpdest ) );
                                                        entry->m_bSource = true;
                                                }
                                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "processPublishRequest : TAG_SOURCETYPE" ) ) );
                                        } else if ( tag . GetID() == TAG_FILENAME ) {
                                                if (entry->GetCommonFileName().IsEmpty()) {
							if (!tag.GetStr().IsEmpty()) {
                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "processPublishRequest : received TAG_FILENAME with string %s"))
                                              % tag.GetStr());
                entry->SetFileName(tag.GetStr());
							} else {
								AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "processPublishRequest : received TAG_FILENAME with empty string from %s"))
									% dest.humanReadable());
							}
							strInfo += CFormat(wxT("  Name=\"%s\"")) % tag.GetStr();
                                                }
                                        } else if ( tag . GetID() == TAG_FILESIZE ) {
                                                if ( entry->m_uSize == 0 ) {
                                                        entry->m_uSize = tag . GetInt() ;
                                                        // NOTE: always add the 'size' tag, even if it's stored separately in 'size'.
                                                        // the tag is still needed for answering search request
                                                        entry->AddTag( tag );
                                                        strInfo += wxString::Format(wxT("  Size=%" PRIu64 ""), entry->m_uSize);
                                                }
                                        } else if ( tag . GetID() == TAG_SOURCEDEST ) {
                                                if ( !entry->m_tcpdest ) {
                                                        entry->m_tcpdest = tag . GetAddr();
                                                        entry->AddTag( tag );
                                                }
                                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "processPublishRequest : TAG_SOURCEDEST %s" ) )
                                                                 % entry->m_tcpdest.humanReadable() );
                                        } else if (tag.GetID()!=TAG_SOURCEUDEST) {
                                                //TODO: Filter tags
                                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "%s : Filed tag, id: %s (%X), type:%s" ) )
                                                                 % wxString::FromAscii ( __func__ ) % tagnameStr(tag.GetID()) % ( int ) tag . GetID() % tagtypeStr(tag . GetType()) );
                                                entry->AddTag( tag );
                                        }
                                }
                        }
                        if (!strInfo.IsEmpty()) {
                                AddDebugLogLineN(logClientKadUDP, strInfo);
                        }
                } catch ( ... ) {
                        DebugClientOutput ( wxT ( "CKademliaUDPListener::processPublishRequest - ERROR" ), dest );
                        delete entry ;
                        throw;
                }

                if (entry->m_bSource == true) {
                        entry->m_tLifeTime = (uint32_t)time(NULL) + KADEMLIAREPUBLISHTIMES;
                        CEntry *sourceEntry = entry->Copy(); // "downcast" since we didnt knew before if this was a keyword, Kad1 support gets removed soon anyway no need for a beautiful solution here
                        delete entry;
                        entry = NULL;
                        if (indexed->AddSources(file, target, sourceEntry, load)) {
                                if ( !self ) {
                                        // send  publish response with number of sources to requester
                                        CMemFile res ( 25 );
                                        res.WriteUInt128 ( key );
                                        res.WriteUInt32 ( indexed->GetSourceCount ( key ) );
                                        res.WriteUInt32 ( indexed->GetCompleteSourceCount ( key ) );
                                        res.WriteUInt8 ( load );
                                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "KadPublishRes (source) to %s" ) ) % dest.humanReadable() );
                                        AddDebugLogLineN(logKadSourceCount,
                                                         CFormat ( wxT ( "KadPublishRes (source) to %s : %s (complete=%d, total=%d)" ) )
                                                         % dest.humanReadable() % key.ToHexString()
                                                         % indexed->GetCompleteSourceCount ( key )
                                                         % indexed->GetSourceCount ( key ) );

                                        SendPacket ( res, KADEMLIA_PUBLISH_RES, dest, 0, NULL );
                                } else {
                                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "processPublishRequest : source indexed from myself" ) ) );
                                }
                        } else {
                                delete sourceEntry;
                                sourceEntry = NULL;
                        }
                } else {
                        entry->m_tLifeTime = (uint32_t)time(NULL) + KADEMLIAREPUBLISHTIMEK;
                        if (!indexed->AddKeyword(key, target, entry, load)) {
                                //We already indexed the maximum number of keywords.
                                //We do not index anymore but we still send a success..
                                //Reason: Because if a VERY busy node tells the publisher it failed,
                                //this busy node will spread to all the surrounding nodes causing popular
                                //keywords to be stored on MANY nodes..
                                //So, once we are full, we will periodically clean our list until we can
                                //begin storing again..
                                delete entry ;
                                entry = NULL ;
                        }
                        if ( !self ) {
                                CMemFile res ( 17 );
                                res.WriteUInt128 ( key );
                                res.WriteUInt8 ( load );
                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "KadPublishRes (keyword) to %x" ) ) % dest.hashCode() );
                                SendPacket ( res, KADEMLIA_PUBLISH_RES, dest, 0, NULL );
                        } else {
                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "processPublishRequest : keyword indexed from myself" ) ) );
                        }
                }

                count--;
        }
}

// KADEMLIA2_PUBLISH_KEY_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2PublishKeyRequest(CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey)
{
        //Used Pointers
        CIndexed *indexed = CKademlia::GetIndexed();

        CMemFile & bio = packet;
        CUInt128 file = bio.ReadUInt128();

        CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
        distance.XOR(file);

        wxString strInfo;
        uint16_t count = bio.ReadUInt16();
        uint8_t load = 0;
        while (count > 0) {
                strInfo.Clear();

                CUInt128 target = bio.ReadUInt128();

                Kademlia::CKeyEntry* entry = new Kademlia::CKeyEntry();
                try {
                        entry->m_udpdest = dest;
                        entry->m_uKeyID.SetValue(file);
                        entry->m_uSourceID.SetValue(target);
                        entry->m_tLifeTime = (uint32_t)time(NULL) + KADEMLIAREPUBLISHTIMEK;
                        entry->m_bSource = false;
                        TagList taglist ( bio ) ;
                        while ( !taglist.empty() ) {
                                CTag tag = taglist.back() ;
                                taglist.pop_back();

                                if ( tag.isValid() ) {
                                        if (tag. GetID() == TAG_FILENAME) {
                                                if (entry->GetCommonFileName().IsEmpty()) {
                                                  if (!tag.GetStr().IsEmpty()) {
                                                    AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "processPublishRequest : received TAG_FILENAME with string %s"))
                                                    % tag.GetStr());
                                                    entry->SetFileName(tag.GetStr());
                                                  } else {
                                                    AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "Process2PublishKeyRequest : received TAG_FILENAME with empty string from %s"))
                                                    % dest.humanReadable());
                                                  }
                                                  strInfo += CFormat(wxT("  Name=\"%s\"")) % entry->GetCommonFileName();
                                                }
                                        } else if (tag. GetID() == TAG_FILESIZE) {
                                                if (entry->m_uSize == 0) {
                                                        entry->m_uSize = tag. GetInt();
                                                        strInfo += wxString::Format(wxT("  Size=%" PRIu64), entry->m_uSize);
                                                }
                                        } else {
                                                //TODO: Filter tags
                                                entry->AddTag(tag);
                                        }
                                }
                        }
                        if (!strInfo.IsEmpty()) {
                                AddDebugLogLineN(logClientKadUDP, strInfo);
                        }
                } catch(...) {
                        //DebugClientOutput(wxT("CKademliaUDPListener::Process2PublishKeyRequest"),ip,port,packetData,lenPacket);
                        delete entry;
                        throw;
                }

                if (!indexed->AddKeyword(file, target, entry, load)) {
                        //We already indexed the maximum number of keywords.
                        //We do not index anymore but we still send a success..
                        //Reason: Because if a VERY busy node tells the publisher it failed,
                        //this busy node will spread to all the surrounding nodes causing popular
                        //keywords to be stored on MANY nodes..
                        //So, once we are full, we will periodically clean our list until we can
                        //begin storing again..
                        delete entry;
                        entry = NULL;
                }
                count--;
        }
        CMemFile packetdata(17);
        packetdata.WriteUInt128(file);
        packetdata.WriteUInt8(load);
        DebugSend(Kad2PublishRes, dest);
        SendPacket(packetdata, KADEMLIA2_PUBLISH_RES, dest, senderKey, NULL);
}

// KADEMLIA2_PUBLISH_SOURCE_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2PublishSourceRequest(CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey)
{
        //Used Pointers
        CIndexed *indexed = CKademlia::GetIndexed();

        CMemFile & bio = packet;
        CUInt128 file = bio.ReadUInt128();

        CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
        distance.XOR(file);

        wxString strInfo;
        uint8_t load = 0;
        bool flag = false;
        CUInt128 target = bio.ReadUInt128();
        Kademlia::CEntry* entry = new Kademlia::CEntry();
        try {
                entry->m_udpdest = dest;
                entry->m_uKeyID.SetValue(file);
                entry->m_uSourceID.SetValue(target);
                entry->m_bSource = false;
                entry->m_tLifeTime = (uint32_t)time(NULL) + KADEMLIAREPUBLISHTIMES;
                bool addUDPPortTag = true;
                TagList taglist ( bio ) ;
                while ( !taglist.empty() ) {
                        CTag tag = taglist.back() ;
                        taglist.pop_back();

                        if ( tag.isValid() ) {
                                if (tag. GetID() == TAG_SOURCETYPE) {
                                        if (entry->m_bSource == false) {
                                                entry->AddTag( tag );
                                                if (addUDPPortTag) entry->AddTag( CTag ( TAG_SOURCEUDEST, entry->m_udpdest ) );
                                                entry->m_bSource = true;
                                                addUDPPortTag = false;
                                        }
                                } else if (tag. GetID() == TAG_FILESIZE) {
                                        if (entry->m_uSize == 0) {
                                                entry->m_uSize = tag. GetInt();
                                                strInfo += wxString::Format(wxT("  Size=%" PRIu64), entry->m_uSize);
                                        }
                                } else if (tag. GetID() == TAG_SOURCEDEST) {
                                        if (!entry->m_tcpdest.isValid()) {
                                                entry->m_tcpdest = tag. GetAddr();
                                                entry->AddTag(tag);
                                        }
                                } else if (tag. GetID() == TAG_SOURCEUDEST) {
                                        if (addUDPPortTag && tag. GetAddr().isValid() && addUDPPortTag) {
                                                entry->m_udpdest = tag. GetAddr();
                                                entry->AddTag(tag);
                                                addUDPPortTag = false;
                                        }
                                } else {
                                        //TODO: Filter tags
                                        entry->AddTag(tag);
                                }
                        }
                }
                if (addUDPPortTag && entry->m_bSource) {
                        entry->AddTag(CTag(TAG_SOURCEUDEST, entry->m_udpdest));
                }
                if (!strInfo.IsEmpty()) {
                        AddDebugLogLineN(logClientKadUDP, strInfo);
                }
        } catch(...) {
                //DebugClientOutput(wxT("CKademliaUDPListener::Process2PublishSourceRequest"),ip,port,packetData,lenPacket);
                delete entry;
                throw;
        }

        if (entry->m_bSource == true) {
                if (indexed->AddSources(file, target, entry, load)) {
                        flag = true;
                } else {
                        delete entry;
                        entry = NULL;
                }
        } else {
                delete entry;
                entry = NULL;
        }
        if (flag) {
                CMemFile packetdata(25);
                packetdata.WriteUInt128(file);
                packetdata.WriteUInt32 ( indexed->GetSourceCount ( file ) );
                packetdata.WriteUInt32 ( indexed->GetCompleteSourceCount ( file ) );
                packetdata.WriteUInt8(load);
                DebugSend(Kad2PublishRes, dest);
                SendPacket(packetdata, KADEMLIA2_PUBLISH_RES, dest, senderKey, NULL);
        }
}

// KADEMLIA_PUBLISH_RES
// Used only by Kad1.0
void CKademliaUDPListener::ProcessPublishResponse ( CMemFile & packet, const CI2PAddress & dest )
{
        // Verify packet is expected size
        CHECK_PACKET_MIN_SIZE(16);
        CHECK_TRACKED_PACKET(KADEMLIA_PUBLISH_REQ);
        CSearchManager::ProcessPublishResult ( packet, dest );
}

// KADEMLIA2_PUBLISH_RES
// Used only by Kad2.0
void CKademliaUDPListener::Process2PublishResponse(CMemFile & packet, const CI2PAddress & from, const CKadUDPKey& senderKey)
{
        if (!IsOnOutTrackList(from, KADEMLIA2_PUBLISH_KEY_REQ) && !IsOnOutTrackList(from, KADEMLIA2_PUBLISH_SOURCE_REQ) && !IsOnOutTrackList(from, KADEMLIA2_PUBLISH_NOTES_REQ)) {
                throw wxString::Format(wxT("***NOTE: Received unrequested response packet, size (%" PRIu64 ") in "), packet.GetLength()) + wxString::FromAscii(__FUNCTION__);
        }
        CSearchManager::ProcessPublishResult ( packet, from );
        CMemFile & bio = packet;
        if (bio.GetLength() > bio.GetPosition()) {
                // for future use
                uint8_t options = bio.ReadUInt8();
                bool requestACK = (options & 0x01) > 0;
                if (requestACK && !senderKey.IsEmpty()) {
                        DebugSend(Kad2PublishResAck, from);
                        SendNullPacket(KADEMLIA2_PUBLISH_RES_ACK, from, senderKey, NULL);
                }
        }
}

//KADEMLIA_SRC_NOTES_REQ
// Used only by Kad1.0
void CKademliaUDPListener::ProcessSearchNotesRequest ( CMemFile & packet, const CI2PAddress & dest )
{
        // Verify packet is expected size
        CHECK_PACKET_MIN_SIZE(32);

        CMemFile & bio = packet ;
        CUInt128 target = bio.ReadUInt128();
        // This info is currently not used.
// 	CUInt128 source = bio.ReadUInt128();

        CKademlia::GetIndexed()->SendValidNoteResult ( target, dest, false, 0, 0 );
}

// KADEMLIA2_SEARCH_NOTES_REQ
// Used only by Kad2.0
void CKademliaUDPListener::Process2SearchNotesRequest( CMemFile & bio, const CI2PAddress & dest, const CKadUDPKey& senderKey)
{
//     CMemFile bio(packetData, lenPacket);
        CUInt128 target = bio.ReadUInt128();
        uint64_t fileSize = bio.ReadUInt64();
        CKademlia::GetIndexed()->SendValidNoteResult(target, dest, true, fileSize, senderKey);
}

//KADEMLIA_PUB_NOTES_REQ
// Used only by Kad1.0
void CKademliaUDPListener::ProcessPublishNotesRequest ( CMemFile & packet, const CI2PAddress & dest, bool self )
{
        // Verify packet is expected size
        CHECK_PACKET_MIN_SIZE(37);

//     // check if we are UDP firewalled
//     if (CUDPFirewallTester::IsFirewalledUDP(true)) {
//         //We are firewalled. We should not index this entry and give publisher a false report.
//         return;
//     }

        CMemFile & bio = packet;
        CUInt128 target = bio.ReadUInt128();

        CUInt128 distance ( CKademlia::GetPrefs()->GetKadID() );
        distance.XOR ( target );

//     if( thePrefs::FilterLanIPs() && distance.Get32BitChunk(0) > SEARCHTOLERANCE) {
//         return;
//     }

        CUInt128 source = bio.ReadUInt128();

        Kademlia::CEntry* entry = new Kademlia::CEntry();
        try {
                entry->m_udpdest = dest;
                entry->m_uKeyID.SetValue(target);
                entry->m_uSourceID.SetValue(source);
                TagList taglist( bio );
                for (TagList::const_iterator itTag=taglist.begin(); itTag!=taglist.end(); itTag++) {
                        const CTag & tag = *itTag;
                        switch (tag.GetID()) {
                        case TAG_FILENAME :
                                if (entry->GetCommonFileName().IsEmpty()) {
					if (!tag.GetStr().IsEmpty()) {
						entry->SetFileName(tag.GetStr());
					} else {
						AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "ProcessPublishNotesRequest : received TAG_FILENAME with empty string from %s"))
							% dest.humanReadable());
					}
                                }
                                break ;
                        case TAG_FILESIZE :
                                if (entry->m_uSize == 0) {
                                        entry->m_uSize = tag.GetInt();
                                }
                                break ;
                        default :
                                //TODO: Filter tags
                                entry->AddTag(tag);
                        }
                }
                entry->m_bSource = false;
        } catch(...) {
                //DebugClientOutput(wxT("CKademliaUDPListener::processPublishNotesRequest"),ip,port,packetData,lenPacket);
                delete entry;
                entry = NULL;
                throw;
        }

        if (entry == NULL) {
                throw wxString(wxT("CKademliaUDPListener::processPublishNotesRequest: entry == NULL"));
        } else if (entry->GetTagCount() == 0 || entry->GetTagCount() > 5) {
                delete entry;
                throw wxString(wxT("CKademliaUDPListener::processPublishNotesRequest: entry->GetTagCount() == 0 || entry->GetTagCount() > 5"));
        }

        uint8_t load = 0;
        if (CKademlia::GetIndexed()->AddNotes(target, source, entry, load)) {
                if ( !self ) {
                        CMemFile packetdata(17);
                        packetdata.WriteUInt128(target);
                        packetdata.WriteUInt8(load);
                        DebugSend(KadPublishNotesRes, dest);
                        SendPacket(packetdata, KADEMLIA_PUBLISH_RES, dest, 0, NULL);
                }
        } else {
                delete entry;
        }

}

// KADEMLIA2_PUBLISH_NOTES_REQ
// Used only by Kad2.0
void CKademliaUDPListener::Process2PublishNotesRequest(CMemFile & packet, const CI2PAddress & dest, const CKadUDPKey& senderKey)
{
//     // check if we are UDP firewalled
//     if (CUDPFirewallTester::IsFirewalledUDP(true)) {
//         //We are firewalled. We should not index this entry and give publisher a false report.
//         return;
//     }

        CMemFile & bio = packet;
        CUInt128 target = bio.ReadUInt128();

        CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
        distance.XOR(target);

//     // Shouldn't LAN IPs already be filtered?
//     if (thePrefs::FilterLanIPs() && distance.Get32BitChunk(0) > SEARCHTOLERANCE) {
//         return;
//     }

        CUInt128 source = bio.ReadUInt128();

        Kademlia::CEntry* entry = new Kademlia::CEntry();
        try {
                entry->m_udpdest = dest;
                entry->m_uKeyID.SetValue(target);
                entry->m_uSourceID.SetValue(source);
                entry->m_bSource = false;
                TagList taglist( bio );
                for (TagList::const_iterator itTag=taglist.begin(); itTag!=taglist.end(); itTag++) {
                        const CTag & tag = *itTag;
                        switch (tag.GetID()) {
                        case TAG_FILENAME:
                                if (entry->GetCommonFileName().IsEmpty()) {
					if (!tag.GetStr().IsEmpty()) {
						entry->SetFileName(tag.GetStr());
					} else {
						AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "Process2PublishNotesRequest : received TAG_FILENAME with empty string from %s"))
							% dest.humanReadable());
					}
                                }
                                break;
                        case TAG_FILESIZE:
                                if (entry->m_uSize == 0) {
                                        entry->m_uSize = tag. GetInt();
                                }
                                break;
                        default:
                                //TODO: Filter tags
                                entry->AddTag(tag);
                        }
                }
        } catch(...) {
                //DebugClientOutput(wxT("CKademliaUDPListener::Process2PublishNotesRequest"),ip,port,packetData,lenPacket);
                delete entry;
                entry = NULL;
                throw;
        }

        uint8_t load = 0;
        if (CKademlia::GetIndexed()->AddNotes(target, source, entry, load)) {
                CMemFile packetdata(17);
                packetdata.WriteUInt128(target);
                packetdata.WriteUInt8(load);
                DebugSend(Kad2PublishRes, dest);
                SendPacket(packetdata, KADEMLIA2_PUBLISH_RES, dest, senderKey, NULL);
        } else {
                delete entry;
        }
}

// KADEMLIA2_PING
void CKademliaUDPListener::Process2Ping(const CI2PAddress & dest, const CKadUDPKey& senderKey)
{
        // can be used just as PING, currently it is however only used to determine one's external port
        CMemFile packetdata(0);
//     packetdata.WriteUInt16(port);
        DebugSend(Kad2Pong, dest);
        SendPacket(packetdata, KADEMLIA2_PONG, dest, senderKey, NULL);
}

// KADEMLIA2_PONG
void CKademliaUDPListener::Process2Pong(CMemFile & packet, const CI2PAddress & dest)
{
        CHECK_PACKET_MIN_SIZE(0);
        CHECK_TRACKED_PACKET(KADEMLIA2_PING);

        // Is this one of our legacy challenge packets?
        CUInt128 contactID;
        if (IsLegacyChallenge(CUInt128((uint32_t)0), dest, KADEMLIA2_PING, contactID)) {
                // yup it is, set the contact as verified
                if (!CKademlia::GetRoutingZone()->VerifyContact(contactID, dest)) {
                        AddDebugLogLineN(logKadRouting, wxT("Unable to find valid sender in routing table (sender: ")
                                         + dest.humanReadable() + wxT(")"));
                }
#ifdef __DEBUG__
                else {
                        AddDebugLogLineN(logClientKadUDP, wxT("Verified contact with legacy challenge (Kad2Ping) - ")
                                         + dest.humanReadable());
                }
#endif
                return; // we do not actually care for its other content
        }

        theApp->ShowConnectionState();
}




void CKademliaUDPListener::SendPacket ( const CMemFile & data, uint8_t opcode, const CI2PAddress & dest, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID )
{
        AddTrackedOutPacket(dest, opcode);
        std::unique_ptr<CPacket> packet(new CPacket ( data, OP_KADEMLIAHEADER, opcode ));

//     AddDebugLogLineN(logClientKadUDP, CFormat(wxT("CKademliaUDPListener::SendPacket (size=%dB)")) % data.GetLength() );

#ifdef __PACKET_SEND_DUMP__
        //bool deb = (opcode == KADEMLIA_PUBLISH_REQ) ;
        bool deb = packet->GetPacketSize() > 200 ;
        uint32_t siz = packet->GetPacketSize();
#endif

        if ( packet->GetPacketSize() > 200 ) {
                packet->PackPacket();
#ifdef __PACKET_SEND_DUMP__

                if ( siz > packet->GetPacketSize() ) {
                        printf ( "Paquet COMPRESSE envoyé : opcode = %x taille = %d\n", opcode, packet->GetPacketSize() );
                        DumpMem ( packet->GetDataBuffer(), packet->GetPacketSize() );
                }

#endif
        }

#ifdef __PACKET_SEND_DUMP__
        if ( false && siz <= packet->GetPacketSize() ) {
                printf ( "Paquet non compressé envoyé :\n" );
                DumpMem ( packet->GetPacket(), 100 );
        }

#endif

        uint8_t cryptData[16];
        uint8_t *cryptKey;
        if (cryptTargetID != NULL) {
                cryptKey = (uint8_t *)&cryptData;
                cryptTargetID->StoreCryptValue(cryptKey);
        } else {
                cryptKey = NULL;
        }
        theStats::AddUpOverheadKad ( packet->GetPacketSize() );
        theApp->clientudp->SendPacket(std::move(packet), dest, true, cryptKey, true, targetKey.GetKeyValue(theApp->GetPublicDest(false)));
}

bool CKademliaUDPListener::FindNodeIDByIP(CKadClientSearcher* requester, uint32_t tcphash, const CI2PAddress & udpdest)
{
        // send a hello packet to the given IP in order to get a HELLO_RES with the NodeID

        // we will drop support for Kad1 soon, so don't bother sending two packets in case we don't know if kad2 is supported
        // (if we know that it's not, this function isn't called in the first place)
        AddDebugLogLineN(logClientKadUDP, wxT("FindNodeIDByIP: Requesting NodeID from ") + udpdest.humanReadable() + wxT(" by sending Kad2HelloReq"));
        DebugSend(Kad2HelloReq, udpdest);
        SendMyDetails(KADEMLIA2_HELLO_REQ, udpdest, 1, 0, NULL, false); // todo: we send this unobfuscated, which is not perfect, see this can be avoided in the future
        FetchNodeID_Struct sRequest = { tcphash, ::GetTickCount() + SEC2MS(60), requester };
        m_fetchNodeIDRequests.push_back(sRequest);
        return true;
}

void CKademliaUDPListener::ExpireClientSearch(CKadClientSearcher* expireImmediately)
{
        uint32_t now = ::GetTickCount();
        for (FetchNodeIDList::iterator it = m_fetchNodeIDRequests.begin(); it != m_fetchNodeIDRequests.end();) {
                FetchNodeIDList::iterator it2 = it++;
                if (it2->requester == expireImmediately) {
                        m_fetchNodeIDRequests.erase(it2);
                } else if (it2->expire < now) {
                        it2->requester->KadSearchNodeIDByIPResult(KCSR_TIMEOUT, NULL);
                        m_fetchNodeIDRequests.erase(it2);
                }
        }
}

void CKademliaUDPListener::SendLegacyChallenge(const CI2PAddress & dest, const CUInt128& contactID, bool kad2)
{
        // We want to verify that a pre-0.49a contact is valid and not sent from a spoofed IP.
        // Because those versions don't support any direct validating, we send a KAD_REQ with a random ID,
        // which is our challenge. If we receive an answer packet for this request, we can be sure the
        // contact is not spoofed
#ifdef __DEBUG__
        CContact contact = CKademlia::GetRoutingZone()->GetContact(contactID);
        if (not contact.IsInvalid()) {
                if (contact. GetType() < 2) {
                        AddDebugLogLineN(logClientKadUDP, wxT("Sending challenge to a long known contact (should be verified already) - ") + dest.humanReadable());
                }
        } else {
                wxFAIL;
        }
#endif

        if (HasActiveLegacyChallenge(dest)) {
                // don't send more than one challenge at a time
                return;
        }
        CMemFile packetdata(kad2 ? 33 : 49);
        packetdata.WriteUInt8(KADEMLIA_FIND_VALUE);
        CUInt128 challenge;
        do
                challenge = GetRandomUint128();
        while (challenge == 0);

        // Put the target we want into the packet. This is our challenge
        packetdata.WriteUInt128(challenge);
        // Add the ID of the contact we are contacting for sanity checks on the other end.
        packetdata.WriteUInt128(contactID);
        if (kad2) {
                DebugSendF(wxT("Kad2Req(SendLegacyChallenge)"), dest);
                // those versions we send those requests to don't support encryption / obfuscation
                SendPacket(packetdata, KADEMLIA2_REQ, dest, 0, NULL);
                AddLegacyChallenge(contactID, challenge, dest, KADEMLIA2_REQ);
        } else {
                packetdata.WriteUInt128(CContact::Self().GetClientID()); // used in kad1(imule) to help bootstrap
                DebugSendF(wxT("KadReq(SendLegacyChallenge)"), dest);
                SendPacket(packetdata, KADEMLIA_REQ_DEPRECATED, dest, 0, NULL);
                AddLegacyChallenge(contactID, challenge, dest, KADEMLIA_REQ_DEPRECATED);
        }
}


void CKademliaUDPListener::DebugClientOutput ( const wxString& place, const CI2PAddress & dest )
{
        printf ( "Error on %s received from: %x\n", ( const char* ) unicode2char ( place ),/*(const char*)unicode2char(*/dest.hashCode() /*)*/ );
        CClientList::SourceList clientslist = theApp->clientlist->GetClientsByIP ( dest );
        if ( !clientslist.empty() ) {
                for ( CClientList::SourceList::iterator it = clientslist.begin(); it != clientslist.end(); ++it ) {
                        printf("Ip Matches: %s\n",(const char*)unicode2char(it->GetClientFullInfo()));
                }
        } else {
                printf ( "No ip match\n" );
        }
}
// File_checked_for_headers