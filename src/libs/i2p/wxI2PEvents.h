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

#ifndef WXI2PEVENTS_H
#define WXI2PEVENTS_H

#include <wx/defs.h>
#include <wx/event.h>   // Needed for wxEvent
#include <wx/socket.h>  // Needed for wxSocketNotify

class wxI2PSocketBase;
// --------------------------------------------------------------------------
// wxSocketEvent
// --------------------------------------------------------------------------



//! error codes

wxString        i2pstrerror ( int code );

wxString        wxstrerror ( int code );

class CI2PSocketEvent : public wxSocketEvent
{
public:
        CI2PSocketEvent ( wxEventType eventType ) {
                SetEventType ( eventType ) ;
        }
        int       GetSamError() const { return m_samError; }
        void      SetSamError ( int err ) {m_samError = err; }
        virtual   wxI2PSocketBase * GetSocket();

        virtual wxEvent *Clone() const { return new CI2PSocketEvent(*this); }


protected:
        int m_samError ;


};


#endif
