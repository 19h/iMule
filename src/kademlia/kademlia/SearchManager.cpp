//
// This file is part of the imule Project.
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

#include <wx/wx.h>

#include "Search.h"
#include <common/Macros.h>

#include "Indexed.h"
#include "Defines.h"
#include "../routing/Contact.h"
#include "../../MemFile.h"
#include "../../Logger.h"
#include "../../RandomFunctions.h"		// Needed for GetRandomUInt128()
#include "../../OtherFunctions.h"		// Needed for DeleteContents()
#include "Kademlia.h"

#include <wx/tokenzr.h>

#if defined(__SUNPRO_CC)
#define __FUNCTION__ __FILE__+__LINE__
#endif

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

uint32_t CSearchManager::m_nextID = 0;

SearchMap CSearchManager::m_searches;

bool CSearchManager::IsSearching(uint32_t searchID) throw()
{
        // Check if this searchID is within the searches
        for (SearchMap::const_iterator it = m_searches.begin(); it != m_searches.end(); ++it) {
                if (it->second->GetSearchID() == searchID) {
                        return true;
                }
        }
        return false;
}

void CSearchManager::StopSearch ( uint32_t searchID, bool delayDelete )
{
        // Stop a specific searchID
        for (SearchMap::iterator it = m_searches.begin(); it != m_searches.end(); ++it) {
                if (it->second->GetSearchID() == searchID) {
                        // Do not delete as we want to get a chance for late packets to be processed.
                        if (delayDelete) {
                                it->second->StopLookingForPeers();
                        } else {
                                // Delete this search now.
                                // If this method is changed to continue looping, take care of the iterator as we will already
                                // be pointing to the next entry and the for-loop could cause you to iterate past the end.
                                delete it->second;
                                m_searches.erase(it++);
                        }
                        return;
                }
        }
}

void CSearchManager::StopAllSearches()
{
        // Stop and delete all searches.
        DeleteContents(m_searches);
}

bool CSearchManager::StartSearch(CSearch* search)
{
        // A search object was created, now try to start the search.
        if (AlreadySearchingFor(search->GetTarget())) {
                // There was already a search in progress with this target.
                delete search;
                return false;
        }
        // Add to the search map
        m_searches[search->GetTarget()] = search;
        // Start the search.
        search->Go();
        return true;
}

CSearch* CSearchManager::PrepareFindKeywords(const wxString& keyword, CMemFile* searchTermsData, uint32_t searchid)
{
#ifdef __DEBUG__
        wxString debugStr;
        //         int line = 0;
#define DEBSTR(X) debugStr << (X) << wxT(" ")
#else
#define DEBSTR(X)
#endif
        std::unique_ptr<CSearch> s;
        try {
                WordList words;
                GetWords(keyword, &words, true);

                // Make sure we have a keyword list                
                if (words.empty()) {
                        throw wxString(_("Kademlia: search keyword too short"));
                }

                wxString wstrKeyword = words.front();
                AddDebugLogLineN(logKadSearch, CFormat(wxT("Keyword for search: %s")) % wstrKeyword);

                // Kry - I just decided to assume everyone is unicoded
                // GonoszTopi - seconded
                CUInt128 target;
                KadGetKeywordHash(wstrKeyword, &target);

                // Create a keyword search object.
                s.reset(new CSearch(target));
                s->SetSearchType(CSearch::KEYWORD);
                words.swap(s->m_words);

                // Verify that we are not already searching for this target.
                if (AlreadySearchingFor(s->m_target)) {
                        throw _("Kademlia: Search keyword is already on search list: ") + wstrKeyword;
                }

                s->SetSearchTermData(searchTermsData);
                // Inc our searchID
                // If called from external client use predefined search id
                s->SetSearchID((searchid & 0xffffff00) == 0xffffff00 ? searchid : ++m_nextID);
                
        } catch (const CEOFException& err) {
                wxString strError = wxT("CEOFException in ") + wxString::FromAscii(__FUNCTION__) + wxT(": ") + err.what();
                throw strError;
        } catch (const CInvalidPacket& err) {
                wxString strError = wxT("CInvalidPacket exception in ") + wxString::FromAscii(__FUNCTION__) + wxT(": ") + err.what();
                throw strError;
        }
        // Insert search into map
        m_searches[s->GetTarget()] = s.get();
        // Start search
        s->Go();
        return s.release();
}

CSearch* CSearchManager::PrepareLookup(uint32_t type, bool start, const CUInt128& id)
{
        // Prepare a kad lookup.
        // Make sure this target is not already in progress.
        if (AlreadySearchingFor(id)) {
                return NULL;
        }

        // Create a new search.
        CSearch *s = new CSearch(id);

        // Set type and target.
        s->SetSearchType(type);

        try {
                switch(type) {
                case CSearch::STOREKEYWORD:
                        if (!Kademlia::CKademlia::GetIndexed()->SendStoreRequest(id)) {
                                delete s;
                                return NULL;
                        }
                        break;
                }

                s->SetSearchID(++m_nextID);
                if (start) {
                        m_searches[id] = s;
                        s->Go();
                }
        } catch (const CEOFException& err) {
                delete s;
                AddDebugLogLineN(logKadSearch, wxT("CEOFException in CSearchManager::PrepareLookup: ") + err.what());
                return NULL;
        } catch (...) {
                AddDebugLogLineN(logKadSearch, wxT("Exception in CSearchManager::PrepareLookup"));
                delete s;
                throw;
        }

        return s;
}

void CSearchManager::FindNode(const CUInt128& id, bool complete)
{
        if ( AlreadySearchingFor ( id ) ) {
                return;
        }
        // Do a node lookup.
        CSearch *s = new CSearch(id);
        if (complete) {
                s->SetSearchType(CSearch::NODECOMPLETE);
        } else {
                s->SetSearchType(CSearch::NODE);
        }
        StartSearch(s);
}

bool CSearchManager::IsFWCheckUDPSearch(const CUInt128& target)
{
        // Check if this target is in the search map.
        SearchMap::const_iterator it = m_searches.find(target);
        if (it != m_searches.end()) {
                return (it->second->GetSearchType() == CSearch::NODEFWCHECKUDP);
        }
        return false;
}

void CSearchManager::GetWords(const wxString& str, WordList *words, bool allowDuplicates)
{
        wxString current_word;
        wxStringTokenizer tkz(str, GetInvalidKeywordChars());
        while (tkz.HasMoreTokens()) {
                current_word = tkz.GetNextToken();
                // TODO: We'd need a safe way to determine if a sequence which contains only 3 chars is a real word.
                // Currently we do this by evaluating the UTF-8 byte count. This will work well for Western locales,
                // AS LONG AS the min. byte count is 3(!). If the byte count is once changed to 2, this will not
                // work properly any longer because there are a lot of Western characters which need 2 bytes in UTF-8.
                // Maybe we need to evaluate the Unicode character values itself whether the characters are located
                // in code ranges where single characters are known to represent words.
                if (strlen((const char *)(current_word.utf8_str())) >= 3) {
                        current_word.MakeLower();
                        if (!allowDuplicates) {
                                words->remove(current_word);
                        }
                        words->push_back(current_word);
                }
        }
}


void CSearchManager::JumpStart ()
{
        // Find any searches that has stalled and jumpstart them.
        // This will also prune all searches.
        //         time_t now = time ( NULL );

        std::set<CUInt128> toDelete ;
        for ( SearchMap::iterator current_it = m_searches.begin(); current_it != m_searches.end(); current_it++ ) {
                if ( ! current_it->second->JumpStart() )
                        toDelete.insert ( current_it->first );
        }

        for ( std::set<CUInt128>::iterator it = toDelete.begin(); it != toDelete.end(); it++ ) {
                delete m_searches[*it] ;
                m_searches.erase ( *it );
        }
}

void CSearchManager::UpdateStats () throw ()
{
        uint8_t m_totalFile = 0;
        uint8_t m_totalStoreSrc = 0;
        uint8_t m_totalStoreKey = 0;
        uint8_t m_totalSource = 0;
        uint8_t m_totalNotes = 0;
        uint8_t m_totalStoreNotes = 0;

        for ( SearchMap::const_iterator it = m_searches.begin(); it != m_searches.end(); ++it ) {
                switch(it->second->GetSearchType()) {
                case CSearch::FILE: {
                        m_totalFile++;
                        break;
                }
                case CSearch::STOREFILE: {
                        m_totalStoreSrc++;
                        break;
                }
                case CSearch::STOREKEYWORD:	{
                        m_totalStoreKey++;
                        break;
                }
                case CSearch::FINDSOURCE: {
                        m_totalSource++;
                        break;
                }
                case CSearch::STORENOTES: {
                        m_totalStoreNotes++;
                        break;
                }
                case CSearch::NOTES: {
                        m_totalNotes++;
                        break;
                }
                default:
                        break;
                }
        }

        CPrefs *prefs = CKademlia::GetPrefs();
        prefs->SetTotalFile(m_totalFile);
        prefs->SetTotalStoreSrc(m_totalStoreSrc);
        prefs->SetTotalStoreKey(m_totalStoreKey);
        prefs->SetTotalSource(m_totalSource);
        prefs->SetTotalNotes(m_totalNotes);
        prefs->SetTotalStoreNotes(m_totalStoreNotes);
}

void CSearchManager::ProcessPublishResult(CMemFile & bio, const CI2PAddress & from)
{
        CUInt128 target = bio.ReadUInt128();
        CSearch *s = NULL;
        SearchMap::const_iterator it = m_searches.find(target);

        if ( it != m_searches.end() ) {
                s = it->second;
        }

        if (s == NULL) {
                AddDebugLogLineN(logKadSearch,
                                 CFormat(wxT("Search for key %s either never existed or receiving late results (CSearchManager::processPublishResults)")) % target.ToHexString());
                return;
        }

        s->ProcessPublishResult( bio, from ) ;
}


/**
 * Processes a contact list received after a target lookup
 * @param target
 * @param fromDest
 * @param results
 */
void CSearchManager::ProcessResponse(const CUInt128& target, const CI2PAddress & fromDest, const ContactList & results )
{
        // We got a response to a kad lookup.
        CSearch *s = NULL;
        SearchMap::const_iterator it = m_searches.find(target);
        if ( it != m_searches.end() ) {
                s = it->second;
        }

        // If this search was deleted before this response, delete contacts and abort, otherwise process them.
        if (s == NULL) {
                AddDebugLogLineN(logKadSearch,
                                 CFormat(wxT("CSearchManager::ProcessResponse : Search for %s from %s either never existed or receiving late results"))
                                 % target.ToHexString() % fromDest.humanReadable());
        } else {
                s->ProcessResponse( fromDest, results );
        }
}

void CSearchManager::ProcessResult(const CMemFile & bio, const CI2PAddress & fromDest)
{
        // What search does this relate to
        CUInt128 target = bio.ReadUInt128();
        CSearch *s = NULL;
        AddDebugLogLineN(logKadSearch,
                         CFormat ( wxT ( "CSearchManager::processSearchResultList(search = %s)" ) )
                         % target.ToHexString() );

        SearchMap::const_iterator it = m_searches.find(target);
        if ( it != m_searches.end() ) {
                s = it->second;
        }

        // If this search was deleted before these results, delete contacts and abort, otherwise process them.
        if (s == NULL) {
                AddDebugLogLineN(logKadSearch,
                                 wxT ( "Search either never existed or receiving late results (CSearchManager::processResult)" ) );
        } else {
                s->ProcessSearchResultList(bio, fromDest);
        }
}

bool CSearchManager::FindNodeSpecial(const CUInt128& id, CKadClientSearcher *requester)
{
        // Do a node lookup.
        AddDebugLogLineN(logKadSearch, wxT("Starting NODESPECIAL Kad Search for ") + id.ToHexString());
        CSearch *search = new CSearch(id);
        search->SetSearchType(CSearch::NODESPECIAL);
        search->SetNodeSpecialSearchRequester(requester);
        return StartSearch(search);
}

bool CSearchManager::FindNodeFWCheckUDP()
{
        CancelNodeFWCheckUDPSearch();
        CUInt128 id(GetRandomUint128());
        AddDebugLogLineN(logKadSearch, wxT("Starting NODEFWCHECKUDP Kad Search"));
        CSearch *search = new CSearch(id);
        search->SetSearchType(CSearch::NODEFWCHECKUDP);
        return StartSearch(search);
}

void CSearchManager::CancelNodeSpecial(CKadClientSearcher* requester)
{
        // Stop a specific nodespecialsearch
        for (SearchMap::iterator it = m_searches.begin(); it != m_searches.end(); ++it) {
                if (it->second->GetNodeSpecialSearchRequester() == requester) {
                        it->second->SetNodeSpecialSearchRequester(NULL);
                        it->second->StopLookingForPeers();
                        return;
                }
        }
}

void CSearchManager::CancelNodeFWCheckUDPSearch()
{
        // Stop node searches done for udp firewallcheck
        for (SearchMap::iterator it = m_searches.begin(); it != m_searches.end(); ++it) {
                if (it->second->GetSearchType() == CSearch::NODEFWCHECKUDP) {
                        it->second->StopLookingForPeers();
                }
        }
}
// File_checked_for_headers
