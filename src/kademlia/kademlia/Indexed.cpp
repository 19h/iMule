//
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

#include "Indexed.h"


#include <protocol/Protocols.h>
#include <protocol/ed2k/Constants.h>
#include <protocol/kad/Constants.h>
#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad2/Client2Client/UDP.h>
#include <common/Macros.h>
#include <tags/FileTags.h>

#include "../routing/Contact.h"
#include "../net/KademliaUDPListener.h"
#include "../utils/KadUDPKey.h"
#include "../../CFile.h"
#include "../../MemFile.h"
#include "../../amule.h"
#include "../../Preferences.h"
#include "../../Logger.h"
#include "../../Packet.h"
#include "Kademlia.h"

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

wxString CIndexed::m_kfilename;
wxString CIndexed::m_sfilename;
wxString CIndexed::m_loadfilename;

CIndexed::CIndexed()
{
        m_sfilename = theApp->ConfigDir + wxT("src_index.dat");
        m_kfilename = theApp->ConfigDir + wxT("key_index.dat");
        m_loadfilename = theApp->ConfigDir + wxT("load_index.dat");
        m_lastClean = time(NULL) + (60*30);
        m_totalIndexSource = 0;
        m_totalIndexKeyword = 0;
        m_totalIndexNotes = 0;
        m_totalIndexLoad = 0;
        ReadFile();
}

void CIndexed::ReadFile()
{
        try {
                uint32_t totalLoad = 0;
                uint32_t totalSource = 0;
                uint32_t totalKeyword = 0;

                CFile load_file;
                if (CPath::FileExists(m_loadfilename) && load_file.Open(m_loadfilename, CFile::read)) {
                        uint32_t version = load_file.ReadUInt32();
                        AddDebugLogLineN ( logKadIndex, CFormat ( wxT ( "readFile: version = %d" ) ) % version );
                        if (version < 2) {
                                /*time_t savetime =*/ load_file.ReadUInt32(); //  Savetime is unused now

                                uint32_t numLoad = load_file.ReadUInt32();
                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: numLoad = %d" ) ) % numLoad );
                                while (numLoad) {
                                        CUInt128 keyID = load_file.ReadUInt128();
                                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: keyID = %s" ) ) % keyID.ToHexString() );
                                        uint32_t thisload = load_file.ReadUInt32();
                                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: thisload = %d" ) ) % thisload );
                                        if ( AddLoad ( keyID, thisload ) ) {
                                                totalLoad++;
                                        }
                                        numLoad--;
                                }
                        }
                        load_file.Close();
                }

                // keywords
                CFile k_file;
                if (CPath::FileExists(m_kfilename) && k_file.Open(m_kfilename, CFile::read)) {
                        try {
                                uint32_t version = k_file.ReadUInt32();
                                AddDebugLogLineN( logKadIndex, CFormat ( wxT ( "readFile: version = %d" ) ) % version );
                                if (version < 4) {
                                        time_t savetime = k_file.ReadUInt32();
                                        AddDebugLogLineN( logKadIndex, CFormat ( wxT ( "readFile: savetime = %d" ) ) % savetime );
                                        if (savetime > time(NULL)) {
                                                CUInt128 id = k_file.ReadUInt128();
                                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: id = %s" ) ) % id.ToHexString() );

                                                if ( Kademlia::CKademlia::GetPrefs()->GetKadID() == id ) {
                                                        uint32_t numKeys = k_file.ReadUInt32();
                                                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: numKeys = %d" ) ) % numKeys );
                                                        while (numKeys) {
                                                                CUInt128 keyID = k_file.ReadUInt128();
                                                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: keyID = %s" ) ) % keyID.ToHexString() );
                                                                uint32_t numSource = k_file.ReadUInt32();
                                                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: numSources = %d" ) ) % numSource );
                                                                while (numSource) {
                                                                        CUInt128 sourceID = k_file.ReadUInt128();
                                                                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: sourceID = %s" ) ) % sourceID.ToHexString() );

                                                                        if (version<2) {
                                                                                Kademlia::CKeyEntry * toaddN = new Kademlia::CKeyEntry;
                                                                                toaddN->m_bSource = false;
                                                                                uint32_t expire = k_file.ReadUInt32();
                                                                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: expire = %d" ) ) % expire );

                                                                                toaddN->m_tLifeTime = expire;
                                                                                toaddN->m_taglist = TagList ( k_file );

                                                                                for ( TagList::const_iterator it = toaddN->m_taglist.begin();
                                                                                                it != toaddN->m_taglist.end();
                                                                                                it++ ) {
                                                                                        const CTag & tag = *it;

                                                                                        if ( tag.isValid() ) {
                                                                                                if ( tag.GetID() == TAG_FILENAME ) {
                                                                                                        if (toaddN->GetCommonFileName().IsEmpty()) {
                                                                                                                toaddN->SetFileName(tag.GetStr());
                                                                                                        }
                                                                                                } else if ( tag.GetID() == TAG_FILESIZE ) {
                                                                                                        toaddN->m_uSize = tag.GetInt();
                                                                                                        // NOTE: always add the 'size' tag, even if it's stored separately in 'size'. the tag is still needed for answering search request
                                                                                                } else if ( tag.GetID() == TAG_SOURCEDEST ) {
                                                                                                        toaddN->m_tcpdest = tag.GetAddr();
                                                                                                } else if ( tag.GetID() == TAG_SOURCEUDEST ) {
                                                                                                        toaddN->m_udpdest = tag.GetAddr();
                                                                                                }
                                                                                        }
                                                                                }

                                                                                uint8_t load = 0;

                                                                                if ( AddKeyword ( keyID, sourceID, toaddN, load ) ) {
                                                                                        totalKeyword++;
                                                                                } else {
                                                                                        delete toaddN ;
                                                                                }
                                                                        } else { // version >= 2
                                                                                uint32_t numName = k_file.ReadUInt32();
                                                                                while (numName) {
                                                                                        Kademlia::CKeyEntry* toAdd = new Kademlia::CKeyEntry();
                                                                                        toAdd->m_uKeyID = keyID;
                                                                                        toAdd->m_uSourceID = sourceID;
                                                                                        toAdd->m_bSource = false;
                                                                                        toAdd->m_tLifeTime = k_file.ReadUInt32();
                                                                                        if (version >= 3) {
                                                                                                toAdd->ReadPublishTrackingDataFromFile(&k_file);
                                                                                        }
                                                                                        TagList taglst( k_file );

                                                                                        for ( TagList::const_iterator it = taglst.begin();
                                                                                                        it != taglst.end();
                                                                                                        it++ ) {
                                                                                                const CTag & tag = *it;

                                                                                                if ( tag.isValid() ) {
                                                                                                        switch ( tag.GetID() ) {
                                                                                                        case TAG_FILENAME :
                                                                                                                if (toAdd->GetCommonFileName().IsEmpty()) {
                                                                                                                        toAdd ->SetFileName(tag.GetStr());
                                                                                                                }
                                                                                                                break ;
                                                                                                        case TAG_FILESIZE :
                                                                                                                toAdd ->m_uSize = tag.GetInt();
                                                                                                                break ;
                                                                                                        case TAG_SOURCEDEST :
                                                                                                                toAdd ->m_tcpdest = tag.GetAddr();
                                                                                                                toAdd->AddTag(tag);
                                                                                                                break ;
                                                                                                        case TAG_SOURCEUDEST :
                                                                                                                toAdd ->m_udpdest = tag.GetAddr();
                                                                                                                toAdd->AddTag(tag);
                                                                                                                break ;
                                                                                                        default :
                                                                                                                toAdd->AddTag(tag);
                                                                                                        }
                                                                                                }
                                                                                        }
                                                                                        uint8_t load;
                                                                                        if (AddKeyword(keyID, sourceID, toAdd, load)) {
                                                                                                totalKeyword++;
                                                                                        } else {
                                                                                                delete toAdd;
                                                                                        }
                                                                                        numName--;
                                                                                }
                                                                        }
                                                                        numSource--;
                                                                }
                                                                numKeys--;
                                                        }
                                                }
                                        }
                                }
                        } catch ( wxString s ) {
                                AddLogLineC(CFormat ( _ ( "Kad index file %s is buggy and has not been loaded completely.\n Message : " ) )
                                            % m_kfilename % s );
                        }
                        k_file.Close();
                }

                // sources
                //CBufferedFileIO s_file;
                CFile s_file;
                if (CPath::FileExists(m_sfilename) && s_file.Open(m_sfilename, CFile::read)) {
                        try {
                                uint32_t version = s_file.ReadUInt32();
                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: version = %d" ) ) % version );

                                if ( version < 3) {
                                        time_t savetime = s_file.ReadUInt32();
                                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: savetime = %d" ) ) % savetime );
                                        if (savetime > time(NULL)) {
                                                uint32_t numKeys = s_file.ReadUInt32();
                                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: numKeys = %d" ) ) % numKeys );


                                                while (numKeys) {
                                                        CUInt128 keyID = s_file.ReadUInt128();
                                                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: keyID = %s" ) ) % keyID.ToHexString() );

                                                        uint32_t numSource = s_file.ReadUInt32();
                                                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: numSources = %d" ) ) % numSource );
                                                        while (numSource) {
                                                                CUInt128 sourceID = s_file.ReadUInt128();
                                                                AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: sourceID = %s" ) ) % sourceID.ToHexString() );

                                                                if (version<2) {
                                                                        Kademlia::CEntry * toaddN = new Kademlia::CEntry;
                                                                        toaddN->m_uSourceID = true;
                                                                        uint32_t test = s_file.ReadUInt32();
                                                                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "readFile: test = %d" ) ) % test );

                                                                        toaddN->m_tLifeTime = test;

                                                                        toaddN->m_taglist = TagList ( s_file );

                                                                        for ( TagList::const_iterator it = toaddN->m_taglist.begin();
                                                                                        it != toaddN->m_taglist.end();
                                                                                        it++ ) {
                                                                                const CTag & tag = *it ;

                                                                                if ( tag.isValid() ) {
                                                                                        if ( tag.GetID() == TAG_SOURCEDEST ) {
                                                                                                toaddN->m_tcpdest = tag.GetAddr();
                                                                                        } else if ( tag.GetID() == TAG_SOURCEUDEST ) {
                                                                                                toaddN->m_udpdest = tag.GetAddr();
                                                                                        }
                                                                                }
                                                                        }

                                                                        toaddN->m_uKeyID.SetValue ( keyID );

                                                                        toaddN->m_uSourceID.SetValue ( sourceID );
                                                                        uint8_t load = 0;

                                                                        if ( AddSources ( keyID, sourceID, toaddN, load ) ) {
                                                                                totalSource++;
                                                                        } else {
                                                                                delete toaddN ;
                                                                        }

                                                                } else { // version >= 2
                                                                        uint32_t numName = s_file.ReadUInt32();
                                                                        while (numName) {
                                                                                Kademlia::CEntry* toAdd = new Kademlia::CEntry();
                                                                                toAdd->m_bSource = true;
                                                                                toAdd->m_tLifeTime = s_file.ReadUInt32();
                                                                                toAdd ->m_taglist = TagList( s_file ) ;

                                                                                for ( TagList::const_iterator it = toAdd ->m_taglist.begin();
                                                                                                it != toAdd ->m_taglist.end();
                                                                                                it++ ) {
                                                                                        const CTag & tag = *it ;
                                                                                        if ( tag.isValid() ) {
                                                                                                switch (tag.GetID()) {
                                                                                                case TAG_SOURCEDEST :
                                                                                                        toAdd->m_tcpdest = tag.GetAddr();
                                                                                                        break;
                                                                                                case TAG_SOURCEUDEST :
                                                                                                        toAdd->m_udpdest = tag.GetAddr();
                                                                                                        break;
                                                                                                default:
                                                                                                        break;
                                                                                                }
                                                                                        }
                                                                                }
                                                                                toAdd->m_uKeyID.SetValue(keyID);
                                                                                toAdd->m_uSourceID.SetValue(sourceID);
                                                                                uint8_t load;
                                                                                if (AddSources(keyID, sourceID, toAdd, load)) {
                                                                                        totalSource++;
                                                                                } else {
                                                                                        delete toAdd;
                                                                                }
                                                                                numName--;
                                                                        }
                                                                }
                                                                numSource--;
                                                        }
                                                        numKeys--;
                                                }
                                        }
                                }
                        } catch ( wxString s ) {
                                AddLogLineC(CFormat(_("Kad index file %s is buggy and has not been loaded completely.\n Message : "))
                                            % m_sfilename % s );
                        }
                        s_file.Close();

                        m_totalIndexSource = totalSource;
                        m_totalIndexKeyword = totalKeyword;
                        m_totalIndexLoad = totalLoad;
                        AddDebugLogLineN(logKadIndex, CFormat(wxT("Read %u source, %u keyword, and %u load entries")) % totalSource % totalKeyword % totalLoad);
                }
        } catch (const CSafeIOException& err) {
                AddDebugLogLineC(logKadIndex, wxT("CSafeIOException in CIndexed::readFile: ") + err.what());
        } catch (const CInvalidPacket& err) {
                AddDebugLogLineC(logKadIndex, wxT("CInvalidPacket Exception in CIndexed::readFile: ") + err.what());
        } catch (const wxString& e) {
                AddDebugLogLineC(logKadIndex, wxT("Exception in CIndexed::readFile: ") + e);
        }
}

CIndexed::~CIndexed()
{
        try {
                time_t now = time(NULL);
                uint32_t s_total = 0;
                uint32_t k_total = 0;
                uint32_t l_total = 0;

                CFile load_file;
                if (load_file.Open(m_loadfilename, CFile::write)) {
                        load_file.WriteUInt32(1); // version
                        load_file.WriteUInt32(now);
                        wxASSERT(m_Load_map.size() < 0xFFFFFFFF);
                        load_file.WriteUInt32((uint32_t)m_Load_map.size());
                        for (LoadMap::iterator it = m_Load_map.begin(); it != m_Load_map.end(); ++it ) {
                                Load* load = it->second;
                                wxASSERT(load);
                                if (load) {
                                        load_file.WriteUInt128(load->keyID);
                                        load_file.WriteUInt32(load->time);
                                        l_total++;
                                        delete load;
                                }
                        }
                        load_file.Close();
                }

                CFile s_file;
                if (s_file.Open(m_sfilename, CFile::write)) {
                        s_file.WriteUInt32(2); // version
                        s_file.WriteUInt32(now + KADEMLIAREPUBLISHTIMES);
                        wxASSERT(m_Sources_map.size() < 0xFFFFFFFF);
                        s_file.WriteUInt32((uint32_t)m_Sources_map.size());
                        for (SrcHashMap::iterator itSrcHash = m_Sources_map.begin(); itSrcHash != m_Sources_map.end(); ++itSrcHash ) {
                                SrcHash* currSrcHash = itSrcHash->second;
                                s_file.WriteUInt128(currSrcHash->keyID);

                                CKadSourcePtrList& KeyHashSrcMap = currSrcHash->m_Source_map;
                                wxASSERT(KeyHashSrcMap.size() < 0xFFFFFFFF);
                                s_file.WriteUInt32((uint32_t)KeyHashSrcMap.size());

                                for (CKadSourcePtrList::iterator itSource = KeyHashSrcMap.begin(); itSource != KeyHashSrcMap.end(); ++itSource) {
                                        Source* currSource = *itSource;
                                        s_file.WriteUInt128(currSource->sourceID);

                                        CKadEntryPtrList& SrcEntryList = currSource->entryList;
                                        wxASSERT(SrcEntryList.size() < 0xFFFFFFFF);
                                        s_file.WriteUInt32((uint32_t)SrcEntryList.size());
                                        for (CKadEntryPtrList::iterator itEntry = SrcEntryList.begin(); itEntry != SrcEntryList.end(); ++itEntry) {
                                                Kademlia::CEntry* currName = *itEntry;
                                                s_file.WriteUInt32(currName->m_tLifeTime);
                                                currName->WriteTagList(&s_file);
                                                delete currName;
                                                s_total++;
                                        }
                                        delete currSource;
                                }
                                delete currSrcHash;
                        }
                        s_file.Close();
                }

                CFile k_file;
                if (k_file.Open(m_kfilename, CFile::write)) {
                        k_file.WriteUInt32(3); // version
                        k_file.WriteUInt32(now + KADEMLIAREPUBLISHTIMEK);
                        k_file.WriteUInt128(Kademlia::CKademlia::GetPrefs()->GetKadID());

                        wxASSERT(m_Keyword_map.size() < 0xFFFFFFFF);
                        k_file.WriteUInt32((uint32_t)m_Keyword_map.size());

                        for (KeyHashMap::iterator itKeyHash = m_Keyword_map.begin(); itKeyHash != m_Keyword_map.end(); ++itKeyHash ) {
                                KeyHash* currKeyHash = itKeyHash->second;
                                k_file.WriteUInt128(currKeyHash->keyID);

                                CSourceKeyMap& KeyHashSrcMap = currKeyHash->m_Source_map;
                                wxASSERT(KeyHashSrcMap.size() < 0xFFFFFFFF);
                                k_file.WriteUInt32((uint32_t)KeyHashSrcMap.size());

                                for (CSourceKeyMap::iterator itSource = KeyHashSrcMap.begin(); itSource != KeyHashSrcMap.end(); ++itSource ) {
                                        Source* currSource = itSource->second;
                                        k_file.WriteUInt128(currSource->sourceID);

                                        CKadEntryPtrList& SrcEntryList = currSource->entryList;
                                        wxASSERT(SrcEntryList.size() < 0xFFFFFFFF);
                                        k_file.WriteUInt32((uint32_t)SrcEntryList.size());

                                        for (CKadEntryPtrList::iterator itEntry = SrcEntryList.begin(); itEntry != SrcEntryList.end(); ++itEntry) {
                                                Kademlia::CKeyEntry* currName = static_cast<Kademlia::CKeyEntry*>(*itEntry);
                                                wxASSERT(currName->IsKeyEntry());
                                                k_file.WriteUInt32(currName->m_tLifeTime);
                                                currName->WritePublishTrackingDataToFile(&k_file);
                                                currName->WriteTagList(&k_file);
                                                currName->DirtyDeletePublishData();
                                                delete currName;
                                                k_total++;
                                        }
                                        delete currSource;
                                }
                                CKeyEntry::ResetGlobalTrackingMap();
                                delete currKeyHash;
                        }
                        k_file.Close();
                }
                AddDebugLogLineN(logKadIndex, CFormat(wxT("Wrote %u source, %u keyword, and %u load entries")) % s_total % k_total % l_total);

                for (SrcHashMap::iterator itNoteHash = m_Notes_map.begin(); itNoteHash != m_Notes_map.end(); ++itNoteHash) {
                        SrcHash* currNoteHash = itNoteHash->second;
                        CKadSourcePtrList& KeyHashNoteMap = currNoteHash->m_Source_map;

                        for (CKadSourcePtrList::iterator itNote = KeyHashNoteMap.begin(); itNote != KeyHashNoteMap.end(); ++itNote) {
                                Source* currNote = *itNote;
                                CKadEntryPtrList& NoteEntryList = currNote->entryList;
                                for (CKadEntryPtrList::iterator itNoteEntry = NoteEntryList.begin(); itNoteEntry != NoteEntryList.end(); ++itNoteEntry) {
                                        delete *itNoteEntry;
                                }
                                delete currNote;
                        }
                        delete currNoteHash;
                }

                m_Notes_map.clear();
        } catch (const CSafeIOException& err) {
                AddDebugLogLineC(logKadIndex, wxT("CSafeIOException in CIndexed::~CIndexed: ") + err.what());
        } catch (const CInvalidPacket& err) {
                AddDebugLogLineC(logKadIndex, wxT("CInvalidPacket Exception in CIndexed::~CIndexed: ") + err.what());
        } catch (const wxString& e) {
                AddDebugLogLineC(logKadIndex, wxT("Exception in CIndexed::~CIndexed: ") + e);
        }
}

void CIndexed::Clean()
{
        time_t tNow = time(NULL);
        if (m_lastClean > tNow) {
                return;
        }

        uint32_t k_Removed = 0;
        uint32_t s_Removed = 0;
        uint32_t s_Total = 0;
        uint32_t k_Total = 0;

        KeyHashMap::iterator itKeyHash = m_Keyword_map.begin();
        while (itKeyHash != m_Keyword_map.end()) {
                KeyHashMap::iterator curr_itKeyHash = itKeyHash++; // Don't change this to a ++it!
                KeyHash* currKeyHash = curr_itKeyHash->second;

                for (CSourceKeyMap::iterator itSource = currKeyHash->m_Source_map.begin(); itSource != currKeyHash->m_Source_map.end(); ) {
                        CSourceKeyMap::iterator curr_itSource = itSource++; // Don't change this to a ++it!
                        Source* currSource = curr_itSource->second;

                        CKadEntryPtrList::iterator itEntry = currSource->entryList.begin();
                        while (itEntry != currSource->entryList.end()) {
                                k_Total++;

                                Kademlia::CKeyEntry* currName = static_cast<Kademlia::CKeyEntry*>(*itEntry);
                                wxASSERT(currName->IsKeyEntry());
                                if (!currName->m_bSource && currName->m_tLifeTime < tNow) {
                                        k_Removed++;
                                        itEntry = currSource->entryList.erase(itEntry);
                                        delete currName;
                                        continue;
                                } else if (currName->m_bSource) {
                                        wxFAIL;
                                } else {
                                        currName->CleanUpTrackedPublishers();	// intern cleanup
                                }
                                ++itEntry;
                        }

                        if (currSource->entryList.empty()) {
                                currKeyHash->m_Source_map.erase(curr_itSource);
                                delete currSource;
                        }
                }

                if (currKeyHash->m_Source_map.empty()) {
                        m_Keyword_map.erase(curr_itKeyHash);
                        delete currKeyHash;
                }
        }

        SrcHashMap::iterator itSrcHash = m_Sources_map.begin();
        while (itSrcHash != m_Sources_map.end()) {
                SrcHashMap::iterator curr_itSrcHash = itSrcHash++; // Don't change this to a ++it!
                SrcHash* currSrcHash = curr_itSrcHash->second;

                CKadSourcePtrList::iterator itSource = currSrcHash->m_Source_map.begin();
                while (itSource != currSrcHash->m_Source_map.end()) {
                        Source* currSource = *itSource;

                        CKadEntryPtrList::iterator itEntry = currSource->entryList.begin();
                        while (itEntry != currSource->entryList.end()) {
                                s_Total++;

                                Kademlia::CEntry* currName = *itEntry;
                                if (currName->m_tLifeTime < tNow) {
                                        s_Removed++;
                                        itEntry = currSource->entryList.erase(itEntry);
                                        delete currName;
                                } else {
                                        ++itEntry;
                                }
                        }

                        if (currSource->entryList.empty()) {
                                itSource = currSrcHash->m_Source_map.erase(itSource);
                                delete currSource;
                        } else {
                                ++itSource;
                        }
                }

                if (currSrcHash->m_Source_map.empty()) {
                        m_Sources_map.erase(curr_itSrcHash);
                        delete currSrcHash;
                }
        }

        m_totalIndexSource = s_Total - s_Removed;
        m_totalIndexKeyword = k_Total - k_Removed;
        AddDebugLogLineN(logKadIndex, CFormat(wxT("Removed %u keyword out of %u and %u source out of %u")) % k_Removed % k_Total % s_Removed % s_Total);
        m_lastClean = tNow + MIN2S(30);
}

bool CIndexed::AddKeyword(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CKeyEntry* entry, uint8_t& load)
{
        if (!entry) {
                return false;
        }

        wxCHECK(entry->IsKeyEntry(), false);

        if (m_totalIndexKeyword > KADEMLIAMAXENTRIES) {
                load = 100;
                return false;
        }

        if (entry->m_uSize == 0 || entry->GetCommonFileName().IsEmpty() || entry->GetTagCount() == 0 || entry->m_tLifeTime < time(NULL)) {
                return false;
        }

        KeyHashMap::iterator itKeyHash = m_Keyword_map.find(keyID);
        KeyHash* currKeyHash = NULL;
        if (itKeyHash == m_Keyword_map.end()) {
                Source* currSource = new Source;
                currSource->sourceID.SetValue(sourceID);
                entry->MergeIPsAndFilenames(NULL); // IpTracking init
                currSource->entryList.push_front(entry);
                currKeyHash = new KeyHash;
                currKeyHash->keyID.SetValue(keyID);
                currKeyHash->m_Source_map[currSource->sourceID] = currSource;
                m_Keyword_map[currKeyHash->keyID] = currKeyHash;
                load = 1;
                m_totalIndexKeyword++;
                return true;
        } else {
                currKeyHash = itKeyHash->second;
                size_t indexTotal = currKeyHash->m_Source_map.size();
                if (indexTotal > KADEMLIAMAXINDEX) {
                        load = 100;
                        //Too many entries for this Keyword..
                        return false;
                }
                Source* currSource = NULL;
                CSourceKeyMap::iterator itSource = currKeyHash->m_Source_map.find(sourceID);
                if (itSource != currKeyHash->m_Source_map.end()) {
                        currSource = itSource->second;
                        if (currSource->entryList.size() > 0) {
                                if (indexTotal > KADEMLIAMAXINDEX - 5000) {
                                        load = 100;
                                        //We are in a hot node.. If we continued to update all the publishes
                                        //while this index is full, popular files will be the only thing you index.
                                        return false;
                                }
                                // also check for size match
                                CKeyEntry *oldEntry = NULL;
                                for (CKadEntryPtrList::iterator itEntry = currSource->entryList.begin(); itEntry != currSource->entryList.end(); ++itEntry) {
                                        CKeyEntry *currEntry = static_cast<Kademlia::CKeyEntry*>(*itEntry);
                                        wxASSERT(currEntry->IsKeyEntry());
                                        if (currEntry->m_uSize == entry->m_uSize) {
                                                oldEntry = currEntry;
                                                currSource->entryList.erase(itEntry);
                                                break;
                                        }
                                }
                                entry->MergeIPsAndFilenames(oldEntry);	// oldEntry can be NULL, that's ok and we still need to do this call in this case
                                if (oldEntry == NULL) {
                                        m_totalIndexKeyword++;
                                        AddDebugLogLineN(logKadIndex, wxT("Multiple sizes published for file ") + entry->m_uSourceID.ToHexString());
                                }
                                delete oldEntry;
                                oldEntry = NULL;
                        } else {
                                m_totalIndexKeyword++;
                                entry->MergeIPsAndFilenames(NULL); // IpTracking init
                        }
                        load = (uint8_t)((indexTotal * 100) / KADEMLIAMAXINDEX);
                        currSource->entryList.push_front(entry);
                        return true;
                } else {
                        currSource = new Source;
                        currSource->sourceID.SetValue(sourceID);
                        entry->MergeIPsAndFilenames(NULL); // IpTracking init
                        currSource->entryList.push_front(entry);
                        currKeyHash->m_Source_map[currSource->sourceID] = currSource;
                        m_totalIndexKeyword++;
                        load = (indexTotal * 100) / KADEMLIAMAXINDEX;
                        return true;
                }
        }
}


bool CIndexed::AddSources(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8_t& load)
{
        if (!entry) {
                return false;
        }

        if( not entry->m_tcpdest.isValid() or entry->GetTagCount() == 0 or entry->m_tLifeTime < time(NULL)) {
                return false;
        }

        SrcHash* currSrcHash = NULL;
        SrcHashMap::iterator itSrcHash = m_Sources_map.find(keyID);
        if (itSrcHash == m_Sources_map.end()) {
                Source* currSource = new Source;
                currSource->sourceID.SetValue(sourceID);
                currSource->entryList.push_front(entry);
                currSrcHash = new SrcHash;
                currSrcHash->keyID.SetValue(keyID);
                currSrcHash->m_Source_map.push_front(currSource);
                m_Sources_map[currSrcHash->keyID] =  currSrcHash;
                m_totalIndexSource++;
                load = 1;
                return true;
        } else {
                currSrcHash = itSrcHash->second;
                size_t size = currSrcHash->m_Source_map.size();

                for (CKadSourcePtrList::iterator itSource = currSrcHash->m_Source_map.begin(); itSource != currSrcHash->m_Source_map.end(); ++itSource) {
                        Source* currSource = *itSource;
                        if (currSource->entryList.size()) {
                                CEntry* currEntry = currSource->entryList.front();
                                wxASSERT(currEntry != NULL);
                                if (currEntry->m_tcpdest == entry->m_tcpdest || currEntry->m_udpdest == entry->m_udpdest) {
                                        CEntry* currName = currSource->entryList.front();
                                        currSource->entryList.pop_front();
                                        delete currName;
                                        currSource->entryList.push_front(entry);
                                        load = (size * 100) / KADEMLIAMAXSOURCEPERFILE;
                                        return true;
                                }
                        } else {
                                //This should never happen!
                                currSource->entryList.push_front(entry);
                                wxFAIL;
                                load = (size * 100) / KADEMLIAMAXSOURCEPERFILE;
                                return true;
                        }
                }
                if (size > KADEMLIAMAXSOURCEPERFILE) {
                        Source* currSource = currSrcHash->m_Source_map.back();
                        currSrcHash->m_Source_map.pop_back();
                        wxASSERT(currSource != NULL);
                        Kademlia::CEntry* currName = currSource->entryList.back();
                        currSource->entryList.pop_back();
                        wxASSERT(currName != NULL);
                        delete currName;
                        currSource->sourceID.SetValue(sourceID);
                        currSource->entryList.push_front(entry);
                        currSrcHash->m_Source_map.push_front(currSource);
                        load = 100;
                        return true;
                } else {
                        Source* currSource = new Source;
                        currSource->sourceID.SetValue(sourceID);
                        currSource->entryList.push_front(entry);
                        currSrcHash->m_Source_map.push_front(currSource);
                        m_totalIndexSource++;
                        load = (size * 100) / KADEMLIAMAXSOURCEPERFILE;
                        return true;
                }
        }

        return false;
}

bool CIndexed::AddNotes(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8_t& load)
{
        if (!entry) {
                return false;
        }

        if ( not entry->m_tcpdest.isValid() or entry->GetTagCount() == 0) {
                return false;
        }

        SrcHash* currNoteHash = NULL;
        SrcHashMap::iterator itNoteHash = m_Notes_map.find(keyID);
        if (itNoteHash == m_Notes_map.end()) {
                Source* currNote = new Source;
                currNote->sourceID.SetValue(sourceID);
                currNote->entryList.push_front(entry);
                currNoteHash = new SrcHash;
                currNoteHash->keyID.SetValue(keyID);
                currNoteHash->m_Source_map.push_front(currNote);
                m_Notes_map[currNoteHash->keyID] = currNoteHash;
                load = 1;
                m_totalIndexNotes++;
                return true;
        } else {
                currNoteHash = itNoteHash->second;
                size_t size = currNoteHash->m_Source_map.size();

                for (CKadSourcePtrList::iterator itSource = currNoteHash->m_Source_map.begin(); itSource != currNoteHash->m_Source_map.end(); ++itSource) {
                        Source* currNote = *itSource;
                        if( currNote->entryList.size() ) {
                                CEntry* currEntry = currNote->entryList.front();
                                wxASSERT(currEntry!=NULL);
                                if (currEntry->m_tcpdest == entry->m_tcpdest || currEntry->m_uSourceID == entry->m_uSourceID) {
                                        CEntry* currName = currNote->entryList.front();
                                        currNote->entryList.pop_front();
                                        delete currName;
                                        currNote->entryList.push_front(entry);
                                        load = (size * 100) / KADEMLIAMAXNOTESPERFILE;
                                        return true;
                                }
                        } else {
                                //This should never happen!
                                currNote->entryList.push_front(entry);
                                wxFAIL;
                                load = (size * 100) / KADEMLIAMAXNOTESPERFILE;
                                m_totalIndexNotes++;
                                return true;
                        }
                }
                if (size > KADEMLIAMAXNOTESPERFILE) {
                        Source* currNote = currNoteHash->m_Source_map.back();
                        currNoteHash->m_Source_map.pop_back();
                        wxASSERT(currNote != NULL);
                        CEntry* currName = currNote->entryList.back();
                        currNote->entryList.pop_back();
                        wxASSERT(currName != NULL);
                        delete currName;
                        currNote->sourceID.SetValue(sourceID);
                        currNote->entryList.push_front(entry);
                        currNoteHash->m_Source_map.push_front(currNote);
                        load = 100;
                        return true;
                } else {
                        Source* currNote = new Source;
                        currNote->sourceID.SetValue(sourceID);
                        currNote->entryList.push_front(entry);
                        currNoteHash->m_Source_map.push_front(currNote);
                        load = (size * 100) / KADEMLIAMAXNOTESPERFILE;
                        m_totalIndexNotes++;
                        return true;
                }
        }
}

bool CIndexed::AddLoad(const CUInt128& keyID, uint32_t timet)
{
        Load* load = NULL;

        if ((uint32_t)time(NULL) > timet) {
                return false;
        }

        LoadMap::iterator it = m_Load_map.find(keyID);
        if (it != m_Load_map.end()) {
                wxFAIL;
                return false;
        }

        load = new Load();
        load->keyID.SetValue(keyID);
        load->time = timet;
        m_Load_map[load->keyID] = load;
        m_totalIndexLoad++;
        return true;
}

void CIndexed::SendValidKeywordResult(const CUInt128& keyID, const SSearchTerm* pSearchTerms, const CI2PAddress & dest, bool kad2, uint16_t startPosition, const CKadUDPKey& senderKey)
{
        std::list<CEntry*> results ;
        KeyHash* currKeyHash = NULL;
        KeyHashMap::iterator itKeyHash = m_Keyword_map.find(keyID);
        int16_t count = 0;
        if (itKeyHash != m_Keyword_map.end()) {
                currKeyHash = itKeyHash->second;
                const uint16_t maxResults = 300;
                count = 0 - startPosition;

                // we do 2 loops: In the first one we ignore all results which have a trustvalue below 1
                // in the second one we then also consider those. That way we make sure our 300 max results are not full
                // of spam entries. We could also sort by trustvalue, but we would risk to only send popular files this way
                // on very hot keywords
                bool onlyTrusted = true;
                //                 DEBUG_ONLY( uint32_t dbgResultsTrusted = 0; )
                //                 DEBUG_ONLY( uint32_t dbgResultsUntrusted = 0; )

                do {
                        for (CSourceKeyMap::iterator itSource = currKeyHash->m_Source_map.begin(); itSource != currKeyHash->m_Source_map.end(); ++itSource) {
                                Source* currSource =  itSource->second;

                                for (CKadEntryPtrList::iterator itEntry = currSource->entryList.begin(); itEntry != currSource->entryList.end(); ++itEntry) {
                                        Kademlia::CKeyEntry* currName = static_cast<Kademlia::CKeyEntry*>(*itEntry);
                                        wxASSERT(currName->IsKeyEntry());
                                        if ((onlyTrusted ^ (currName->GetTrustValue() < 1.0)) && (!pSearchTerms || currName->SearchTermsMatch(pSearchTerms))) {
                                                if (count < 0) {
                                                        count++;
                                                } else if ((uint16_t)count < maxResults) {
                                                        count++;
                                                        results.push_back ( currName ) ;
                                                } else {
                                                        itSource = currKeyHash->m_Source_map.end();
                                                        --itSource;
                                                        break;
                                                }
                                        }
                                }
                        }

                        if (onlyTrusted && count < (int)maxResults) {
                                onlyTrusted = false;
                        } else {
                                break;
                        }
                } while (!onlyTrusted);
        }
        // mkvore: moved and added "or kad2" in order to answer requests even if we don't know, so that the client don't wait for us
        if (count > 0 or kad2) {
                if (kad2)
		  {DebugSend(Kad2SearchRes(keyword), dest);}
                else
		  {DebugSend(KadSearchRes(keyword), dest);}

                SendResults(keyID, results, dest, kad2, senderKey);
        }
        Clean();
}

void CIndexed::SendValidSourceResult(const CUInt128& keyID, const CI2PAddress & dest, bool kad2, uint16_t startPosition, uint64_t fileSize, const CKadUDPKey& senderKey)
{
        std::list<CEntry*> results ;
        SrcHash* currSrcHash = NULL;
        SrcHashMap::iterator itSrcHash = m_Sources_map.find(keyID);
        int16_t     count = 0;
        if (itSrcHash != m_Sources_map.end()) {
                currSrcHash = itSrcHash->second;
                uint16_t maxResults = 300;
                count       = 0 - startPosition;

                for (CKadSourcePtrList::iterator itSource = currSrcHash->m_Source_map.begin(); itSource != currSrcHash->m_Source_map.end(); ++itSource) {
                        Source* currSource = *itSource;
                        if (currSource->entryList.size()) {
                                Kademlia::CEntry* currName = currSource->entryList.front();
                                if (count < 0) {
                                        count++;
                                } else if (count < maxResults) {
                                        if (!fileSize || !currName->m_uSize || currName->m_uSize == fileSize) {
                                                results.push_back( currName ) ;
                                                count++;
                                        }
                                } else {
                                        break;
                                }
                        }
                }

        }
        // mkvore: moved and added "or kad2" in order to answer requests even if we don't know, so that the client don't wait for us
        if (count > 0 or kad2) {
                if (kad2)
		  {DebugSend(Kad2SearchRes(sources), dest);}
                else
		  {DebugSend(KadSearchRes(sources), dest);}
                SendResults(keyID, results, dest, kad2, senderKey);
        }
        Clean();
}

void CIndexed::SendValidNoteResult(const CUInt128& keyID, const CI2PAddress & dest, bool kad2, uint64_t fileSize, const CKadUDPKey& senderKey)
{
        std::list<CEntry*> results ;
        SrcHash* currNoteHash = NULL;
        SrcHashMap::iterator itNote = m_Notes_map.find(keyID);
        uint16_t      count =   0 ;
        if (itNote != m_Notes_map.end()) {
                currNoteHash = itNote->second;
                uint16_t maxResults = 150;

                for (CKadSourcePtrList::iterator itSource = currNoteHash->m_Source_map.begin(); itSource != currNoteHash->m_Source_map.end(); ++itSource ) {
                        Source* currNote = *itSource;
                        if (currNote->entryList.size()) {
                                Kademlia::CEntry* currName = currNote->entryList.front();
                                if (count < maxResults) {
                                        if (!fileSize || !currName->m_uSize || fileSize == currName->m_uSize) {
                                                results.push_back( currName );
                                                count++;
                                        }
                                } else {
                                        break;
                                }
                        }
                }

        }
        // mkvore: moved and added "or kad2" in order to answer requests even if we don't know, so that the client don't wait for us
        if (count > 0 or kad2) {
                if (kad2)
		  {DebugSend(Kad2SearchRes(notes), dest);}
                else
		  {DebugSend(KadSearchRes(notes), dest);}
                SendResults(keyID, results, dest, kad2, senderKey);
        }
}

void CIndexed::SendResults ( const CUInt128& keyID, std::list<CEntry*> results, const CI2PAddress & dest, bool kad2, const CKadUDPKey& senderKey )
{
        std::list<CEntry*>::const_iterator ititSource = results.begin();
        CMemFile packetdata( 2 * CPacket::UDPPacketMaxDataSize() ) ;
        uint16_t count = 0;
        uint64_t posNresults = 0;
        size_t posLastResult = 0 ;
        bool initPacket = true ;
        do { // mkvore: send an answer even when there is not
                // packet initialization
                if (initPacket) {
                        packetdata.Reset();
                        if (kad2) {
                                packetdata.WriteUInt128(Kademlia::CKademlia::GetPrefs()->GetKadID());
                        }
                        packetdata.WriteUInt128 ( keyID );
                        posNresults = packetdata.GetPosition() ;
                        packetdata.WriteUInt16 ( 0 );           // number of results. Will be overriden by "count"
                        count = 0;
                        initPacket = false ;                    // unset initPacket flag
                        posLastResult = (uint32_t)packetdata.GetPosition() ; // packet length before writing the result
                }
                if (ititSource!=results.end()) { // mkvore: send an answer even when there is not
                        Kademlia::CEntry * currName = ( *ititSource ) ;
                        packetdata.WriteUInt128 ( currName->m_uSourceID );
                        currName->WriteTagList( &packetdata ) ;

                        // test : if packet size is too big, revert last write
                        //        and reset initPacket flag
                        if (packetdata.GetPosition() > CPacket::UDPPacketMaxDataSize()) {
                                if (count!=0) {
                                        initPacket = true ;
                                } else {
                                        ++ititSource ;  // forget this result if it cannot fit in a packet
                                }
                        } else {
                                posLastResult = (uint32_t)packetdata.GetPosition() ; // packet length after writing the result
                                count++;
                                ++ititSource ;
                        }
                }
                // test : send packet if it is full or if we reached its max size
                if( initPacket or (ititSource==results.end()) ) {
                        packetdata.Seek ( posNresults );
                        packetdata.WriteUInt16 ( count );
                        packetdata.SetLength( posLastResult ) ;
                        if (kad2) {
                                DebugSend(Kad2SearchRes, dest);
                                CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA2_SEARCH_RES, dest, senderKey, NULL);
                        } else {
                                DebugSend(KadSearchRes,  dest);
                                CKademlia::GetUDPListener()->SendPacket(packetdata, KADEMLIA_SEARCH_RES, dest , senderKey, NULL );
                        }
                }
                // loop to previous result if it has been discarded
                if ( initPacket ) {
                        AddDebugLogLineN(logKadIndex, CFormat ( wxT ( "KadSearchRes to %x : send more results" ) ) % dest.hashCode() );
                }

        } while (ititSource!=results.end()); // mkvore: send an answer even when there is not
}

bool CIndexed::SendStoreRequest(const CUInt128& keyID)
{
        Load* load = NULL;
        LoadMap::iterator it = m_Load_map.find(keyID);
        if (it != m_Load_map.end()) {
                load = it->second;
                if (load->time < (uint32_t)time(NULL)) {
                        m_Load_map.erase(it);
                        m_totalIndexLoad--;
                        delete load;
                        return true;
                }
                return false;
        }
        return true;
}

SSearchTerm::SSearchTerm()
{
        type = AND;
        tag = NULL;
        left = NULL;
        right = NULL;
}

SSearchTerm::~SSearchTerm()
{
        if (type == String) {
                delete astr;
        }
        delete tag;
}

uint32_t CIndexed::GetSourceCount ( const CUInt128& keyID ) const

{
        SrcHashMap::const_iterator itSrcHash = m_Sources_map.find ( keyID );

        if ( itSrcHash == m_Sources_map.end() ) {
                return 0;
        } else
                return itSrcHash->second->m_Source_map.size();
}

uint32_t CIndexed::GetCompleteSourceCount ( const CUInt128& keyID ) const
{
        SrcHashMap::const_iterator itSrcHash = m_Sources_map.find ( keyID );

        if ( itSrcHash == m_Sources_map.end() ) {
                return 0;
        }

        const SrcHash& currSrcHash = *(itSrcHash->second);

        // counting complete sources
        uint32_t complete = 0;

        for ( CKadSourcePtrList::const_iterator itSource = currSrcHash.m_Source_map.begin();
                        itSource != currSrcHash.m_Source_map.end();
                        ++itSource ) {
                for (CKadEntryPtrList::const_iterator itEntry = (*itSource)->entryList.begin();
                                itEntry != (*itSource)->entryList.end();
                                itEntry++ ) {
                        uint32_t flags = (uint32_t) (*itEntry)->GetIntTagValue ( TAG_FLAGS ) ;
                        complete += ( ( flags & TAG_FLAGS_COMPLETEFILE ) ? 1 : 0 );
                }
        }

        return complete ;
}
// File_checked_for_headers
