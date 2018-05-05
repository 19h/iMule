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

#include "Logger.h"
#include <common/Format.h>

#include "wxI2PEvents.h"
#include "wxI2PSocketClient.h"
#include "wxI2PSocketServer.h"
#include "CSamSession.h"

#include "myMutexLocker.h"

/**
 *   class WXI2PSOCKETCLIENT
 */
CSamLogger *wxI2PSocketClient::s_log = &CSamLogger::none;

/// Constructor

wxI2PSocketClient::wxI2PSocketClient (wxSocketFlags flags):
        wxI2PSocketBase (flags)
{
        m_sam_server = NULL;
        m_socket = new wxSocketClient (flags);
        m_socket->SetEventHandler (*this, I2PParentSocketEvent);
        m_socket->SetNotify (wxSOCKET_LOST_FLAG | wxSOCKET_CONNECTION_FLAG |
                             wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG);
        m_socket->Notify (true);
}

wxI2PSocketClient::~wxI2PSocketClient ()
{
}

bool wxI2PSocketClient::Destroy ()
{
        bool ret = m_socket ? m_socket->Destroy () : true;
        m_socket = nullptr;
        return wxI2PSocketBase::Destroy() and ret;
}

bool wxI2PSocketClient::Close ()
{
        if (m_sam_server)
                m_sam_server->DetachClient (this);
        m_sam_server = NULL;
        bool ret = m_socket ? m_socket->Close () : true;
        return wxI2PSocketBase::Close () and ret;
}

void wxI2PSocketClient::SetPeer (const CI2PAddress & from)
{
        m_there = from;
}


/// wxI2PSocket::     GetPeer      ( CI2PAddress & addr )

bool wxI2PSocketClient::GetPeer (CI2PAddress & addr)
{
        return (addr = m_there).isValid ();
}

/// wxI2PSocket::     GetPeer      (  )

CI2PAddress wxI2PSocketClient::GetPeer ()
{
        return m_there;
}

void wxI2PSocketClient::Flush ()
{
}



void wxI2PSocketClient::SetTimeout (long int t)
{
        wxI2PSocketBase::SetTimeout (t);
        if (m_state == TRANSFERRED)
                m_socket->SetTimeout (t);
}

void wxI2PSocketClient::SetFlags (wxSocketFlags flags)
{
        wxI2PSocketBase::SetFlags (flags);
        if (m_state == TRANSFERRED)
                m_socket->SetFlags (flags);
}

void wxI2PSocketClient::SetEventHandler (wxEvtHandler & handler, int id)
{
        wxI2PSocketBase::SetEventHandler (handler, id);
        /*if (m_state==TRANSFERRED) {
           m_socket->SetEventHandler(handler,id);
           } */
}

void wxI2PSocketClient::SetNotify (wxSocketEventFlags flags)
{
        wxI2PSocketBase::SetNotify (flags);
        if (m_state == TRANSFERRED) {
                m_socket->SetNotify (flags);
        }
}

void wxI2PSocketClient::Notify (bool notify)
{
        wxI2PSocketBase::Notify (notify);
        if (m_state == TRANSFERRED) {
                m_socket->Notify (notify);
        }
}

bool wxI2PSocketClient::Error () const
{
        //if (m_state==TRANSFERRED) {
        return m_socket->Error ();
        /*} else {
        return wxI2PSocketBase::Error();
        } */
}

wxSocketError wxI2PSocketClient::LastError () const
{
        //if (m_state==TRANSFERRED) {
        return m_socket->LastError ();
        /*} else {
        return wxI2PSocketBase::LastError();
        } */
}

bool wxI2PSocketClient::WaitOnConnect (long seconds, long milliseconds)
{
        wxDateTime limit = wxDateTime::UNow ()
                           + wxTimeSpan (0, 0,
                                         seconds == -1 ? m_behave.m_timeout : seconds,
                                         milliseconds);
        while (true) {
                // check if currently connecting
                switch (m_state) {
                case TRANSFERRED:
                        return true;
                case STOPPED:
                        return false;
                case CONNECTING:
                        break;
                default:
                        return false;
                }
                if (wxDateTime::UNow () >= limit) {
                        return false;
                }
                processEvents ();
        }
}


/// Connect

bool wxI2PSocketClient::Connect (const CI2PAddress & address, bool wait,
                                 wxString options)
{
        s_log->log (CSamLogger::logDEBUG,
                    CFormat (wxT ("connect to dest=%x,  wait=%s"))
                    % address.hashCode ()
                    % (wait ? wxT ("true") : wxT ("false")));

        return Connect (address, wxEmptyString, wait, options);
}




bool wxI2PSocketClient::Connect (const CI2PAddress & address,
                                 const wxString & local, bool wait,
                                 wxString options)
{
        m_there = address;

        // look for a suitable server for the local destination
        m_sam_server = CSam::AttachedSession (this, local, options);
        if (m_sam_server == NULL) {
                s_log->log (CSamLogger::logCRITICAL,
                            CFormat (wxT
                                     ("No server is running with local destination %s"))
                            % local);
                m_lastError = wxSOCKET_INVSOCK;
                m_state = STOPPED;
                return false;
        } else if (m_sam_server->State () != ON) {
                s_log->log (CSamLogger::logCRITICAL,
                            CFormat (wxT
                                     ("A server exists but is not ready at local destination %s"))
                            % local);
                m_lastError = wxSOCKET_INVSOCK;
                m_state = STOPPED;
                Close ();
                return false;
        }

        m_here = CI2PAddress (m_sam_server->GetPubKey ());

        s_log->log (CSamLogger::logDEBUG,
                    CFormat (wxT
                             ("connect to dest=%x,  from dest=%s,   wait=%s"))
                    % address.hashCode ()
                    % (wxT ("iMule_") + GetLocal ().toString ().Left (10))
                    % (wait ? wxT ("true") : wxT ("false")));

        // open a new sam session using this socket
        m_sam = new CSam (m_sam_server->GetPrivKey (),
                          s_i2pOptions + wxT (" ") + options,
                          this);

        // puis lancer la connexion
        if (wait) {
                SetFlags (wxSOCKET_WAITALL | wxSOCKET_BLOCK);
        }
        sam_connect ();

        return (m_state == TRANSFERRED);
}

void wxI2PSocketClient::OnSamEvent (wxSamEvent & WXUNUSED (e))
{
        state_machine ();
}

bool wxI2PSocketClient::state_machine ()
{
        s_log->log (CSamLogger::logDEBUG,
                    wxString::Format (wxT ("state_machine. Sam=%p"), m_sam));
        wxCHECK_MSG (m_sam, false,
                     wxT("wxI2SocketClient::state_machine called but m_sam is null"));
        if (m_expected_state != m_sam->State ()) {
                connect_failed ();
                return false;
        }

        switch (m_sam->State ()) {
        case OFF:
                connect_failed ();
                return false;

        case SERVER_CONNECTED:
                m_expected_state = SAM_HELLOED;
                m_sam->sam_hello ();
                break;

        case SAM_HELLOED:
                m_expected_state = STREAM_OK;
                m_sam->sam_connect_clientsocket (m_there.toString ());
                break;

        case STREAM_OK:
                m_expected_state = ON;
                m_sam->SetState (ON);
                connect_succeeded ();
                break;

        default:
                connect_failed ();
                return false;
        }

        if (m_state == CONNECTING and m_sam and m_sam->IsBlocking ())
                m_sam->sam_handle_input ();

        return true;
}

void wxI2PSocketClient::connect_failed ()
{
        //RestoreStateAll() ;
        m_lastError = wxSOCKET_INVSOCK;
        transfer ();
//    m_state = STOPPED ;
//    AddwxLostEvent();
}

void wxI2PSocketClient::connect_succeeded ()
{
        //RestoreStateAll() ;
        m_lastError = wxSOCKET_NOERROR;
        AddwxConnectEvent ();
        transfer ();
        m_sam = NULL;
        throw CSam::SocketTransferred ();
}

void wxI2PSocketClient::transfer ()
{
        m_socket->SetTimeout (m_behave.m_timeout);
        m_socket->SetFlags (wxI2PSocketBase::GetFlags ());
        m_socket->SetNotify (wxI2PSocketBase::GetEventFlags ());
        m_socket->Notify (wxI2PSocketBase::GetNotify ());
        m_socket->SetClientData (wxI2PSocketBase::GetClientData ());
        m_socket->SetEventHandler (*this, I2PParentSocketEvent);
        m_state = TRANSFERRED;
}

void wxI2PSocketClient::AddwxConnectEvent ()
{
//      m_socket->Write(NULL,0);
//         printf ("wxI2PSocketClient::AddwxConnectEvent() m_socket->m_error = %d\n", m_socket->Error ());
        if (wxI2PSocketBase::GetNotify ()) {
                if (wxI2PSocketBase::GetEventFlags () &
                                wxSOCKET_CONNECTION_FLAG) {
                        s_log->log (CSamLogger::logDEBUG,
                                    CFormat (wxT
                                             ("Post Connection Event here=%x"))
                                    %
                                    wxI2PSocketBase::GetLocal ().hashCode ());

                        CI2PSocketEvent evt (GetEventType ());;
                        evt.m_event = wxSOCKET_CONNECTION;
                        evt.m_clientData = GetClientData ();
                        evt.SetEventObject (dynamic_cast <
                                            wxI2PSocketBase * >(this));
                        GetEventHandler ()->AddPendingEvent (evt);
//                      GetEventHandler()->ProcessEvent ( evt );
                }
                if (wxI2PSocketBase::GetEventFlags () & wxSOCKET_OUTPUT_FLAG) {
                        s_log->log (CSamLogger::logDEBUG,
                                    CFormat (wxT
                                             ("Post Output Event here=%x"))
                                    %
                                    wxI2PSocketBase::GetLocal ().hashCode ());

                        CI2PSocketEvent evt (GetEventType ());;
                        evt.m_event = wxSOCKET_OUTPUT;
                        evt.m_clientData = GetClientData ();
                        evt.SetEventObject (dynamic_cast <
                                            wxI2PSocketBase * >(this));
                        GetEventHandler ()->AddPendingEvent (evt);
//                      GetEventHandler()->ProcessEvent ( evt );
                }
        }
}

wxString eventString (int eventId)
{
        switch (eventId) {
        case wxSOCKET_INPUT:
                return wxT ("wxSOCKET_INPUT");
        case wxSOCKET_OUTPUT:
                return wxT ("wxSOCKET_OUTPUT");
        case wxSOCKET_CONNECTION:
                return wxT ("wxSOCKET_CONNECTION");
        case wxSOCKET_LOST:
                return wxT ("wxSOCKET_LOST");
        }
        return wxT ("wxSOCKET_UNKNOWN_EVENT_ID");
}

void wxI2PSocketClient::ProcessParentSocketEvent (wxSocketEvent & e)
{
        if (m_state == TRANSFERRED) {
                s_log->log (CSamLogger::logDEBUG,
                            CFormat (wxT
                                     ("Retransmitting event %s to HANDLER     %x (there=%x)"))
                            % eventString (e.GetSocketEvent ()) %
                            (uint64_t) GetEventHandler ()
                            % GetPeer ().hashCode ());

                wxI2PSocketBase::ProcessParentSocketEvent (e);
        } else if (m_sam) {
                s_log->log (CSamLogger::logDEBUG,
                            CFormat (wxT
                                     ("Retransmitting event %s to CSamSession %x (here=%x)"))
                            % eventString (e.GetSocketEvent ()) %
                            (long long unsigned int) m_sam %
                            wxI2PSocketBase::GetLocal ().hashCode ());
                m_sam->ProcessEvent (e);
        } else if (m_state == STOPPED) {
                s_log->log (CSamLogger::logERROR,
                            CFormat (wxT
                                     ("Event %s received by stopped socket (here=%x)"))
                            % eventString (e.GetSocketEvent ())
                            % wxI2PSocketBase::GetLocal ().hashCode ());
        } else {
                wxASSERT_MSG (false,
                              wxT
                              ("socket event with non transferred socket without a sam session"));
        }
}
