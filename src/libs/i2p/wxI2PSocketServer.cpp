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
#include <iostream>

#include "Logger.h"
#include <common/Format.h>

#include "wxI2PEvents.h"
#include "wxI2PSocketBase.h"
#include "wxI2PSocketServer.h"
#include "CSamSession.h"
#include "myMutexLocker.h"
#include "MuleThread.h"

/**
 *
 *                       WXI2PSOCKETSERVER CLASS
 *
 */


CSamLogger * wxI2PSocketServer::s_log = &CSamLogger::none ;

//!                      Constructor


/**
 * socket server on a specified destination, or a new destination if addr is empty
 */

wxI2PSocketServer::wxI2PSocketServer ( const wxString & addr,
                                       wxSocketFlags flags, wxString options )
        : wxI2PSocketBase(flags)
{
//         SetLocal( CI2PAddress(addr) ) ;
        m_accepter = NULL ;

        m_sam = CSam::NewSession( this, addr, s_i2pOptions + wxT(" ") + options );

        sam_connect();

        /*if (m_state == RUNNING) {
                m_state     = CONNECTING ;
                m_forwarder = new Forwarder(*this, m_sam->getSessionName(),
                                             laddr.Service(), GetFlags());
        }*/
        m_lastError = wxSOCKET_NOERROR ;

        s_log->log ( CSamLogger::logDEBUG, wxT ( "Exiting constructor" ) );
}



wxI2PSocketServer::~wxI2PSocketServer()
{
        if (m_accepter)
                m_accepter->Destroy() ;
        m_accepter = NULL ;
}

bool wxI2PSocketServer::Error()
{
        return wxI2PSocketBase::Error();
}

/**
 * starting and stoping a listening session
 */

void wxI2PSocketServer::StartListening()
{
        wiMutexLocker lock(m_accepter_mutex);
        bool start ;
        /* should we start ? */
        start = ( GetNotify()==true ) && ( GetEventFlags() & wxSOCKET_CONNECTION_FLAG ) ;
        if (start and m_state == RUNNING and m_accepter == NULL)
                m_accepter = new Accepter(*this, m_sam->getSessionName(), GetFlags());
}

void wxI2PSocketServer::StopListening()
{
        wiMutexLocker lock(m_accepter_mutex);
        if (m_state != RUNNING or m_accepter == NULL)
                return ;
        m_accepter->Destroy();
        m_accepter = NULL ;
}

bool wxI2PSocketServer::Close()
{
        Notify(false);
        StopListening();
        return wxI2PSocketBase::Close();
}

bool wxI2PSocketServer::Destroy()
{
        StopListening();
        return wxI2PSocketBase::Destroy();
}


/**
 * STATE MACHINE connecting the SAM stream session socket
 */
void wxI2PSocketServer::OnSamEvent(wxSamEvent & e)
{
        if (!state_machine()) {
                wxI2PSocketBase::m_lastError = (wxSOCKET_INVSOCK);
                AddwxLostEvent(e.GetStatus());
        } else if (m_state==RUNNING) {
                StartListening();
                AddwxConnectEvent(e.GetStatus());
        }
}

bool wxI2PSocketServer::state_machine()
{
        wxIPV4address addr;
        if (m_expected_state != m_sam->State()) {
                m_state = STOPPED ;
                return false ;
        }

        switch (m_sam->State()) {
        case OFF:
                m_state = STOPPED ;
                return false ;

        case SERVER_CONNECTED:
                m_expected_state = SAM_HELLOED ;
                m_sam->sam_hello();
                break;

        case KEYS_GENERATED:
                m_sam->SetPrivKey(m_sam->privKeyAnswer);
                m_sam->SetPubKey(m_sam->pubKeyAnswer);
                wxI2PSocketBase::SetLocal(CI2PAddress(m_sam->pubKeyAnswer));
                SetPrivKey(m_sam->GetPrivKey());

        case SAM_HELLOED:
                if (m_sam->GetPrivKey()==wxEmptyString) {
                        m_expected_state = KEYS_GENERATED ;
                        m_sam->sam_generate_keys() ;
                } else {
                        m_expected_state = SESSION_CREATED ;
                        m_sam->sam_stream_session_create();
                }
                break;

        case SESSION_CREATED:
                m_expected_state = NAMING_LOOKEDUP;
                m_sam->sam_naming_lookup(wxT("ME"));
                break;

        case NAMING_LOOKEDUP:
                SetLocal( m_sam->m_lookupResult.key );
                m_sam->SetPubKey(m_sam->m_lookupResult.key.toString());
                m_expected_state = ON;
                m_sam->SetState(ON);
                m_state = RUNNING ;
                break ;

        default:
                m_state = STOPPED ;
                return false ;
        }

        if (m_sam->IsBlocking() and m_state==CONNECTING)
                m_sam->sam_handle_input() ;

        return true;
}


void wxI2PSocketServer::OnAccepterEnd(bool ready, samerr_t err)
{
        wiMutexLocker lock(m_accepter_mutex);
        if (m_accepter==NULL or !ready) {
                AddwxLostEvent(err);
        } else {
                AddwxConnectEvent(err);
        }
}

/**
 * STATE MACHINE connecting the forwarder socket
 */
wxI2PSocketServer::Accepter::Accepter( wxI2PSocketServer & parent,
                                       const wxString & nick,
                                       wxSocketFlags flags )
        : wxI2PSocketClient(flags), m_parent(parent)
{
        m_nick = nick ;
        m_available = false ;
        m_sam = new CSam( nick, s_i2pOptions, this ) ;

        sam_connect();

        s_log->log ( CSamLogger::logDEBUG, wxT ( "Accepter: Exiting constructor" ) );
}


void wxI2PSocketServer::Accepter::OnSamEvent(wxSamEvent &  e)
{
        state_machine(e.GetStatus());
}

bool wxI2PSocketServer::Accepter::state_machine(samerr_t err)
{
        if (m_expected_state != m_sam->State()) {
                m_parent.OnAccepterEnd(false,err);
                return false ;
        }

        switch (m_sam->State()) {
        case OFF:
                m_parent.OnAccepterEnd(false,err);
                return false ;

        case SERVER_CONNECTED:
                m_expected_state = SAM_HELLOED ;
                m_sam->sam_hello();
                break;

        case SAM_HELLOED:
                m_expected_state = STREAM_OK;
                m_sam->sam_accept_incoming_stream( m_nick );
                break;

        case STREAM_OK :
                m_expected_state = ON;
                m_sam->SetState(   ON );
                m_state = TRANSFERRED ;
                break;

        default:
                m_expected_state = OFF;
                m_sam->SetState(   OFF );
                m_parent.OnAccepterEnd(false,err);
                return false ;
        }

        if (m_sam->IsBlocking() and m_sam->State()!=m_expected_state)
                m_sam->sam_handle_input() ;

        return true;
}

void wxI2PSocketServer::Accepter::ProcessParentSocketEvent( wxSocketEvent & e)
{
        if (m_state==TRANSFERRED and
                        (e.GetSocketEvent()==wxSOCKET_INPUT or e.GetSocketEvent()==wxSOCKET_LOST)) {
                s_log->log ( CSamLogger::logDEBUG,
                             CFormat ( wxT ( "Accepter received event %s " ) )
                             % (e.GetSocketEvent() == wxSOCKET_INPUT ? wxT("wxSOCKET_INPUT") : wxT("wxSOCKET_LOST") ) );

                m_available = true ;
                m_parent.OnAccepterEnd( e.GetSocketEvent()==wxSOCKET_INPUT, SAM_NONE );
        } else {
                wxI2PSocketClient::ProcessParentSocketEvent(e) ;
        }
}

/**
 * wxSocketBase methods
 */


void wxI2PSocketServer::SetEventHandler(wxEvtHandler& handler, int id)
{
        wxI2PSocketBase::SetEventHandler(handler,id);
        //if (m_server) wxSocketServer::SetEventHandler(handler,id);
}

void wxI2PSocketServer::Notify ( bool notify )
{
        wxI2PSocketBase::Notify(notify);
        StartListening();
}

void wxI2PSocketServer::SetNotify ( wxSocketEventFlags flags )
{
        wxI2PSocketBase::SetNotify(flags);
        StartListening();
}

void wxI2PSocketServer::SetClientData ( void *data )
{
        wxI2PSocketBase::SetClientData(data);
}

bool wxI2PSocketServer::GetLocal( CI2PAddress & addr )
{
        return wxI2PSocketBase::GetLocal(addr);
}

bool wxI2PSocketServer::IsOk() const
{
        return wxI2PSocketBase::IsOk();
}


//!                      Accept

wxI2PSocketClient * wxI2PSocketServer::Accept ( bool wait )
{
        wxI2PSocketClient * newSock = new wxI2PSocketClient() ;
        wxI2PSocketServer::AcceptWith( *newSock, wait ) ;
        return newSock ;
}


//!                      AcceptWith

bool wxI2PSocketServer::AcceptWith ( wxI2PSocketClient & socket, bool  WXUNUSED(wait) )
{
        {
                wiMutexLocker lock(m_accepter_mutex);
                if (! m_accepter or ! m_accepter->IsAvailable()) return false ;
                delete socket.m_socket;
                socket.m_socket = m_accepter->m_socket ;
                m_accepter->m_socket = NULL ;
                m_accepter->Destroy() ;
                m_accepter = NULL ;
        }
        StartListening();
        socket.SaveStateAll();
        socket.SetTimeout(1);
        socket.SetFlags(wxSOCKET_WAITALL | wxSOCKET_BLOCK);
        socket.Notify(false);
        sam_pubkey_t key ;
        socket.transfer();
        socket.m_socket->Read(key, BASE64_PUBKEY_LEN+1);
        socket.RestoreStateAll();
        socket.transfer();

        if (socket.LastCount()==BASE64_PUBKEY_LEN+1) {
                if (key[BASE64_PUBKEY_LEN]=='\n') {
                        key[BASE64_PUBKEY_LEN] = 0 ;
                        socket.SetLocal(GetLocal());
                        socket.SetPeer(CI2PAddress(key));
                        socket.m_state = TRANSFERRED ;
                }
        }
        if (socket.m_state==TRANSFERRED)
                return true ;
        else
                return false ;
}

void wxI2PSocketServer::ProcessParentSocketEvent( wxSocketEvent & e)
{
        wxI2PSocketBase::ProcessParentSocketEvent(e);
}

void wxI2PSocketServer::sam_logback ( wxString s)
{
        s_log->log ( CSamLogger::logDEBUG, CFormat (
                             wxT ( "SAM message : %s" ) )
                     % s );
}

