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
#ifndef __wxI2PSocketBase__
#define __wxI2PSocketBase__

#include <stack>
#include <wx/string.h>
#include <wx/object.h>
#include <wx/socket.h>
#include <wx/event.h>
#include "CI2PAddress.h"
#include "CSamDefines.h"
//#include "CSamSession.h"
//#include "wxI2PEvents.h"
#include "MuleThread.h"
#include <map>
#include "wxI2PEvents.h"
#include "SamEvents.h"
#include "CSamSession.h"

class CSam ;

//DECLARE_EVENT_TYPE( I2PParentSocketEvent, wxID_ANY )
wxDECLARE_EVENT( I2PParentSocketEvent, wxSocketEvent );

class wxI2PSocketBase : public wxEvtHandler, public CSamLogged//, public wxSocketClient
{
        friend class CSam;
public:
        static void setI2POptions(wxString opts);
        static void setI2POptions(const std::map<wxString,wxString> socket_opts,
                                  const std::map<wxString,wxString> router_opts) ;
        void init();

        void sam_connect() ;

        wxI2PSocketBase(wxSocketFlags flags);

        ~wxI2PSocketBase() ;

        inline wxI2PSocketBase &  Read (void *buffer, wxUint32 nbytes) ;

        inline wxI2PSocketBase &  Write (const void *buffer, wxUint32 nbytes) ;

        virtual bool Close()   ;

        virtual bool Destroy() ;

        bool SetLocal(const CI2PAddress & pubkey);

        bool     GetLocal( CI2PAddress & addr );

        CI2PAddress          GetLocal();

        wxString GetPrivKey() ;
        void     SetPrivKey(wxString) ;

        virtual void SetEventHandler(wxEvtHandler& handler, wxEventTypeTag<CI2PSocketEvent> type);

        virtual wxEvtHandler * GetEventHandler () const {return m_behave.evtHandler;}

        virtual wxEventTypeTag<CI2PSocketEvent> GetEventType() const {return m_behave.evtType;}


        inline bool            Error() const                {return m_lastError!=wxSOCKET_NOERROR;}
        inline wxSocketError   LastError() const            {return m_lastError;}
        inline wxUint32        LastCount() const            {return m_lastCount;}

        inline void            setError(wxSocketError e)    {m_lastError = e;}
        inline void            setCount(wxUint32 c)         {m_lastCount = c;}

        virtual void SetNotify ( wxSocketEventFlags flags );
        virtual void Notify ( bool notify );
        wxSocketEventFlags GetEventFlags() { return m_behave.eventFlags; }
        bool GetNotify() { return m_behave.notify; }


        void SetClientData ( void *data );

        void * GetClientData();

        virtual bool IsOk() const;
        bool Ok() const { return IsOk(); }

        void RestoreStateAll();

        void SaveStateAll();

        void SetFlags ( wxSocketFlags flags );
        wxSocketFlags GetFlags() { return m_behave.flags; }

        void SetTimeout(long int seconds) {m_behave.m_timeout = seconds;}

        void HandleSamEvent(wxSamEvent& e) {OnSamEvent(e);}

        void OnParentSocketEvent( wxSocketEvent & e) {ProcessParentSocketEvent(e);}

protected:
        virtual void ProcessParentSocketEvent( wxSocketEvent & ) ;
        virtual void OnSamEvent(wxSamEvent&) {}
        virtual bool state_machine() {return false;}

        CI2PAddress m_here    ;   // local address
        wxString    m_privKey ;
        CSam * m_sam ;         // sam session associated (except for accepted stream sockets)
        CSam::conn_state_t m_expected_state ; // expected state of the SAM oject

        enum State {NOT_STARTED, CONNECTING, RUNNING, TRANSFERRED, STOPPED };
        State m_state ;
        State m_nextState ;

        long m_startState ;
        wiMutex m_stateMutex ;
        wiCondition m_stateCondition;
        struct Behavior {
                Behavior() ;
                long int m_timeout ;
                wxSocketFlags		flags;
                wxSocketEventFlags	eventFlags;
                bool 			notify;
                void * 			data;
                wxEventTypeTag<CI2PSocketEvent> evtType;
                wxEvtHandler *      evtHandler;
        };




        Behavior m_behave;
        std::stack<Behavior> m_behaveHistory ;

        wxSocketError m_lastError;
        wxUint32      m_lastCount;

        bool m_beingDeleted ;

        /**
         * Event posting
         */
        void AddwxInputEvent()  ;
        void AddwxLostEvent(samerr_t err)   ;
        void AddwxConnectEvent(samerr_t err);
        void processEvents()	;

        wxSocketClient * m_socket ;

protected:
        static wxString s_i2pOptions;


        DECLARE_NO_COPY_CLASS(wxI2PSocketBase);
        //         DECLARE_EVENT_TABLE()

public:
        static CSamLogger * s_log ;
public:
        static void setLogger(CSamLogger & l) {s_log = &l;}
};



wxI2PSocketBase &  wxI2PSocketBase::Read (void *buffer, wxUint32 nbytes)
{
        if (m_socket) m_socket->Read(buffer,nbytes) ;
        return (*this);
}

wxI2PSocketBase &  wxI2PSocketBase::Write (const void *buffer, wxUint32 nbytes)
{
        if (m_socket) m_socket->Write(buffer,nbytes) ;
        return (*this);
}



#endif
