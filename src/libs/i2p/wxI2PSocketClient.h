//
// C++ Interface: wxI2PSocketClient
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//


#ifndef __wxI2PSocketClient__
#define __wxI2PSocketClient__

#include "wxI2PSocketBase.h"
#include "CI2PAddress.h"
#include "MuleThread.h"

class wxI2PSocketServer;


class wxI2PSocketClient
        : public wxI2PSocketBase
{
        friend class wxI2PSocketServer ;
        friend class CSam;

public:
        static void createServer();

        wxI2PSocketClient(wxSocketFlags flags = wxSOCKET_NONE);

        ~wxI2PSocketClient();

        bool Connect(const CI2PAddress & dest, bool wait = true,
                     wxString options = wxEmptyString);

        bool Connect(const CI2PAddress & dest, const wxString & local, bool wait = true,
                     wxString options = wxEmptyString);

        bool Accept(bool wait = true) ;

        bool WaitOnConnect(long seconds = -1, long milliseconds = 0);

        //void sam_stream_statusback ( CSam::samerr_t result, wxString message );

        virtual bool Close();

        virtual bool Destroy();

        bool GetPeer ( CI2PAddress & addr );

        virtual CI2PAddress GetLocal() { return wxI2PSocketBase::GetLocal(); }

        virtual void SetLocal( const CI2PAddress & addr ) {wxI2PSocketBase::SetLocal(addr);}

        virtual void SetTimeout(long int) ;

        virtual void SetEventHandler(wxEvtHandler& handler, int id = wxID_ANY);

        virtual void SetNotify ( wxSocketEventFlags flags );

        virtual void Notify ( bool notify );

        virtual bool            Error() const ;

        virtual wxSocketError   LastError() const ;

        inline wxUint32        LastCount() const {return m_socket->LastCount();}

        virtual void Flush();

        virtual void SetFlags(wxSocketFlags);

        CI2PAddress GetPeer();

private:

        void SetPeer ( const CI2PAddress & from );

        static wiMutex             s_mutex     ;

        CI2PAddress m_there ;
protected:
        CSam * m_sam_server ;   // SAM server holding the local destination

        virtual void OnSamEvent(wxSamEvent&);

        virtual bool state_machine();

        void connect_failed() ;

        void connect_succeeded();

        void transfer();

        virtual void AddwxConnectEvent();

        virtual void ProcessParentSocketEvent( wxSocketEvent & ) ;

private:
        static CSamLogger * s_log ;
public:
        static void setLogger(CSamLogger & l) {s_log = &l;}
};

#endif
