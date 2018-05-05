//
// This file is part of the imule Project.
//
// Copyright (c) 2003-2006 imule Team ( mkvore@mail.i2p / http://www.imule.i2p )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef CHATSELECTOR_H
#define CHATSELECTOR_H

#include "MuleTextCtrl.h"
#include "MuleNotebook.h"
#include "Types.h"				// Needed for uint16
#include "i2p/CI2PAddress.h"

class CClientRef;
class CFriend;


/**
 * This class is used to display chat sessions.
 */
class CChatSession : public CMuleTextCtrl
{
public:
        CChatSession(wxWindow *parent, wxWindowID id = -1, const wxString& value = wxT(""), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxTextCtrlNameStr );
        ~CChatSession();

        CI2PAddress 	m_client_dest;
        uint64	m_client_id;
        bool	m_active;

        /**
         * Appends the specified text.
         *
         * @param text The text to add.
         * @param style The style of the new text.
         * @param newline If a newline should be added to the end of the line.
         *
         * If newline is false, then no newlines will be at the end of added text,
         * even if the passed string ends with newlines. Multiline strings are
         * broken into indivudual lines and each are timestamped with the same date.
         */
        void AddText( const wxString& text, const wxTextAttr& style, bool newline = true );
};


class CChatSelector : public CMuleNotebook
{
public:
        CChatSelector(wxWindow* parent, wxWindowID id, const wxPoint& pos, wxSize siz, long style);
        virtual			~CChatSelector() {};
        CChatSession*		StartSession(const CI2PAddress & client_dest, const wxString& client_name, bool show = true);
        void			EndSession(uint64 client_id = 0);
        CChatSession*		GetPageByClientID(uint64 client_id);
        int			GetTabByClientID(uint64 client_id);
        bool			ProcessMessage(const CI2PAddress &  sender_dest, const wxString& message);
        bool			SendMessage(const wxString& message, const wxString& client_name = wxEmptyString, const CI2PAddress & to_id = CI2PAddress::null);
        void			ConnectionResult(bool success, const wxString& message, uint64 id);
        void			RefreshFriend(uint64 toupdate_id, const wxString& new_name);
        void			ShowCaptchaResult(uint64 id, bool ok);
        bool			GetCurrentClient(CClientRef&) const;
};

#endif
// File_checked_for_headers