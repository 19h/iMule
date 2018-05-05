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

#ifndef __CONTACT_H__
#define __CONTACT_H__

#include <set>

#include "../utils/UInt128.h"
#include "../kademlia/Prefs.h"
#include "../utils/KadUDPKey.h"
#include "../../SafeFile.h"
#include "i2p/CI2PAddress.h"
#include "wx/object.h"
#include "MuleThread.h"
#include <memory>

////////////////////////////////////////
namespace Kademlia
{
        ////////////////////////////////////////
        
        class CContact
        {
        public:
                struct BadTypeException : CInvalidPacket { BadTypeException(uint8_t i) : CInvalidPacket(wxT("CContact::BadTypeException : ") + CFormat(wxT("type=%u")) % i) {}};
                struct BadVersionException : CInvalidPacket { BadVersionException(uint8_t i) : CInvalidPacket(wxT("CContact::BadVersionException : ") + CFormat(wxT("version=%u")) % i) {}};
                struct ContactWithNullClientID : CInvalidPacket { ContactWithNullClientID(uint32_t ip) : CInvalidPacket(wxT("CContact::ContactWithNullClientID : ") + CFormat(wxT("dest=%x")) % ip) {}};
                
                CContact();
                
                static CContact Self();
                
                ~CContact() {}
                
                #ifndef CLIENT_GUI
                CContact ( CFileDataIO & file, bool withkadversion, int8_t fileversion );
                
                CContact ( const CUInt128 &clientID, const CI2PAddress & udpDest, const CI2PAddress & tcpDest, uint8_t version,
                           const CKadUDPKey& kadKey, bool destVerified,
                           const CUInt128 &target /*= CKademlia::GetPrefs()->GetKadID()*/);
                
                void SetClientID(const CUInt128& clientID) throw();
                #endif
                const CUInt128& GetClientID() const throw()		{ return GetData()->m_clientID; }
                const wxString  GetClientIDString() const		{ return GetData()->m_clientID.ToHexString(); }
                
                const CUInt128& GetDistance() const throw()		{ return GetData()->m_distance; }
                const wxString GetDistanceString() const		{ return GetData()->m_distance.ToBinaryString(); }
                
                CI2PAddress 	GetIPAddress() const throw()		{ return GetData()->m_udpDest; }
                void	 	SetIPAddress(const CI2PAddress&  udpdest) throw()		{ if (GetData()->m_udpDest != udpdest) { SetIPVerified(false); GetData()->m_udpDest = udpdest; } }
                
                CI2PAddress     GetTCPDest() const			{ return GetData()->m_tcpDest;}
                void            SetTCPDest(const CI2PAddress& dest)	{ GetData()->m_tcpDest = dest;}
                
                CI2PAddress     GetUDPDest() const			{ return GetData()->m_udpDest;}
                void            SetUDPDest ( const CI2PAddress & dest )	{ GetData()->m_udpDest = dest;}
                
                uint8_t         GetType() const throw()			{ return GetData()->m_type;}
                
                #ifndef CLIENT_GUI
                void            UpdateType() throw();
                void            CheckingType() throw();
                #endif
                
                time_t          GetCreatedTime() const throw()         { return GetData()->m_created;     }
                
                void            SetExpireTime ( time_t value ) throw()	{GetData()->m_expires = value;}
                time_t          GetExpireTime() const throw()         	{ return GetData()->m_expires;     }
                
                time_t          GetLastTypeSet() const throw()         { return GetData()->m_lastTypeSet; }
                
                time_t          GetLastSeen() const         { return GetData()->m_lastTypeUpdate; }
                
                uint8_t  GetVersion() const throw()         { return GetData()->m_version; }
                void     SetVersion(uint8_t value) throw()      { GetData()->m_version = value; }
                
                const CKadUDPKey& GetUDPKey() const throw()     { return GetData()->m_udpKey; }
                void     SetUDPKey(const CKadUDPKey& key) throw()   { GetData()->m_udpKey = key; }
                
                bool     IsIPVerified() const throw()           { return GetData()->m_ipVerified; }
                void     SetIPVerified(bool ipVerified) throw()     { GetData()->m_ipVerified = ipVerified; }
                
                bool    GetReceivedHelloPacket() const throw()      { return GetData()->m_receivedHelloPacket; }
                void    SetReceivedHelloPacket() throw()        { GetData()->m_receivedHelloPacket = true; }
                
                
                bool            IsInvalid() const ;
                bool            IsValid() const {return !IsInvalid();}
                
                void            WriteToFile( CFileDataIO & file ) const;
                void            WriteToKad1Contact( CFileDataIO & file ) const;
                void            WriteToKad2Contact( CFileDataIO & file ) const;
                
                #ifndef CLIENT_GUI
                void            AddedToKadNodes();
                void            RemovedFromKadNodes();
                #endif
                bool            InKadNodes() const                           {return GetData()->m_inKadNodes;}
                
                wiMutex &       GetDataMutex()                               {return GetData()->m_mutex;}
                const wxString  GetInfoString( void ) const;
                
                bool     	CheckIfKad2() throw()              { return GetData()->m_checkKad2 ? GetData()->m_checkKad2 = false, true : false; }
                
                static const std::set<CContact> GetKadContacts() {
                        return s_kadContacts ;
                }
                bool operator< (const CContact& ) const;
                bool operator==(const CContact& ) const;
                
        private:
                void            initContact(); // Common initialization goes here
                
                class Data
                {
                public:
                        bool        m_inKadNodes;               // flag indicating if contact is to be counted in statistics
                        CUInt128	m_clientID;
                        CUInt128	m_distance;
                        CI2PAddress	m_tcpDest;
                        CI2PAddress	m_udpDest;
                        
                        uint8_t		m_type; /* set to 3 in initContact,
                        set to 2, 1 or 0 in updateType according to time since contact creation
                        (called by SetAlive, when receiving a response from contact)
                        incremented in checkingType */
                        time_t		m_lastTypeSet;
                        time_t		m_lastTypeUpdate;
                        time_t		m_expires;
                        time_t		m_created;
                        uint8_t		m_version;
                        bool        	m_checkKad2;
                        bool        	m_ipVerified;
                        bool		m_receivedHelloPacket;
                        CKadUDPKey	m_udpKey;
                        
                        wiMutex     	m_mutex;
                };
                
                static std::set<CContact>    s_kadContacts ;
                std::shared_ptr<Data> m_data;
                std::shared_ptr<Data> GetData() const   { return m_data; }
        };
} // End namespace

#endif // __CONTACT_H__
// File_checked_for_headers
