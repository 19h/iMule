//
// C++ Interface: wxI2PSocketBase
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef __wxI2PDatagramSocket__
#define __wxI2PDatagramSocket__

#include "wxI2PSocketBase.h"
#include "CI2PAddress.h"
#include "CSamDefines.h"
#include "MuleThread.h"

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/socket.h>

#include <memory>

/** datagrams coming from SAM */


class CDatagramMessage
{
public:
        CDatagramMessage ( const CI2PAddress & dest, std::unique_ptr<char[]> & message, long len ) {
                length = len;
                destination = dest;
                payload = std::move(message);
        }
        CDatagramMessage() {
                length = 0;
                destination = CI2PAddress::null;
        }
        CDatagramMessage & operator= (CDatagramMessage & m) {
                wxFAIL;
                length      = m.length ;
                destination = m.destination;
                payload     = std::move(m.payload);
                return (*this);
        }

        long       length;
        std::unique_ptr<char[]>     payload;
        CI2PAddress destination;
};




/** wxI2PDatagramSocket class */


class wxI2PDatagramSocket : public wxI2PSocketBase
{
public:
        wxI2PDatagramSocket (const wxString & dest,
                             wxSocketFlags flags = wxSOCKET_NONE, wxString i2pOptions=wxT(""));

        ~wxI2PDatagramSocket();

        virtual bool Destroy();

        virtual bool Close();

        virtual wxI2PDatagramSocket& RecvFrom ( CI2PAddress& address, void * buffer, wxUint32 nbytes );

        virtual wxI2PDatagramSocket& SendTo ( const CI2PAddress& address, const void * buffer, wxUint32 nbytes );

        virtual void sam_logback ( wxString ) ;

        static const uint32_t & MaxDatagramSize();

public:
        virtual void SetClientData ( void *data );

        bool     GetLocal( CI2PAddress & addr );

        virtual CI2PAddress GetLocal() { return wxI2PSocketBase::GetLocal(); }

        virtual void SetEventHandler(wxEvtHandler& handler, int id = wxID_ANY);

        virtual void SetNotify ( wxSocketEventFlags flags );

        virtual void Notify ( bool notify );

        virtual bool            Error() const ;

        virtual wxSocketError   LastError() const ;

        inline wxUint32        LastCount() const {return wxI2PSocketBase::LastCount();}

        virtual bool IsOk() const;

protected:
        virtual void OnSamEvent(wxSamEvent&);
        virtual bool state_machine() ;
        virtual void sam_dgramback ();

private:
        static CSamLogger * s_log ;

        static size_t MAX_MESSAGE_QUEUE_LENGTH;

        std::deque<std::unique_ptr<CDatagramMessage>>  m_receiveMessages ;
        wiMutex                                        m_receiveMessages_mutex ;
public:
        static void setLogger(CSamLogger & l) {s_log = &l;}
};

#endif
