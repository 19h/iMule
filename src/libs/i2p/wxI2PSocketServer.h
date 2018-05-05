//
// C++ Interface: wxI2PSocketServer
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef __wxI2PSocketServer__
#define __wxI2PSocketServer__


#include <wx/wx.h>
#include <wx/socket.h>
#include <wx/filename.h>

#include "wxI2PSocketClient.h"
#include "MuleThread.h"

class wxI2PSocketServer
        : public wxI2PSocketBase
{
        friend class wxI2PSocket;
        friend class wxI2PSocketClient;

public:
        wxI2PSocketServer(const wxString & addr = wxEmptyString,
                          wxSocketFlags flags = wxSOCKET_NONE,
                          wxString options = wxEmptyString);

        ~wxI2PSocketServer();

        wxI2PSocketClient * Accept(bool wait=true);

        bool 	      AcceptWith(wxI2PSocketClient& socket, bool wait = true);

        /**
         * wxSocketBase methods
         */

        virtual bool Error() ;

        /**
         * state machine
         */
public:
        void OnAccepterEnd(bool ready, samerr_t err);
        virtual void OnSamEvent(wxSamEvent & e);
        virtual bool state_machine();


        /**
         * SAM messages
         */
        virtual void sam_logback ( wxString ) ;

protected:

        void wxAddConnectionEvent();

public:
        virtual bool Close();
        virtual bool Destroy();
        virtual void SetLocal( const CI2PAddress & addr ) {wxI2PSocketBase::SetLocal(addr);}
        virtual void SetEventHandler(wxEvtHandler& handler, int id = wxID_ANY);
        virtual void Notify ( bool notify );
        virtual void SetNotify ( wxSocketEventFlags flags );
        void SetClientData ( void *data );
        bool     GetLocal( CI2PAddress & addr );
        virtual wxSocketFlags GetFlags() { return wxI2PSocketBase::GetFlags(); }
        virtual CI2PAddress GetLocal() { return wxI2PSocketBase::GetLocal(); }
        virtual bool IsOk() const;

        void StartListening();
        void StopListening();

protected:

private:
        //wxSocketServer * m_server ;
        wxIPV4address  * m_laddr  ;

protected:
        virtual void ProcessParentSocketEvent( wxSocketEvent & ) ;
        wiMutex m_accepter_mutex ;
        class Accepter : public wxI2PSocketClient
        {
        private:
                wxString m_nick;
                int m_port;
                wxI2PSocketServer & m_parent ;
                bool m_available ;

        public:
                Accepter(  wxI2PSocketServer & parent,
                           const wxString & nick,
                           wxSocketFlags flags  ) ;
                virtual void OnSamEvent(wxSamEvent & e);
                virtual void ProcessParentSocketEvent( wxSocketEvent & e);
                virtual bool state_machine(samerr_t err);
                bool         IsAvailable() { return m_available ; }
        } * m_accepter ;

private:
        static CSamLogger * s_log ;
public:
        static void setLogger(CSamLogger & l) {s_log = &l;}
};

#endif
