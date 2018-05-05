//
// C++ Interface: wxI2PClient
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef _wxSAMSESSION_H
#define _wxSAMSESSION_H

#include <wx/wx.h>
#include <wx/socket.h>

#include "Types.h"

#include <map>
#include <deque>
#include <set>
#include <exception>
#include <memory>

#include "wxI2PEvents.h"
#include "SamEvents.h"
#include "CI2PAddress.h"
#include "CSamDefines.h"
#include "coroutine.h"
#include "myMutexLocker.h"

class wxI2PSocketBase     ;
class wxI2PSocketServer   ;
class wxI2PSocketClient   ;
class wxI2PDatagramSocket ;
class CDatagramMessage    ;

/* The maximum length a SAM non-data reply can be */
/* I think this is the answer to DEST GENERATE, which contains
    a public and a private key */
#define SAM_REPLY_LEN 2048



/**
 * SAM SESSION
 */

class CSam : public wxEvtHandler, private CSamLogger, public CSamLogged
{
        /**
         * Some LibSAM variable types
         */
        friend class wxI2PSocketBase     ;
        friend class wxI2PSocketClient   ;
        friend class wxI2PSocketServer   ;
        friend class wxI2PDatagramSocket ;

public:
        //! Enumeration of the connection steps for reaching the I2P router
        typedef sam_conn_state_t conn_state_t;

        void SetBlocking(bool); // wether the steps must block the gui
        bool IsBlocking() {return m_blocking;}
private:
        bool m_blocking ;

private:
        static CSamLogger * s_log ;
        virtual bool shouldLog ( Level ) {return true;}
        virtual void logMessage ( wxString m ) { std::cout << m.ToAscii() << std::endl; }
public:
        static void setLogger ( CSamLogger & logger ) { s_log = &logger ; }


        class exception : public std::exception
        {
        public:
                exception ( samerr_t, const char * );
                virtual const char* what() const throw();
                virtual samerr_t    code() const throw () {return m_code;}
        private:
                const char * m_message;
                samerr_t     m_code ;
        };


protected:
        typedef wxInt32  ssize_t;                                   //! signed size to return errors (-1)

        typedef wxUint32 usize_t;                                   //! unsigned size
private:
        typedef std::map<wxString, wxString> args_t ;               //! a map of (param,value) pairs for parsing router answers
protected:

        class buff_t                                                //! buffer that deletes its data on destruction
        {
        public:
                buff_t ( char * d, usize_t s ) {data = d; size = s;}

                ~buff_t() {delete [] data;}

                char * data;
                usize_t size;
                DECLARE_NO_COPY_CLASS ( buff_t );
        };



        class write_buff_t : public buff_t                       // write buffer
        {
        public:
                wiMutex * mutex ;
                wiCondition * condition ;
                bool queued  ;   // queued for being sent as soon as possible
                bool sending ;   // in the process of being sent...
                // waiting for answer from the server
                DECLARE_NO_COPY_CLASS ( write_buff_t );
        };



        /**
         * Exceptions
         */

        struct UnknownStreamId {};

        struct ErrorInReading {};

        struct ErrorInWriting {};

        struct SocketTransferred {};

        /**
         * Public functions
         */

public:
        //CSam();

        CSam ( const wxString & privKey = wxEmptyString,  wxString options = wxEmptyString ) ;
        
        CSam ( const wxString & privKey,  wxString options, wxI2PSocketClient * i2psocketclient) ;
        
        ~CSam();

        bool samOk() ;

        void SetSamSocket();

        static void setSamAddress ( std::map<wxString, wxString> );

        static uint16_t s_samTcpPort;

        static uint16_t s_samUdpPort;

        static wxString s_samIP;

        wxString        m_samIP;

        uint16_t        m_samPort;


        /** SAM controls - connection */
        bool        sam_close ();

        /* SAM controls - utilities */
        void        sam_naming_lookup ( wxString name );

        bool        sam_read_buffer ( );

        /**
        * LibSAM callbacks - functions in our code that are called by LibSAM when
        * something happens
        */

        void            OnSamSocketEvent ( wxSocketEvent& event );

        wxString        getSessionName();

protected:
        void            sam_connect();


private:
        /**
         * Private functions
         */
        void            sam_state_machine();

        void            sam_start_connect();

        void            sam_connected();

        void            sam_generate_keys() ;

        void            sam_dest_reply( wxString pub, wxString priv );

        void            sam_stream_session_create();

        void            sam_datagram_session_create();

        void            sam_forward_incoming_streams(wxString nick, int port);

        void            sam_accept_incoming_stream(wxString nick);

        void            sam_connect_clientsocket(wxString there);

        void            sam_hello();

        samerr_t        sam_dgram_send ( const CI2PAddress & dest, const void *data, usize_t size );

        void            sam_hello_reply ( const char * reply );

        samerr_t        sam_session_status ( wxString result, wxString message );



        void            sam_namingback ( wxString name, const CI2PAddress & pubkey,
                                         samerr_t result,
                                         wxString message ) ;
                                         
        void            sam_dgramback ( const CI2PAddress & dest, std::unique_ptr<char[]> data, usize_t size ) ;

        void            sam_connection_complete();

        bool            sam_readable();

        ssize_t         sam_read1 ( void * buf, usize_t n ) ;

        bool            sam_parse ( char * ) ;

        ssize_t         sam_read2 ( void * buf, usize_t n ) ;

        ssize_t         sam_discard ( usize_t n ) ;

        buff_t *        prepareMsg ( wxString cmd, const char * data = NULL, usize_t size = 0 );

        void            sam_queue ( buff_t * buffer );



        virtual void sam_diedback() ;

        virtual void sam_diedback ( samerr_t ) ;

        /**
         * posting events to client
         */

        void sam_stream_statusback ( samerr_t result, wxString message ) ;

        //! utilities


private:

        void            postOutputEvent();

        void            postInputEvent();

        /**
         * Some member variables
         */

        // the state of the connection to I2P router

        wiCondition      m_conn_state_condition ;
        wiMutex          m_conn_state_lock ;

        conn_state_t     State() {return m_conn_state;}
        void             SetState( conn_state_t s) {m_conn_state = s;}
        void             change_state( conn_state_t s , samerr_t err = SAM_OK , wxString message = wxEmptyString );

        // the error
        int              m_err_domain   ;
        samerr_t         m_err_code     ;
        wxString         m_err_message  ;
        void raise_error( samerr_t err, wxString message = wxEmptyString );

        // if sam is handling a listening streaming session
        bool             m_listening_socket_server ;
        // the state of the sam's client
        conn_state_t     m_conn_state ;
        // if sam's client is in the middle of something
        bool             m_busy        ;
        bool             GetLock()     ;
        void             WaitLock()    ;
        void             ReleaseLock() ;


        wxString   m_session_options ;

        bool             m_stopping ;

        wxIPV4address    m_defaultSamAddress; // the default address of the router

        wxSocketClient * m_socket ;           // the connection to the router

        bool             m_external_socket;   // if the socket is managed externally

        bool             m_inSending     ;

        wiMutex          m_inSendingLock ;

        bool             m_inReading     ;

        wiMutex          m_inReadingLock ;

        wxSocketEventFlags m_notifyFlags ;

protected:

        /**
         * keys
         */
        wxString         m_pubKey  ;         // public key
        wxString         m_privKey ;         // private key
        void             SetPubKey (wxString d);
        void             SetPrivKey(wxString d);
        wxString         GetPrivKey() { return wxString(m_privKey.c_str()) ; };
        wxString         GetPubKey()  { return wxString( m_pubKey.c_str()) ; };

        /**
         * data read, available for clients
         */
        wxString         privKeyAnswer ; // answers to some LOOKUP request
        wxString         pubKeyAnswer  ;

        struct {wxString name; wxString message; samerr_t result; CI2PAddress key; } m_lookupResult ;

        std::unique_ptr<CDatagramMessage> m_datagram ;    // datagram received

        /* read context : reads a message until '\n' */

        struct ReadCtx {
                cr_context ( ReadCtx );
                char message[SAM_REPLY_LEN];
                usize_t nleft;
                ssize_t nread;
                char *p;
                wiMutex mutex ;
        } ;

        ReadCtx readCtx ;

        /* read methods */
        void sam_handle_input();
        void sam_handle_input_reentrant();



        /* parse context : parses a message read in read context */

        struct ParseCtx {
                cr_context ( ParseCtx );
                wxString cmd1;
                wxString cmd2;
                wxString message;
                args_t   args ;
                std::unique_ptr<char[]> payload;
                usize_t   payloadSize;
        } ;

        ParseCtx  parseCtx ;

        /* read context : reads the payload of a message, following '\n' */

        struct PayloadCtx {
                cr_context ( PayloadCtx );
                usize_t nleft;
                ssize_t nread;
                char *p;
                wiMutex mutex ;
        } ;

        PayloadCtx payloadCtx ;

        wiMutex                     m_waitingWriteMsgQueueMutex ;
        std::deque<buff_t *>        m_waitingWriteMsgQueue ;


        struct WriteCtx {
                int __s;
                WriteCtx();
                ~WriteCtx();
                void deleteMsg();
                buff_t * msg   ;           // the complete message
                usize_t nleft;           // bytes to write
                char * p;                // a pointer to the data that remains to send
                wiMutex mutex ;
        } ;

        WriteCtx writeCtx ;

        void sam_handle_output();

        void sam_queue_msg ( char * msg );


        /**
         * handling session and local destinations
         */

        static std::map<wxString, CSam *> s_sessions ;
        static wiMutex                    s_sessions_lock ;
        // the ID of a CSam object in s_sessions
        wxString                          m_registrationID ;

        //std::set<wxI2PSocketBase *> m_clients_set ;
        wxI2PSocketBase * m_master_client ;
        uint32_t m_clients_count ;
        wiMutex s_clients_set_lock ;

        static CSam * AttachedSession(wxI2PSocketBase *, const wxString & dest=wxEmptyString, wxString options = wxEmptyString ) ;
        static CSam * NewSession     (wxI2PSocketBase *, const wxString & dest=wxEmptyString, wxString options = wxEmptyString ) ;
        void AttachClient(wxI2PSocketBase *);
        void DetachClient(wxI2PSocketBase *);
        void HandleDetach(wxI2PSocketBase *);
        void NotifyClients( wxSamEvent& event ) ;
        void ReleaseSamSocket();
};

#endif
