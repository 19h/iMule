//
// This file is part of imule Project
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne ( http://www.emule-project.net )

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

#include "RoutingBin.h"
#include "../../Logger.h"
#include "../../NetworkFunctions.h"
#include "../../RandomFunctions.h"
#include "../routing/RoutingZone.h" // for debug messages
#include "kademlia/kademlia/Search.h"

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CRoutingBin::GlobalTrackingMap	CRoutingBin::s_globalContactIPs;

#define MAX_CONTACTS_IP		1

CRoutingBin::~CRoutingBin()
{
        for (ContactList::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
                AdjustGlobalTracking((*it).GetIPAddress(), false);
                if (!m_dontDeleteContacts) {
                        // delete *it;
                }
        }
}

bool CRoutingBin::AddContact ( CContact contact )
{
        wxASSERT ( !contact.IsInvalid() );

        //         bool retVal = false;
        // Look first in the entry list
        CContact c = GetContact ( contact . GetClientID() );
        if (c.IsValid()) {
                return false;
        }
        
        // Several checks to make sure that we don't store multiple contacts from the same IP or too many contacts from the same subnet
        // This is supposed to add a bit of protection against several attacks and raise the resource needs (IPs) for a successful contact on the attacker side
        // Such IPs are not banned from Kad, they still can index, search, etc so multiple KAD clients behind one IP still work

        if (!CheckGlobalIPLimits(contact.GetIPAddress())) {
                return false;
        }

        // If not full, add to the end of list
        if (m_entries.size() < K) {
                AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "RoutingBin %lld : AddContact: push_back contact %s with type %d, %d remaining before" ) )
                                                        % (uint64_t)this % contact.GetInfoString() % contact.GetType() % CRoutingZone::s_ContactsSet.size() );
                m_entries.push_back(contact);
                AdjustGlobalTracking(contact.GetIPAddress(), true);
                contact.AddedToKadNodes();
                return true;
        }
        return false;
}

void CRoutingBin::SetAlive (CContact contact)
{
        wxASSERT(!contact.IsInvalid());
        // Check if we already have a contact with this ID in the list.
        CContact test = GetContact(contact.GetClientID());
        wxASSERT(contact == test);
        if (!test.IsInvalid()) {
                // Mark contact as being alive.
                test.UpdateType();
                // Move to the end of the list
                PushToBottom(test);
        }
}

void CRoutingBin::SetTCPPort ( const CI2PAddress & dest, const CI2PAddress & tcpDest )
{
        if ( m_entries.empty() ) {
                return;
        }

        CContact c;

        ContactList::iterator it;

        for ( it = m_entries.begin(); it != m_entries.end(); ++it ) {
                c = *it;

                if ( ( dest == c . GetUDPDest() ) ) {
                        c . SetTCPDest ( tcpDest );
                        c . UpdateType();
                        // Move to the end of the list
                        m_entries.remove ( c );
                        m_entries.push_back ( c );
                        break;
                }
        }
}

CContact CRoutingBin::GetContact(const CUInt128& id) const throw()
{
        CContact retVal ;
        ContactList::const_iterator it;

        for ( it = m_entries.begin(); it != m_entries.end(); ++it ) {
                if ( it->GetClientID() == id ) {
                        retVal = *it;
                        break;
                }
        }

        return retVal;
}

CContact CRoutingBin::GetContact ( uint32 dest, bool tcpPort ) const throw()

{
        for (ContactList::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
                CContact contact = *it;
                if ((!tcpPort && dest == contact.GetUDPDest()) || (tcpPort && dest == contact.GetTCPDest())) {
                        return contact;
                }
        }
        return CContact();
}

void CRoutingBin::GetNumContacts(uint32_t& nInOutContacts, uint32_t& nInOutFilteredContacts, uint8_t minVersion) const throw()
{
        // count all nodes which meet the search criteria and also report those who don't
        for (ContactList::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
                if (it->GetVersion() >= minVersion) {
                        nInOutContacts++;
                } else {
                        nInOutFilteredContacts++;
                }
        }
}

void CRoutingBin::GetEntries ( ContactList & result, bool emptyFirst ) const
{
        if ( emptyFirst ) {
                result . clear();
        }

        if ( m_entries.size() > 0 ) {
                result . insert ( result . end(), m_entries.begin(), m_entries.end() );
        }
}


void CRoutingBin::GetClosestToTarget ( uint32_t maxType, uint32_t maxRequired, TargetContactMap & result, bool emptyFirst ) const
{
        // If we have to clear the bin, do it now.

        if ( emptyFirst ) {
                result . clear();
        }

        // No entries, no closest.
        if ( m_entries.size() == 0 ) {
                return;
        }

        // First put results in sort order for target so we can insert them correctly.
        // We don't care about max results at this time.
        for ( ContactList::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it ) {
                if ( it->GetType() <= maxType && it->IsIPVerified()) {
                        result.add(*it);
                }
        }

        // Remove any extra results by least wanted first.
        while ( result . size() > maxRequired ) {
                // Remove from results
                result . pop_back();
        }
}


void CRoutingBin::AdjustGlobalTracking(const CI2PAddress & ip, bool increase)
{
        // IP
        uint32_t sameIPCount = 0;
        GlobalTrackingMap::const_iterator itIP = s_globalContactIPs.find(ip);
        if (itIP != s_globalContactIPs.end()) {
                sameIPCount = itIP->second;
        }
        if (increase) {
                if (sameIPCount >= MAX_CONTACTS_IP) {
                        AddDebugLogLineN(logKadRouting, wxT("Global IP Tracking inconsistency on increase (") + ip.humanReadable() + wxT(")"));
                        wxFAIL;
                }
                sameIPCount++;
        } else { /* if (!increase) */
                if (sameIPCount == 0) {
                        AddDebugLogLineN(logKadRouting, wxT("Global IP Tracking inconsistency on decrease (") + ip.humanReadable() + wxT(")"));
                        wxFAIL;
                }
                sameIPCount--;
        }
        if (sameIPCount != 0) {
                s_globalContactIPs[ip] = sameIPCount;
        } else {
                s_globalContactIPs.erase(ip);
        }

}

bool CRoutingBin::ChangeContactIPAddress(CContact *contact, const CI2PAddress & newIP)
{
        // Called if we want to update an indexed contact with a new IP. We have to check if we actually allow such a change
        // and if adjust our tracking. Rejecting a change will in the worst case lead a node contact to become invalid and purged later,
        // but it also protects against a flood of malicous update requests from one IP which would be able to "reroute" all
        // contacts to itself and by that making them useless
        if (contact->GetIPAddress() == newIP) {
                return true;
        }

        // iMule: GetContact does not return a pointer // wxASSERT(GetContact(contact->GetClientID()) == contact);

        // no more than 1 KadID per IP
        uint32_t sameIPCount = 0;
        GlobalTrackingMap::const_iterator itIP = s_globalContactIPs.find(newIP);
        if (itIP != s_globalContactIPs.end()) {
                sameIPCount = itIP->second;
        }
        if (sameIPCount >= MAX_CONTACTS_IP) {
                AddDebugLogLineN(logKadRouting, wxT("Rejected kad contact IP change on update (old IP=") + contact->GetIPAddress().humanReadable() + wxT(", requested IP=") + newIP.humanReadable() + wxT(") - too many contacts with the same IP (global)"));
                return false;
        }

        // everything fine
        AddDebugLogLineN(logKadRouting, wxT("Index contact IP change allowed ") + contact->GetIPAddress().humanReadable() + wxT(" -> ") + newIP.humanReadable());
        AdjustGlobalTracking(contact->GetIPAddress(), false);
        contact->SetIPAddress(newIP);
        AdjustGlobalTracking(contact->GetIPAddress(), true);
        return true;
}

void CRoutingBin::PushToBottom(CContact contact)
{
        wxASSERT(GetContact(contact.GetClientID()) == contact);

        m_entries.remove(contact);
        m_entries.push_back(contact);
}

CContact CRoutingBin::GetRandomContact(uint32_t maxType, uint32_t minKadVersion) const
{
        if (m_entries.empty()) {
                return CContact();
        }

        // Find contact
        CContact lastFit;
        uint32_t randomStartPos = GetRandomUint16() % m_entries.size();
        uint32_t index = 0;

        for (ContactList::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
                if ((it)->GetType() <= maxType && (it)->GetVersion() >= minKadVersion) {
                        if (index >= randomStartPos) {
                                return *it;
                        } else {
                                lastFit = *it;
                        }
                }
                index++;
        }

        return lastFit;
}


void CRoutingBin::SetAllContactsVerified()
{
        for (ContactList::iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
                it->SetIPVerified(true);
        }
}


bool CRoutingBin::CheckGlobalIPLimits(const CI2PAddress & ip)
{
        // no more than 1 KadID per IP
        uint32_t sameIPCount = 0;
        GlobalTrackingMap::const_iterator itIP = s_globalContactIPs.find(ip);
        if (itIP != s_globalContactIPs.end()) {
                sameIPCount = itIP->second;
        }
        if (sameIPCount >= MAX_CONTACTS_IP) {
                AddDebugLogLineN(logKadRouting, wxT("Ignored kad contact (IP=") + ip.humanReadable() + wxT(") - too many contacts with the same IP (global)"));
                return false;
        }
        return true;
}
bool CRoutingBin::HasOnlyLANNodes() const throw()
{
        for (ContactList::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
                if (!::IsLanIP(it->GetIPAddress())) {
                        return false;
                }
        }
        return true;
}
