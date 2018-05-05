#include "i2p/wxI2PSocketBase.h"
#include "I2PConnectionManager.h"

#include "Preferences.h"		// Needed for CPreferences
#include "ServerSocket.h"		// Needed for CServerSocket
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ServerUDPSocket.h"		// Needed for CServerUDPSocket
#include "ClientUDPSocket.h"		// Needed for CClientUDPSocket & CMuleUDPSocket
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Prefs.h"
#include "Logger.h"
#include <common/Format.h>		// Needed for CFormat
#include "i2p/CI2PRouter.h"             // Needed for CI2PRouter
#include "i2p/CSamSession.h"
#include "i2p/wxI2PDatagramSocket.h"
#include "i2p/wxI2PSocketServer.h"
#include "i2p/wxI2PSocketClient.h"
#include <common/EventIDs.h>

#define DELAY_BEFORE_RECONNECTION 1000*60

wxDEFINE_EVENT(CONN_MANAGER_UDP_EVENT, CI2PSocketEvent);
wxDEFINE_EVENT(CONN_MANAGER_TCP_EVENT, CI2PSocketEvent);
wxDEFINE_EVENT(CONN_MANAGER_TIMEOUT,   wxTimerEvent); // used as an int (id). wxTimer event type is always wxEVT_TIMER.


/**
 * logs de la bibliothèque wxI2P
 */

class wxCLogger : public CSamLogger
{
public:
        wxCLogger(DebugType t) { this->type = t;}
private:
        DebugType type ;
protected:
        inline virtual void logMessage ( Level level, wxString string ) {
                if (level>=logERROR) {
                        AddDebugLogLineC(type, string) ;
                } else {
                        AddDebugLogLineN(type, string) ;
                }
        }
        virtual bool shouldLog ( Level l ) {
                return ( l == logCRITICAL ) or theLogger.IsEnabled(type);
        }
};

wxCLogger logUDP(logI2PDatagramSocket);
wxCLogger logBase(logI2PSocketBase);
wxCLogger logTCP(logI2PSocketServer);
wxCLogger logTCPClient(logI2PSocketClient);
wxCLogger logSam(logSamSession);

/**
 *  Constructor
 */
I2PConnectionManager::I2PConnectionManager() : m_timer( this, CONN_MANAGER_TIMEOUT )
{
        m_tcp_state = CONN_OFF ;
        m_udp_state = CONN_OFF ;
        m_net_state = CONN_OFF ;
        CSam::setLogger(logSam);
        wxI2PSocketBase    ::setLogger( logBase      );
        wxI2PSocketClient  ::setLogger( logTCPClient );
        wxI2PSocketServer  ::setLogger( logTCP       );
        wxI2PDatagramSocket::setLogger( logUDP       );
        Bind(CONN_MANAGER_UDP_EVENT, &I2PConnectionManager::OnDatagramSocketEvent, this);
        Bind(CONN_MANAGER_TCP_EVENT, &I2PConnectionManager::OnListenSocketEvent,   this);
        Bind(wxEVT_TIMER, &I2PConnectionManager::OnDelayTimeoutEvent,   this, CONN_MANAGER_TIMEOUT);
}

void I2PConnectionManager::start()
{
        // initialize Kad ID in order to set UDP and TCP destinations accordingly
        Kademlia::CPrefs::getInstance();

        // launch the state machine
        if ( m_net_state == CONN_OFF ) {
                m_target = NET_START ;
                state_machine();
        }
}

void I2PConnectionManager::stop()
{
        m_target = NET_STOP;
        state_machine();
}



void
I2PConnectionManager::OnDelayTimeoutEvent (wxTimerEvent & WXUNUSED(evt))
{
  m_target = NET_START;
  state_machine();
}


void I2PConnectionManager::OnListenSocketEvent( CI2PSocketEvent & event )
{
        wxASSERT( event.GetClientData() == theApp->listensocket );

        switch ( event.GetSocketEvent() ) {
        case wxSOCKET_CONNECTION :

                tcp_server_connected();

                break;

        case wxSOCKET_LOST :

                tcp_server_has_died( i2pstrerror( event.GetSamError() ) );

                break;

        default:
                wxASSERT(false);
        }
}

void I2PConnectionManager::OnDatagramSocketEvent( CI2PSocketEvent & event )
{
        void * tmp = theApp->clientudp;
        if (!tmp)
                return ;
        wxASSERT( event.GetClientData() == tmp );

        switch ( event.GetSocketEvent() ) {
        case wxSOCKET_CONNECTION :

                udp_server_connected();

                break;

        case wxSOCKET_LOST :

                udp_server_has_died( i2pstrerror( event.GetSamError() ) );

                break;

        default:
                AddDebugLogLineC(logGeneral, _ ( "Discarding received datagram : UDP socket not connected" ) );
        }
}


void I2PConnectionManager::state_machine()
{
        {
                static bool first_start = true ;
                // wait for main loop running

                if (theApp->IsOnShutDown()) return ;

                switch ( m_target ) {
                case NET_START :
                case NET_DELAYED_START :
                        if ( first_start ) {
                                start_i2p_router() ;

                                start_ed2k_server_connection();

                                first_start = false ;

                                if ( thePrefs::GetI2PServerInternal() )
                                        m_target = NET_DELAYED_START ;
                        }
                        theApp->SetNetworkStarting() ;
                        if ( m_target == NET_DELAYED_START ) {
                                AddLogLineC(CFormat (_ ( "Next connection try in %d seconds" ) ) % (wxInt32) (DELAY_BEFORE_RECONNECTION / 1000));

                                m_timer.Start( DELAY_BEFORE_RECONNECTION, true  ) ;
                                break ;
                        }
                        m_net_state = CONN_STARTING ;
                        if ( m_udp_state != CONN_ON ) {
                                m_udp_state = CONN_STARTING ;
                                start_udp_server();
                        } else if ( m_tcp_state != CONN_ON ) {
                                m_tcp_state = CONN_STARTING ;
                                start_tcp_server() ;
                        } else {
                                m_net_state = CONN_ON ;
                                transfer_connections_to_imule();
                        }
                        break;

                case NET_STOP :
                        theApp->SetNetworkStopped()  ;
                        kill_connections();
                        m_net_state = CONN_OFF ;
                        break;
                }
        }
        return ;
}


void I2PConnectionManager::start_i2p_router()
{
#ifdef INTERNAL_ROUTER

        // I2P internal router

        if ( thePrefs::GetI2PServerInternal() && theApp->i2prouter == NULL ) {
                AddLogLineC(_ ( "Starting I2P server" ) );

                theApp->i2prouter = new CI2PRouter ( thePrefs::GetI2PRouterProps() );

                AddLogLineC(_ ( "I2P server started. Connections should be available in a few minutes." ) );
        }
#endif
}

void I2PConnectionManager::start_ed2k_server_connection()
{
        // I2P - ed2k server //

        AddDebugLogLineN(logGeneral, _ ( "Starting fake connection to ed2k server" ) );

        if ( ! theApp->serverconnect ) {
                theApp->serverconnect = new CServerConnect ( theApp->serverlist,
                                Kademlia::CPrefs::getInstance()->GetKadID().ToHexString() + wxT ( "ED2K_Servers" ) );
        }
}

void I2PConnectionManager::stop_ed2k_server_connection()
{
        if ( theApp->serverconnect ) {
                delete theApp->serverconnect ;
                theApp->serverconnect = NULL;
        }
}

void I2PConnectionManager::start_tcp_server()
/**
 * 		Create the ListenSocket (imule TCP socket).
 *		Used for Client Port / Connections from other clients,
 *		Client to Client Source Exchange and Kad network
 */
{
        AddDebugLogLineN ( logGeneral, _ ( "Opening port for TCP communications (file transfers)" ) );

        wxI2PSocketBase::setI2POptions ( thePrefs::GetI2PTCPSocketProps(),
                                         thePrefs::GetI2PRouterProps() ) ;

        if (! theApp->listensocket) {
                theApp->listensocket = new CListenSocket (
                        thePrefs::GetTcpPrivKey() );
        }
        if (!theApp->listensocket->Error()) {
                theApp->listensocket->Notify(false);
                theApp->listensocket->SetEventHandler( *this, CONN_MANAGER_TCP_EVENT );
                theApp->listensocket->SetNotify( wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG );
                theApp->listensocket->SetClientData( theApp->listensocket );
                theApp->listensocket->Notify(true);
        } else {
                AddLogLineC(CFormat (_ ( "Failed to open TCP server. Retrying in %d seconds" ) ) % (wxInt32) (DELAY_BEFORE_RECONNECTION / 1000));
                wxString info = _ (
                        "This version does NOT include an internal I2P router.\n"
                        "In order to run iMule, you have to install and run the java I2P router.\n"
                        "You can fetch it here :\n"
                        "\n"
                        " http://www.i2p2.de/download\n"
                        "\n"
                        "PLEASE ENABLE the SAMBridge client in your router configuration tab\n"
                        "     (probably by browsing http://localhost:7657/configclients.jsp)\n"
                        "     (you may have to restart your I2P router after this change)\n" );

                AddLogLineC(CFormat (info));
        }
        m_timer.Start( DELAY_BEFORE_RECONNECTION, true  ) ;
}

void I2PConnectionManager::start_udp_server()
/**
 * Create the UDP socket.
 * Used for extended eMule protocol, Queue Rating, File Reask Ping.
 */
{
        AddDebugLogLineN ( logGeneral, _ ( "Opening port for UDP communications (kademlia distributed index)" ) );

        wxI2PSocketBase::setI2POptions ( thePrefs::GetI2PUDPSocketProps(),
                                         thePrefs::GetI2PRouterProps() ) ;

        if (! theApp->clientudp) {
                theApp->clientudp = new CClientUDPSocket (
                        thePrefs::GetUdpPrivKey() );
        }
        void * tmp = theApp->clientudp ;
        if (!tmp)
                return;
        theApp->clientudp->Close();
        theApp->clientudp->Notify ( false );
        theApp->clientudp->SetClientData( theApp->clientudp );
        theApp->clientudp->SetEventHandler ( *this, CONN_MANAGER_UDP_EVENT );
        theApp->clientudp->SetNotify ( wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG |
                                       wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG );
        theApp->clientudp->Notify ( true );
        theApp->clientudp->Open();
        m_timer.Start( DELAY_BEFORE_RECONNECTION, true  ) ;
}

void I2PConnectionManager::tcp_server_connected()
{
        m_timer.Stop();
        theApp->listensocket->Notify( false );
        CI2PAddress addr;
        theApp->listensocket->GetLocal ( addr );
        AddDebugLogLineN(logGeneral, CFormat ( _( "*** Server socket (TCP) listening on %s : %s\n" ) )
                     % addr.humanReadable()
                     % addr.toString() );
        if (!thePrefs::GetTcpPrivKey().StartsWith(addr.toString())) {
                thePrefs::SetTcpPrivKey(theApp->listensocket->GetPrivKey()) ;
                theApp->glob_prefs->Save();
        }

        m_tcp_state = CONN_ON ;
        state_machine();
}

void I2PConnectionManager::udp_server_connected()
{
        m_timer.Stop();
        theApp->clientudp->Notify( false );
        CI2PAddress addr;
        theApp->clientudp->GetLocal ( addr );
        AddDebugLogLineN(logGeneral, CFormat ( _( "*** Server socket (UDP) listening on %s : %s\n" ) )
                     % addr.humanReadable()
                     % addr.toString() );
        if (!thePrefs::GetUdpPrivKey().StartsWith(addr.toString())) {
                thePrefs::SetUdpPrivKey(theApp->clientudp->GetPrivKey()) ;
                theApp->glob_prefs->Save();
        }
	theApp->SetPublicDest(addr);

        m_udp_state = CONN_ON ;
        state_machine();
}



void I2PConnectionManager::transfer_connections_to_imule()
{
        AddLogLineC( _ ( "Connection to the underlying I2P network successful" ) );

        theApp->SetNetworkStarted() ;
        theApp->StartSearchNetwork();
        theApp->FetchIfNewVersionIsAvailable();
}





void I2PConnectionManager::kill_connections()
{
        m_timer.Stop();
        stop_udp_server();
        stop_tcp_server();
}

void I2PConnectionManager::stop_udp_server()
{
        if ( m_udp_state != CONN_OFF && theApp->clientudp ) {
                theApp->clientudp->Destroy() ;
                theApp->clientudp = NULL ;
                m_udp_state = CONN_OFF ;
        }
}

void I2PConnectionManager::stop_tcp_server()
{
        if ( m_tcp_state != CONN_OFF && theApp->listensocket ) {
                theApp->listensocket->Destroy();
                theApp->listensocket= NULL ;
                m_tcp_state = CONN_OFF ;
        }
}

void I2PConnectionManager::tcp_server_has_died( wxString msg )
{
        if ( m_tcp_state == CONN_ON ) {
                AddLogLineC(CFormat( _ ( "TCP server has lost its connection to I2P network (%s)" ) )
                              % msg );
        } else {
                AddLogLineN ( CFormat( _ ( "TCP server cannot connect to the I2P network (%s)" ) )
                              % msg );
        }
        m_target = NET_STOP;
        state_machine();
        m_target = NET_DELAYED_START;
        state_machine();
}

void I2PConnectionManager::udp_server_has_died( wxString msg )
{
        if ( m_udp_state == CONN_ON ) {
                AddLogLineC(CFormat( _ ( "UDP server has lost its connection to I2P network (%s)" ) )
                              % msg );
        } else {
                AddLogLineN ( CFormat( _ ( "UDP server cannot connect to the I2P network (%s)" ) )
                              % msg );
        }
        m_target = NET_STOP;
        state_machine();
        m_target = NET_DELAYED_START;
        state_machine();
}






/*


		AddDebugLogLineN(logGeneral, _ ( "listensocket created" ) );

		AddLogLineN ( CFormat ( _ ( "connection attempt %d failed" ) ) % tries );

		AddDebugLogLineN(logGeneral, CFormat ( _ ( "listensocket deleted" ) ) );

*/



// 		CI2PAddress lsaddr;
//
// 		listensocket->GetLocal ( lsaddr );
//
// 		*msg << CFormat ( wxT ( "*** TCP socket (TCP) listening on %x : %s\n" ) )
// 		% lsaddr.hashCode()
// 		% lsaddr.toString();


// This command just sets a flag to control maximun number of connections.
// Notify(true) has already been called to the ListenSocket, so events may
// be already comming in.

// 		if ( m_app_state == APP_STATE_SHUTTINGDOWN ) return NULL;
//
// 		if ( !listensocket->IsOk() )
// 		{
// 			// If we wern't able to start listening, we need to warn the user
// 			wxString err;
// 			err = CFormat ( _ ( "Address %s is not available. You will be LOWID\n" ) )
// 					% ( Kademlia::CPrefs::getInstance()->GetKadID().toHexString() + wxT ( "ListenSocket" ) ) ;
// 			*msg << err;
// 			AddLogLineC(err );
// 			err.Clear();
// 			err = CFormat ( _ ( "Dest %s is not available!\n\n"
// 			                    "This means that you will be LOWID.\n\n"
// 			                    "Check your network to make sure the port is open for output and input." ) ) %
// 					( Kademlia::CPrefs::getInstance()->GetKadID().toHexString() + wxT ( "ListenSocket" ) );
// 			ShowAlert ( err, _ ( "Error" ), wxOK | wxICON_ERROR );
// 		}

// Create the server for socket clients
//

// wxI2PSocketClient::createServer() ;

