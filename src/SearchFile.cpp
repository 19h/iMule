//
// This file is part of the aMule Project.
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

#include "SearchFile.h"			// Interface declarations.

#include <tags/FileTags.h>

#include "amule.h"				// Needed for theApp
#include "CanceledFileList.h"
#include "MemFile.h"			// Needed for CMemFile
#include "Preferences.h"		// Needed for thePrefs
#include "GuiEvents.h"
#include "Logger.h"
#include "PartFile.h"                  // Needed for CPartFile::CanAddSource
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "KnownFileList.h"		// Needed for CKnownFileList

CSearchFile::CSearchFile(const CMemFile& data, wxUIntPtr searchID, const CI2PAddress & serverDest, const wxString& directory, bool kademlia)
        : m_parent(NULL),
          m_showChildren(false),
          m_searchID(searchID),
          m_sourceCount(0),
          m_completeSourceCount(0),
          m_kademlia(kademlia),
          m_downloadStatus(NEW),
          m_directory(directory),
          m_clientServerDest(serverDest),
          m_kadPublishInfo(0)
{
        m_abyFileHash = data.ReadHash();
        SetDownloadStatus();
        
        if (directory.empty()) // if this is not an OP_ASKSHAREDFILESDIRANS (shared file response)
                m_clientDest  = data.ReadAddress();
        else
                data.ReadUInt32();
        
        if ( !m_clientDest.isValid() ) {
                m_clientDest = CI2PAddress::null;
        }

        m_taglist = TagList(data);
        SetFileName (CPath(GetStrTagValue(TAG_FILENAME)));
	SetFileSize (GetIntTagValue(TAG_FILESIZE));
        m_iUserRating = (uint32_t)(GetIntTagValue ( TAG_FILERATING ) & 0xF) / 3;
        m_sourceCount = (uint32_t)GetIntTagValue ( TAG_SOURCES );
        m_completeSourceCount = (uint32_t)GetIntTagValue ( TAG_COMPLETE_SOURCES );
        AddDebugLogLineN(logServer, CFormat(wxT("Shared file entry received : \"%s\" size=%u")) % GetFileName() %GetFileSize());
        if (!GetFileName().IsOk()) {
                throw CInvalidPacket(wxT("No filename in search result"));
        }
}


CSearchFile::CSearchFile(const CSearchFile& other)
        : CAbstractFile(other),
          CECID(),	// create a new ID for now
          m_parent(other.m_parent),
          m_showChildren(other.m_showChildren),
          m_searchID(other.m_searchID),
          m_sourceCount(other.m_sourceCount),
          m_completeSourceCount(other.m_completeSourceCount),
          m_kademlia(other.m_kademlia),
          m_downloadStatus(other.m_downloadStatus),
          m_directory(other.m_directory),
          m_clients(other.m_clients),
          m_clientDest(other.m_clientDest),
          m_clientServerDest(other.m_clientServerDest),
          m_kadPublishInfo(other.m_kadPublishInfo)
{
        for (size_t i = 0; i < other.m_children.size(); ++i) {
                m_children.push_back(new CSearchFile(*other.m_children.at(i)));
        }
}


CSearchFile::~CSearchFile()
{
        for (size_t i = 0; i < m_children.size(); ++i) {
                delete m_children.at(i);
        }
}


void CSearchFile::AddClient(const ClientStruct& client)
{
        for (std::list<ClientStruct>::const_iterator it = m_clients.begin(); it != m_clients.end(); ++it) {
                if (client.m_dest == it->m_dest) return;
        }
        m_clients.push_back(client);
}


void CSearchFile::MergeResults(const CSearchFile& other)
{
        // Sources
        if (m_kademlia) {
                m_sourceCount = std::max(m_sourceCount, other.m_sourceCount);
                m_completeSourceCount = std::max(m_completeSourceCount, other.m_completeSourceCount);
        } else {
                m_sourceCount += other.m_sourceCount;
                m_completeSourceCount += other.m_completeSourceCount;
        }

        // Publish info
        if (m_kadPublishInfo == 0) {
                if (other.m_kadPublishInfo != 0) {
                        m_kadPublishInfo = other.m_kadPublishInfo;
                }
        } else {
                if (other.m_kadPublishInfo != 0) {
                        m_kadPublishInfo =
                                std::max(m_kadPublishInfo & 0xFF000000, other.m_kadPublishInfo & 0xFF000000) |
                                std::max(m_kadPublishInfo & 0x00FF0000, other.m_kadPublishInfo & 0x00FF0000) |
                                (((m_kadPublishInfo & 0x0000FFFF) + (other.m_kadPublishInfo & 0x0000FFFF)) >> 1);
                }
        }

        // Rating
        if (m_iUserRating != 0) {
                if (other.m_iUserRating != 0) {
                        m_iUserRating = (m_iUserRating + other.m_iUserRating) / 2;
                }
        } else {
                if (other.m_iUserRating != 0) {
                        m_iUserRating = other.m_iUserRating;
                }
        }

        // copy possible available sources from new result
        if (other.GetClientDest().isValid()) {
                // pre-filter sources which would be dropped by CPartFile::AddSources
                if (CPartFile::CanAddSource(other.GetClientDest(), other.GetClientServerDest())) {
                        CSearchFile::ClientStruct client(other.GetClientDest(), other.GetClientServerDest());
                        AddClient(client);
                }
        }
}

void CSearchFile::AddChild(CSearchFile* file)
{
        wxCHECK_RET(file, wxT("Not a valid child!"));
        wxCHECK_RET(!file->GetParent(), wxT("Search-result can only be child of one other result"));
        wxCHECK_RET(!file->HasChildren(), wxT("Result already has children, cannot become child."));
        wxCHECK_RET(!GetParent(), wxT("A child cannot have children of its own"));
        wxCHECK_RET(GetFileHash() == file->GetFileHash(), wxT("Mismatching child/parent hashes"));
        wxCHECK_RET(GetFileSize() == file->GetFileSize(), wxT("Mismatching child/parent sizes"));

        // If no children exists, then we add the current item.
        if (GetChildren().empty()) {
                // Merging duplicate names instead of adding a new one
                if (file->GetFileName() == GetFileName()) {
                        AddDebugLogLineN(logSearch, CFormat(wxT("Merged results for '%s'")) % GetFileName());
                        MergeResults(*file);
                        delete file;
                        return;
                } else {
                        // The first child will always be the first result we received.
                        AddDebugLogLineN(logSearch, CFormat(wxT("Created initial child for result '%s'")) % GetFileName());
                        m_children.push_back(new CSearchFile(*this));
                        m_children.back()->m_parent = this;
                }
        }

        file->m_parent = this;

        for (size_t i = 0; i < m_children.size(); ++i) {
                CSearchFile* other = m_children.at(i);
                // Merge duplicate filenames
                if (other->GetFileName() == file->GetFileName()) {
                        other->MergeResults(*file);
                        UpdateParent();
                        delete file;
                        return;
                }
        }

        // New unique child.
        m_children.push_back(file);
        UpdateParent();

        if (ShowChildren()) {
                Notify_Search_Add_Result(file);
        }
}


void CSearchFile::UpdateParent()
{
        wxCHECK_RET(!m_parent, wxT("UpdateParent called on child item"));

        uint32_t sourceCount = 0;		// ed2k: sum of all sources, kad: the max sources found
        uint32_t completeSourceCount = 0;	// ed2k: sum of all sources, kad: the max sources found
        uint32_t differentNames = 0;		// max known different names
        uint32_t publishersKnown = 0;		// max publishers known
        uint32_t trustValue = 0;		// average trust value
        unsigned publishInfoTags = 0;
        unsigned ratingCount = 0;
        unsigned ratingTotal = 0;
        CSearchResultList::const_iterator best = m_children.begin();
        for (CSearchResultList::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
                const CSearchFile* child = *it;

                // Locate the most common name
                if (child->GetSourceCount() > (*best)->GetSourceCount()) {
                        best = it;
                }

                // Sources
                if (m_kademlia) {
                        sourceCount = std::max(sourceCount, child->m_sourceCount);
                        completeSourceCount = std::max(completeSourceCount, child->m_completeSourceCount);
                } else {
                        sourceCount += child->m_sourceCount;
                        completeSourceCount += child->m_completeSourceCount;
                }

                // Publish info
                if (child->GetKadPublishInfo() != 0) {
                        differentNames = std::max(differentNames, (child->GetKadPublishInfo() & 0xFF000000) >> 24);
                        publishersKnown = std::max(publishersKnown, (child->GetKadPublishInfo() & 0x00FF0000) >> 16);
                        trustValue += child->GetKadPublishInfo() & 0x0000FFFF;
                        publishInfoTags++;
                }

                // Rating
                if (child->HasRating()) {
                        ratingCount++;
                        ratingTotal += child->UserRating();
                }

                // Available sources
                if (child->GetClientDest().isValid()) {
                        CSearchFile::ClientStruct client(child->GetClientDest(), child->GetClientServerDest());
                        AddClient(client);
                }
                for (std::list<ClientStruct>::const_iterator cit = child->m_clients.begin(); cit != child->m_clients.end(); ++cit) {
                        AddClient(*cit);
                }
        }

        m_sourceCount = sourceCount;
        m_completeSourceCount = completeSourceCount;

        if (publishInfoTags > 0) {
                m_kadPublishInfo = ((differentNames & 0x000000FF) << 24) | ((publishersKnown & 0x000000FF) << 16) | ((trustValue / publishInfoTags) & 0x0000FFFF);
        } else {
                m_kadPublishInfo = 0;
        }

        if (ratingCount > 0) {
                m_iUserRating = ratingTotal / ratingCount;
        } else {
                m_iUserRating = 0;
        }

        SetFileName((*best)->GetFileName());
}

void CSearchFile::SetDownloadStatus()
{
        bool isPart		= theApp->downloadqueue->GetFileByID(m_abyFileHash) != NULL;
        bool isKnown	= theApp->knownfiles->FindKnownFileByID(m_abyFileHash) != NULL;
        bool isCanceled	= theApp->canceledfiles->IsCanceledFile(m_abyFileHash);

        if (isCanceled && isPart) {
                m_downloadStatus = QUEUEDCANCELED;
        } else if (isCanceled) {
                m_downloadStatus = CANCELED;
        } else if (isPart) {
                m_downloadStatus = QUEUED;
        } else if (isKnown) {
                m_downloadStatus = DOWNLOADED;
        } else {
                m_downloadStatus = NEW;
        }
        // Update status of children too
        for (CSearchResultList::iterator it = m_children.begin(); it != m_children.end(); it++) {
                Notify_Search_Update_Sources(*it);
        }
}

// File_checked_for_headers