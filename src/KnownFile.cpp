//
// This file is part of the imule Project.
//
// Parts of this file are based on work from pan One (http://home-3.tiscali.nl/~meost/pms/)
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


#include "KnownFile.h"		// Do_not_auto_remove

#include <protocol/kad/Constants.h>
#include <protocol/ed2k/Client2Client/TCP.h>
#include <protocol/ed2k/ClientSoftware.h>
#include <protocol/Protocols.h>
#include <tags/FileTags.h>

#include <wx/config.h>

#ifdef CLIENT_GUI
#include "UpDownClientEC.h"	// Needed for CUpDownClient
#else
#include "updownclient.h"	// Needed for CUpDownClient
#endif

#include "MemFile.h"		// Needed for CMemFile
#include "Tag.h"		// Needed for CTag
#include "Packet.h"		// Needed for CPacket
#include "Preferences.h"	// Needed for CPreferences
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "amule.h"		// Needed for theApp
#include "PartFile.h"		// Needed for SavePartFile
#include "ClientList.h" 	// Needed for clientlist (buddy support)
#include "Logger.h"
#include <tags/FileTags.h>
#include "kademlia/kademlia/Entry.h"
#include "GuiEvents.h"		// Needed for Notify_*
#include "SearchFile.h"		// Needed for CSearchFile
#include "FileArea.h"		// Needed for CFileArea
#include "FileAutoClose.h"	// Needed for CFileAutoClose
#include "Server.h"			// Needed for CServer

#include "CryptoPP_Inc.h"       // Needed for MD4

#include <common/Format.h>
#include "i2p/CI2PAddress.h"
#include <memory>

CFileStatistic::CFileStatistic() :
        requested(0),
        transferred(0),
        accepted(0),
        alltimerequested(0),
        alltimetransferred(0),
        alltimeaccepted(0)
{
}

#ifndef CLIENT_GUI

void CFileStatistic::AddRequest()
{
        requested++;
        alltimerequested++;
        theApp->knownfiles->requested++;
        theApp->sharedfiles->UpdateItem(fileParent);
}

void CFileStatistic::AddAccepted()
{
        accepted++;
        alltimeaccepted++;
        theApp->knownfiles->accepted++;
        theApp->sharedfiles->UpdateItem(fileParent);
}

void CFileStatistic::AddTransferred(uint64 bytes)
{
        transferred += bytes;
        alltimetransferred += bytes;
        theApp->knownfiles->transferred += bytes;
        theApp->sharedfiles->UpdateItem(fileParent);
}

#endif // CLIENT_GUI


/* Abstract File (base class)*/

CAbstractFile::CAbstractFile()
        :
        m_iRating(0),
        m_hasComment(false),
        m_iUserRating(0),
        m_nFileSize(0)
{
}


CAbstractFile::CAbstractFile(const CAbstractFile& other)
        :
        m_abyFileHash(other.m_abyFileHash),
        m_strComment(other.m_strComment),
        m_iRating(other.m_iRating),
        m_hasComment(other.m_hasComment),
        m_iUserRating(other.m_iUserRating),
        m_taglist(other.m_taglist),
        m_nFileSize(other.m_nFileSize),
        m_fileName(other.m_fileName)
{
        /* // TODO: Currently it's not safe to duplicate the entries, but isn't needed either.
        	CKadEntryPtrList::const_iterator it = other.m_kadNotes.begin();
        	for (; it != other.m_kadNotes.end(); ++it) {
        		m_kadNotes.push_back(new Kademlia::CEntry(**it));
        	}
        */
}


void CAbstractFile::SetFileName(const CPath& fileName)
{
        m_fileName = fileName;
}

uint64_t CAbstractFile::GetIntTagValue(uint8_t tagname) const
{
        for (TagList::const_iterator i=m_taglist.begin(); i!=m_taglist.end(); i++) {
                const CTag & pTag = *i;
                if ((pTag . GetID() == tagname) && pTag . IsInt()) {
                        return pTag . GetInt();
                }
        }
        return 0;
}

bool CAbstractFile::GetIntTagValue(uint8_t tagname, uint64_t& ruValue) const
{
        for (TagList::const_iterator i=m_taglist.begin(); i!=m_taglist.end(); i++) {
                const CTag& pTag = *i;
                if ((pTag . GetID() == tagname) && pTag . IsInt()) {
                        ruValue = pTag . GetInt();
                        return true;
                }
        }
        return false;
}


const wxString& CAbstractFile::GetStrTagValue(uint8 tagname) const
{
        for (TagList::const_iterator i=m_taglist.begin(); i!=m_taglist.end(); i++) {
                const CTag& pTag = *i;
                if (pTag . GetID()==tagname && pTag . IsStr()) {
                        return pTag . GetStr();
                }
        }
        return EmptyString;
}

#ifdef _no_tagnames_in_imule_
const wxString& CAbstractFile::GetStrTagValue(const wxString& tagname) const
{
        ArrayOfCTag::const_iterator it = m_taglist.begin();
        for (; it != m_taglist.end(); ++it) {
                if ((*it).IsStr() && ((*it).GetName() == tagname)) {
                        return (*it).GetStr();
                }
        }
        return EmptyString;
}
#endif

const CTag& CAbstractFile::GetTag(uint8_t tagname, uint8_t tagtype) const
{
        for (TagList::const_iterator i=m_taglist.begin(); i!=m_taglist.end(); i++) {
                const CTag& pTag = *i;
                if ((pTag . GetID()==tagname) && pTag . GetType()==tagtype) {
                        return pTag;
                }
        }
        return CTag::null;
}

#ifdef _no_tag_pointer_in_imule

const CTag *CAbstractFile::GetTag(uint8 tagname) const
{
        ArrayOfCTag::const_iterator it = m_taglist.begin();
        for (; it != m_taglist.end(); ++it) {
                if ((*it).GetNameID() == tagname) {
                        return &(*it);
                }
        }
        return NULL;
}

#endif
const CTag& CAbstractFile::GetTag(uint8_t tagname) const
{
        for (TagList::const_iterator i=m_taglist.begin(); i!=m_taglist.end(); i++) {
                const CTag& pTag = *i;
                if (pTag . GetID()==tagname) {
                        return pTag;
                }
        }
        return CTag::null;
}

void CAbstractFile::AddTagUnique(const CTag& pTag)
{
        for (TagList::iterator i=m_taglist.begin(); i!=m_taglist.end(); i++) {
                CTag& pCurTag = (*i);
                if ( (pCurTag . GetID()!=0 && pCurTag . GetID()==pTag . GetID())
                                && pCurTag . GetType() == pTag . GetType()) {
                        (*i) = pTag;
                        return;
                }
        }
        m_taglist.push_back(pTag);
}

#ifndef CLIENT_GUI
bool CAbstractFile::AddNote(Kademlia::CEntry &pEntry)
{
        bool added = true ;
        CKadEntriesList::iterator it = m_kadNotes.begin();
        for (; it != m_kadNotes.end(); ++it) {
                const Kademlia::CEntry& entry = *it;
                //if(entry->ip == pEntry->ip || !entry->sourceID.compareTo(pEntry->sourceID)) {
                if(entry.m_udpdest == pEntry.m_udpdest || entry.m_uSourceID == pEntry.m_uSourceID) {
                        m_kadNotes.erase(it);
                        added = false ;
                        break ;
                }
        }
        m_kadNotes.push_front(pEntry);
        return added;
}
#else
bool CAbstractFile::AddNote(Kademlia::CEntry &)
{
        return false ;
}
#endif

/* Known File */

CKnownFile::CKnownFile()
{
        Init();
}

CKnownFile::CKnownFile(uint32 ecid) : CECID(ecid)
{
        Init();
}


//#warning Experimental: Construct a CKnownFile from a CSearchFile
CKnownFile::CKnownFile(const CSearchFile &searchFile)
        :
// This will copy the file hash
        CAbstractFile(static_cast<const CAbstractFile &>(searchFile))
{
        Init();

        // Use CKnownFile::SetFileName()
        SetFileName(searchFile.GetFileName());

        // Use CKnownFile::SetFileSize()
        SetFileSize(searchFile.GetFileSize());

}


void CKnownFile::Init()
{
        m_showSources = false;
        m_showPeers = false;
        m_nCompleteSourcesTime = time(NULL);
        m_nSourcesCount = 0;
        m_nCompleteSourcesCount = 0;
        m_nCompleteSourcesCountLo = 0;
        m_nCompleteSourcesCountHi = 0;
        m_bCommentLoaded = false;
        m_iPartCount = 0;
        m_iED2KPartCount = 0;
        m_iED2KPartHashCount = 0;
        m_PublishedED2K = false;
        kadFileSearchID = 0;
        m_lastPublishTimeKadSrc = 0;
        m_lastPublishTimeKadNotes = 0;
        m_lastDateChanged = 0;
        m_bAutoUpPriority = thePrefs::GetNewAutoUp();
        m_iUpPriority = ( m_bAutoUpPriority ) ? PR_HIGH : PR_NORMAL;

        statistic.fileParent = this;

#ifndef CLIENT_GUI
        m_pAICHHashSet = new CAICHHashSet(this);
#endif
}


void CKnownFile::SetFileSize(uint64 nFileSize)
{
        CAbstractFile::SetFileSize(nFileSize);
#ifndef CLIENT_GUI
        m_pAICHHashSet->SetFileSize(nFileSize);
#endif

        // Examples of parthashs, hashsets and filehashs for different filesizes
        // according the ed2k protocol
        //----------------------------------------------------------------------
        //
        //File size: 3 bytes
        //File hash: 2D55E87D0E21F49B9AD25F98531F3724
        //Nr. hashs: 0
        //
        //
        //File size: 1*PARTSIZE
        //File hash: A72CA8DF7F07154E217C236C89C17619
        //Nr. hashs: 2
        //Hash[  0]: 4891ED2E5C9C49F442145A3A5F608299
        //Hash[  1]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
        //
        //
        //File size: 1*PARTSIZE + 1 byte
        //File hash: 2F620AE9D462CBB6A59FE8401D2B3D23
        //Nr. hashs: 2
        //Hash[  0]: 121795F0BEDE02DDC7C5426D0995F53F
        //Hash[  1]: C329E527945B8FE75B3C5E8826755747
        //
        //
        //File size: 2*PARTSIZE
        //File hash: A54C5E562D5E03CA7D77961EB9A745A4
        //Nr. hashs: 3
        //Hash[  0]: B3F5CE2A06BF403BFB9BFFF68BDDC4D9
        //Hash[  1]: 509AA30C9EA8FC136B1159DF2F35B8A9
        //Hash[  2]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
        //
        //
        //File size: 3*PARTSIZE
        //File hash: 5E249B96F9A46A18FC2489B005BF2667
        //Nr. hashs: 4
        //Hash[  0]: 5319896A2ECAD43BF17E2E3575278E72
        //Hash[  1]: D86EF157D5E49C5ED502EDC15BB5F82B
        //Hash[  2]: 10F2D5B1FCB95C0840519C58D708480F
        //Hash[  3]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
        //
        //
        //File size: 3*PARTSIZE + 1 byte
        //File hash: 797ED552F34380CAFF8C958207E40355
        //Nr. hashs: 4
        //Hash[  0]: FC7FD02CCD6987DCF1421F4C0AF94FB8
        //Hash[  1]: 2FE466AF8A7C06DA3365317B75A5ACFE
        //Hash[  2]: 873D3BF52629F7C1527C6E8E473C1C30
        //Hash[  3]: BCE50BEE7877BB07BB6FDA56BFE142FB
        //

        // File size       Data parts      ED2K parts      ED2K part hashs
        // ---------------------------------------------------------------
        // 1..PARTSIZE-1   1               1               0(!)
        // PARTSIZE        1               2(!)            2(!)
        // PARTSIZE+1      2               2               2
        // PARTSIZE*2      2               3(!)            3(!)
        // PARTSIZE*2+1    3               3               3

        if (nFileSize == 0) {
                //wxFAIL; // Kry - Why commented out by lemonfan? it can never be 0
                m_iPartCount = 0;
                m_iED2KPartCount = 0;
                m_iED2KPartHashCount = 0;
                m_sizeLastPart = 0;
                return;
        }

        // nr. of data parts
        m_iPartCount = nFileSize / PARTSIZE + 1;
        // size of last part
        m_sizeLastPart = nFileSize % PARTSIZE;
        // file with size of n * PARTSIZE
        if (m_sizeLastPart == 0) {
                m_sizeLastPart = PARTSIZE;
                m_iPartCount--;
        }

        // nr. of parts to be used with OP_FILESTATUS
        m_iED2KPartCount = nFileSize / PARTSIZE + 1;

        // nr. of parts to be used with OP_HASHSETANSWER
        m_iED2KPartHashCount = nFileSize / PARTSIZE;
        if (m_iED2KPartHashCount != 0) {
                m_iED2KPartHashCount += 1;
        }
}


void CKnownFile::AddUploadingClient(CUpDownClient* client)
{
        m_ClientUploadList.insert(CCLIENTREF(client, wxT("CKnownFile::AddUploadingClient m_ClientUploadList")));

        SourceItemType type = UNAVAILABLE_SOURCE;
        switch (client->GetUploadState()) {
        case US_UPLOADING:
        case US_ONUPLOADQUEUE:
                type = AVAILABLE_SOURCE;
                break;
        default: {
                // Any other state is UNAVAILABLE_SOURCE by default.
        }
        }

        Notify_SharedCtrlAddClient(this, CCLIENTREF(client, wxT("CKnownFile::AddUploadingClient Notify_SharedCtrlAddClient")), type);

        UpdateAutoUpPriority();
}


void CKnownFile::RemoveUploadingClient(CUpDownClient* client)
{
        if (m_ClientUploadList.erase(CCLIENTREF(client, wxEmptyString))) {
                Notify_SharedCtrlRemoveClient(client->ECID(), this);
                UpdateAutoUpPriority();
        }
}


#ifdef CLIENT_GUI

CKnownFile::CKnownFile(CEC_SharedFile_Tag *tag) : CECID(tag->ID())
{
        Init();

        m_abyFileHash = tag->FileHash();
        SetFileSize(tag->SizeFull());
        m_AvailPartFrequency.insert(m_AvailPartFrequency.end(), m_iPartCount, 0);
        m_queuedCount = 0;
}

CKnownFile::~CKnownFile()
{
}

void CKnownFile::UpdateAutoUpPriority()
{
}


#else // ! CLIENT_GUI

CKnownFile::~CKnownFile()
{
        SourceSet::iterator it = m_ClientUploadList.begin();
        for ( ; it != m_ClientUploadList.end(); ++it ) {
                it->ClearUploadFileID();
        }

        delete m_pAICHHashSet;
}


void CKnownFile::SetFilePath(const CPath& filePath)
{
        m_filePath = filePath;
}


bool CKnownFile::WriteHashsetToFile(CFileDataIO* file)
{
        // hashset
        file->WriteHash(m_abyFileHash);
        AddDebugLogLineN( logKnownFiles, CFormat( wxT("Write file hash : %s") ) % m_abyFileHash.Encode() );

        size_t parts = m_hashlist.size();
        file->WriteUInt64(parts);
        AddDebugLogLineN( logKnownFiles, CFormat( wxT("Write parts : %lu") ) % parts );

        for (size_t i = 0; i < parts; i++) {
                file->WriteHash(m_hashlist[i]);
                AddDebugLogLineN( logKnownFiles, CFormat( wxT("Write m_hashlist[%d]=%s") ) % i % m_hashlist[i].Encode() );
        }
        return true;
}


// needed for memfiles. its probably better to switch everything to CFile...
bool CKnownFile::LoadHashsetFromFile(const CFileDataIO* file, bool checkhash)
{
        CMD4Hash checkid = file->ReadHash();

        uint16 parts = file->ReadUInt64();
        m_hashlist.clear();
        for (uint16 i = 0; i < parts; ++i) {
                CMD4Hash cur_hash = file->ReadHash();
                m_hashlist.push_back(cur_hash);
        }

        // SLUGFILLER: SafeHash - always check for valid m_hashlist
        if (!checkhash) {
                m_abyFileHash = checkid;
                if (parts <= 1) {	// nothing to check
                        return true;
                }
        } else {
                if ( m_abyFileHash != checkid ) {
                        return false;	// wrong file?
                } else {
                        if (parts != GetED2KPartHashCount()) {
                                return false;
                        }
                }
        }
        // SLUGFILLER: SafeHash

        // trust noone ;-)
        // lol, useless comment but made me lmao
        // wtf you guys are weird.

        if (!m_hashlist.empty()) {
                CreateHashFromHashlist(m_hashlist, &checkid);
        }

        if ( m_abyFileHash == checkid ) {
                return true;
        } else {
                m_hashlist.clear();
                return false;
        }
}


bool CKnownFile::WriteTagsToFile(CFileDataIO* file, TagList additionnal_tags)
{
        TagList tags = additionnal_tags;

        tags.push_back(CTag(TAG_FILENAME, CPath::ToUniv(GetFileName())));

        tags.push_back(CTag(TAG_FILESIZE, GetFileSize()));

        // statistic
        uint64_t tran;
        tran=statistic.alltimetransferred ;
        tags.push_back(CTag(TAG_ATTRANSFERRED, tran));

        tags.push_back(CTag(TAG_ATREQUESTED, uint64_t(statistic.GetAllTimeRequests())));

        tags.push_back(CTag(TAG_ATACCEPTED, uint64_t(statistic.GetAllTimeAccepts())));

        // priority N permission
        tags.push_back(CTag(TAG_ULPRIORITY, uint64_t(IsAutoUpPriority() ? PR_AUTO : m_iUpPriority)));

        //AICH Filehash
        if (HasProperAICHHashSet()) {
                tags.push_back(CTag(TAG_AICH_HASH, m_pAICHHashSet->GetMasterHash().GetString()));
        }

        // Kad sources
        if (m_lastPublishTimeKadSrc) {
                tags.push_back(CTag(TAG_KADLASTPUBLISHSRC, uint64_t(m_lastPublishTimeKadSrc)));
        }

        // Kad notes
        if (m_lastPublishTimeKadNotes) {
                tags.push_back(CTag(TAG_KADLASTPUBLISHNOTES, uint64_t(m_lastPublishTimeKadNotes)));
        }

        //other tags
        /*for (size_t j = 0; j < m_taglist.GetCount(); j++){
        	if (m_taglist[j]->IsInt() || m_taglist[j]->IsStr()) {
        		m_taglist[j]->WriteTo(file);
        	}
        }*/
        for (TagList::const_iterator i = m_taglist.begin(); i!=m_taglist.end(); i++)
                tags.push_back(*i);
        tags.WriteTo(file);
        return true;
}



bool CKnownFile::LoadTagsFromFile(const CFileDataIO* file)
{
        TagList taglist(*file, true);
        for (TagList::const_iterator it=taglist.begin(); it!=taglist.end(); it++) {
                const CTag & newtag = *it;
                switch(newtag.GetID()) {

                case TAG_FILENAME:
                        {
                                // Unlike eMule, we actually prefer the second
                                // filename tag, since we use it to specify the
                                // 'universial' filename (see CPath::ToUniv).
                                CPath path = CPath::FromUniv(newtag.GetStr());

                                // May be invalid, if from older versions where
                                // filenames were saved as UTF8 utf8strOptBOM.
                                if (path.IsOk()) {
                                        SetFileName(path);
                                }
                                else {
                                        SetFileName(CPath(newtag.GetStr()));
                                }
                        }
                        break;

                case TAG_FILESIZE:
                        SetFileSize(newtag.GetInt());
                        m_AvailPartFrequency.clear();
                        m_AvailPartFrequency.insert(
                                m_AvailPartFrequency.begin(),
                                GetPartCount(), 0);
                        break;

                case TAG_ATTRANSFERRED:
                        statistic.alltimetransferred += newtag.GetInt();
                        break;

                case TAG_ATREQUESTED:
                        statistic.alltimerequested = (uint32_t)newtag.GetInt();
                        break;

                case TAG_ATACCEPTED:
                        statistic.alltimeaccepted = (uint32_t)newtag.GetInt();
                        break;

                case TAG_ULPRIORITY:
                        m_iUpPriority = (uint8_t) newtag.GetInt();
                        if( m_iUpPriority == PR_AUTO ) {
                                m_iUpPriority = PR_HIGH;
                                m_bAutoUpPriority = true;
                        } else {
                                if (	m_iUpPriority != PR_VERYLOW &&
                                                m_iUpPriority != PR_LOW &&
                                                m_iUpPriority != PR_NORMAL &&
                                                m_iUpPriority != PR_HIGH &&
                                                m_iUpPriority != PR_VERYHIGH &&
                                                m_iUpPriority != PR_POWERSHARE) {
                                        m_iUpPriority = PR_NORMAL;
                                }

                                m_bAutoUpPriority = false;
                        }
                        break;

                case TAG_PERMISSIONS:
                        // Ignore it, it's not used anymore.
                        break;

                case TAG_AICH_HASH: {
                        CAICHHash hash;
                        bool hashSizeOk =
                                hash.DecodeBase32(newtag.GetStr()) == CAICHHash::GetHashSize();
                        wxASSERT(hashSizeOk);
                        if (hashSizeOk) {
                                m_pAICHHashSet->SetMasterHash(hash, AICH_HASHSETCOMPLETE);
                        }
                        break;
                }

                case TAG_KADLASTPUBLISHSRC:
                        SetLastPublishTimeKadSrc( newtag.GetInt() );

                        if(GetLastPublishTimeKadSrc() > (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES) {
                                //There may be a posibility of an older client that saved a random number here.. This will check for that..
                                SetLastPublishTimeKadSrc(0);
                        }
                        break;

                case TAG_KADLASTPUBLISHNOTES:
                        SetLastPublishTimeKadNotes( newtag.GetInt() );
                        break;

                case TAG_KADLASTPUBLISHKEY:
                        // Just purge it
                        wxASSERT( newtag.IsInt() );
                        break;

                case TAG_SOURCES :
                case TAG_COMPLETE_SOURCES :
                        break;
                default:
                        // Store them here and write them back on saving.
                        m_taglist.push_back(newtag);
                }
        }

        // overwrite these values : must wait for Kad network answers
        // before setting them
        AddTagUnique( CTag(TAG_SOURCES,  uint64_t(0) ) );
        AddTagUnique( CTag(TAG_COMPLETE_SOURCES, uint64_t(0) ) );
        return true;
}


bool CKnownFile::WriteDateToFile(CFileDataIO* file)
{
        file->WriteUInt32(m_lastDateChanged);
        return true;
}
bool CKnownFile::LoadDateFromFile(const CFileDataIO* file)
{
        m_lastDateChanged = file->ReadUInt32();

        return true;
}


bool CKnownFile::LoadFromFile(const CFileDataIO* file)
{
        // SLUGFILLER: SafeHash - load first, verify later
        bool ret1 = LoadDateFromFile(file);
        bool ret2 = LoadHashsetFromFile(file,false);
        bool ret3 = LoadTagsFromFile(file);
        UpdatePartsInfo();
        // Final hash-count verification, needs to be done after the tags are loaded.
        AddDebugLogLineN(logKnownFiles,
                         CFormat( wxT("Load from file : ret1=%s, ret2=%s, ret3=%s, GetED2KPartHashCount=%lld, GetHashCount=%lld") )
                         % (ret1 ? wxT("success") : wxT("failure"))
                         % (ret2 ? wxT("success") : wxT("failure"))
                         % (ret3 ? wxT("success") : wxT("failure"))
                         % GetED2KPartHashCount()
                         % GetHashCount() ) ;
        return ret1 && ret2 && ret3 && GetED2KPartHashCount()==GetHashCount();
        // SLUGFILLER: SafeHash
}


bool CKnownFile::WriteToFile(CFileDataIO* file)
{
        wxCHECK(!IsPartFile(), false);

        bool ret1 = WriteDateToFile(file);
        bool ret2 = WriteHashsetToFile(file);
        bool ret3 = WriteTagsToFile(file);
        return ret1 && ret2 && ret3;
}


void CKnownFile::CreateHashFromHashlist(const ArrayOfCMD4Hash& hashes, CMD4Hash* Output)
{
        wxCHECK_RET(hashes.size(), wxT("No input to hash from in CreateHashFromHashlist"));

        std::vector<byte> buffer(hashes.size() * MD4HASH_LENGTH);
        std::vector<byte>::iterator it = buffer.begin();

        for (size_t i = 0; i < hashes.size(); ++i) {
                it = STLCopy_n(hashes[i].GetHash(), MD4HASH_LENGTH, it);
        }

        CreateHashFromInput(&buffer[0], buffer.size(), Output, NULL);
}


void CKnownFile::CreateHashFromFile(CFileAutoClose& file, uint64 offset, uint32 Length, CMD4Hash* Output, CAICHHashTree* pShaHashOut)
{
        wxCHECK_RET(Length, wxT("No input to hash from in CreateHashFromFile"));

        CFileArea area;
        area.ReadAt(file, offset, Length);

        CreateHashFromInput(area.GetBuffer(), Length, Output, pShaHashOut);
        area.CheckError();
}


void CKnownFile::CreateHashFromInput(const byte* input, uint32 Length, CMD4Hash* Output, CAICHHashTree* pShaHashOut )
{
        wxASSERT_MSG(Output || pShaHashOut, wxT("Nothing to do in CreateHashFromInput"));
        { wxCHECK_RET(input, wxT("No input to hash from in CreateHashFromInput")); }
        wxASSERT(Length <= PARTSIZE); // We never hash more than one PARTSIZE

        CMemFile data(input, Length);

        uint32 Required = Length;
        byte   X[64*128];

        uint32	posCurrentEMBlock = 0;
        uint32	nIACHPos = 0;
        std::unique_ptr<CAICHHashAlgo> pHashAlg(CAICHHashSet::GetNewHashAlgo());

        // This is all AICH.
        while (Required >= 64) {
                uint32 len = Required / 64;
                if (len > sizeof(X)/(64 * sizeof(X[0]))) {
                        len = sizeof(X)/(64 * sizeof(X[0]));
                }

                data.Read(&X, len * 64);

                // SHA hash needs 180KB blocks
                if (pShaHashOut) {
                        if (nIACHPos + len*64 >= EMBLOCKSIZE) {
                                uint32 nToComplete = EMBLOCKSIZE - nIACHPos;
                                pHashAlg->Add(X, nToComplete);
                                wxASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
                                pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg.get());
                                posCurrentEMBlock += EMBLOCKSIZE;
                                pHashAlg->Reset();
                                pHashAlg->Add(X+nToComplete,(len*64) - nToComplete);
                                nIACHPos = (len*64) - nToComplete;
                        } else {
                                pHashAlg->Add(X, len*64);
                                nIACHPos += len*64;
                        }
                }

                Required -= len*64;
        }
        // bytes to read
        Required = Length % 64;
        if (Required != 0) {
                data.Read(&X,Required);

                if (pShaHashOut != NULL) {
                        if (nIACHPos + Required >= EMBLOCKSIZE) {
                                uint32 nToComplete = EMBLOCKSIZE - nIACHPos;
                                pHashAlg->Add(X, nToComplete);
                                wxASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
                                pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg.get());
                                posCurrentEMBlock += EMBLOCKSIZE;
                                pHashAlg->Reset();
                                pHashAlg->Add(X+nToComplete, Required - nToComplete);
                                nIACHPos = Required - nToComplete;
                        } else {
                                pHashAlg->Add(X, Required);
                                nIACHPos += Required;
                        }
                }
        }
        if (pShaHashOut != NULL) {
                if(nIACHPos > 0) {
                        pShaHashOut->SetBlockHash(nIACHPos, posCurrentEMBlock, pHashAlg.get());
                        posCurrentEMBlock += nIACHPos;
                }
                wxASSERT( posCurrentEMBlock == Length );
                wxCHECK2( pShaHashOut->ReCalculateHash(pHashAlg.get(), false), );
        }

        if (Output != NULL) {
#ifdef __WEAK_CRYPTO__
                CryptoPP::Weak::MD4 md4_hasher;
#else
                CryptoPP::MD4 md4_hasher;
#endif
                md4_hasher.CalculateDigest(Output->GetHash(), input, Length);
        }
}


const CMD4Hash& CKnownFile::GetPartHash(uint16 part) const
{
        wxASSERT( part < m_hashlist.size() );

        return m_hashlist[part];
}

CPacket* CKnownFile::CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions)
{
        // Kad reviewed

        if (m_ClientUploadList.empty()) {
                return NULL;
        }

        if ((((CKnownFile*)forClient->GetRequestFile() != this)
                        && ((CKnownFile*)forClient->GetUploadFile() != this)) || forClient->GetUploadFileID() != GetFileHash()) {
                wxString file1 = _("Unknown");
                if (forClient->GetRequestFile() && forClient->GetRequestFile()->GetFileName().IsOk()) {
                        file1 = forClient->GetRequestFile()->GetFileName().GetPrintable();
                } else if (forClient->GetUploadFile() && forClient->GetUploadFile()->GetFileName().IsOk()) {
                        file1 = forClient->GetUploadFile()->GetFileName().GetPrintable();
                }
                wxString file2 = _("Unknown");
                if (GetFileName().IsOk()) {
                        file2 = GetFileName().GetPrintable();
                }
                AddDebugLogLineN(logKnownFiles, wxT("File mismatch on source packet (K) Sending: ") + file1 + wxT("  From: ") + file2);
                return NULL;
        }

        const BitVector& rcvstatus = forClient->GetUpPartStatus();
        bool SupportsUploadChunksState = !rcvstatus.empty();
        //wxASSERT(rcvstatus.size() == GetPartCount()); // Obviously!
        if (rcvstatus.size() != GetPartCount()) {
                // Yuck. Same file but different part count? Seriously fucked up.
                AddDebugLogLineN(logKnownFiles, CFormat(wxT("Impossible situation: different partcounts for the same known file: %i (client) and %i (file)")) % rcvstatus.size() % GetPartCount());
                return NULL;
        }

        CMemFile data(1024);

        uint8 byUsedVersion;
        bool bIsSX2Packet;
        if (forClient->SupportsSourceExchange2() && byRequestedVersion > 0) {
                // the client uses SourceExchange2 and requested the highest version he knows
                // and we send the highest version we know, but of course not higher than his request
                byUsedVersion = std::min(byRequestedVersion, (uint8)SOURCEEXCHANGE2_VERSION);
                bIsSX2Packet = true;
                data.WriteUInt8(byUsedVersion);

                // we don't support any special SX2 options yet, reserved for later use
                if (nRequestedOptions != 0) {
                        AddDebugLogLineN(logKnownFiles, CFormat(wxT("Client requested unknown options for SourceExchange2: %" PRIu16)) % nRequestedOptions);
                }
        } else {
                byUsedVersion = forClient->GetSourceExchange1Version();
                bIsSX2Packet = false;
                if (forClient->SupportsSourceExchange2()) {
                        AddDebugLogLineN(logKnownFiles, wxT("Client which announced to support SX2 sent SX1 packet instead"));
                }
        }

        uint16 nCount = 0;

        data.WriteHash(forClient->GetUploadFileID());
        data.WriteUInt16(nCount);
        uint32 cDbgNoSrc = 0;

        SourceSet::const_iterator it = m_ClientUploadList.begin();
        for ( ; it != m_ClientUploadList.end(); it++ ) {
                const CUpDownClient *cur_src = it->GetClient();

                if (	// cur_src->HasLowID() ||
                        cur_src == forClient ||
                        !(	cur_src->GetUploadState() == US_UPLOADING ||
                                cur_src->GetUploadState() == US_ONUPLOADQUEUE)) {
                        continue;
                }

                bool bNeeded = false;

                if ( SupportsUploadChunksState ) {
                        const BitVector& srcstatus = cur_src->GetUpPartStatus();
                        if ( !srcstatus.empty() ) {
                                //wxASSERT(srcstatus.size() == GetPartCount()); // Obviously!
                                if (srcstatus.size() != GetPartCount()) {
                                        continue;
                                }
                                if ( cur_src->GetUpPartCount() == forClient->GetUpPartCount() ) {
                                        for (int x = 0; x < GetPartCount(); x++ ) {
                                                if ( srcstatus.get(x) && !rcvstatus.get(x) ) {
                                                        // We know the receiving client needs
                                                        // a chunk from this client.
                                                        bNeeded = true;
                                                        break;
                                                }
                                        }
                                }
                        } else {
                                cDbgNoSrc++;
                                // This client doesn't support upload chunk status.
                                // So just send it and hope for the best.
                                bNeeded = true;
                        }
                } else {
                        // remote client does not support upload chunk status,
                        // search sources which have at least one complete part
                        // we could even sort the list of sources by available
                        // chunks to return as much sources as possible which
                        // have the most available chunks. but this could be
                        // a noticeable performance problem.
                        const BitVector& srcstatus = cur_src->GetUpPartStatus();
                        if ( !srcstatus.empty() ) {
                                //wxASSERT(srcstatus.size() == GetPartCount());
                                if (srcstatus.size() != GetPartCount()) {
                                        continue;
                                }
                                for (int x = 0; x < GetPartCount(); x++ ) {
                                        if ( srcstatus.get(x) ) {
                                                // this client has at least one chunk
                                                bNeeded = true;
                                                break;
                                        }
                                }
                        } else {
                                // This client doesn't support upload chunk status.
                                // So just send it and hope for the best.
                                bNeeded = true;
                        }
                }

                if ( bNeeded ) {
                        nCount++;
                        //data.WriteAddress(cur_src->GetServerDest());
                        data.WriteAddress(cur_src->GetTCPDest());
                        //data.WriteAddress(cur_src->GetUDPDest());
                        //data.WriteHash(cur_src->GetUserHash());

                        if (byUsedVersion >= 2) {
                                data.WriteHash(cur_src->GetUserHash());
                        }

                        if (byUsedVersion >= 4) {
                                // CryptSettings - SourceExchange V4
                                // 5 Reserved (!)
                                // 1 CryptLayer Required
                                // 1 CryptLayer Requested
                                // 1 CryptLayer Supported
                                const uint8 uSupportsCryptLayer	= cur_src->SupportsCryptLayer() ? 1 : 0;
                                const uint8 uRequestsCryptLayer	= cur_src->RequestsCryptLayer() ? 1 : 0;
                                const uint8 uRequiresCryptLayer	= cur_src->RequiresCryptLayer() ? 1 : 0;
                                const uint8 byCryptOptions = (uRequiresCryptLayer << 2) | (uRequestsCryptLayer << 1) | (uSupportsCryptLayer << 0);
                                data.WriteUInt8(byCryptOptions);
                        }

                        if (nCount > 500) {
                                break;
                        }
                }
        }

        if (!nCount) {
                return 0;
        }

        data.Seek(bIsSX2Packet ? 17 : 16, wxFromStart);
        data.WriteUInt16(nCount);

        CPacket* result = new CPacket(data, OP_EMULEPROT, bIsSX2Packet ? OP_ANSWERSOURCES2 : OP_ANSWERSOURCES);

        if ( result->GetPacketSize() > 354 ) {
                result->PackPacket();
        }

        return result;
}


void CKnownFile::CreateOfferedFilePacket(
        CMemFile *files,
        CServer *pServer,
        CUpDownClient *pClient)
{

        // This function is used for offering files to the local server and for sending
        // shared files to some other client. In each case we send our IP+Port only, if
        // we have a HighID.

        wxASSERT(!(pClient && pServer));

        SetPublishedED2K(true);
        files->WriteHash(GetFileHash());

        uint32 nClientID = 0;
//         uint16 nClientPort = 0;

        if (pServer) {
                if (pServer->GetTCPFlags() & SRV_TCPFLG_COMPRESSION) {
#define FILE_COMPLETE_ID		0xfbfbfbfb
#define FILE_COMPLETE_PORT	0xfbfb
#define FILE_INCOMPLETE_ID	0xfcfcfcfc
#define FILE_INCOMPLETE_PORT	0xfcfc
                        // complete   file: ip 251.251.251 (0xfbfbfbfb) port 0xfbfb
                        // incomplete file: op 252.252.252 (0xfcfcfcfc) port 0xfcfc
                        if (GetStatus() == PS_COMPLETE) {
                                nClientID = FILE_COMPLETE_ID;
//                                 nClientPort = FILE_COMPLETE_PORT;
                        } else {
                                nClientID = FILE_INCOMPLETE_ID;
//                                 nClientPort = FILE_INCOMPLETE_PORT;
                        }
                } else {
                        if (theApp->IsConnectedED2K() && !::IsLowID(theApp->GetED2KID())) {
                                nClientID = theApp->GetID();
//                                 nClientPort = thePrefs::GetPort();
                        }
                }
        } else {
                // Do not merge this with the above case - this one
                // also checks Kad status.
                if (theApp->IsConnected() && !theApp->IsFirewalled()) {
                        nClientID = theApp->GetID();
//                         nClientPort = thePrefs::GetPort();
                }
        }

        files->WriteUInt32(nClientID);
//         files->WriteUInt16(nClientPort);

        TagList tags;
        AddDebugLogLineN(logServer, CFormat(wxT("CreateOfferedFilePacket: \"%s\" size %u"))%GetFileName().GetPrintable()%GetFileSize());
        // The printable filename is used because it's destined for another user.
        tags.push_back(CTag(TAG_FILENAME, GetFileName().GetPrintable()));
        tags.push_back(CTag(TAG_FILESIZE, GetFileSize()));

        if (GetFileRating()) {
                tags.push_back(CTagVarInt(TAG_FILERATING, GetFileRating(), (pClient && pClient->GetVBTTags()) ? 0 : 32));
        }

        // NOTE: Archives and CD-Images are published+searched with file type "Pro"
        bool bAddedFileType = false;
        if (pServer && (pServer->GetTCPFlags() & SRV_TCPFLG_TYPETAGINTEGER)) {
                // Send integer file type tags to newer servers
                EED2KFileType eFileType = GetED2KFileTypeSearchID(GetED2KFileTypeID(GetFileName()));
                if (eFileType >= ED2KFT_AUDIO && eFileType <= ED2KFT_CDIMAGE) {
                        tags.push_back(CTagInt32(TAG_FILETYPE, eFileType));
                        bAddedFileType = true;
                }
        }
        if (!bAddedFileType) {
                // Send string file type tags to:
                //	- newer servers, in case there is no integer type available for the file type (e.g. emulecollection)
                //	- older servers
                //	- all clients
                wxString strED2KFileType(GetED2KFileTypeSearchTerm(GetED2KFileTypeID(GetFileName())));
                if (!strED2KFileType.IsEmpty()) {
                        tags.push_back(CTagString(TAG_FILETYPE, strED2KFileType));
                }
        }

        // There, we could add MetaData info, if we ever get to have that.

        tags.WriteTo(files);
        
        //         EUtf8Str eStrEncode;
        // 
        //         bool unicode_support =
        //                 // eservers that support UNICODE.
        //                 (pServer && (pServer->GetUnicodeSupport()))
        //                 ||
        //                 // clients that support unicode
        //                 (pClient && pClient->GetUnicodeSupport());
        //         eStrEncode = unicode_support ? utf8strRaw : utf8strNone;
        //         files->WriteUInt32(tags.size());
        // 
        //         for (TagList::iterator it = tags.begin(); it != tags.end(); ++it ) {
        //                 CTag& pTag = *it;
        //                 pTag.WriteTo(files, eStrEncode);
        //         }
}


// Updates priority of file if autopriority is activated
void CKnownFile::UpdateAutoUpPriority()
{
        if (IsAutoUpPriority()) {
                uint32 queued = GetQueuedCount();
                uint8 priority = PR_NORMAL;

                if (queued > 20) {
                        priority = PR_LOW;
                } else if (queued > 1) {
                        priority = PR_NORMAL;
                } else {
                        priority = PR_HIGH;
                }

                if (GetUpPriority() != priority) {
                        SetUpPriority(priority, false);
                        Notify_SharedFilesUpdateItem(this);
                }
        }
}

void CKnownFile::SetFileCommentRating(const wxString& strNewComment, int8 iNewRating)
{
        if (m_strComment != strNewComment || m_iRating != iNewRating) {
                SetLastPublishTimeKadNotes(0);
                wxString strCfgPath = wxT("/") + m_abyFileHash.Encode() + wxT("/");

                wxConfigBase* cfg = wxConfigBase::Get();
                if (strNewComment.IsEmpty() && iNewRating == 0) {
                        cfg->DeleteGroup(strCfgPath);
                } else {
                        cfg->Write( strCfgPath + wxT("Comment"), strNewComment);
                        cfg->Write( strCfgPath + wxT("Rate"), (int)iNewRating);
                }

                m_strComment = strNewComment;

                m_iRating = iNewRating;

                SourceSet::iterator it = m_ClientUploadList.begin();
                for ( ; it != m_ClientUploadList.end(); it++ ) {
                        it->SetCommentDirty();
                }
        }
}


void CKnownFile::SetUpPriority(uint8 iNewUpPriority, bool m_bsave)
{
        m_iUpPriority = iNewUpPriority;
        if( IsPartFile() && m_bsave ) {
                ((CPartFile*)this)->SavePartFile();
        }
}

void CKnownFile::SetPublishedED2K(bool val)
{
        m_PublishedED2K = val;
        Notify_SharedFilesUpdateItem(this);
}

bool CKnownFile::PublishNotes()
{
        if(m_lastPublishTimeKadNotes > (uint32)time(NULL)) {
                return false;
        }

        if(!GetFileComment().IsEmpty()) {
                m_lastPublishTimeKadNotes = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMEN;
                return true;
        }

        if(GetFileRating() != 0) {
                m_lastPublishTimeKadNotes = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMEN;
                return true;
        }

        return false;
}

bool CKnownFile::PublishSrc()
{
        if( GetNextPublishTimeKadSrc() > time(NULL)) {
                return false;
        }

        SetLastPublishTimeKadSrc((uint32_t)time(NULL));
        return true;

}

void CKnownFile::UpdatePartsInfo()
{
        // Cache part count
        uint16 partcount = GetPartCount();
        bool flag = (time(NULL) - m_nCompleteSourcesTime > 0);

        // Ensure the frequency-list is ready
        if ( m_AvailPartFrequency.size() != GetPartCount() ) {
                m_AvailPartFrequency.clear();
                m_AvailPartFrequency.insert(m_AvailPartFrequency.begin(), GetPartCount(), 0);
        }

        if (flag) {
                ArrayOfUInts16 count;
                count.reserve(m_ClientUploadList.size());

                SourceSet::iterator it = m_ClientUploadList.begin();
                for ( ; it != m_ClientUploadList.end(); it++ ) {
                        CUpDownClient* client = it->GetClient();
                        if ( !client->GetUpPartStatus().empty() && client->GetUpPartCount() == partcount ) {
                                count.push_back(client->GetUpCompleteSourcesCount());
                        }
                }

                m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;

                if( partcount > 0) {
                        m_nCompleteSourcesCount = m_AvailPartFrequency[0];
                }
                for (uint16 i = 1; i < partcount; ++i) {
                        if( m_nCompleteSourcesCount > m_AvailPartFrequency[i]) {
                                m_nCompleteSourcesCount = m_AvailPartFrequency[i];
                        }
                }
                count.push_back(m_nCompleteSourcesCount);

                int32 n = count.size();
                if (n > 0) {
                        std::sort(count.begin(), count.end(), std::less<uint16>());

                        // calculate range
                        int i = n >> 1;			// (n / 2)
                        int j = (n * 3) >> 2;	// (n * 3) / 4
                        int k = (n * 7) >> 3;	// (n * 7) / 8

                        // For complete files, trust the people your uploading to more...

                        // For low guess and normal guess count
                        //	- If we see more sources then the guessed low and
                        //	normal, use what we see.
                        //	- If we see less sources then the guessed low,
                        //	adjust network accounts for 100%, we account for
                        //	0% with what we see and make sure we are still
                        //	above the normal.
                        // For high guess
                        //	Adjust 100% network and 0% what we see.
                        if (n < 20) {
                                if ( count[i] < m_nCompleteSourcesCount ) {
                                        m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
                                } else {
                                        m_nCompleteSourcesCountLo = count[i];
                                }
                                m_nCompleteSourcesCount= m_nCompleteSourcesCountLo;
                                m_nCompleteSourcesCountHi = count[j];
                                if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount ) {
                                        m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
                                }
                        } else {
                                // Many sources..
                                // For low guess
                                //	Use what we see.
                                // For normal guess
                                //	Adjust network accounts for 100%, we account for
                                //	0% with what we see and make sure we are still above the low.
                                // For high guess
                                //	Adjust network accounts for 100%, we account for 0%
                                //	with what we see and make sure we are still above the normal.

                                m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
                                m_nCompleteSourcesCount = count[j];
                                if( m_nCompleteSourcesCount < m_nCompleteSourcesCountLo ) {
                                        m_nCompleteSourcesCount = m_nCompleteSourcesCountLo;
                                }
                                m_nCompleteSourcesCountHi= count[k];
                                if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount ) {
                                        m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
                                }
                        }
                }
                m_nCompleteSourcesTime = time(NULL) + (60);
        }

        Notify_SharedFilesUpdateItem(this);
}


void CKnownFile::UpdateUpPartsFrequency( CUpDownClient* client, bool increment )
{
        if ( m_AvailPartFrequency.size() != GetPartCount() ) {
                m_AvailPartFrequency.clear();
                m_AvailPartFrequency.insert(m_AvailPartFrequency.begin(), GetPartCount(), 0);
                if ( !increment ) {
                        return;
                }
        }

        const BitVector& freq = client->GetUpPartStatus();
        unsigned int size = freq.size();
        if ( size != m_AvailPartFrequency.size() ) {
                return;
        }

        if ( increment ) {
                for ( unsigned int i = 0; i < size; ++i ) {
                        if ( freq.get(i) ) {
                                m_AvailPartFrequency[i]++;
                        }
                }
        } else {
                for ( unsigned int i = 0; i < size; ++i ) {
                        if ( freq.get(i) ) {
                                m_AvailPartFrequency[i]--;
                        }
                }
        }
}

void CKnownFile::ClearPriority()
{
        if ( !m_bAutoUpPriority ) return;
        m_iUpPriority = ( m_bAutoUpPriority ) ? PR_HIGH : PR_NORMAL;
        UpdateAutoUpPriority();
}

void GuessAndRemoveExt(CPath& name)
{
        wxString ext = name.GetExt();

        // Remove common two-part extensions, such as "tar.gz"
        if (ext == wxT("gz") || ext == wxT("bz2")) {
                name = name.RemoveExt();
                if (name.GetExt() == wxT("tar")) {
                        name = name.RemoveExt();
                }
                // might be an extension if length == 3
                // and also remove some common non-three-character extensions
        } else if (ext.Length() == 3  ||
                        ext == wxT("7z")   ||
                        ext == wxT("rm")   ||
                        ext == wxT("jpeg") ||
                        ext == wxT("mpeg")
                  ) {
                name = name.RemoveExt();
        }
}
void CKnownFile::SetFileName(const CPath& filename)
{
        CAbstractFile::SetFileName(filename);
        wordlist.clear();
        // Don't publish extension. That'd kill the node indexing e.g. "avi".
        CPath tmpName = GetFileName();
        GuessAndRemoveExt(tmpName);
        Kademlia::CSearchManager::GetWords(tmpName.GetPrintable(), &wordlist);
}

#endif // CLIENT_GUI

//For File Comment //
void CKnownFile::LoadComment() const
{
#ifndef CLIENT_GUI
        wxString strCfgPath = wxT("/") + m_abyFileHash.Encode() + wxT("/");

        wxConfigBase* cfg = wxConfigBase::Get();

        m_strComment = cfg->Read( strCfgPath + wxT("Comment"), wxEmptyString);
        m_iRating = cfg->Read( strCfgPath + wxT("Rate"), 0l);
#endif

        m_bCommentLoaded = true;

}


wxString CKnownFile::GetAICHMasterHash() const
{
#ifdef CLIENT_GUI
        return m_AICHMasterHash;
#else
        if (HasProperAICHHashSet()) {
                return m_pAICHHashSet->GetMasterHash().GetString();
        }

        return wxEmptyString;
#endif
}


bool CKnownFile::HasProperAICHHashSet() const
{
#ifdef CLIENT_GUI
        return m_AICHMasterHash.Length() != 0;
#else
        return m_pAICHHashSet->HasValidMasterHash() &&
               (m_pAICHHashSet->GetStatus() == AICH_HASHSETCOMPLETE ||
                m_pAICHHashSet->GetStatus() == AICH_VERIFIED);
#endif
}

wxString CKnownFile::GetFeedback() const
{
        return	  wxString(_("File name")) + wxT(": ") + GetFileName().GetPrintable() + wxT("\n")
                  + _("File size") + wxT(": ") + CastItoXBytes(GetFileSize()) + wxT("\n")
                  + _("Share ratio") + CFormat(wxT(": %.2lf%%\n")) % (((double)statistic.GetAllTimeTransferred() / (double)GetFileSize()) * 100.0)
                  + _("Uploaded") + wxT(": ") + CastItoXBytes(statistic.GetTransferred()) + wxT(" (") + CastItoXBytes(statistic.GetAllTimeTransferred()) + wxT(")\n")
                  + _("Requested") + CFormat(wxT(": %" PRIu16 " (%" PRIu32 ")\n")) % statistic.GetRequests() % statistic.GetAllTimeRequests()
                  + _("Accepted") + CFormat(wxT(": %" PRIu16 " (%" PRIu32 ")\n")) % statistic.GetAccepts() % statistic.GetAllTimeAccepts()
                  + _("On Queue") + CFormat(wxT(": %" PRIu16 "\n")) % GetQueuedCount()
                  + _("Complete sources") + CFormat(wxT(": %" PRIu16 "\n")) % m_nCompleteSourcesCount
                  + _("Total sources") + CFormat(wxT(": %" PRIu16 "\n")) % m_nSourcesCount;
}
// File_checked_for_headers