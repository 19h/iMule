//
// C++ Interface: wxI2PEvents
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

//
// This file is part of the imule Project.
//
// Copyright (c) 2003-2006 imule Team ( mkvore@mail.i2p / http://www.imule.i2p )
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

#ifndef SAMEVENTS_H
#define SAMEVENTS_H

#include <wx/defs.h>
#include <wx/event.h>	// Needed for wxEvent


// --------------------------------------------------------------------------
// wxSocketEvent
// --------------------------------------------------------------------------



//! error codes
enum samerr_t {
        /* see i2pstrerror() for detailed descriptions of these */
        /* no error code - not used by me (you can use it in your program) */
        SAM_NULL = 0,
        /* error codes from SAM itself (SAM_OK is not an actual "error") */
        SAM_OK, SAM_CANT_REACH_PEER, SAM_DUPLICATED_DEST, SAM_DUPLICATED_ID,
        SAM_I2P_ERROR,
        SAM_INVALID_KEY, SAM_KEY_NOT_FOUND, SAM_PEER_NOT_FOUND, SAM_TIMEOUT,
        SAM_UNKNOWN,
        /* error codes from LibSAM */
        SAM_BAD_STYLE, SAM_BAD_VERSION, SAM_CALLBACKS_UNSET, SAM_SOCKET_ERROR,
        SAM_TOO_BIG, SAM_FAILED, SAM_PARSE_FAILED,
        /* socket errors */
        SAM_DISCONNECTED,
        SAM_NONE
};

enum sam_conn_state_t {
        OFF,
        SERVER_CONNECTED,
        SAM_HELLOED,
        DEST_CREATED,
        SESSION_CREATED,
        NAMING_LOOKEDUP,
        KEYS_GENERATED,
        INCOMING_STREAMS_FORWARDED,
        STREAM_OK,
        DATAGRAM_INCOMING,
        ON
};

class wxSamEvent: public wxEvent
{
public:

        /**
         * constructors
         */
        inline wxSamEvent(sam_conn_state_t state);

        wxSamEvent( const wxSamEvent & event) : wxEvent(event) {
                m_state   = event.m_state ;
                m_status  = event.m_status ;
                m_message = wxString( event.GetMessage().c_str() );
        }
        // Required for sending with wxQueueEvent()
        wxEvent* Clone() const { return new wxSamEvent(*this); }

        /**
         * fields access
         */
        void SetMessage(const wxString & m) { m_message = wxString(m.c_str()); }
        wxString GetMessage() const { return wxString(m_message.c_str()); }

        void SetStatus(samerr_t status) {m_status = status;}
        samerr_t  GetStatus() const { return m_status; }

        /**
         * fields
         */
private:
        sam_conn_state_t m_state ;
        samerr_t         m_status ;
        wxString         m_message ;
};

wxDECLARE_EVENT(wxSAM_EVENT, wxSamEvent);
inline wxSamEvent::wxSamEvent( sam_conn_state_t state) :  wxEvent(0, wxSAM_EVENT) {
        m_state = state ;
}

#endif
