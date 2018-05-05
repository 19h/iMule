//
// This file is part of imule Project
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne ( http://www.emule-project.net )
// Copyright (C)2007-2011 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )

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

// This work is based on the java implementation of the Kademlia protocol.
// Kademlia: Peer-to-peer routing based on the XOR metric
// Copyright (c) 2002-2011  Petar Maymounkov ( petar@post.harvard.edu )
// http://kademlia.scs.cs.nyu.edu

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

/**
 * The *Zone* is just a node in a binary tree of *Zone*s.
 * Each zone is either an internal node or a leaf node.
 * Internal nodes have "bin == null" and "subZones[i] != null",
 * leaf nodes have "subZones[i] == null" and "bin != null".
 *
 * All key unique id's are relative to the center (self), which
 * is considered to be 000..000
 */
#include "RoutingZone.h"

#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad2/Client2Client/UDP.h>
#include <common/Macros.h>

#include "Contact.h"
#include "RoutingBin.h"
#include "../kademlia/Defines.h"
#include "../kademlia/SearchManager.h"
#include "../kademlia/UDPFirewallTester.h"
#include "../net/KademliaUDPListener.h"
#include "../utils/KadUDPKey.h"
#include "../../amule.h"
#include "../../CFile.h"
#include "../../Logger.h"
#include "../../NetworkFunctions.h"
#include "../../IPFilter.h"
#include "../../RandomFunctions.h"

#include <cmath>

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

// This is just a safety precaution
#define CONTACT_FILE_LIMIT	500

wxString CRoutingZone::m_filename;
CUInt128 CRoutingZone::me((uint32_t)0);

ContactSet      CRoutingZone::s_ContactsSet ;

size_t CRoutingZone::s_nodes_dat_size = 0 ;
CRoutingZone::CRoutingZone()
{
        // Can only create routing zone after prefs
        // Set our KadID for creating the contact tree
        me = CKademlia::GetPrefs()->GetKadID();
        // Set the preference file name.
        m_filename = theApp->ConfigDir + wxT("nodes.dat");
        Init(NULL, 0, CUInt128((uint32_t)0));
}

void CRoutingZone::Init(CRoutingZone *super_zone, int level, const CUInt128& zone_index)
{
        // Init all Zone vars
        // Set this zone's parent
        m_superZone = super_zone;
        // Set this zone's level
        m_level = level;
        // Set this zone's CUInt128 index
        m_zoneIndex = zone_index;
        // Mark this zone as having no leafs.
        m_subZones[0] = NULL;
        m_subZones[1] = NULL;
        // Create a new contact bin as this is a leaf.
        m_bin = new CRoutingBin();

        // Set timer so that zones closer to the root are processed earlier.
        m_nextSmallTimer = time(NULL) + m_zoneIndex.Get32BitChunk(3);

        // Start this zone.
        StartTimer();

        // If we are initializing the root node, read in our saved contact list.
        if ((m_superZone == NULL) && (m_filename.Length() > 0)) {
                ReadFile();
        }
}

CRoutingZone::~CRoutingZone()
{
        // Root node is processed first so that we can write our contact list and delete all branches.
        if ((m_superZone == NULL) && (m_filename.Length() > 0)) {
                WriteFile();
                ContactSet toRemove = s_ContactsSet ;
                for ( ContactSet::iterator it=toRemove.begin(); it!=toRemove.end(); it++) {
                        CContact c(*it);
                        wxASSERT( c.InKadNodes() );
                        c.RemovedFromKadNodes();
                }
                wxASSERT( s_ContactsSet.size()==0 );
        }

        // If this zone is a leaf, delete our contact bin.
        if (IsLeaf()) {
                delete m_bin;
        } else {
                // If this zone is a branch, delete its leafs.
                delete m_subZones[0];
                delete m_subZones[1];
        }
}

void CRoutingZone::ReadFile(const wxString& specialNodesdat)
{
        AddDebugLogLineN(logKadRouting, _ ( "CRoutingZone::readFile called" ) );
        if (m_superZone != NULL || (m_filename.IsEmpty() && specialNodesdat.IsEmpty())) {
                wxFAIL;
                return;
        }

        bool doHaveVerifiedContacts = false;
        // Read in the saved contact list
        try {
                uint32_t numContacts = 0;
                uint32_t validContacts = 0;
                CFile file;
                if (CPath::FileExists(specialNodesdat.IsEmpty() ? m_filename : specialNodesdat) && file.Open(m_filename, CFile::read)) {
                        // Get how many contacts in the saved list.
                        // NOTE: Older clients put the number of contacts here...
                        //       Newer clients always have 0 here to prevent older clients from reading it.
                        numContacts = file.ReadUInt32();
                        uint32_t fileVersion = 0;
                        if (numContacts == 0) {
                                if (file.GetLength() >= 8) {
                                        fileVersion = file.ReadUInt32();
                                        if (fileVersion == 3) {
                                                uint32_t bootstrapEdition = file.ReadUInt32();
                                                if (bootstrapEdition == 1) {
                                                        // this is a special bootstrap-only nodes.dat, handle it in a separate reading function
                                                        ReadBootstrapNodesDat(file);
                                                        file.Close();
                                                        return;
                                                }
                                        }
                                        if (fileVersion >= 1 && fileVersion <= 3) {
                                                numContacts = file.ReadUInt32();
                                        }
                                }
                        }
                        //                         DEBUG_ONLY( unsigned kad1Count = 0; )
                        for (uint32_t i = 0; i < numContacts; i++) {
                                AddDebugLogLineN(logKadRouting, wxString::Format ( _ ( "Reading Kad contact %" PRIu32 "" ), i+1 ) );
                                try {
                                        CContact contact( file, fileVersion!=0, fileVersion );
                                        bool verified = contact.IsIPVerified();
                                        AddDebugLogLineN(logKadRouting, wxString::Format ( _ ( "Kad contact %" PRIu32 " read" ), i+1 ) );
                                        doHaveVerifiedContacts = doHaveVerifiedContacts or verified;

                                        if (contact.GetType() < 4) {
                                                if(contact . GetUDPDest().isValid()) {
                                                        if (!theApp->ipfilter->IsFiltered(contact.GetTCPDest())) {
                                                                // This was not a dead contact, inc counter if add was successful
                                                                if (contact.GetUDPDest()!=CContact::Self().GetUDPDest() && AddUnfiltered(contact, verified, false, true, false)) {
                                                                        AddDebugLogLineN(logKadRouting, wxString::Format ( _ ( "Added Kad contact %" PRIu32 "" ), i+1 ) );
                                                                        validContacts++;
                                                                }
                                                        }
                                                }
                                        }
                                } catch ( CContact::BadTypeException e ) {
                                        AddDebugLogLineC(logKadRouting, CFormat(_("Kad contact %u has a bad type (%s). Discarded.")) % (i+1) % e.what());
                                } catch ( CContact::ContactWithNullClientID e ) {
					AddDebugLogLineC(logKadRouting, CFormat(_("Kad contact %u has a null client id. Discarded. (%s)")) % (i+1) % e.what());
				}
                        }
                        file.Close();
                        AddDebugLogLineN( logKadRouting, CFormat(wxPLURAL("Read %u Kad contact", "Read %u Kad contacts", validContacts)) % validContacts);
                        if (!doHaveVerifiedContacts) {
                                AddDebugLogLineN(logKadRouting, wxT("No verified contacts found in nodes.dat - might be an old file version. Setting all contacts verified for this time to speed up Kad bootstrapping."));
                                SetAllContactsVerified();
                        }
                }
                if (validContacts == 0) {
                        AddLogLineC(_("No contacts found, please bootstrap, or download a nodes.dat file."));
                }
                s_nodes_dat_size = validContacts ;
        } catch (const CSafeIOException& e) {
                AddDebugLogLineN(logKadRouting, wxT("IO error in CRoutingZone::readFile: ") + e.what());
        } catch ( std::exception& e) {
                AddDebugLogLineN(logKadRouting, CFormat(wxT("Error in CRoutingZone::readFile: %s")) % wxString(e.what(),wxConvLocal));
        } catch (CMuleException e) {
                AddDebugLogLineC(logKadRouting, CFormat(wxT("Contact file %s is corrupted. You should download a new one if you have trouble connecting to Kad.\nError message : %s"))
                                 % m_filename % e.what());
        }
}

void CRoutingZone::ReadBootstrapNodesDat(CFileDataIO& file)
{
        // Bootstrap versions of nodes.dat files are in the style of version 1 nodes.dats. The difference is that
        // they will contain more contacts, 500-1000 instead of 50, and those contacts are not added into the routing table
        // but used to sent Bootstrap packets to. The advantage is that on a list with a high ratio of dead nodes,
        // we will be able to bootstrap faster than on a normal nodes.dat and more important, if we would deliver
        // a normal nodes.dat with eMule, those 50 nodes would be kinda DDOSed because everyone adds them to their routing
        // table, while with this style, we don't actually add any of the contacts to our routing table in the end and we
        // ask only one of those 1000 contacts one time (well or more until we find an alive one).
        if (!CKademlia::s_bootstrapList.empty()) {
                wxFAIL;
                return;
        }
        uint32_t numContacts = file.ReadUInt32();
        if (numContacts != 0) {
                uint32_t validContacts = 0;
                while (numContacts) {
                        CContact contact(file, true /*=withkadversion*/, 0 /*lecture de udpdest*/) ;

                        if (contact.GetUDPDest().isValid() && contact.GetUDPDest()!=CContact::Self().GetUDPDest()) {
                                if (!theApp->ipfilter->IsFiltered(contact.GetTCPDest()) && (contact.GetVersion() > 1)) { // only kad2 nodes
                                        // we want the 50 nodes closest to our own ID (provides randomness between different users and gets has good chances to get a bootstrap with close Nodes which is a nice start for our routing table)
                                        CUInt128 distance = contact.GetClientID() ^ me;
                                        validContacts++;
                                        // don't bother if we already have 50 and the farest distance is smaller than this contact
                                        if (CKademlia::s_bootstrapList.size() < 50 || CKademlia::s_bootstrapList.back().GetDistance() > distance) {
                                                // look where to put this contact into the proper position
                                                bool inserted = false;
                                                for (ContactList::iterator it = CKademlia::s_bootstrapList.begin(); it != CKademlia::s_bootstrapList.end(); ++it) {
                                                        if (it->GetDistance() > contact.GetDistance()) {
                                                                CKademlia::s_bootstrapList.insert(it, contact);
                                                                inserted = true;
                                                                break;
                                                        }
                                                }
                                                if (!inserted) {
                                                        wxASSERT(CKademlia::s_bootstrapList.size() < 50);
                                                        CKademlia::s_bootstrapList.push_back(contact);
                                                } else if (CKademlia::s_bootstrapList.size() > 50) {
                                                        CKademlia::s_bootstrapList.pop_back();
                                                }
                                        }
                                }
                        }
                        numContacts--;
                }
                AddLogLineN(CFormat(wxPLURAL("Read %u Kad contact", "Read %u Kad contacts", CKademlia::s_bootstrapList.size())) % CKademlia::s_bootstrapList.size());
                AddDebugLogLineN(logKadRouting, CFormat(wxT("Loaded Bootstrap nodes.dat, selected %u out of %u valid contacts")) % CKademlia::s_bootstrapList.size() % validContacts);
        }
        if (CKademlia::s_bootstrapList.size() == 0) {
                AddLogLineC(_("No contacts found, please bootstrap, or download a nodes.dat file."));
        }
}

wxString CRoutingZone::exportContacts(uint32 maxstrsize)
{
        unsigned int count = 0;
        CContact c;
        wxString ret;


        ContactList contacts;
        GetBootstrapContacts ( contacts, 200 );
        size_t contacts_size = contacts.size() ;
        if ( thePrefs::GetWriteMyselfToNodesDat() ) contacts_size++ ;

        AddDebugLogLineN(logKadRouting, CFormat ( _ ( "Export file : %d kad contacts, %d bootstrap contacts." ) )
                         % CContact::GetKadContacts().size()
                         % contacts_size );

        ContactList::const_iterator it;

        if ( thePrefs::GetWriteMyselfToNodesDat() ) {
                CContact myself = CContact::Self() ;
                ret += myself.GetUDPDest().toString()+wxT("\n") ;
                AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "Writing self kad info : %s" ) ) % myself.GetInfoString() );
        }

        for ( it = contacts.begin(); it != contacts.end(); ++it ) {
                c = *it;
                wxString cs = c.GetUDPDest().toString() + wxT("\n");
                if (ret.size() + cs.size() > maxstrsize) break ;
                ret += cs;
                count++;
                AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "Writing node kad info : %s" ) ) % c . GetInfoString() );
        }
        AddDebugLogLineN(logKadRouting, wxString::Format ( wxT ( "Wrote %u contacts to file." ), count ) );
        return ret;
}



void CRoutingZone::WriteFile ()
{
        // don't overwrite a bootstrap nodes.dat with an empty one, if we didn't finish probing
        if (!CKademlia::s_bootstrapList.empty() && GetNumContacts() == 0) {
                AddDebugLogLineN(logKadRouting, wxT("Skipped storing nodes.dat, because we have an unfinished bootstrap of the nodes.dat version and no contacts in our routing table"));
                return;
        }

        // The bootstrap method gets a very nice sample of contacts to save.
        ContactList contacts;
        GetBootstrapContacts(contacts, 200);
        ContactList::size_type numContacts = contacts.size();
        numContacts = std::min<ContactList::size_type>(numContacts, CONTACT_FILE_LIMIT); // safety precaution, should not be above
        if (numContacts < 25 && s_nodes_dat_size > numContacts) {
                AddLogLineN(CFormat(wxPLURAL("Only %d Kad contact available, nodes.dat not written", "Only %d Kad contacts available, nodes.dat not written", numContacts)) % numContacts);
                return;
        }
        if ( thePrefs::GetWriteMyselfToNodesDat() ) numContacts++ ;
        try {
                unsigned int count = 0;
                CContact c;
                CFile file;
                if (file.Open(m_filename, CFile::write_safe)) {
                        // Start file with 0 to prevent older clients from reading it.
                        file.WriteUInt32(0);
                        // Now tag it with a version which happens to be 2.
                        file.WriteUInt32(2);
                        // file.WriteUInt32(0); // if we would use version >= 3 this would mean that this is a normal nodes.dat
                        AddDebugLogLineN(logKadRouting, CFormat ( _ ( "Write file : %d kad contacts, %d bootstrap contacts." ) )
                                         % CContact::GetKadContacts().size()
                                         % numContacts );
                        file.WriteUInt32 ( (uint32_t) numContacts);
                        if ( thePrefs::GetWriteMyselfToNodesDat() ) {
                                count++;
                                CContact myself = CContact::Self() ;
                                myself.WriteToFile( file ) ;
                                AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "Writing self kad info : %s" ) ) % myself.GetInfoString() );
                        }
                        for ( ContactList::const_iterator it = contacts.begin(); count < numContacts; ++it, count++ ) {
                                it->WriteToFile( file ) ;
                                AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "Writing node kad info : %s" ) ) % it->GetInfoString() );
                        }
                        file.Close();
                }
                AddLogLineN(CFormat(wxPLURAL("Wrote %d Kad contact", "Wrote %d Kad contacts", count)) % count);
                s_nodes_dat_size = numContacts - (thePrefs::GetWriteMyselfToNodesDat() ? 1 : 0);
        } catch (const CIOFailureException& e) {
                AddDebugLogLineC(logKadRouting, wxT("IO failure in CRoutingZone::writeFile: ") + e.what());
        }
}

#if 0
void CRoutingZone::WriteBootstrapFile()
{
        AddDebugLogLineC(logKadRouting, wxT("Writing special bootstrap nodes.dat - not intended for normal use"));
        try {
                // Write a saved contact list.
                CUInt128 id;
                CFile file;
                if (file.Open(m_filename, CFile::write)) {
                        // The bootstrap method gets a very nice sample of contacts to save.
                        ContactMap mapContacts;
                        CUInt128 random(CUInt128((uint32_t)0), 0);
                        CUInt128 distance = random;
                        distance ^= me;
                        GetClosestTo(2, random, distance, 1200, &mapContacts, false, false);
                        // filter out Kad1 nodes
                        for (ContactMap::iterator it = mapContacts.begin(); it != mapContacts.end(); ) {
                                ContactMap::iterator itCur = it++;
                                CContact* contact = itCur->second;
                                if (contact->GetVersion() <= 1) {
                                        mapContacts.erase(itCur);
                                }
                        }
                        // Start file with 0 to prevent older clients from reading it.
                        file.WriteUInt32(0);
                        // Now tag it with a version which happens to be 3.
                        file.WriteUInt32(3);
                        file.WriteUInt32(1); // using version >= 3, this means that this is not a normal nodes.dat
                        file.WriteUInt32((uint32_t)mapContacts.size());
                        for (ContactMap::const_iterator it = mapContacts.begin(); it != mapContacts.end(); ++it) {
                                CContact* contact = it->second;
                                file.WriteUInt128(contact->GetClientID());
                                file.WriteUInt32(contact->GetIPAddress());
                                file.WriteUInt16(contact->GetUDPPort());
                                file.WriteUInt16(contact->GetTCPPort());
                                file.WriteUInt8(contact->GetVersion());
                        }
                        file.Close();
                        AddDebugLogLineN(logKadRouting, CFormat(wxT("Wrote %u contacts to bootstrap file.")) % mapContacts.size());
                } else {
                        AddDebugLogLineC(logKadRouting, wxT("Unable to store Kad file: ") + m_filename);
                }
        } catch (const CIOFailureException& e) {
                AddDebugLogLineC(logKadRouting, wxT("CFileException in CRoutingZone::writeFile") + e.what());
        }
}
#endif
bool CRoutingZone::CanSplit() const throw()
{
        // Max levels allowed.
        if (m_level >= 127) {
                return false;
        }

        // Check if this zone is allowed to split.
        return ((m_zoneIndex < KK || m_level < KBASE) && m_bin->GetSize() == K);
}

// Returns true if a contact was added or updated, false if the routing table was not touched.
bool CRoutingZone::Add(CContact contact, bool & destVerified, bool update, bool fromNodesDat, bool fromHello)
{
        if ( contact.GetUDPDest().isValid()) {
                if (!theApp->ipfilter->IsFiltered(contact.GetUDPDest())) {
                        return AddUnfiltered(contact, destVerified, update, fromNodesDat, fromHello);
                }
        }
        return false;
}

// Returns true if a contact was added or updated, false if the routing table was not touched.
bool CRoutingZone::AddUnfiltered(CContact contact, bool& destVerified, bool update, bool fromNodesDat, bool fromHello)
{
        if (contact . GetClientID() != me) {
                if (fromNodesDat) {
                        contact.CheckIfKad2(); // do not test nodes which we loaded from our nodes.dat for Kad2 again
                } else if (fromHello) {
                        contact.SetReceivedHelloPacket();
                }
                if (Add(contact, update, destVerified)) {
                        wxASSERT(!update);
                        return true;
                } else {
                        return update;
                }
        }
        return false;
}

bool CRoutingZone::Add(CContact contact, bool& update, bool& outDestVerified)
{
        // If we're not a leaf, call add on the correct branch.
        if (!IsLeaf()) {
                return m_subZones[contact. GetDistance().GetBitNumber(m_level)]->Add(contact, update, outDestVerified);
        } else {
                // Do we already have a contact with this KadID?
                CContact contactUpdate = m_bin->GetContact(contact. GetClientID());
                if (not contactUpdate.IsInvalid()) {
                        if (update) {
                                if (contactUpdate. GetUDPKey().GetKeyValue(theApp->GetPublicDest(false)) != 0 && contactUpdate. GetUDPKey().GetKeyValue(theApp->GetPublicDest(false)) != contact. GetUDPKey().GetKeyValue(theApp->GetPublicDest(false))) {
                                        // if our existing contact has a UDPSender-Key (which should be the case for all > = 0.49a clients)
                                        // except if our IP has changed recently, we demand that the key is the same as the key we received
                                        // from the packet which wants to update this contact in order to make sure this is not a try to
                                        // hijack this entry
                                        AddDebugLogLineN(logKadRouting, wxT("Sender (") + contact. GetUDPDest().humanReadable() + wxT(") tried to update contact entry but failed to provide the proper sender key (Sent Empty: ") + (contact. GetUDPKey().GetKeyValue(theApp->GetPublicDest(false)) == 0 ? wxT("Yes") : wxT("No")) + wxT(") for the entry (") + contactUpdate. GetUDPDest().humanReadable() + wxT(") - denying update"));
                                        update = false;
                                } else if (contactUpdate. GetVersion() >= 1 && contactUpdate. GetVersion() < 6 && contactUpdate. GetReceivedHelloPacket()) {
                                        // legacy kad2 contacts are allowed only to update their RefreshTimer to avoid having them hijacked/corrupted by an attacker
                                        // (kad1 contacts do not have this restriction as they might turn out as kad2 later on)
                                        // only exception is if we didn't receive a HELLO packet from this client yet
                                        if (contactUpdate. GetTCPDest() == contact. GetTCPDest() && contactUpdate. GetVersion() == contact. GetVersion() && contactUpdate. GetUDPDest() == contact. GetUDPDest()) {
                                                wxASSERT(!contact. IsIPVerified()); // legacy kad2 nodes should be unable to verify their IP on a HELLO
                                                outDestVerified = contactUpdate. IsIPVerified();
                                                m_bin->SetAlive(contactUpdate);
                                                AddDebugLogLineN(logKadRouting, wxString::Format(wxT("Updated kad contact refreshtimer only for legacy kad2 contact (") + contactUpdate. GetUDPDest().humanReadable() + wxT(", %" PRIu8 ")"), contactUpdate.GetVersion()));
                                        } else {
                                                AddDebugLogLineN(logKadRouting, wxString::Format(wxT("Rejected value update for legacy kad2 contact (") + contactUpdate. GetUDPDest().humanReadable() + wxT(" -> ") + contact. GetUDPDest().humanReadable() + wxT(", %" PRIu8 " -> %" PRIu8 ")"), contactUpdate.GetVersion(), contact.GetVersion()));
                                                update = false;
                                        }
                                } else {
#ifdef __DEBUG__
                                        // just for outlining, get removed anyway
                                        //debug logging stuff - remove later
                                        if (contact. GetUDPKey().GetKeyValue(theApp->GetPublicDest(false)) == 0) {
                                                if (contact. GetVersion() >= 6 && contact. GetType() < 2) {
                                                        AddDebugLogLineN(logKadRouting, wxT("Updating > 0.49a + type < 2 contact without valid key stored ") + contact. GetUDPDest().humanReadable());
                                                }
                                        } else {
                                                AddDebugLogLineN(logKadRouting, wxT("Updating contact, passed key check ") + contact. GetUDPDest().humanReadable());
                                        }

                                        if (contactUpdate. GetVersion() >= 1 && contactUpdate. GetVersion() < 6) {
                                                wxASSERT(!contactUpdate. GetReceivedHelloPacket());
                                                AddDebugLogLineN(logKadRouting, wxString::Format(wxT("Accepted update for legacy kad2 contact, because of first HELLO (") + contactUpdate. GetUDPDest().humanReadable() + wxT(" -> ") +contact. GetUDPDest().humanReadable() + wxT(", %" PRIu8 " -> %" PRIu8 ")"), contactUpdate.GetVersion(), contact.GetVersion()));
                                        }
#endif
                                        // All other nodes (Kad1, Kad2 > 0.49a with UDPKey checked or not set, first hello updates) are allowed to do full updates
                                        // do not let Kad1 responses overwrite Kad2 ones
                                        if (m_bin->ChangeContactIPAddress(&contactUpdate, contact. GetIPAddress()) && contact. GetVersion() >= contactUpdate. GetVersion()) {
                                                contactUpdate. SetUDPDest(contact. GetUDPDest());
                                                contactUpdate. SetTCPDest(contact. GetTCPDest());
                                                contactUpdate. SetVersion(contact. GetVersion());
                                                contactUpdate. SetUDPKey(contact. GetUDPKey());
                                                // don't unset the verified flag (will clear itself on ipchanges)
                                                if (!contactUpdate. IsIPVerified()) {
                                                        contactUpdate. SetIPVerified(contact. IsIPVerified());
                                                }
                                                outDestVerified = contactUpdate. IsIPVerified();
                                                m_bin->SetAlive(contactUpdate);
                                                if (contact. GetReceivedHelloPacket()) {
                                                        contactUpdate. SetReceivedHelloPacket();
                                                }
                                        } else {
                                                update = false;
                                        }
                                }
                        }
                        return false;
                } else if (m_bin->GetRemaining()) {
                        update = false;
                        // This bin is not full, so add the new contact
                        return m_bin->AddContact(contact);
                } else if (CanSplit()) {
                        // This bin was full and split, call add on the correct branch.
                        Split();
                        return m_subZones[contact. GetDistance().GetBitNumber(m_level)]->Add(contact, update, outDestVerified);
                } else {
                        update = false;
                        return false;
                }
        }
}

CContact CRoutingZone::GetContact(const CUInt128& id) const throw()
{
        if (IsLeaf()) {
                return m_bin->GetContact(id);
        } else {
                CUInt128 distance = CKademlia::GetPrefs()->GetKadID();
                distance ^= id;
                return m_subZones[distance.GetBitNumber(m_level)]->GetContact(id);
        }
}

CContact CRoutingZone::GetContact(uint32 dest, bool tcpPort) const throw()
{
        if (IsLeaf()) {
                return m_bin->GetContact(dest, tcpPort);
        } else {
                CContact contact = m_subZones[0]->GetContact(dest, tcpPort);
                return (contact.IsValid()) ? contact : m_subZones[1]->GetContact(dest, tcpPort);
        }
}

CContact CRoutingZone::GetRandomContact(uint32_t maxType, uint32_t minKadVersion) const
{
        if (IsLeaf()) {
                return m_bin->GetRandomContact(maxType, minKadVersion);
        } else {
                unsigned zone = GetRandomUint16() & 1 /* GetRandomUint16() % 2 */;
                CContact contact = m_subZones[zone]->GetRandomContact(maxType, minKadVersion);
                return (contact.IsValid()) ? contact : m_subZones[1 - zone]->GetRandomContact(maxType, minKadVersion);
        }
}

void CRoutingZone::GetClosestToTarget ( uint32_t maxType, const CUInt128 &distance, uint32_t maxRequired, TargetContactMap & result, bool emptyFirst ) const
{
        // If leaf zone, do it here
        if (IsLeaf()) {
                m_bin->GetClosestToTarget ( maxType, maxRequired, result, emptyFirst );
                return;
        }

        // otherwise, recurse in the closer-to-the-target subzone first
        int closer = distance.GetBitNumber(m_level);
        m_subZones[closer]->GetClosestToTarget ( maxType, distance, maxRequired, result, emptyFirst );

        // if still not enough tokens found, recurse in the other subzone too
        if ( result . size() < maxRequired ) {
                m_subZones[1-closer]->GetClosestToTarget ( maxType, distance, maxRequired, result, false );
        }
}

void CRoutingZone::GetAllEntries ( ContactList & result, bool emptyFirst ) const
{
        if (IsLeaf()) {
                m_bin->GetEntries(result, emptyFirst);
        } else {
                m_subZones[0]->GetAllEntries(result, emptyFirst);
                m_subZones[1]->GetAllEntries(result, false);
        }
}

void CRoutingZone::TopDepth ( int depth, ContactList & result, bool emptyFirst ) const
{
        if (IsLeaf()) {
                m_bin->GetEntries(result, emptyFirst);
        } else if (depth <= 0) {
                RandomBin(result, emptyFirst);
        } else {
                m_subZones[0]->TopDepth(depth-1, result, emptyFirst);
                m_subZones[1]->TopDepth(depth-1, result, false);
        }
}

void CRoutingZone::RandomBin ( ContactList & result, bool emptyFirst ) const
{
        if (IsLeaf()) {
                m_bin->GetEntries(result, emptyFirst);
        } else {
                m_subZones[rand()&1]->RandomBin(result, emptyFirst);
        }
}

uint32_t CRoutingZone::GetMaxDepth() const throw()
{
        if (IsLeaf()) {
                return 0;
        }
        return 1 + std::max(m_subZones[0]->GetMaxDepth(), m_subZones[1]->GetMaxDepth());
}

void CRoutingZone::Split()
{
        StopTimer();

        m_subZones[0] = GenSubZone(0);
        m_subZones[1] = GenSubZone(1);

        ContactList entries;
        m_bin->GetEntries(entries);
        delete m_bin;
        m_bin = NULL;

        for (ContactList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
                if (!m_subZones[it->GetDistance().GetBitNumber(m_level)]->m_bin->AddContact(*it)) {
                }
        }
}

uint32_t CRoutingZone::Consolidate()
{
        uint32_t mergeCount = 0;

        if (IsLeaf()) {
                return mergeCount;
        }

        wxASSERT(m_bin == NULL);

        if (!m_subZones[0]->IsLeaf()) {
                mergeCount += m_subZones[0]->Consolidate();
        }
        if (!m_subZones[1]->IsLeaf()) {
                mergeCount += m_subZones[1]->Consolidate();
        }

        if (m_subZones[0]->IsLeaf() && m_subZones[1]->IsLeaf() && GetNumContacts() < K / 2) {
                m_bin = new CRoutingBin();

                m_subZones[0]->StopTimer();
                m_subZones[1]->StopTimer();

                ContactList list0;
                ContactList list1;
                m_subZones[0]->m_bin->GetEntries(list0);
                m_subZones[1]->m_bin->GetEntries(list1);

                delete m_subZones[0];
                delete m_subZones[1];

                m_subZones[0] = NULL;
                m_subZones[1] = NULL;

                for (ContactList::const_iterator it = list0.begin(); it != list0.end(); ++it) {
                        m_bin->AddContact(*it);
                }
                for (ContactList::const_iterator it = list1.begin(); it != list1.end(); ++it) {
                        m_bin->AddContact(*it);
                }

                StartTimer();

                mergeCount++;
        }
        return mergeCount;
}

CRoutingZone *CRoutingZone::GenSubZone(unsigned side)
{
        wxASSERT(side <= 1);

        CUInt128 newIndex(m_zoneIndex);
        newIndex <<= 1;
        newIndex += side;
        return new CRoutingZone(this, m_level + 1, newIndex);
}

void CRoutingZone::StartTimer()
{
        //         time_t now = time ( NULL );
        // Start filling the tree, closest bins first.
        m_nextBigTimer = time(NULL) + SEC(10);
        CKademlia::AddEvent(this);
}

void CRoutingZone::StopTimer()
{
        CKademlia::RemoveEvent(this);
}

bool CRoutingZone::OnBigTimer() const
{
        if (IsLeaf() && (m_zoneIndex < KK || m_level < KBASE || m_bin->GetRemaining() >= (K * 0.8))) {
                RandomLookup();
                return true;
        }

        return false;
}

//This is used when we find a leaf and want to know what this sample looks like.
//We fall back two levels and take a sample to try to minimize any areas of the
//tree that will give very bad results.
uint32_t CRoutingZone::EstimateCount() const
{
        if (!IsLeaf()) {
                return 0;
        }

        if (m_level < KBASE) {
                return (uint32_t)(pow(2.0, (int)m_level) * K);
        }

        CRoutingZone* curZone = m_superZone->m_superZone->m_superZone;

        // Find out how full this part of the tree is.
        float modify = ((float)curZone->GetNumContacts()) / (float)(K * 2);

        // First calculate users assuming the tree is full.
        // Modify count by bin size.
        // Modify count by how full the tree is.

        // LowIDModififier
        // Modify count by assuming 20% of the users are firewalled and can't be a contact for < 0.49b nodes
        // Modify count by actual statistics of Firewalled ratio for >= 0.49b if we are not firewalled ourself
        // Modify count by 40% for >= 0.49b if we are firewalled ourself (the actual Firewalled count at this date on kad is 35-55%)
        const float firewalledModifyOld = 1.f;
        float firewalledModifyNew = 0;
        if (CUDPFirewallTester::IsFirewalledUDP(true)) {
                firewalledModifyNew = 1.40f;	// we are firewalled and can't get the real statistics, assume 40% firewalled >=0.49b nodes
        } else if (CKademlia::GetPrefs()->StatsGetFirewalledRatio(true) > 0) {
                firewalledModifyNew = 1.0 + (CKademlia::GetPrefs()->StatsGetFirewalledRatio(true));	// apply the firewalled ratio to the modify
                wxASSERT(firewalledModifyNew > 1.0 && firewalledModifyNew < 1.90);
        }
        float newRatio = CKademlia::GetPrefs()->StatsGetKadV8Ratio();
        float firewalledModifyTotal = 0;
        if (newRatio > 0 && firewalledModifyNew > 0) {	// weight the old and the new modifier based on how many new contacts we have
                firewalledModifyTotal = (newRatio * firewalledModifyNew) + ((1 - newRatio) * firewalledModifyOld);
        } else {
                firewalledModifyTotal = firewalledModifyOld;
        }
        wxASSERT(firewalledModifyTotal >= 1.0 && firewalledModifyTotal < 1.90);

        return (uint32_t)(pow(2.0, (int)m_level - 2) * (float)K * modify * firewalledModifyTotal);
}

void CRoutingZone::OnSmallTimer()
{
        if (!IsLeaf()) {
                return;
        }

        CContact c ;
        time_t now = time(NULL);
        ContactList entries;

        // Remove dead entries
        m_bin->GetEntries(entries);
        for (ContactList::iterator it = entries.begin(); it != entries.end(); ++it) {
                c = *it;
                AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "OnSmallTimer examining contact %s" ) )
                                 % c . GetUDPDest().humanReadable() );
                if (c.GetType()==4 && CRoutingZone::s_ContactsSet.size() > thePrefs::GetKeepKadContacts()) {
                        if ((c.GetExpireTime() > 0) && (c.GetExpireTime() <= now)) {
                                m_bin->RemoveContact(c);
                                continue;
                        }
                }
                if (c.GetExpireTime() == 0) {
                        c.SetExpireTime(now);
                        AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "SetExpireTime of %s to %ld" ) )
                                         % c . GetUDPDest().humanReadable() % c . GetExpireTime() );
                }
        }

        c = m_bin->GetOldest();
        if (c.IsValid()) {
                if (c.GetExpireTime() >= now || c.GetType() == 4) {
                        AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "GetOldest returned %s (type %u) who expires in %lds" ) )
                                         % c . GetUDPDest().humanReadable() % c.GetType() % (c.GetExpireTime()-now));
                        m_bin->PushToBottom(c);
                        c = CContact();
                }
        }

        if (c.IsValid()) {
                AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "ping %s (version %u, type %u) with KadHelloReq to keep it alive (onSmallTimer of routing zone)" ) )
                                 % c . GetUDPDest().humanReadable()  % c.GetVersion() % c.GetType());
                c . CheckingType();
                if (c.GetVersion() >= 6) {
                        DebugSend(Kad2HelloReq, c.GetUDPDest());
                        CUInt128 clientID = c.GetClientID();
                        CKademlia::GetUDPListener()->SendMyDetails(KADEMLIA2_HELLO_REQ, c.GetUDPDest(), c.GetVersion(), c.GetUDPKey(), &clientID, false);
                        if (c.GetVersion() >= 8) {
                                // FIXME:
                                // This is a bit of a work around for statistic values. Normally we only count values from incoming HELLO_REQs for
                                // the firewalled statistics in order to get numbers from nodes which have us on their routing table,
                                // however if we send a HELLO due to the timer, the remote node won't send a HELLO_REQ itself anymore (but
                                // a HELLO_RES which we don't count), so count those statistics here. This isn't really accurate, but it should
                                // do fair enough. Maybe improve it later for example by putting a flag into the contact and make the answer count
                                CKademlia::GetPrefs()->StatsIncUDPFirewalledNodes(false);
                                CKademlia::GetPrefs()->StatsIncTCPFirewalledNodes(false);
                        }
                } else if (c.GetVersion() >= 2) {
                        DebugSend(Kad2HelloReq, c.GetUDPDest());
                        CKademlia::GetUDPListener()->SendMyDetails(KADEMLIA2_HELLO_REQ, c.GetUDPDest(), c.GetVersion(), 0, NULL, false);
                        wxASSERT(c.GetUDPKey() == CKadUDPKey(0));
                } else {
                        DebugSend(KadHelloReq, c.GetUDPDest());
                        CKademlia::GetUDPListener()->SendMyDetails(KADEMLIA_HELLO_REQ_DEPRECATED, c.GetUDPDest(), 0, 0, NULL, false);
                        if (c.CheckIfKad2()) {
                                DebugSend(Kad2HelloReq, c.GetUDPDest());
                                CKademlia::GetUDPListener()->SendMyDetails(KADEMLIA2_HELLO_REQ, c.GetUDPDest(), 1, 0, NULL, false);
                        }
                }
        }
}

void CRoutingZone::RandomLookup() const
{
        // Look-up a random client in this zone
        CUInt128 prefix(m_zoneIndex);
        prefix <<= 128 - m_level;
        CUInt128 random(prefix, m_level);
        random ^= me;
        CSearchManager::FindNode(random, false);
}

uint32_t CRoutingZone::GetNumContacts() const throw()
{
        if (IsLeaf()) {
                return m_bin->GetSize();
        } else {
                return m_subZones[0]->GetNumContacts() + m_subZones[1]->GetNumContacts();
        }
}

void CRoutingZone::GetNumContacts(uint32_t& nInOutContacts, uint32_t& nInOutFilteredContacts, uint8_t minVersion) const throw()
{
        if (IsLeaf()) {
                m_bin->GetNumContacts(nInOutContacts, nInOutFilteredContacts, minVersion);
        } else {
                m_subZones[0]->GetNumContacts(nInOutContacts, nInOutFilteredContacts, minVersion);
                m_subZones[1]->GetNumContacts(nInOutContacts, nInOutFilteredContacts, minVersion);
        }
}

uint32_t CRoutingZone::GetBootstrapContacts(ContactList &results, uint32_t maxRequired) const
{
        wxASSERT(m_superZone == NULL);

        results.clear();

        uint32_t count = 0;
        ContactList top;
        TopDepth(LOG_BASE_EXPONENT, top);
        if (top.size() > 0) {
                for (ContactList::const_iterator it = top.begin(); it != top.end(); ++it) {
                        results.push_back(*it);
                        count++;
                        if (count == maxRequired) {
                                break;
                        }
                }
        }

        return count;
}

bool CRoutingZone::VerifyContact(const CUInt128& id, const CI2PAddress & dest)
{
        CContact contact = GetContact(id);
        if (contact.IsInvalid()) {
                return false;
        } else if (dest != contact.GetUDPDest()) {
                return false;
        } else {
                if (contact.IsIPVerified()) {
                        AddDebugLogLineN(logKadRouting, wxT("Sender already verified (sender: ") + dest.humanReadable() + wxT(")"));
                } else {
                        contact.SetIPVerified(true);
                }
                return true;
        }
}

void CRoutingZone::SetAllContactsVerified()
{
        if (IsLeaf()) {
                m_bin->SetAllContactsVerified();
        } else {
                m_subZones[0]->SetAllContactsVerified();
                m_subZones[1]->SetAllContactsVerified();
        }
}

bool CRoutingZone::IsAcceptableContact(const CContact toCheck) const
{
        // Check if we know a contact with the same ID or IP but notmatching IP/ID and other limitations, similar checks like when adding a node to the table except allowing duplicates
        // we use this to check KADEMLIA_RES routing answers on searches
        CContact duplicate = GetContact(toCheck.GetClientID());
        if (duplicate.IsValid()) {
                if (duplicate.IsIPVerified() && duplicate.GetUDPDest() != toCheck.GetUDPDest() ) {
                        // already existing verified node with different IP
                        return false;
                } else {
                        // node exists already in our routing table, that's fine
                        return true;
                }
        }
        // if the node is not yet known, check if our IP limitations would hit
        return CRoutingBin::CheckGlobalIPLimits(toCheck.GetUDPDest());
}

bool CRoutingZone::HasOnlyLANNodes() const throw()
{
        if (IsLeaf()) {
                return m_bin->HasOnlyLANNodes();
        } else {
                return m_subZones[0]->HasOnlyLANNodes() && m_subZones[1]->HasOnlyLANNodes();
        }
}

bool CRoutingZone::wouldLikeToAdd ( const CUInt128 &id ) const
{
        if ( id == me ) return false;

        CUInt128 distance ( me );

        distance.XOR ( id );

        return wouldLikeToAddByDistance ( distance, id );
}




bool CRoutingZone::wouldLikeToAddByDistance ( const CUInt128 &distance, const CUInt128 &id ) const

{
        if ( !IsLeaf() ) {
                return m_subZones[distance.GetBitNumber ( m_level ) ]->wouldLikeToAddByDistance ( distance, id );
        } else {
                if ( ! m_bin->GetContact ( id ) . IsInvalid() ) {
                        return false;
                } else if ( m_bin->GetRemaining() > 0 ) {
                        return true;
                } else if ( CanSplit() ) {
                        return true;
                } else if ( m_superZone != NULL ) {
                        if ( m_superZone->m_subZones[0]->IsLeaf()
                                        && m_superZone->m_subZones[1]->IsLeaf()
                                        && ( m_superZone->GetNumContacts() < K / 2 ) )
                                // merge would be possible
                                return true;
                }
        }

        return false;
}

void CRoutingZone::remove ( const CUInt128 &id )
{
        CUInt128 distance ( me ^ id );
        if ( !IsLeaf() ) {
                m_subZones[distance.GetBitNumber(m_level)]->remove(id);
        } else {
                CContact c = m_bin->GetContact (id);
                if (c.IsValid()) {
                        m_bin->RemoveContact(c);
                }
        }
}

