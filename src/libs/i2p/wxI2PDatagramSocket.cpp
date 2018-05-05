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
#include <Logger.h>
#include "wxI2PEvents.h"
#include "wxI2PDatagramSocket.h"
#include "CSamDefines.h"
#include "CSamSession.h"
#include <common/Format.h>
#include <iostream>

// max number of incoming messages to be stored in the queue
size_t wxI2PDatagramSocket::MAX_MESSAGE_QUEUE_LENGTH = 100 ;

CSamLogger * wxI2PDatagramSocket::s_log = &CSamLogger::none ;

const uint32_t & wxI2PDatagramSocket::MaxDatagramSize()
{
        static const uint32_t l = SAM_DGRAM_PAYLOAD_MAX ;
        return l ;
}


wxI2PDatagramSocket::wxI2PDatagramSocket (const wxString & dest,
                wxSocketFlags flags,
                wxString i2pOptions)
        : wxI2PSocketBase(flags)
{
        m_sam = new CSam(dest, s_i2pOptions + wxT(" ") + i2pOptions) ;
        m_sam->AttachClient(this);
        sam_connect() ;
}

wxI2PDatagramSocket::~wxI2PDatagramSocket()
{
}

bool wxI2PDatagramSocket::Close()
{
        return
                wxI2PSocketBase::Close() ;
}

bool wxI2PDatagramSocket::Destroy()
{
        return
                wxI2PSocketBase::Destroy() ;
}


/**
 * STATE MACHINE connecting the SAM datagram socket
 */
void wxI2PDatagramSocket::OnSamEvent(wxSamEvent & e)
{
        if (!state_machine()) {
                if (e.GetStatus()==SAM_OK)
                        s_log->log(CSamLogger::logCRITICAL,
                                wxT("SAM OK received but internal state machine is not ok"));
                wxI2PSocketBase::m_lastError = (wxSOCKET_INVSOCK);
                m_state = STOPPED;
                AddwxLostEvent(e.GetStatus());
        } else if (m_sam->State() == ON) {
                if (e.GetStatus()!=SAM_OK)
                        s_log->log(CSamLogger::logCRITICAL,
                                   wxT("no SAM OK received but internal state machine says ok and SAM state is ON"));
                        wxI2PSocketBase::m_lastError = wxSOCKET_NOERROR;
                AddwxConnectEvent(e.GetStatus());
        } else {
                /* can be a Datagram or another state machine event. Do not log. */
                //                 s_log->log(CSamLogger::logDEBUG,
                //                         CFormat(wxT("SamEvent received. SamEvent message is : %s"))
                //                         % e.GetMessage());
        }
}


bool wxI2PDatagramSocket::state_machine()
{
        wxIPV4address addr;
        if (m_expected_state != m_sam->State()) {
                m_state = STOPPED ;
                return false ;
        }

        switch (m_sam->State()) {
        case OFF:
                m_state = STOPPED;
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
                        m_sam->sam_datagram_session_create();
                }
                break;

        case SESSION_CREATED:
                m_expected_state = NAMING_LOOKEDUP;
                m_sam->sam_naming_lookup(wxT("ME"));
                break;

        case NAMING_LOOKEDUP:
                SetLocal( m_sam->m_lookupResult.key );
                m_sam->SetPubKey(m_sam->m_lookupResult.key.toString());
                m_expected_state = DATAGRAM_INCOMING;
                m_sam->SetState(ON);
                m_state = RUNNING;
                break;

        case DATAGRAM_INCOMING:
                sam_dgramback();
                break;

        default:
                m_state = STOPPED;
                return false ;
        }

        if (m_sam->IsBlocking() and m_state==CONNECTING)
                m_sam->sam_handle_input() ;

        return true;
}



/**
 * logging
 */

void wxI2PDatagramSocket::sam_logback ( wxString s)
{
        s_log->log( CSamLogger::logDEBUG, CFormat (wxT ( "SAM message : %s" ) ) % s );
}



/**
 * wxSocketBase methods
 */


void wxI2PDatagramSocket::SetEventHandler(wxEvtHandler& handler, int id)
{
        wxI2PSocketBase::SetEventHandler(handler, id);
        //wxDatagramSocket::SetEventHandler(handler,id);
}

void wxI2PDatagramSocket::SetClientData ( void *data )
{
        wxI2PSocketBase::SetClientData(data);
        //wxDatagramSocket::SetClientData(data);
}



void wxI2PDatagramSocket::SetNotify ( wxSocketEventFlags flags )
{
        wxI2PSocketBase::SetNotify(flags);
}


void wxI2PDatagramSocket::Notify ( bool notify )
{
        wxI2PSocketBase::Notify(notify);
}

bool wxI2PDatagramSocket::Error() const
{
        return wxI2PSocketBase::Error();
}

wxSocketError   wxI2PDatagramSocket::LastError() const
{
        return wxI2PSocketBase::LastError();
}


bool wxI2PDatagramSocket::IsOk() const
{
        return (m_state == RUNNING) ;
}


bool wxI2PDatagramSocket::GetLocal( CI2PAddress & addr )
{
        return wxI2PSocketBase::GetLocal(addr);
}




void wxI2PDatagramSocket::sam_dgramback()
{
        wiMutexLocker lock(m_receiveMessages_mutex );
        if ( m_receiveMessages.size() > MAX_MESSAGE_QUEUE_LENGTH ) {
                m_receiveMessages.pop_front();
        }

        m_receiveMessages.push_back ( std::move(m_sam->m_datagram) );

        s_log->log( CSamLogger::logDEBUG,
                    CFormat ( wxT ( "(%s:%d:%s) %s" ) )
                    % wxString::FromAscii ( __FILE__ )
                    % __LINE__
                    % wxString::FromAscii ( __func__ )
                    % wxT("message available" )
                  ) ;
        AddwxInputEvent();
}

wxI2PDatagramSocket& wxI2PDatagramSocket::RecvFrom ( CI2PAddress & address, void * buffer, uint32_t nbytes )
{
        try {
                wiMutexLocker lock(m_receiveMessages_mutex);
                if ( m_receiveMessages.size()==0 ) {
                        m_lastCount = 0 ;
                        m_lastError = wxSOCKET_NOERROR ;
                        return (*this) ;
                }
                std::unique_ptr<CDatagramMessage> & message = m_receiveMessages.front();
                uint32_t length = message->length ;
                address = CI2PAddress ( message->destination )  ;
                memcpy ( buffer, message->payload.get(), std::min ( nbytes, length ) );
                m_receiveMessages.pop_front();
                m_lastCount = length;
                m_lastError = wxSOCKET_NOERROR;
                sam_logback(CFormat(wxT("DATAGRAM RECV SIZE=%d")) % m_lastCount);
        } catch ( ... ) {
                sam_logback(wxT("wxI2PDatagramSocket::RecvFrom - Error"));
                m_lastCount = 0;
                m_lastError = wxSOCKET_INVSOCK ;
        }

        return ( *this );
}



wxI2PDatagramSocket& wxI2PDatagramSocket::SendTo ( const CI2PAddress& address, const void * buffer, uint32_t nbytes )
{
        sam_logback(CFormat(wxT("DATAGRAM SEND SIZE=%d")) % nbytes);
        if ( nbytes>SAM_DGRAM_PAYLOAD_MAX ) {
                m_lastError = wxSOCKET_NOERROR;
                m_lastCount = nbytes;

                //m_lastError = wxSOCKET_MEMERR ;
                //m_lastCount = 0 ;
                s_log->log ( CSamLogger::logCRITICAL,
                             CFormat ( wxT ( "DATAGRAM to send is too big. limit=%db message=%d" ) )
                             % SAM_DGRAM_PAYLOAD_MAX % nbytes ) ;
                return (*this);
        }

        if (m_sam) {
                m_sam->sam_dgram_send( address, buffer, nbytes ) ;
                m_lastError = wxSOCKET_NOERROR ;
                m_lastCount = nbytes;
        } else {
                m_lastError = wxSOCKET_INVSOCK ;
                m_lastCount = 0;
        }
        return ( *this );
}
