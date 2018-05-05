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

#include "Server.h"		// Interface declarations.
#include "Tag.h"		// Needed for CTag

#include <tags/ServerTags.h>

#include "NetworkFunctions.h" // Needed for StringIPtoUint32
#include "OtherStructs.h"	// Needed for ServerMet_Struct
#include "amule.h"

CServer::CServer(ServerMet_Struct* in_data)
{
        dest = in_data->dest;

        Init();
}

CServer::CServer ( const CI2PAddress & in_dest )
{

        dest = in_dest;

        Init();

        // GonoszTopi - Init() would clear dynip !
        // Check that the ip isn't in fact 0.0.0.0
        if (!dest) {
                // If ip == 0, the address is a hostname
                dynip = dest.getAlias();
        }
}


// copy constructor
CServer::CServer(CServer* pOld) : CECID(pOld->ECID())
{
        wxASSERT(pOld != NULL);

        TagList::iterator it = pOld->m_taglist.begin();
        for ( ; it != pOld->m_taglist.end(); ++it ) {
                m_taglist.push_back ( *it );
        }
        dest = pOld->dest;
        realdest = pOld->realdest;
        staticservermember=pOld->IsStaticMember();
        tagcount = pOld->tagcount;
        files = pOld->files;
        users = pOld->users;
        preferences = pOld->preferences;
        ping = pOld->ping;
        failedcount = pOld->failedcount;
        lastpinged = pOld->lastpinged;
        lastpingedtime = pOld->lastpingedtime;
        m_dwRealLastPingedTime = pOld->m_dwRealLastPingedTime;
        description = pOld->description;
        listname = pOld->listname;
        dynip = pOld->dynip;
        maxusers = pOld->maxusers;
        m_strVersion = pOld->m_strVersion;
        m_uTCPFlags = pOld->m_uTCPFlags;
        m_uUDPFlags = pOld->m_uUDPFlags;
        challenge = pOld->challenge;
        softfiles = pOld->softfiles;
        hardfiles = pOld->hardfiles;
        m_uDescReqChallenge = pOld->m_uDescReqChallenge;
        m_uLowIDUsers = pOld->m_uLowIDUsers;
        lastdescpingedcout = pOld->lastdescpingedcout;
        m_auxPorts = pOld->m_auxPorts;
        m_dwServerKeyUDP = pOld->m_dwServerKeyUDP;
        m_bCryptPingReplyPending = pOld->m_bCryptPingReplyPending;
        m_dwIPServerKeyUDP = pOld->m_dwIPServerKeyUDP;
        m_nObfuscationPortTCP = pOld->m_nObfuscationPortTCP;
        m_nObfuscationPortUDP = pOld->m_nObfuscationPortUDP;
}

CServer::~CServer()
{
}

void CServer::Init()
{


        realdest = CI2PAddress::null;
        tagcount = 0;
        files = 0;
        users = 0;
        preferences = 0;
        ping = 0;
        description.Clear();
        listname.Clear();
        dynip.Clear();
        failedcount = 0;
        lastpinged = 0;
        lastpingedtime = 0;
        m_dwRealLastPingedTime = 0;
        staticservermember=false;
        maxusers=0;
        m_uTCPFlags = 0;
        m_uUDPFlags = 0;
        challenge = 0;
        softfiles = 0;
        hardfiles = 0;
        m_strVersion = _("Unknown");
        m_uLowIDUsers = 0;
        m_uDescReqChallenge = 0;
        lastdescpingedcout = 0;
        m_auxPorts.Clear();
        m_lastdnssolve = 0;
        m_dnsfailure = false;

        m_dwServerKeyUDP = 0;
        m_bCryptPingReplyPending = false;
        m_dwIPServerKeyUDP = 0;
        m_nObfuscationPortTCP = 0;
        m_nObfuscationPortUDP = 0;

}


bool CServer::AddTagFromFile(CFileDataIO* servermet)
{
        if (servermet == NULL) {
                return false;
        }
        TagList taglist(*servermet);
        for ( TagList::const_iterator it = taglist.begin(); it != taglist.end(); it++ ) {
        const CTag & tag = *it ;

        switch ( tag.GetID() ) {
        case ST_SERVERNAME:
                if (listname.IsEmpty()) {
                        listname = tag.GetStr();
                }
                break;

        case ST_DESCRIPTION:
                if (description.IsEmpty()) {
                        description = tag.GetStr();
                }
                break;

        case ST_PREFERENCE:
                preferences = tag.GetInt();
                break;

        case ST_PING:
                ping = tag.GetInt();
                break;

        case ST_DYNIP:
                if (dynip.IsEmpty()) {
                        dynip = tag.GetStr();
                }
                break;

        case ST_FAIL:
                failedcount = tag.GetInt();
                break;

        case ST_LASTPING:
                lastpinged = tag.GetInt();
                break;

        case ST_MAXUSERS:
                maxusers = tag.GetInt();
                break;

        case ST_SOFTFILES:
                softfiles = tag.GetInt();
                break;

        case ST_HARDFILES:
                hardfiles = tag.GetInt();
                break;

        case ST_VERSION:
                if (tag.IsStr()) {
                        // m_strVersion defaults to _("Unknown"), so check for that as well
                        if ((m_strVersion.IsEmpty()) || (m_strVersion == _("Unknown"))) {
                                m_strVersion = tag.GetStr();
                        }
                } else if (tag.IsInt()) {
                        m_strVersion = CFormat(wxT("%u.%u")) % (tag.GetInt() >> 16) % (tag.GetInt() & 0xFFFF);
                } else {
                        wxFAIL;
                }
                break;

        case ST_UDPFLAGS:
                m_uUDPFlags = tag.GetInt();
                break;

        case ST_AUXPORTSLIST:
                m_auxPorts = tag.GetStr();

                realdest = dest;
                break;

        case ST_LOWIDUSERS:
                m_uLowIDUsers = tag.GetInt();
                break;

        case ST_UDPKEY:
                m_dwServerKeyUDP = tag.GetInt();
                break;

        case ST_UDPKEYIP:
                m_dwIPServerKeyUDP = tag.GetInt();
                break;

        case ST_TCPPORTOBFUSCATION:
                m_nObfuscationPortTCP = (uint16)tag.GetInt();
                break;

        case ST_UDPPORTOBFUSCATION:
                m_nObfuscationPortUDP = (uint16)tag.GetInt();
                break;

        case ST_FILES:
                files = tag.GetInt();
                break;
        case ST_USERS:
                users = tag.GetInt();
                break;
        default:
                wxFAIL;
        }
        }

        return true;
}


void CServer::SetListName(const wxString& newname)
{
        listname = newname;
}

void CServer::SetDescription(const wxString& newname)
{
        description = newname;
}

void CServer::SetID(uint32 newip)
{
        wxASSERT(newip);
        ip = newip;
        ipfull = Uint32toStringIP(ip);
}

void CServer::SetDynIP(const wxString& newdynip)
{
        dynip = newdynip;
}


void CServer::SetLastDescPingedCount(bool bReset)
{
        if (bReset) {
                lastdescpingedcout = 0;
        } else {
                lastdescpingedcout++;
        }
}

uint32 CServer::GetServerKeyUDP(bool bForce) const
{
        if ( (m_dwIPServerKeyUDP != 0 && m_dwIPServerKeyUDP == theApp->GetUdpDest()) || bForce) {
                return m_dwServerKeyUDP;
        } else {
                return 0;
        }
}

void CServer::SetServerKeyUDP(uint32 dwServerKeyUDP)
{
        wxASSERT( theApp->GetUdpDest().isValid() || dwServerKeyUDP == 0 );
        m_dwServerKeyUDP = dwServerKeyUDP;
        m_dwIPServerKeyUDP = theApp->GetUdpDest().hashCode();
}
// File_checked_for_headers
