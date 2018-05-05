//
// This file is part of the imule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/menu.h>
#include <wx/intl.h>
#include <wx/notebook.h>
#include <wx/wupdlock.h>

#include "MuleNotebook.h"	// Interface declarations

#include "common/MenuIDs.h"
#include "common/Format.h"
#include "Logger.h"

DEFINE_LOCAL_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSING)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_ALL_PAGES_CLOSED)

CMuleNotebook::CMuleNotebook( wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long )
        : wxAuiNotebook(parent, id, pos, size)
{
        m_popup_enable = true;
        m_popup_widget = NULL;
        Bind(wxEVT_AUINOTEBOOK_TAB_RIGHT_DOWN, &CMuleNotebook::OnRMButton, this);
        Bind(wxEVT_MENU, &CMuleNotebook::OnPopupClose,       this,  MP_CLOSE_TAB);
        Bind(wxEVT_MENU, &CMuleNotebook::OnPopupCloseAll,    this,  MP_CLOSE_ALL_TABS);
        Bind(wxEVT_MENU, &CMuleNotebook::OnPopupCloseOthers, this,  MP_CLOSE_OTHER_TABS);
}


CMuleNotebook::~CMuleNotebook()
{
        // Ensure that all notifications gets sent
        DeleteAllPages();
}


bool CMuleNotebook::DeletePage(size_t nPage)
{
        wxCHECK_MSG(nPage < GetPageCount(), false,
                    wxT("Trying to delete invalid page-index in CMuleNotebook::DeletePage"));

        // Send out close event
        wxNotebookEvent evt( wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSING, GetId(), nPage );
        evt.SetEventObject(this);
        ProcessEvent( evt );

        // and finally remove the actual page
        bool result = wxAuiNotebook::DeletePage( nPage );

        return result;
}


bool CMuleNotebook::DeleteAllPages()
{
        wxWindowUpdateLocker noUpdates(this);
        
        bool result = true;
        while (  GetPageCount() ) {
                result &= DeletePage( 0 );
        }
        return result;
}


void CMuleNotebook::EnablePopup( bool enable )
{
        m_popup_enable = enable;
}


void CMuleNotebook::SetPopupHandler( wxWindow* widget )
{
        m_popup_widget = widget;
}


//#warning wxMac does not support selection by right-clicking on tabs!
void CMuleNotebook::OnRMButton(wxAuiNotebookEvent& event)
{
        int tab = event.GetSelection();

        // Cases where we shouldn't be showing a popup-menu.
        if ( !GetPageCount() || !m_popup_enable || tab == wxNOT_FOUND) {
                event.Skip();
                return;
        }

        SetSelection(tab);

        // Should we send the event to a specific widget?
        if ( m_popup_widget ) {
                wxMouseEvent evt(wxEVT_RIGHT_DOWN);
                evt.SetState( wxGetMouseState() );
                // Map the coordinates onto the parent
                evt.SetPosition( m_popup_widget->ScreenToClient( wxGetMousePosition() ) );                
                m_popup_widget->GetEventHandler()->AddPendingEvent( evt );
        } else {
                wxMenu menu(_("Close"));
                menu.Append(MP_CLOSE_TAB, wxString(_("Close tab")));
                menu.Append(MP_CLOSE_ALL_TABS, wxString(_("Close all tabs")));
                menu.Append(MP_CLOSE_OTHER_TABS, wxString(_("Close other tabs")));

                PopupMenu( &menu );
        }
}


void CMuleNotebook::OnPopupClose(wxCommandEvent& WXUNUSED(evt))
{
        DeletePage( GetSelection() );
}


void CMuleNotebook::OnPopupCloseAll(wxCommandEvent& WXUNUSED(evt))
{
        DeleteAllPages();
}


void CMuleNotebook::OnPopupCloseOthers(wxCommandEvent& WXUNUSED(evt))
{
        wxNotebookPage* current = GetPage( GetSelection() );

        for ( int i = GetPageCount() - 1; i >= 0; i-- ) {
                if ( current != GetPage( i ) )
                        DeletePage( i );
        }
}

// File_checked_for_headers