/*
 * Copyright (c) 2004, Matthew P. Cashdollar <mpc@innographx.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *     * Neither the name of the author nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define SAM_WIRETAP

#include <wx/wx.h>
#include <wx/socket.h>
#include <wx/tokenzr.h>

#include "Logger.h"		// for the macro AddLogLineM

#include "CSamSession.h"
#include "CSamDefines.h"
#include "wxI2PSocketBase.h"
#include "wxI2PEvents.h"
#include "wxI2PSocketClient.h"
#include "wxI2PDatagramSocket.h"

CSamLogger CSamLogger::none;

CSamLogger *CSam::s_log = &CSamLogger::none;
wiMutex CSam::s_sessions_lock;
std::map < wxString, CSam * >CSam::s_sessions;

#define SAM_HELLO_CMD	wxT("HELLO VERSION MIN=3.0 MAX=3.0\n")
#define SAM_HELLO_REPLY	 "HELLO REPLY RESULT=OK VERSION=3.0"

/**
 *               CSam::exception
 * @param m : the message returned by what()
 */


CSam::exception::exception (samerr_t c, const char *m)
{
        m_code = c;
        m_message = m;
}

const char *CSam::exception::what () const throw ()
{
        return m_message;
}


void CSam::OnSamSocketEvent (wxSocketEvent & event)
{

        switch (event.GetSocketEvent ()) {
        case wxSOCKET_CONNECTION:
                s_log->log (logINFO, wxT ("SAM socket connection event"));

                sam_connected ();
                break;

        case wxSOCKET_INPUT:
                s_log->log (logDEBUG,
                            wxT
                            ("SAM has detected incoming data from the I2P router"));
                try {
                        sam_handle_input_reentrant ();
                } catch (ErrorInReading e) {
                        raise_error (SAM_DISCONNECTED);
                }
                break;

        case wxSOCKET_OUTPUT:
                s_log->log (logDEBUG,
                            wxT ("SAM socket output buffer available"));

                try {
                        sam_handle_output ();
                } catch (ErrorInWriting e) {
                        s_log->log (logERROR,
                                    wxT
                                    ("SAM failed to write on its socket"));
                        raise_error (SAM_DISCONNECTED);
                }
                break;

        case wxSOCKET_LOST:
                s_log->log (logINFO,
                            wxT
                            ("SAM has lost its connection with the I2P router"));

                raise_error (SAM_DISCONNECTED);

                break;

        default:
                break;
        }
}



uint16_t CSam::s_samTcpPort = 7656;

uint16_t CSam::s_samUdpPort = 7655;

wxString CSam::s_samIP = wxT ("127.0.0.1");


void CSam::setSamAddress (std::map < wxString, wxString > opts)
{
        if (opts.count (wxT ("sam.host")) != 0) {
                s_samIP = opts[wxT ("sam.host")];
        }

        if (opts.count (wxT ("sam.tcp.port")) != 0) {
                unsigned long port = 7656;
                opts[wxT ("sam.tcp.port")].ToULong (&port);
                s_samTcpPort = (uint16_t) port;
        }
        if (opts.count (wxT ("sam.udp.port")) != 0) {
                unsigned long port = 7655;
                opts[wxT ("sam.udp.port")].ToULong (&port);
                s_samUdpPort = (uint16_t) port;
        }
}




/**
 * Dumps a buffer to a wxString
 */

wxString privateDumpMemToStr (const void *buff, int n, const wxString & msg,
                              bool ok = true)
{
        const unsigned char *p = (const unsigned char *) buff;

        int lines = (n + 15) / 16;


        wxString result;

        // Allocate aproximetly what is needed
        result.Alloc ((lines + 1) * 80);


        if (!msg.IsEmpty ()) {
                result +=
                        msg + wxT (" - ok=") +
                        (ok ? wxT ("true, ") : wxT ("false, "));
        }

        result += wxString::Format (wxT ("%d bytes\n"), n);

        for (int i = 0; i < lines; ++i) {
                // Show address
                result += wxString::Format (wxT ("%08x  "), i * 16);

                // Show two columns of hex-values

                for (int j = 0; j < 2; ++j) {
                        for (int k = 0; k < 8; ++k) {
                                int pos = 16 * i + 8 * j + k;

                                if (pos < n) {
                                        result +=
                                                wxString::Format (wxT
                                                                  ("%02x "),
                                                                  p[pos]);
                                } else {
                                        result += wxT ("   ");
                                }
                        }

                        result += wxT (" ");
                }

                result += wxT ("|");

                // Show a column of ascii-values

                for (int k = 0; k < 16; ++k) {
                        int pos = 16 * i + k;

                        if (pos < n) {
                                if (isspace (p[pos])) {
                                        result += wxT (" ");
                                } else if (!isgraph (p[pos])) {
                                        result += wxT (".");
                                } else {
                                        result += (wxChar) p[pos];
                                }
                        } else {
                                result += wxT (" ");
                        }
                }

                result += wxT ("|\n");
        }

        result.Shrink ();

        return result;
}


/**
 *
 *
 *
 *
 *
 *
 *
 *                                         Constructors / destructor
 *
 *
 *
 */


/**
 * Constructor for servers (wxI2PDatagramSocket or wxI2PSocketServer)
 * @param sessionName : local name that will be associated with the local I2P destination in router's files
 * @param type : SAM_STREAM, SAM_DGRAM or SAM_RAW:
 * @param options : param=value' list passed to I2P router
 */

CSam::CSam (const wxString & privKey, wxString options):
        m_conn_state_condition (m_conn_state_lock),
        m_conn_state_lock()
{
        m_registrationID = privKey;
        m_privKey = privKey;
        m_conn_state = OFF;
        m_listening_socket_server = false;
        m_external_socket = false;
        m_err_code = SAM_OK;
        m_stopping = false;
        m_busy = false;
        m_master_client = NULL;
        m_clients_count = 0;
        m_samIP = wxString (s_samIP.c_str ());
        m_samPort = s_samTcpPort;



        m_inReading = false;

        m_inSending = false;

        m_session_options = options;

        m_socket = NULL;

        m_socket = new wxSocketClient();
        m_socket->SetEventHandler (*this);
        SetSamSocket();
}

CSam::CSam ( const wxString& privKey, wxString options, wxI2PSocketClient* i2psocketclient ) :
m_conn_state_condition (m_conn_state_lock),
m_conn_state_lock()
{
        m_registrationID = privKey;
        m_privKey = privKey;
        m_conn_state = OFF;
        m_listening_socket_server = false;
        m_err_code = SAM_OK;
        m_stopping = false;
        m_busy = false;
        m_clients_count = 0;
        m_samIP = wxString (s_samIP.c_str ());
        m_samPort = s_samTcpPort;
        
        m_inReading = false;
        m_inSending = false;
        m_session_options = options;
        
        AttachClient(i2psocketclient);
        m_socket = i2psocketclient->m_socket;
        m_external_socket = true;
        SetBlocking(false);
        SetSamSocket();
}


void CSam::SetSamSocket()
{
        SetBlocking (false);
        m_socket->SetNotify (wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG |
                             wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
        m_socket->Notify (true);
        Bind(wxEVT_SOCKET, &CSam::OnSamSocketEvent, this);
}

void CSam::ReleaseSamSocket ()
{
        m_socket = NULL;
        m_external_socket = false;
}

void CSam::SetBlocking (bool block)
{
        if (m_socket) {
                if (block) {
                        m_socket->SetFlags (wxSOCKET_WAITALL |
                                            wxSOCKET_BLOCK);
                } else {
                        m_socket->SetFlags (wxSOCKET_NOWAIT);
                }
        }
        m_blocking = block;
}


CSam::~CSam ()
{
        sam_close ();
        s_log->log (logDEBUG,
                    wxString::Format (wxT ("SAM %p deleted"), this));
}

bool CSam::samOk ()
{
        return (m_socket ? m_socket->IsConnected () : false);
}

/**
 * Closes the connection to the SAM host
 * @return : true on success, false on failure
 */
bool CSam::sam_close ()
{
        m_stopping = true;
        m_conn_state = OFF;

        if (m_socket and not m_external_socket) {
                m_socket->Notify (false);
                m_socket->Destroy ();
                s_log->log (logINFO, wxT ("Main session connection closed"));
                m_socket = NULL;
        } else {
                s_log->log (logINFO, wxT ("Client connection closed by SAM"));
        }
        return true;
}




/**
 * Start the (long) connection process to I2P router
 */
void CSam::sam_start_connect ()
{
        wxIPV4address sam_addr;
        sam_addr.Hostname (s_samIP);
        sam_addr.Service (s_samTcpPort);

        m_socket->SetTimeout (10000);

        if (m_socket->Connect (sam_addr, m_blocking)) {
                sam_connected ();
        } else if (IsBlocking ()) {
                raise_error (SAM_SOCKET_ERROR,
                             wxT ("Sam server is unreachable"));
                return;
        }
}

/**
 * Sets SAM state to "connected"
 */
void CSam::sam_connected ()
{
        if (m_socket->IsConnected ())
                change_state (SERVER_CONNECTED);
        else
                raise_error (SAM_SOCKET_ERROR,
                             wxT
                             ("Connexion to server received, but socket is not connected"));
}


void CSam::sam_hello ()
{
        sam_queue (prepareMsg (SAM_HELLO_CMD));
}

// state changes when a HELLO reply is received
void CSam::sam_hello_reply (const char *reply)
{
        if (m_conn_state != SERVER_CONNECTED) {
                throw new exception (SAM_SOCKET_ERROR,
                                     "Sam has received HELLO but did not ask for it");
        } else if (strncmp (reply, SAM_HELLO_REPLY, strlen (SAM_HELLO_REPLY))
                        != 0) {
                throw new exception (SAM_BAD_VERSION,
                                     "Bad SAM version in your I2P router");
        } else {
                change_state (SAM_HELLOED);
        }
}

wxString CSam::getSessionName ()
{
        return wxT ("iMule_") + m_privKey.Left (10);
}



void CSam::sam_datagram_session_create ()
{
        wxString cmd =
                wxString (wxT ("SESSION CREATE STYLE=DATAGRAM DESTINATION="))
                << m_privKey << wxT (" ID=")
                << getSessionName ()
                // incompatibility with i2p 9.4 : << wxT (" i2cp.messageReliability=BestEffort ")
                << wxT (" i2cp.messageReliability=None ")
                << m_session_options << wxT ("\n");

        sam_queue (prepareMsg (cmd));
}


void CSam::sam_stream_session_create ()
{
        wxString cmd =
                wxString (wxT ("SESSION CREATE STYLE=STREAM DESTINATION="))
                << m_privKey << wxT (" ID=")
                << getSessionName ()
                // incompatibility with i2p 9.4 : << wxT (" i2cp.messageReliability=BestEffort ")
                << wxT (" i2cp.messageReliability=None ")
                << m_session_options << wxT ("\n");

        sam_queue (prepareMsg (cmd));
}


void CSam::sam_forward_incoming_streams (wxString nick, int port)
{
        wxString cmd = wxString (wxT ("STREAM FORWARD ID="))
                       << nick << wxT (" PORT=")
                       << port << wxT ("\n");

        sam_queue (prepareMsg (cmd));
}

void CSam::sam_accept_incoming_stream (wxString nick)
{
        wxString cmd = wxString (wxT ("STREAM ACCEPT ID="))
                       << nick << wxT ("\n");

        sam_queue (prepareMsg (cmd));
}



void CSam::sam_connect_clientsocket (wxString there)
{
        wxString cmd = wxString (wxT ("STREAM CONNECT ID="))
                       << getSessionName ()
                       << wxT (" DESTINATION=")
                       << there << wxT ("\n");

        sam_queue (prepareMsg (cmd));
}



samerr_t CSam::sam_session_status (wxString result, wxString message)
{
#define SAM_SESSTATUS_REPLY_OK "SESSION STATUS RESULT=OK"
#define SAM_SESSTATUS_REPLY_DD "SESSION STATUS RESULT=DUPLICATED_DEST"
#define SAM_SESSTATUS_REPLY_I2E "SESSION STATUS RESULT=I2P_ERROR"
#define SAM_SESSTATUS_REPLY_IK "SESSION STATUS RESULT=INVALID_KEY"

        if (result == wxT ("OK")) {
                change_state (SESSION_CREATED);
        } else if (result == wxT ("DUPLICATED_DEST")) {
                raise_error (SAM_DUPLICATED_DEST, message);
        } else if (result == wxT ("I2P_ERROR")) {
                raise_error (SAM_I2P_ERROR, message);
        } else if (result == wxT ("INVALID_KEY")) {
                raise_error (SAM_INVALID_KEY, message);
        } else if (result == wxT ("DUPLICATED_ID")) {
                raise_error (SAM_DUPLICATED_ID, message);
        } else {
                raise_error (SAM_UNKNOWN,
                             wxT
                             ("SESSION STATUS answer received with unknown status"));
        }
        return SAM_OK;
}


/**
 * Performs a base 64 private and public key generation
 */

void CSam::sam_generate_keys ()
{
        wxString cmd = wxString (wxT ("DEST GENERATE\n"));
        buff_t *msg = prepareMsg (cmd);
        sam_queue (msg);
}

void CSam::sam_dest_reply (wxString pub, wxString priv)
{
        privKeyAnswer = priv;
        pubKeyAnswer = pub;
        change_state (KEYS_GENERATED);
}


/**
 * Performs a base 64 public key lookup on the specified `name'
 *
 * name - name to lookup, or ME to lookup our own name
 */
void CSam::sam_naming_lookup (wxString name)
{

        wxString cmd =
                wxString (wxT ("NAMING LOOKUP NAME=")) << name << wxT ("\n");

        buff_t *msg = prepareMsg (cmd);

        sam_queue (msg);
}

void CSam::sam_namingback (wxString name, const CI2PAddress & pubkey,
                           samerr_t result, wxString message)
{
        m_lookupResult.result = result;
        m_lookupResult.name = name;
        m_lookupResult.message = message;
        m_lookupResult.key = pubkey;
        change_state (NAMING_LOOKEDUP);
}

/**
 * Sends data to a destination in a datagram
 *
 * dest - base 64 destination of who we're sending to
 * data - the data we're sending
 * size - the size of the data
 *
 * Returns: SAM_OK on success
 */

samerr_t CSam::sam_dgram_send (const CI2PAddress & dest,
                               const void *data, usize_t size)
{
        wxString cmd;

        if (size < 1 || size > SAM_DGRAM_PAYLOAD_MAX) {
                s_log->log (logCRITICAL,
                            wxString (wxT ("Invalid data send size (")) <<
                            size << wxT (" bytes)"));
                return SAM_TOO_BIG;
        }

        cmd = wxString (wxT ("DATAGRAM SEND DESTINATION=")) <<
              dest.toString ()

              << wxT (" SIZE=") << size << wxT ("\n");

        try {
                buff_t *msg = prepareMsg (cmd, (char *) data, size);
                sam_queue (msg);
        } catch (ErrorInWriting e) {
                return SAM_SOCKET_ERROR;
        }

        return SAM_OK;
}

/**
 * receives a datagram from the sam socket
 * @param dest public key of the datagram sender
 * @param data payload
 * @param size size of the payload
 */
void CSam::sam_dgramback (const CI2PAddress & dest, std::unique_ptr<char[]> data, usize_t size)
{
        m_datagram.reset(new CDatagramMessage (dest, data, size));
        s_log->log (logDEBUG,
                    wxString::Format (wxT ("Datagram received, size = %" PRIu32 ""),
                                      size));
        change_state (DATAGRAM_INCOMING);
}

/**
 * Parses incoming data and calls the appropriate callback
 *
 * s - string of data that we read (read past tense)
 * returns : true if everything was parse, false if reintrance is necessary (waiting for message data)
 *
 */

bool CSam::sam_parse (char *msg)
{
        bool ok;
        wxString arg;

        cr_start (parseCtx);

        parseCtx.cmd1 = wxT ("");
        parseCtx.cmd2 = wxT ("");
        parseCtx.args.clear ();


        // the real parsing
        {
                wxString message = wxString::FromAscii (msg);

                wxStringTokenizer tkz (message, wxT (" \t\n"),
                                       wxTOKEN_STRTOK);

                if (tkz.HasMoreTokens ())
                        parseCtx.cmd1 = tkz.GetNextToken ();

                if (tkz.HasMoreTokens ())
                        parseCtx.cmd2 = tkz.GetNextToken ();

                while (tkz.HasMoreTokens ()) {
                        wxStringTokenizer argTkz (tkz.GetNextToken (),
                                                  wxT ("="), wxTOKEN_STRTOK);
                        wxString argName = argTkz.GetNextToken ();

                        if (argName == wxT ("MESSAGE")) {
                                parseCtx.args[argName] =
                                        argTkz.GetString () + wxT (" ") +
                                        tkz.GetString ();
                                break;
                        }

                        parseCtx.args[argName] = argTkz.GetString ();
                }
        }

        // empty message
        if (parseCtx.cmd1.IsEmpty ()) {
                cr_reinit (parseCtx);
                return true;
        }
        // payload : read binary data that can follow the text message
        if (parseCtx.args.count (wxT ("SIZE")) == 1) {
                unsigned long _pl;
                ok = parseCtx.args[wxT ("SIZE")].ToULong (&_pl);
                if (!ok) {
                        wxASSERT_MSG (ok, wxT ("No number found at all!"));
                        abort ();
                }
                parseCtx.payloadSize = _pl;
                parseCtx.payload.reset(new char[parseCtx.payloadSize]);

                if (parseCtx.payload == NULL) {
                        s_log->log (logCRITICAL, wxT ("Out of memory"));
                        abort ();
                }

                while (sam_read2 (parseCtx.payload.get(), parseCtx.payloadSize) !=
                                (ssize_t) parseCtx.payloadSize)
                        cr_return (false, parseCtx);
        }
        cr_end (parseCtx);

        cr_reinit (parseCtx);


        // the message has been read. Let's take appropriate actions

        wxString message = parseCtx.args[wxT ("MESSAGE")];

        if (parseCtx.cmd1 == wxT ("HELLO")) {
                sam_hello_reply (msg);
        } else if (parseCtx.cmd1 == wxT ("SESSION")
                        && parseCtx.cmd2 == wxT ("STATUS")) {
                arg = parseCtx.args[wxT ("RESULT")];
                sam_session_status (arg, message);
        }

        else if (parseCtx.cmd1 == wxT ("NAMING")
                        && parseCtx.cmd2 == wxT ("REPLY")) {
                arg = parseCtx.args[wxT ("RESULT")];
                if (arg.IsEmpty ()) {
                        s_log->log (logCRITICAL,
                                    wxT ("Naming reply with no result"));
                } else {

                        wxString name = parseCtx.args[wxT ("NAME")];

                        wxASSERT (!name.IsEmpty ());

                        if (arg == wxT ("OK")) {

                                CI2PAddress pubkey (parseCtx.args[wxT("VALUE")]);

                                wxASSERT (pubkey.isValid ());

                                sam_namingback (name, pubkey, SAM_OK,
                                                message);
                        } else if (arg == wxT ("INVALID_KEY")) {
                                sam_namingback (name, CI2PAddress::null,
                                                SAM_INVALID_KEY, message);
                        } else if (arg == wxT ("KEY_NOT_FOUND")) {
                                sam_namingback (name, CI2PAddress::null,
                                                SAM_KEY_NOT_FOUND, message);
                        } else {
                                sam_namingback (name, CI2PAddress::null,
                                                SAM_UNKNOWN, message);
                        }
                }
        } else if (parseCtx.cmd1 == wxT ("STREAM")) {
                AddDebugLogLineN(logSamSession,
                                  wxString::FromAscii (msg));

                if (parseCtx.cmd2 == wxT ("STATUS")) {
                        arg = parseCtx.args[wxT ("RESULT")];
                        wxASSERT (!arg.IsEmpty ());

                        if (arg == wxT ("OK")) {
                                change_state (STREAM_OK, SAM_OK, message);
                        } else {
                                if (arg == wxT ("CANT_REACH_PEER")) {
                                        raise_error (SAM_CANT_REACH_PEER, message);
                                } else if (arg == wxT ("I2P_ERROR")) {
                                        raise_error (SAM_I2P_ERROR, message);
                                } else if (arg == wxT ("INVALID_KEY")) {
                                        raise_error (SAM_INVALID_KEY, message);
                                } else if (arg == wxT ("TIMEOUT")) {
                                        raise_error (SAM_TIMEOUT, message);
                                } else {
                                        raise_error (SAM_UNKNOWN, message);
                                }
                                return true;
                        }
                }
        } else if (parseCtx.cmd1 == wxT ("DATAGRAM")
                        && parseCtx.cmd2 == wxT ("RECEIVED")) {
                CI2PAddress dest;

                arg = parseCtx.args[wxT ("DESTINATION")];
                wxASSERT (!arg.IsEmpty ());
                dest = CI2PAddress::fromString (arg);

                sam_dgramback (dest, move(parseCtx.payload), parseCtx.payloadSize);

                //      ^^^^^^^ data has to be freed ^^^^^^^^^^^
        } else if (parseCtx.cmd1 == wxT ("DEST")) {
                AddDebugLogLineN(logSamSession,
                                  wxString::FromAscii (msg));

                if (parseCtx.cmd2 == wxT ("REPLY")) {
                        wxString arg2;
                        arg = parseCtx.args[wxT ("PUB")];
                        wxASSERT (!arg.IsEmpty ());

                        arg2 = parseCtx.args[wxT ("PRIV")];
                        wxASSERT (!arg.IsEmpty ());

                        sam_dest_reply (arg, arg2);
                } else {
                        raise_error (SAM_UNKNOWN,
                                     wxT
                                     ("DEST answer received without keys"));
                        return false;
                }
        }

        else {
                raise_error (SAM_PARSE_FAILED,
                             wxT ("Failed parsing input from SAM"));
                return false;
        }

        return true;
}

void CSam::change_state (conn_state_t state, samerr_t err, wxString message)
{
        m_conn_state = state;
        m_err_code = err;
        m_err_message = message;

        if (!IsBlocking ()) {
                wxSamEvent e (state);
                e.SetEventObject (this);
                e.SetStatus (err);
                e.SetMessage (message);
                NotifyClients (e);
        }
}

void CSam::raise_error (samerr_t err, wxString message)
{
        if (message.IsEmpty ())
                message = i2pstrerror (err);

        sam_diedback (err);
        change_state (OFF, err, message);
        s_log->log (logERROR,
                    wxString (i2pstrerror (err)) + wxT (" message: ") +
                    message);
}




/**
 * SAM command reader
 *
 * Reads up to `n' bytes of a SAM response into the buffer `buf' OR (preferred)
 * up to the first '\n' which is replaced with a NUL.  The resulting buffer will
 * always be NUL-terminated.
 * Warning: A full buffer will cause the last character read to be lost (it is
 * 		replaced with a NUL in the buffer).
 *
 * buf - buffer
 * n - number of bytes available in `buf', the buffer
 *
 * Returns: number of bytes read, or -1 on error
 */
CSam::ssize_t CSam::sam_read1 (void *buf, usize_t n)
{
        usize_t nleft;
        ssize_t nread;
        char *p;

        memset (buf, (char) 0, n);	/* this forces `buf' to be a string even if there is a sam_read1 error */

        if (m_socket->IsDisconnected ()) {
                s_log->log (logCRITICAL,
                            wxT
                            ("Cannot read from SAM because the SAM connection is closed"));

                sam_diedback ();

                return -1;
        }

        wxASSERT (n > 0);

        p = (char *) buf;
        nleft = n;

        while (nleft > 0) {
                m_socket->Read (p, 1);
                nread = m_socket->LastCount ();

                if (m_socket->Error ()
                                && m_socket->LastError () != wxSOCKET_WOULDBLOCK) {
                        s_log->log (logCRITICAL,
                                    wxString (wxT
                                              ("Read(p,1) failed. wxSOCKET error : "))
                                    << wxstrerror (m_socket->LastError ()));

                        throw ErrorInReading ();
                } else if (nread == 0) {
                        /* EOF */
                        s_log->log (logINFO,
                                    wxT
                                    ("Connection closed by the SAM host"));

                        throw ErrorInReading ();
                }

                wxASSERT (nread == 1);

                nleft--;

                if (*p == '\n') {
                        /* end of SAM response */
                        *p = '\0';
#ifdef SAM_WIRETAP
                        s_log->log (logDEBUG,
                                    wxString () << wxT ("*RR* (read1)")
                                    << wxString::FromAscii ((const char *)
                                                            buf) <<
                                    wxT ("\n"));

#endif
                        return n - nleft;
                }

                p++;
        }

        /* buffer full, which is considered an error */
        s_log->log (logCRITICAL, wxT ("sam_read1() buf full"));

        p--;

        *p = '\0';		/* yes, this causes the loss of the last character */

        return n - nleft;	/* return >= 0 */
}



/**
 * SAM payload reader
 *
 * Reads exactly `n' bytes of a SAM data payload into the buffer `buf'
 *
 * buf - buffer
 * n - number of bytes available in `buf'
 *
 * Returns: number of bytes read, or -1 on error
 *
 * @throw: ErrorInReading
 */
CSam::ssize_t CSam::sam_read2 (void *buf, usize_t n)
{
        cr_start (payloadCtx);

        payloadCtx.nleft = n;
        payloadCtx.nread = 0;
        payloadCtx.p = (char *) buf;


        wxASSERT (n > 0);

        while (payloadCtx.nleft > 0) {
                m_socket->Read (payloadCtx.p, payloadCtx.nleft);
                {
                        ssize_t thisnread = m_socket->LastCount ();

                        if (m_socket->Error ()
                                        && m_socket->LastError () != wxSOCKET_WOULDBLOCK) {
                                {
                                        cr_reinit (payloadCtx);
                                        s_log->log (logCRITICAL,
                                                    wxString (wxT
                                                              ("Read() failed. wxSOCKET_ error : "))
                                                    <<
                                                    wxstrerror
                                                    (m_socket->LastError ()));

                                        if (!m_stopping)
                                                sam_diedback ();

                                        throw ErrorInReading ();
                                }
                        }
#ifdef SAM_WIRETAP
                        s_log->log (logDEBUG,
                                    privateDumpMemToStr (payloadCtx.p,
                                                         thisnread,
                                                         wxString (wxT
                                                                         ("*RR* (read2() read "))
                                                         << thisnread <<
                                                         wxT (" bytes)\n")));

#endif
                        payloadCtx.nread += thisnread;

                        payloadCtx.nleft -= thisnread;

                        payloadCtx.p += thisnread;

                        if (payloadCtx.nleft == 0) {
                                cr_reinit (payloadCtx);
                                return payloadCtx.nread;
                        }
                }

                cr_return (payloadCtx.nread, payloadCtx);
        }

        cr_end (payloadCtx);
        return n;
}




/**
 * SAM reader
 *
 * Reads up to `n' bytes of a SAM response into the buffer `buf' OR (preferred)
 * up to the first '\n' which is replaced with a NUL.  The resulting buffer will
 * always be NUL-terminated.
 * Warning: A full buffer will cause the last character read to be lost (it is
 * 		replaced with a NUL in the buffer).
 *
 * buf - buffer
 * n - number of bytes available in `buf', the buffer
 *
 * Returns: nothing
 */

void CSam::sam_handle_input ()
{
        try {
                if (m_err_code != SAM_OK)
                        return;

                char reply[SAM_REPLY_LEN];

                sam_read1 (reply, SAM_REPLY_LEN);
                sam_parse (reply);

        } catch (CSam::SocketTransferred) {
                ReleaseSamSocket ();
                DetachClient (m_master_client);
        } catch (CSam::exception e) {
                raise_error (e.code (), wxString::FromAscii (e.what ()));
        }
}


void CSam::sam_handle_input_reentrant ()
{
        try {
                if (m_err_code != SAM_OK)
                        return;

                cr_start (readCtx);

                memset (readCtx.message, (char) 0, SAM_REPLY_LEN);
                // this forces `message' to be a string even if there is a sam_read error */

                readCtx.p = readCtx.message;

                readCtx.nleft = SAM_REPLY_LEN;


                while (readCtx.nleft > 0 && m_socket && m_conn_state != OFF) {

                        m_socket->Read (readCtx.p, readCtx.nleft);
                        readCtx.nread = m_socket->LastCount ();

                        if (m_socket->Error ()
                                        && m_socket->LastError () != wxSOCKET_WOULDBLOCK) {
                                s_log->log (logCRITICAL,
                                            wxString (wxT
                                            ("sam_handle_input_reentrant : Read(p,1) failed. wxSOCKET error : "))
                                            <<
                                            wxstrerror (m_socket->LastError
                                                        ()));

                                if (!m_stopping)
                                        sam_diedback ();

                                cr_reinit (readCtx);

                                throw ErrorInReading ();
                        }
                        // look for to '\n'

                        while (readCtx.nread > 0 && *readCtx.p != '\n') {
                                readCtx.p++;
                                readCtx.nread--;
                                readCtx.nleft--;
                        }

                        if (*readCtx.p != '\n') {
#ifdef SAM_WIRETAP
                                s_log->log (logDEBUG,
                                            wxString (wxT
                                                      ("sam_handle_input_reentrant : "))
                                            << readCtx.nread <<
                                            wxT (" remaining in buffer, "));
                                s_log->log (logDEBUG,
                                            privateDumpMemToStr
                                            (readCtx.message,
                                             readCtx.p - readCtx.message,
                                             wxT
                                             ("*RR* (sam_handle_input_reentrant)")));

#endif
                                cr_return (, readCtx);
                        } else {
                                /* end of SAM response */
                                // this forces `message' to be a string even if there is a sam_read error */
                                *readCtx.p = '\0';
                                readCtx.p++;
                                readCtx.nread--;
                                m_socket->Unread (readCtx.p, readCtx.nread);

#ifdef SAM_WIRETAP
                                s_log->log (logDEBUG,
                                            wxString (wxT
                                                      ("sam_handle_input_reentrant : "))
                                            << readCtx.nread <<
                                            wxT (" remaining in buffer, "));
                                s_log->log (logDEBUG,
                                            wxString () <<
                                            wxT
                                            ("*RR* (sam_handle_input_reentrant)")
                                            <<
                                            wxString::FromAscii ((const char
                                                                  *)
                                                                 readCtx.
                                                                 message)
                                            << wxT ("\n"));
#endif
                                cr_redo (readCtx);

                                if (sam_parse (readCtx.message)) {
                                        cr_reinit (readCtx);
                                        memset (readCtx.message, (char) 0,
                                                SAM_REPLY_LEN);
                                        // this forces `message' to be a string even if there is a sam_read error */
                                        readCtx.p = readCtx.message;
                                        readCtx.nleft = SAM_REPLY_LEN;
                                } else
                                        return;

                        }
                }

                /* buffer full, which is considered an error */
                if (readCtx.nleft == 0)
                        s_log->log (logCRITICAL,
                                    wxT ("sam_handle_input_reentrant() buf full"));

                cr_end (readCtx);
        } catch (CSam::SocketTransferred) {
                ReleaseSamSocket ();
                DetachClient (m_master_client);
        } catch (CSam::exception e) {
                raise_error (e.code (), wxString::FromAscii (e.what ()));
        }
}









/**
 * Prepares a message and adds it to the sending queue
 *
 * @param cmd - the first part of the message (command ended by '\n')
 * @param data - the optional data part of the message
 * @param bytes - the size of data
 *
 * @return nothing
 */

/**
 * Prepares a message and adds it to the sending queue
 *
 * @param cmd - the first part of the message (command ended by '\n')
 * @param data - the optional data part of the message
 * @param bytes - the size of data
 *
 * @return nothing
 */

CSam::buff_t * CSam::prepareMsg (wxString cmd, const char *data, usize_t size)
{
        buff_t *msg = new buff_t (new char[strlen (cmd.ToAscii ()) + size],
                                  strlen (cmd.ToAscii ()) + size);

        memcpy (msg->data, cmd.ToAscii (), strlen (cmd.ToAscii ()));

        if (size > 0)
                memcpy (msg->data + strlen (cmd.ToAscii ()), data, size);

        return msg;
}

void CSam::sam_queue (CSam::buff_t * msg)
{
        {
                ASSERT_LOCK (writerLocker, m_waitingWriteMsgQueueMutex,
                             __func__);
                m_waitingWriteMsgQueue.push_back (msg);
        }
        postOutputEvent ();
}




/**
 * ************************************
 *
 *               sam_handle_output
 *
 *
 * ************************************
 */


void CSam::sam_handle_output ()
{
        if (m_err_code != SAM_OK)
                return;
        ASSERT_LOCK (lock, writeCtx.mutex, __func__);
        // test the sam session has not just been deleted

        if (m_stopping)
                return;

        cr_start (writeCtx);

        while (true) {
                {
                        ASSERT_LOCK (lockMsgQueue,
                                     m_waitingWriteMsgQueueMutex, __func__);
                        if (!m_waitingWriteMsgQueue.empty ()) {
                                writeCtx.msg =
                                        m_waitingWriteMsgQueue.front ();
                                m_waitingWriteMsgQueue.pop_front ();
                        }
                        if (writeCtx.msg == NULL)
                                return;
                }

                writeCtx.p = writeCtx.msg->data;

                writeCtx.nleft = writeCtx.msg->size;

                while (writeCtx.nleft > 0) {
                        s_log->log (logDEBUG,
                                    wxString (wxT
                                              ("sam_handle_output : bytes to write : "))
                                    << writeCtx.nleft);
                        m_socket->Write (writeCtx.p, writeCtx.nleft);

                        if (m_socket->Error ()
                                        && m_socket->LastError () != wxSOCKET_WOULDBLOCK) {
                                try {
                                        cr_reinit (writeCtx);
                                        //writeCtx.deleteMsg() ;
                                        s_log->log (logCRITICAL,
                                                    wxString (wxT
                                                              ("Write(writeCtx.p, "))
                                                    << writeCtx.nleft <<
                                                    wxT
                                                    (") failed. wxSOCKET_ error : ")
                                                    <<
                                                    wxstrerror
                                                    (m_socket->LastError ()));
                                } catch ( ...) {
                                }

                                throw ErrorInWriting ();
                        }

                        if (m_socket->Error ())
                                s_log->log (logDEBUG,
                                            wxString (wxT
                                                      ("wxSOCKET_WOULDBLOCK : bytes sent = "))
                                            << m_socket->LastCount ());

#ifdef SAM_WIRETAP
                        s_log->log (logDEBUG,
                                    privateDumpMemToStr (writeCtx.p,
                                                         m_socket->LastCount
                                                         (), wxT ("*WW*")));
#endif
                        writeCtx.p += m_socket->LastCount ();

                        writeCtx.nleft -= m_socket->LastCount ();

                        if (writeCtx.nleft > 0) {
                                s_log->log (logDEBUG,
                                            wxString (wxT
                                                      ("sam_handle_output : bytes remaining : "))
                                            << writeCtx.nleft <<
                                            wxString::Format (wxT
                                                              (" socket flags = %x"),
                                                              m_socket->GetFlags
                                                              ())
                                           );
                        } else
                                s_log->log (logINFO,
                                            wxString (wxT
                                                      ("All bytes sent : "))
                                            << m_socket->LastCount ());

                        if (m_socket->Error ()) {
                                s_log->log (logERROR,
                                            wxT ("sam_handle_ouput exit"));
                                cr_return (, writeCtx);
                        }
                }

                writeCtx.deleteMsg ();

                cr_reinit (writeCtx);
        }

        cr_end (writeCtx);
}


/**
 * write context methods
 */

CSam::WriteCtx::WriteCtx ()
{
        msg = NULL;
        __s = 0;
}

CSam::WriteCtx::~WriteCtx ()
{
        ASSERT_LOCK (lock, mutex, __func__);
        deleteMsg ();
}

void CSam::WriteCtx::deleteMsg ()
{
        if (msg != NULL)
                delete msg;

        msg = NULL;
}







/**
 *
 *
 * Callbacks
 *
 *    most of them should not be called, but are called when the CSamBack is the base
 *    class of a wxI2PSocketXXX object whose constructor has not started yet (because its
 *    constructor first need its parent class' constructor to be processed)
 *
 */

void CSam::sam_connection_complete ()
{
        change_state (ON);
        s_log->log (logINFO,
                    wxString (wxT ("SAM connection open with router : "))
                    << m_defaultSamAddress.Hostname () << wxT (":")
                    << m_defaultSamAddress.Service ());
}

void CSam::sam_diedback (samerr_t WXUNUSED (err))
{
        if (m_conn_state != OFF)
                sam_close ();

        m_conn_state = OFF;
}


void CSam::sam_diedback ()
{
        s_log->log (logCRITICAL, wxT ("SAM session just died"));
}



/**
 *
 * Event posting to trigger write and read state machines
 *
 */

void CSam::postOutputEvent ()
{
        wxSocketEvent event;
        event.m_event = wxSOCKET_OUTPUT;
        event.m_clientData = (void *) "perso";

        if (!IsBlocking ())
                AddPendingEvent (event);
        else
                sam_handle_output ();
}

void CSam::postInputEvent ()
{
        wxSocketEvent event;
        event.m_event = wxSOCKET_INPUT;
        event.m_clientData = (void *) "perso";

        if (!IsBlocking ())
                AddPendingEvent (event);
        else
                sam_handle_input ();
}

bool CSam::GetLock ()
{
        wiMutexLocker lock (m_conn_state_lock);
        if (m_busy)
                return false;
        m_busy = true;
        return true;
}

void CSam::ReleaseLock ()
{
        wiMutexLocker lock (m_conn_state_lock);
        m_busy = false;
        m_conn_state_condition.Broadcast ();
}

void CSam::WaitLock ()
{
        wiMutexLocker lock (m_conn_state_lock);
        while (m_busy)
                m_conn_state_condition.Wait ();
}

void CSam::SetPubKey (wxString d)
{
        m_pubKey = d.c_str ();
}

void CSam::SetPrivKey (wxString d)
{
        wiMutexLocker lock (s_sessions_lock);
        if (s_sessions.count (m_privKey) != 0) {
                if (m_privKey != d) {
                        s_sessions.erase (m_privKey);
                        s_sessions[d] = this;
                }
        }
        m_privKey = d;
}

CSam *CSam::AttachedSession (wxI2PSocketBase * WXUNUSED (client),
                             const wxString & dest,
                             wxString WXUNUSED (options))
{
        CSam *sam = NULL;

        wiMutexLocker lock (s_sessions_lock);
        if (s_sessions.count (dest)) {
                sam = s_sessions[dest];
        } else if (dest == wxEmptyString && s_sessions.size ()) {
                sam = s_sessions.begin ()->second;
        } else {
                return NULL;
        }
        sam->m_clients_count++;
        return sam;
}

CSam *CSam::NewSession (wxI2PSocketBase * client, const wxString & _dest,
                        wxString options)
{
        static long long tempId = 0;
        wxString dest = _dest;
        wxString privKey = dest;
        wiMutexLocker lock (s_sessions_lock);
        if (s_sessions.count (dest))
                return NULL;

        if (dest == wxEmptyString) {
                dest = wxString::Format (wxT ("temp_%lld"), ++tempId);
        }

        s_sessions[dest] = new CSam (privKey, options);
        s_sessions[dest]->m_master_client = client;
        s_sessions[dest]->m_clients_count++;
        s_sessions[dest]->m_registrationID = dest;
        return s_sessions[dest];
}


void CSam::AttachClient (wxI2PSocketBase * client)
{
        wiMutexLocker lock (s_sessions_lock);
        m_master_client = client;
        m_clients_count++;
}

void CSam::DetachClient (wxI2PSocketBase * client)
{
        HandleDetach (client);
}

void CSam::HandleDetach (wxI2PSocketBase * client)
{
        wiMutexLocker lock (s_sessions_lock);
        if (m_master_client == client)
                m_master_client = NULL;
        m_clients_count--;

        if (m_clients_count == 0) {
                if (s_sessions.count (this->m_registrationID))
                        if (this == s_sessions[this->m_registrationID])
                                s_sessions.erase (this->m_registrationID);
                delete this;
        }
}


void CSam::NotifyClients (wxSamEvent & event)
{
        if (m_master_client)
                m_master_client->ProcessEvent (event);
}
