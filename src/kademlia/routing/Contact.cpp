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
// Copyright (c) 2002  Petar Maymounkov ( petar@post.harvard.edu )
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

#include "Contact.h"

#include "kademlia/kademlia/Kademlia.h"

#include "RoutingZone.h"
#include "../../amule.h"
#include "../../Statistics.h"
#include <Logger.h>
#include <common/Format.h>
#include <common/Macros.h>

#include <protocol/kad2/Constants.h>
#include "GuiEvents.h"			// needed for Notify_KadNode_Added
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include <time.h>

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

std::set<CContact> CContact::s_kadContacts ;

void CContact::initContact()
{
        GetData()->m_type = 3;
        GetData()->m_lastTypeSet = std::time(NULL);
        GetData()->m_expires = 0;
        GetData()->m_created = std::time(NULL);
        GetData()->m_lastTypeUpdate = 0;
        GetData()->m_inKadNodes = false;
        GetData()->m_checkKad2 = true;
        GetData()->m_receivedHelloPacket = false;
        GetData()->m_version = 0;
}

CContact::CContact()
{
}

#ifndef CLIENT_GUI
CContact CContact::Self()
{
        static CContact me;
        if ( me.IsInvalid() ) me.m_data.reset( new Data() );
        //wiMutexLocker lock( me.GetDataMutex() );
        me.initContact();
        me.GetData()->m_clientID = CKademlia::GetPrefs()->GetKadID() ;
        me.GetData()->m_udpDest  = theApp->GetUdpDest();
        me.GetData()->m_tcpDest  = theApp->GetTcpDest();
        me.GetData()->m_distance = 0 ;
        me.GetData()->m_type     = 0 ;
        me.GetData()->m_version  = KADEMLIA_VERSION ;
        return me ;
}
#endif

bool CContact::IsInvalid() const
{
        return not GetData();
}


bool CContact::operator< (const CContact & other) const
{
        return GetData()->m_clientID < other.GetData()->m_clientID ;
}

bool CContact::operator==(const CContact & other) const
{
        return GetData()->m_clientID == other.GetData()->m_clientID ;
}

#ifndef CLIENT_GUI
CContact::CContact ( const CUInt128 &clientID, const CI2PAddress & udpDest, const CI2PAddress & tcpDest, uint8_t version,
                     const CKadUDPKey& kadKey, bool destVerified,
                     const CUInt128 &target)
{
        m_data.reset( new Data() );
        initContact();
        SetClientID(clientID);
        GetData()->m_distance = GetData()->m_clientID ^ target ;
        SetUDPDest(udpDest);
        SetTCPDest(tcpDest);
        SetVersion(version);
        SetIPVerified(destVerified);
        SetUDPKey(kadKey);
}

void CContact::SetClientID(const CUInt128& clientID) throw()      
{ 
        GetData()->m_clientID = clientID;
        GetData()->m_distance = CKademlia::GetPrefs()->GetKadID() ^ clientID;
}

CContact::CContact ( CFileDataIO & file, bool withkadversion, int8_t fileversion )
{
        wxASSERT( fileversion!=0 || !withkadversion );
        m_data.reset( new Data() );
        //wiMutexLocker lock( GetDataMutex() );
        initContact();

        if (withkadversion) {
                SetVersion ( file.ReadUInt8()   ) ;
                SetClientID( file.ReadUInt128() );
                SetUDPDest ( file.ReadAddress() );
// 		SetTCPDest ( file.ReadAddress() );
                if (fileversion>=2) {
                        GetData()->m_udpKey.ReadFromFile(file);
                        SetIPVerified( file.ReadUInt8()!=0 );
                }
        } else {
                SetClientID( file.ReadUInt128() );
                SetUDPDest ( file.ReadAddress() );
                SetTCPDest ( file.ReadAddress() );
                file.ReadUInt8()  ; // it was GetData()->m_type=file.ReadUInt8, but it seems better to force type=3 when adding new contact
        }

        AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "CContact::CContact(stream) -> %s") )
                         % GetInfoString() );
	if (GetVersion() > 8) { throw BadVersionException(GetVersion());}
	if (GetClientID()==CUInt128((uint32)0)) { throw ContactWithNullClientID(GetTCPDest()); }
}
#endif

void  CContact::WriteToFile( CFileDataIO & file ) const
{
        file.WriteUInt8     ( GetVersion() );
        file.WriteUInt128   ( GetClientID() );
        file.WriteAddress   ( GetUDPDest() );
// 	file.WriteAddress   ( GetTCPDest() );
        GetUDPKey().StoreToFile(file);
        file.WriteUInt8     ( IsIPVerified() ? 1 : 0);
}

void  CContact::WriteToKad1Contact( CFileDataIO & file ) const
{
        file.WriteUInt128   ( GetClientID() );
        file.WriteAddress   ( GetUDPDest()  );
        file.WriteAddress   ( GetTCPDest()  );
        file.WriteUInt8     ( GetType()     );
}

void  CContact::WriteToKad2Contact( CFileDataIO & file ) const
{
        file.WriteUInt8     ( GetVersion() );
        file.WriteUInt128   ( GetClientID() );
        file.WriteAddress   ( GetUDPDest() );
//     file.WriteAddress   ( GetTCPDest() );
}

#ifndef CLIENT_GUI
void CContact::AddedToKadNodes()
{
        if (!GetData()->m_inKadNodes) {
                AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "Adding contact %s, with type %d, %d remaining" ) )
                                 % GetInfoString() % GetType() % CRoutingZone::s_ContactsSet.size() );
                GetData()->m_inKadNodes = true ;
                CRoutingZone::s_ContactsSet . insert( *this ) ;
                AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "Added contact %s, with type %d, %d remaining" ) )
                                 % GetInfoString() % GetType() % CRoutingZone::s_ContactsSet.size() );
                theStats::AddKadNode();
                Notify_KadNode_Added(*this);
        }
}

void CContact::RemovedFromKadNodes()
{
        wxASSERT( GetData()->m_inKadNodes );
        wxASSERT( CRoutingZone::s_ContactsSet . count( *this ) );
        {
                //wiMutexLocker lock( GetDataMutex() );
                GetData()->m_inKadNodes = false ;
        }
        theStats::RemoveKadNode();
        Notify_KadNode_Deleted(*this);

        CRoutingZone::s_ContactsSet . erase( *this ) ;

        AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "Contact %s removed, %d remaining" ) )
                         % GetInfoString() % CRoutingZone::s_ContactsSet.size() );
}
#endif

const wxString CContact::GetInfoString( void ) const
{
        return CFormat( wxT("(kad: %s) (dist: %s) (udp: %s) (tcp: %s) (type: %u) (version %u)") )
               % GetData()->m_clientID.ToBinaryString().Mid(0,12)
               % GetData()->m_distance.ToBinaryString().Mid(0,12)
               % GetUDPDest().humanReadable()
               % GetTCPDest().humanReadable()
               % GetType()
               % GetVersion();
}





//! Increments m_type up to 4 (dead contact) (processed max every 10 seconds)
//! and gives at least a 2mn delay to the contact to answer.
#ifndef CLIENT_GUI
void CContact::CheckingType() throw()
{
        if (!GetData()->m_inKadNodes)
                return ;

        if(std::time(NULL) - GetData()->m_lastTypeSet < 10 || GetData()->m_type == 4) {
                return;
        }
        {
                //wiMutexLocker lock( GetDataMutex() );
                GetData()->m_lastTypeSet = std::time(NULL);
        }
        if (GetData()->m_expires < GetData()->m_lastTypeSet) GetData()->m_expires = GetData()->m_lastTypeSet + MIN2S(2);
        GetData()->m_type++;
        AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "checking type of contact %s : type set to %d" ) )
                         % GetData()->m_udpDest.humanReadable() % GetData()->m_type );

        Notify_KadNode_Updated(*this); // updating the gui
}


void CContact::UpdateType() throw()
{
        GetData()->m_lastTypeUpdate = std::time(NULL);

        time_t hours = (GetData()->m_lastTypeUpdate-GetData()->m_created)/HR2S(1);
        switch (hours) {
        case 0:
                GetData()->m_type = 2;
                GetData()->m_expires = GetData()->m_lastTypeUpdate + HR2S(1);
                break;
        case 1:
                GetData()->m_type = 1;
                GetData()->m_expires = GetData()->m_lastTypeUpdate + (int)HR2S(1.5);
                break;
        default:
                GetData()->m_type = 0;
                GetData()->m_expires = GetData()->m_lastTypeUpdate + HR2S(2);
        }
        AddDebugLogLineN(logKadRouting, CFormat ( wxT ( "updating type of contact %s : type set to %d" ) )
                         % GetData()->m_udpDest.humanReadable() % GetData()->m_type );

        Notify_KadNode_Updated(*this); // updating the gui
}
#endif

// File_checked_for_headers