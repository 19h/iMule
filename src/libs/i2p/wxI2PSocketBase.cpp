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
#include "wxI2PSocketBase.h"
#include <wx/wx.h>
#include <wx/apptrait.h>
#include <wx/app.h>

#include "common/Format.h"
#include "Logger.h"

//#include "myMutex.h"
#include "wxI2PEvents.h"
#include "CSamSession.h"
#include <amule.h>
#include "MuleThread.h"

/* logger */
CSamLogger * wxI2PSocketBase::s_log = &CSamLogger::none ;


wxString wxI2PSocketBase::s_i2pOptions;

// we only need an id, it is not necessary to make an event type
// but wxDEFINE_EVENT provides both
wxDEFINE_EVENT(I2PParentSocketEvent, wxSocketEvent);

void wxI2PSocketBase::setI2POptions(const std::map<wxString,wxString> socket_opts,
                                    const std::map<wxString,wxString> router_opts)
{
        s_i2pOptions = wxT("") ;
        for ( std::map<wxString, wxString>::const_iterator it = socket_opts.begin() ;
                        it != socket_opts.end() ; it++ ) {
                wxString value = it->second ;

                if (value.Find(' ')!=wxNOT_FOUND ||
                                value.Find('\t')!=wxNOT_FOUND ||
                                value.Find('\n')!=wxNOT_FOUND )
                        s_i2pOptions << wxT(" ") << it->first << wxT("=\"") << value << wxT("\"")  ;
                else
                        s_i2pOptions << wxT(" ") << it->first << wxT("=") << value ;
        }

        CSam::setSamAddress(router_opts) ;
}


wxI2PSocketBase::Behavior::Behavior() : evtType(wxEVT_NULL)
{
        m_timeout = 10 * 60 ; // 10 minutes
        flags = wxSOCKET_NONE;
        notify = false;
        eventFlags = 0 ;
        data = NULL;
        evtHandler = 0 ;
}

/**
 * Constructors
 */

wxI2PSocketBase::wxI2PSocketBase(wxSocketFlags flags) : m_stateCondition(m_stateMutex)
{
        init();
        SetFlags(flags);
        Bind(wxEVT_SOCKET, &wxI2PSocketBase::OnParentSocketEvent, this, I2PParentSocketEvent);
}


void wxI2PSocketBase::init()
{
        m_beingDeleted = false ;
        m_lastCount = 0;
        m_lastError = wxSOCKET_INVSOCK;
        m_state     = NOT_STARTED;
        m_nextState = NOT_STARTED;
        m_sam = NULL ;
        Bind(wxSAM_EVENT, &wxI2PSocketBase::HandleSamEvent, this );
}

/**
 * Destructor
 */
wxI2PSocketBase::~wxI2PSocketBase()
{
#if !wxCHECK_VERSION(3,0,0)
        // Just in case the app called Destroy() *and* then deleted
        // the socket immediately: don't leave dangling pointers.
        wxAppTraits *traits = wxTheApp ? wxTheApp->GetTraits() : NULL;
        if ( traits )
                traits->RemoveFromPendingDelete(this);
#endif
        // Shutdown and close the socket
        if (!m_beingDeleted)
                Close();
}

/**
 * connect to sam bridge
 */

void wxI2PSocketBase::sam_connect()
{
        bool blocking = wxI2PSocketBase::GetFlags() & wxSOCKET_BLOCK ;

        if (blocking) {
                if (!wiThread::IsMain() and wxApp::IsMainLoopRunning())
                        wxMutexGuiEnter();
                m_sam->SetBlocking(true);
        }

        m_state     = CONNECTING ;
        m_expected_state = SERVER_CONNECTED ;
        m_sam->sam_start_connect();

        while (blocking and m_state==CONNECTING) {
                if (!state_machine()) {
                        wxI2PSocketBase::m_lastError = (wxSOCKET_INVSOCK);
                }
        }
        if (blocking) {
                m_sam->SetBlocking(false);
                if ( !wiThread::IsMain() and wxApp::IsMainLoopRunning())
                        wxMutexGuiLeave();
        }
}



/**
 * Posting events
 */

void wxI2PSocketBase::ProcessParentSocketEvent( wxSocketEvent & e)
{
        if (((e.m_event == wxSOCKET_INPUT ) and !(GetEventFlags() & wxSOCKET_INPUT_FLAG )) or
                        ((e.m_event == wxSOCKET_OUTPUT) and !(GetEventFlags() & wxSOCKET_OUTPUT_FLAG)) or
                        ((e.m_event == wxSOCKET_LOST  ) and !(GetEventFlags() & wxSOCKET_LOST_FLAG  )) or
                        ((e.m_event == wxSOCKET_CONNECTION  ) and !(GetEventFlags() & wxSOCKET_CONNECTION_FLAG  )))
                return ;


        CI2PSocketEvent evt ( GetEventType() );;
        evt.m_event = e.m_event ;
        evt.m_clientData = GetClientData() ;
        //evt.SetEventObject ( e.GetSocket() );
        evt.SetEventObject ( this );

        s_log->log ( CSamLogger::logDEBUG,
                     CFormat ( wxT ( "Retransmitting event (here=%x)" ) )
                     % wxI2PSocketBase::GetLocal().hashCode() );
        //GetEventHandler()->AddPendingEvent ( evt );
        GetEventHandler()->ProcessEvent ( evt );
}


void wxI2PSocketBase::AddwxConnectEvent(samerr_t err)
{
        if ( GetNotify() && ( GetEventFlags() & wxSOCKET_CONNECTION_FLAG ) ) {
                s_log->log ( CSamLogger::logDEBUG,
                             CFormat ( wxT ( "Post Connection Event here=%x" ) )
                             % wxI2PSocketBase::GetLocal().hashCode() );

                CI2PSocketEvent evt (GetEventType());;
                evt.m_event = wxSOCKET_CONNECTION ;
                evt.m_clientData = GetClientData() ;
                evt.SetEventObject ( dynamic_cast<wxI2PSocketBase *>(this) );
                evt.SetSamError(err);
                /*        s_log->log ( false, logWxEvents,
                                           CFormat ( wxT ( "(%s:%d:%s)" ) )
                                                   % wxString::FromAscii ( __FILE__ )
                                                   % __LINE__
                                                   % wxString::FromAscii ( __func__ ) );
                */
                GetEventHandler()->AddPendingEvent ( evt );
                //GetEventHandler()->ProcessEvent ( evt );
        }
}

void wxI2PSocketBase::AddwxLostEvent(samerr_t err)
{
        if ( GetNotify() && ( GetEventFlags() & wxSOCKET_LOST_FLAG ) ) {
                CI2PSocketEvent evt(GetEventType());;
                evt.m_event = wxSOCKET_LOST ;
                evt.m_clientData = wxI2PSocketBase::GetClientData() ;
                evt.SetEventObject ( dynamic_cast<wxI2PSocketBase *>(this) );
                evt.SetSamError(err);
                s_log->log ( CSamLogger::logDEBUG,
                             CFormat ( wxT ( "(%s:%d:%s)" ) )
                             % wxString::FromAscii ( __FILE__ )
                             % __LINE__
                             % wxString::FromAscii ( __func__ ) );
                GetEventHandler()->AddPendingEvent ( evt );
                //GetEventHandler()->ProcessEvent ( evt );
                s_log->log ( CSamLogger::logDEBUG, CFormat( wxT ( "Posted connection failed. Message : %s" ) ) %
                             m_sam->m_err_message );
        }
}


void wxI2PSocketBase::AddwxInputEvent()
{
        if ( GetNotify() && ( GetEventFlags() & wxSOCKET_INPUT_FLAG ) ) {
                CI2PSocketEvent evt ( GetEventType() );;
                evt.m_event = wxSOCKET_INPUT ;
                evt.m_clientData = wxI2PSocketBase::GetClientData() ;
                evt.SetEventObject ( dynamic_cast<wxI2PSocketBase *>(this) );
                s_log->log ( CSamLogger::logDEBUG,
                             CFormat ( wxT ( "(%s:%d:%s)" ) )
                             % wxString::FromAscii ( __FILE__ )
                             % __LINE__
                             % wxString::FromAscii ( __func__ ) );
                GetEventHandler()->AddPendingEvent ( evt );
                //GetEventHandler()->ProcessEvent ( evt );
                s_log->log ( CSamLogger::logDEBUG, CFormat( wxT ( "Posted input event." ) ) );
        }
}



/**
 * wxSocketBase methods
 */


bool wxI2PSocketBase::Close()
{
        if (m_sam) {
                m_sam->DetachClient(this);
                m_sam = NULL ;
        }
        m_state = STOPPED ;

        return true;
}


bool wxI2PSocketBase::Destroy()
{
        // Delayed destruction: the socket will be deleted during the next
        // idle loop iteration. This ensures that all pending events have
        // been processed.
        m_beingDeleted = true;

        // Shutdown and close the socket
        Close();

        // Supress events from now on
        Notify(false);

        // schedule this object for deletion
#if wxCHECK_VERSION(3,0,0)
        if ( wxTheApp ) {
                // let the traits object decide what to do with us
	        wxGetApp().ScheduleForDestruction(this);
        } else { // no app or no traits
                // in wxBase we might have no app object at all, don't leak memory
                delete this;
        }
#else
        wxAppTraits *traits = wxTheApp ? wxTheApp->GetTraits() : NULL;
        if ( traits ) {
                // let the traits object decide what to do with us
                traits->ScheduleForDestroy(this);
         } else { // no app or no traits
                 // in wxBase we might have no app object at all, don't leak memory
                 delete this;
	}
#endif
        return true;
}

void wxI2PSocketBase::SetEventHandler ( wxEvtHandler & evtH, wxEventTypeTag<CI2PSocketEvent> eventType )
{
        m_behave.evtHandler = &evtH;
        m_behave.evtType    = eventType;
}

void wxI2PSocketBase::SetFlags ( wxSocketFlags flags )
{

        m_behave.flags = flags;
}

void wxI2PSocketBase::SetNotify ( wxSocketEventFlags flags )
{
        m_behave.eventFlags = flags;
}

void wxI2PSocketBase::Notify ( bool notify )
{
        m_behave.notify = notify;
}

void wxI2PSocketBase::RestoreStateAll()
{
        if ( m_behaveHistory.empty() ) return;

        m_behave = m_behaveHistory.top() ;

        m_behaveHistory.pop();
}

void wxI2PSocketBase::SaveStateAll()
{
        m_behaveHistory.push ( m_behave );
}



void wxI2PSocketBase::SetClientData ( void *data )
{
        m_behave.data = data;
}

void * wxI2PSocketBase::GetClientData()
{
        return m_behave.data;
}


/// wxI2PSocketBase::     IsOk()

bool wxI2PSocketBase::IsOk() const
{
        return m_state == RUNNING ;
}





/// wxI2PSocketBase::     GetLocal      ( CI2PAddress & addr )

bool wxI2PSocketBase::GetLocal ( CI2PAddress & addr )
{
        return ( addr = m_here ).isValid() ;
}

/// wxI2PSocket::     GetLocal      (  )

CI2PAddress wxI2PSocketBase::GetLocal()
{
        return m_here ;
}

wxString wxI2PSocketBase::GetPrivKey()
{
        return m_privKey ;
}

void wxI2PSocketBase::SetPrivKey(wxString p)
{
        m_privKey = p.c_str();
}



bool wxI2PSocketBase::SetLocal(const CI2PAddress & pubkey)
{
        m_here = pubkey  ;
        if (m_here.isValid()) {
                //m_state= RUNNING ;
                m_lastError = wxSOCKET_NOERROR;
                return true ;
        } else {
                //m_state = STOPPED ;
                m_lastError = wxSOCKET_INVSOCK;
                s_log->log ( CSamLogger::logDEBUG, wxT(
                                     "Invalid local destination set") );
                return false;
        }
}

void wxI2PSocketBase::processEvents()
{
        wxGetApp().Yield(true);
// #if wxUSE_THREADS
//         if ( !wiThread::IsMain() )
//             wiThread::Yield();
//         else
// #endif
//             ::wxYield();
}

