//								-*- C++ -*-
// This file is part of the iMule Project.
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

#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "SearchManager.h"
#include <MemFile.h>
#include "i2p/CI2PAddress.h"
#include "kademlia/routing/Contact.h"
#include <functional>

class CKnownFile;
class CTag;

////////////////////////////////////////
namespace Kademlia
{
        class CSearchContact : public CContact
        {
                /* execution dates are specific to searches */
                //                 time_t                m_execution_date;
                unsigned              m_results_count;
        public:
                CSearchContact();
                CSearchContact(const CContact &);
                //                 void                  SetExecutionDelay( time_t e );
                //                 bool                  DeathSentence() const;
                void                  add_result(unsigned n) {m_results_count+=n;}
                unsigned              get_results_count() const {return m_results_count;}
        };
////////////////////////////////////////

class CKadClientSearcher;

class CSearch
{
        friend class CSearchManager;

public:
        uint32_t GetSearchID() const throw()			{ return m_searchID; }
        void	 SetSearchID(uint32_t id) throw()		{ m_searchID = id; }
        uint32_t GetSearchType() const throw()			{ return m_type; }
        void	 SetSearchType(uint32_t val) throw()		{ m_type = val; }
        void	 SetTargetID(const CUInt128& val) throw() 	{ m_target = val; }
        CUInt128 GetTarget() const throw()			{ return m_target; }

        const wxString& GetFileName(void) const throw()		{ return m_fileName; }
        void  SetFileName(const wxString& fileName) throw()	{ m_fileName = fileName; }

        void	 AddFileID(const CUInt128& id)		{ m_fileIDs.push_back(id); }
        void	 PreparePacketForTags(CMemFile* packet, CKnownFile* file);
        bool	 Stopping() const throw()		{ return m_stopping; }

        uint32_t GetNodeLoad() const throw()		{ return m_totalLoadResponses == 0 ? 0 : m_totalLoad / m_totalLoadResponses; }
        uint32_t    GetNodeLoadResponse() const throw()                 { return m_totalLoadResponses;}
        uint32_t    GetNodeLoadTotal() const throw()                    { return m_totalLoad;}
        void        UpdateNodeLoad( uint8_t load ) throw()              { m_totalLoad += load; m_totalLoadResponses++; }

        void        SetSearchTermData( CMemFile * searchTermsData );

        CKadClientSearcher * GetNodeSpecialSearchRequester() const throw() { return m_nodeSpecialSearchRequester; }
        void        SetNodeSpecialSearchRequester(CKadClientSearcher *requester) throw() { m_nodeSpecialSearchRequester = requester; }

        enum {
                NODE = 1 << 0,
                NODECOMPLETE = 1 << 1,
                FILE = 1 << 2,
                KEYWORD = 1 << 3,
                NOTES = 1 << 4,
                STOREFILE = 1 << 5,
                STOREKEYWORD = 1 << 6,
                STORENOTES = 1 << 7,
                FINDBUDDY = 1 << 8,
                FINDSOURCE = 1 << 9,
                NODESPECIAL = 1 << 10,	// nodesearch request from requester "outside" of kad to find the IP of a given NodeID
                NODEFWCHECKUDP = 1 << 11	// find new unknown IPs for a UDP firewallcheck
        };
        wxString typestr();

        CSearch(const CUInt128 & target);
        ~CSearch();

private:
        void        Go();
        void        ProcessResponse(const CI2PAddress & fromDest, const ContactList & results);
        void        ProcessSearchResult( const CUInt128 &answer, TagList & info ) ;
        void        ProcessSearchResultList ( const CMemFile & bio, const CI2PAddress & fromIP ) ;
        void        ProcessResultFile( const CUInt128 &answer, TagList &info );
        void        ProcessResultKeyword( const CUInt128 &answer, TagList &info);
        void        ProcessResultNotes( const CUInt128 &answer, TagList &info);
        bool        JumpStart(void);
        void        SendFindPeersForTarget(CContact contact, bool reaskMore = false);
        void        StopLookingForPeers() throw();    // called void PrepareToStop(); in amule
        void        SendRequestTo(CContact contact); // called void StorePacket(); in amule

        uint8_t     GetRequestContactCount() const throw();  // max number of contacts to send depending on type of search
	uint8_t    GetCompletedListMaxSize() const throw();
        uint32_t    GetSearchMaxAnswers() const throw();     // max number of final results to receive depending on type of search
        time_t      GetSearchLifetime() const throw()  ;
        uint32_t    GetSearchAnswers() const throw()            { return m_searchResults ; }
        uint32_t    GetCompletedPeers() const throw()           { return m_completedPeers; }

        // counts the availability of a shared file in iMule network
        void        ProcessPublishResult ( const CMemFile & bio, const CI2PAddress & fromIP ) ;
	void        addToCompletedPeers( CSearchContact contact );
	CSearchContact   getChargedContact(const CI2PAddress & dest);
        void       chargeclosest(TargetContactMap &, std::function<void(CSearchContact&)>);
	void       unchargeContactTo(CSearchContact contact, ContactMap * _map);
	void       killMaliciousContact(CSearchContact contact);
        void       registerDeadContact(CSearchContact contact);
        
        bool		m_stopping;
        uint32_t	m_type;
        uint32_t	m_completedPeers;         // counts the peers from which we don't wait for anything more
        uint32_t	m_searchResults ;         // counts the number of results received
        uint32_t	m_totalLoad;
        uint32_t	m_totalLoadResponses;
        uint32_t	m_lastActivity;

        uint32_t	m_searchID;
        CUInt128	m_target;
        CMemFile    	m_searchTerms ;		   // imule: replacement for m_searchTermsData and m_searchTermsDataSize)
        WordList	m_words;  // list of words in the search string (populated in CSearchManager::PrepareFindKeywords)
        wxString	m_fileName;
        UIntList	m_fileIDs;
        CKadClientSearcher *m_nodeSpecialSearchRequester; // used to callback result for NODESPECIAL searches

        ContactMap       m_known;                        // known contacts
        TargetContactMap m_ask_for_contacts;             // contacts that can be asked for contacts
        TargetContactMap m_ask_for_data;                 // contacts that will be asked for data
        TargetContactMap m_best_complete;                // list of ALPHA_QUERY closest contacts that have completed the process
        ContactMap       m_trash;                        // contacts that have been taken out of m_best_complete, or with unattended behaviour
        TargetContactMap m_charged_contacts;             // contacts asked for something
        CUInt128         m_closestDistantFound;          // not used for the search itself, but for statistical data collecting
        CSearchContact   m_requestedMoreNodesContact;
};

} // End namespace

#endif //__SEARCH_H__
// File_checked_for_headers