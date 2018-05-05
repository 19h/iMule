#ifndef _I2PCONNECTIONMANAGER_H
#define _I2PCONNECTIONMANAGER_H

#include "amule.h"
#include <wx/timer.h>
#include "i2p/wxI2PEvents.h"

wxDECLARE_EVENT(CONN_MANAGER_UDP_EVENT, CI2PSocketEvent);
wxDECLARE_EVENT(CONN_MANAGER_TCP_EVENT, CI2PSocketEvent);
wxDECLARE_EVENT(CONN_MANAGER_TIMEOUT,   wxTimerEvent);

class I2PConnectionManager : public wxEvtHandler
{
        enum CONNState {
                CONN_OFF  ,
                CONN_STARTING,
                CONN_ON   ,
                CONN_LOST
        };

        enum NETtarget {
                NET_START,
                NET_DELAYED_START,
                NET_STOP,
        };

        CONNState m_tcp_state;

        CONNState m_udp_state;

        CONNState m_net_state;

        NETtarget m_target;


public:
        I2PConnectionManager();
        
        void OnListenSocketEvent( CI2PSocketEvent & event );

        void OnDatagramSocketEvent( CI2PSocketEvent & event );

        void start();

        void stop();

        bool IsOff() { return m_net_state == CONN_OFF; }

private:

        void state_machine();

        void start_i2p_router();

        void start_ed2k_server_connection();

        void stop_ed2k_server_connection();

        void start_udp_server();

        void start_tcp_server();

        void stop_udp_server();

        void stop_tcp_server();

        void kill_connections();

        void udp_server_connected();

        void tcp_server_connected();

        void udp_server_has_died( wxString msg );

        void tcp_server_has_died( wxString msg );

        void transfer_connections_to_imule();

        /* timer */

        void OnDelayTimeoutEvent(wxTimerEvent & evt);
        
        wxTimer m_timer;

//         DECLARE_EVENT_TABLE()
};


#endif
