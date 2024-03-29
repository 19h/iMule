/////////////////////////////////////////////////////////////////////////////
// Name:        server.cpp
// Purpose:     Server for wxSocket demo
// Author:      Guillermo Rodriguez Garcia <guille@iies.es>
// Modified by:
// Created:     1999/09/19
// RCS-ID:      $Id: server.cpp 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) 1999 Guillermo Rodriguez Garcia
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ==========================================================================
// declarations
// ==========================================================================

// --------------------------------------------------------------------------
// headers
// --------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wx.h"
#include "wx/socket.h"
#include <wx/thread.h>

// --------------------------------------------------------------------------
// resources
// --------------------------------------------------------------------------


// --------------------------------------------------------------------------
// classes
// --------------------------------------------------------------------------


// Define a new application type

class MyFrame;
class Testeur;
// Define a new frame type: this is going to be our main frame

class MyFrame : public wxFrame, public wxThreadHelper
{
public:
        MyFrame();
        ~MyFrame();

        // event handlers (these functions should _not_ be virtual)
        void OnQuit ( wxCommandEvent& event );
        void OnTest ( wxCommandEvent& event );
        void OnAbout ( wxCommandEvent& event );

        void Test1 ( wxSocketBase *sock );
        void Test2 ( wxSocketBase *sock );
        void Test3 ( wxSocketBase *sock );

        // convenience functions
        void UpdateStatusBar();

        void * Entry();

        wxTextCtrl     *m_text;
        wxMenu         *m_menuFile;
        wxMenuBar      *m_menuBar;
        bool            m_busy;

        Testeur * m_testeur;

        // any class wishing to process wxWidgets events must use this macro
        DECLARE_EVENT_TABLE()
};

class Testeur : public wxEvtHandler
{
public:
        Testeur(MyFrame * frame) ;
        ~Testeur();

        void OnTest ();
        void OnServerEvent ( wxSocketEvent& event );
        void OnSocketEvent ( wxSocketEvent& event );

        int             m_numClients;
private:
        wxSocketServer *m_server;
        MyFrame * m_frame;

        DECLARE_EVENT_TABLE();
};


// --------------------------------------------------------------------------
// constants
// --------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum {
        // menu items
        SERVER_QUIT = wxID_EXIT,
        SERVER_ABOUT = wxID_ABOUT,

        // id for sockets
        SERVER_ID = 786415,
        SOCKET_ID,
        SERVER_TEST
};

// --------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// --------------------------------------------------------------------------

BEGIN_EVENT_TABLE ( MyFrame, wxFrame )
        EVT_MENU ( SERVER_QUIT,  MyFrame::OnQuit )
        EVT_MENU ( SERVER_ABOUT, MyFrame::OnAbout )
        EVT_MENU ( SERVER_TEST,  MyFrame::OnTest )
END_EVENT_TABLE()


BEGIN_EVENT_TABLE ( Testeur, wxEvtHandler )
        EVT_SOCKET ( SERVER_ID,  Testeur::OnServerEvent )
        EVT_SOCKET ( SOCKET_ID,  Testeur::OnSocketEvent )
END_EVENT_TABLE()

// ==========================================================================
// implementation
// ==========================================================================

// --------------------------------------------------------------------------
// main frame
// --------------------------------------------------------------------------

// frame constructor

MyFrame::MyFrame() : wxFrame ( ( wxFrame * ) NULL, wxID_ANY,
                                       _ ( "wxSocket demo: Server" ),
                                       wxDefaultPosition, wxSize ( 300, 200 ) )
{
        // Make menus
        m_menuFile = new wxMenu();
        m_menuFile->Append ( SERVER_TEST, _ ( "&Test...\tCtrl-T" ), _ ( "Send message to myself" ) );
        m_menuFile->AppendSeparator();
        m_menuFile->Append ( SERVER_ABOUT, _ ( "&About...\tCtrl-A" ), _ ( "Show about dialog" ) );
        m_menuFile->AppendSeparator();
        m_menuFile->Append ( SERVER_QUIT, _ ( "E&xit\tAlt-X" ), _ ( "Quit server" ) );

        // Append menus to the menubar
        m_menuBar = new wxMenuBar();
        m_menuBar->Append ( m_menuFile, _ ( "&File" ) );
        SetMenuBar ( m_menuBar );

#if wxUSE_STATUSBAR
        // Status bar
        CreateStatusBar ( 2 );
#endif // wxUSE_STATUSBAR

        // Make a textctrl for logging
        m_text  = new wxTextCtrl ( this, wxID_ANY,
                                   _ ( "Welcome to wxSocket demo: Server\n" ),
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY );
        wxSocketBase::Initialize();
        //wxThreadHelper::Create();
        //GetThread()->Run();
        m_testeur = new Testeur(this);
        Show(true);
}


void * MyFrame::Entry()
{
        //m_testeur = new Testeur(this);
}

Testeur::Testeur(MyFrame * frame)
{
        m_frame = frame;

        // Create the address - defaults to localhost:0 initially
        wxIPV4address addr;
        addr.Service ( 3001 );

        // Create the socket
        m_server = new wxSocketServer ( addr );

        // We use Ok() here to see if the server is really listening

        if ( ! m_server->Ok() ) {
                m_frame->m_text->AppendText ( _ ( "Could not listen at the specified port !\n\n" ) );
                return;
        } else {
                m_frame->m_text->AppendText ( _ ( "Server listening.\n\n" ) );
        }

        // Setup the event handler and subscribe to connection events
        m_server->SetEventHandler ( *this, SERVER_ID );

        m_server->SetNotify ( wxSOCKET_CONNECTION_FLAG );

        m_server->Notify ( true );

        m_frame->m_busy = false;

        m_numClients = 0;

        m_frame->UpdateStatusBar();
}
MyFrame::~MyFrame()
{
        delete m_testeur;
}

Testeur::~Testeur()
{
        // No delayed deletion here, as the frame is dying anyway
        delete m_server;
}

// event handlers

void MyFrame::OnQuit ( wxCommandEvent& WXUNUSED ( event ) )
{
        // true is to force the frame to close
        Close ( true );
}

void MyFrame::OnAbout ( wxCommandEvent& WXUNUSED ( event ) )
{
        wxMessageBox ( _ ( "wxSocket demo: Server\n(c) 1999 Guillermo Rodriguez Garcia\n" ),
                       _ ( "About Server" ),
                       wxOK | wxICON_INFORMATION, this );
}

void MyFrame::OnTest(wxCommandEvent& WXUNUSED ( event ))
{
        m_testeur->OnTest();
}

void Testeur::OnTest ()
{
        wxSocketClient * client = new wxSocketClient();
        wxIPV4address address;
        address.Hostname ( wxT ( "localhost" ) );
        address.Service ( 7656 );
        client->Connect ( address, true );
        char phrase[] = "HELLO VERSION MIN=1.0 MAX=1.0\n";
        m_frame->m_text->AppendText ( wxString::FromAscii ( phrase ) );
        client->Write ( phrase, 30 );

        client->SetEventHandler ( *this, SOCKET_ID );
        client->SetNotify ( wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG );
        client->Notify ( true );
}

void MyFrame::Test1 ( wxSocketBase *sock )
{
        unsigned char len;
        char *buf;

        m_text->AppendText ( _ ( "Test 1 begins\n" ) );

        // Receive data from socket and send it back. We will first
        // get a byte with the buffer size, so we can specify the
        // exact size and use the wxSOCKET_WAITALL flag. Also, we
        // disabled input events so we won't have unwanted reentrance.
        // This way we can avoid the infamous wxSOCKET_BLOCK flag.

        sock->SetFlags ( wxSOCKET_WAITALL );

        // Read the size
        sock->Read ( &len, 1 );
        buf = new char[len];

        // Read the data
        sock->Read ( buf, len );
        m_text->AppendText ( _ ( "Got the data, sending it back\n" ) );

        // Write it back
        sock->Write ( buf, len );
        delete[] buf;

        m_text->AppendText ( _ ( "Test 1 ends\n\n" ) );
}

void MyFrame::Test2 ( wxSocketBase *sock )
{
#define MAX_MSG_SIZE 10000

        wxString s;
        wxChar *buf = new wxChar[MAX_MSG_SIZE];
        wxUint32 len;

        m_text->AppendText ( _ ( "Test 2 begins\n" ) );

        // We don't need to set flags because ReadMsg and WriteMsg
        // are not affected by them anyway.

        // Read the message
        len = sock->ReadMsg ( buf, MAX_MSG_SIZE * sizeof ( wxChar ) ).LastCount();
        s.Printf ( _ ( "Client says: %s\n" ), buf );
        m_text->AppendText ( s );
        m_text->AppendText ( _ ( "Sending the data back\n" ) );

        // Write it back
        sock->WriteMsg ( buf, len );
        delete[] buf;

        m_text->AppendText ( _ ( "Test 2 ends\n\n" ) );

#undef MAX_MSG_SIZE
}

void MyFrame::Test3 ( wxSocketBase *sock )
{
        unsigned char len;
        char *buf;

        m_text->AppendText ( _ ( "Test 3 begins\n" ) );

        // This test is similar to the first one, but the len is
        // expressed in kbytes - this tests large data transfers.

        sock->SetFlags ( wxSOCKET_WAITALL );

        // Read the size
        sock->Read ( &len, 1 );
        buf = new char[len * 1024];

        // Read the data
        sock->Read ( buf, len * 1024 );
        m_text->AppendText ( _ ( "Got the data, sending it back\n" ) );

        // Write it back
        sock->Write ( buf, len * 1024 );
        delete[] buf;

        m_text->AppendText ( _ ( "Test 3 ends\n\n" ) );
}

void Testeur::OnServerEvent ( wxSocketEvent& event )
{
        wxString s = _ ( "OnServerEvent: " );
        wxSocketBase *sock;

        switch ( event.GetSocketEvent() ) {
        case wxSOCKET_CONNECTION : s.Append ( _ ( "wxSOCKET_CONNECTION\n" ) ); break;

        default                  : s.Append ( _ ( "Unexpected event !\n" ) ); break;
        }

        m_frame->m_text->AppendText ( s );

        // Accept new connection if there is one in the pending
        // connections queue, else exit. We use Accept(false) for
        // non-blocking accept (although if we got here, there
        // should ALWAYS be a pending connection).

        sock = m_server->Accept ( false );

        if ( sock ) {
                m_frame->m_text->AppendText ( _ ( "New client connection accepted\n\n" ) );
        } else {
                m_frame->m_text->AppendText ( _ ( "Error: couldn't accept a new connection\n\n" ) );
                return;
        }

        sock->SetEventHandler ( *this, SOCKET_ID );

        sock->SetNotify ( wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG );
        sock->Notify ( true );

        m_numClients++;
        m_frame->UpdateStatusBar();
}

void Testeur::OnSocketEvent ( wxSocketEvent& event )
{
        wxString s = _ ( "OnSocketEvent: " );
        wxSocketBase *sock = event.GetSocket();

        // First, print a message

        switch ( event.GetSocketEvent() ) {
        case wxSOCKET_INPUT : s.Append ( _ ( "wxSOCKET_INPUT\n" ) ); break;

        case wxSOCKET_LOST  : s.Append ( _ ( "wxSOCKET_LOST\n" ) ); break;

        default             : s.Append ( _ ( "Unexpected event !\n" ) ); break;
        }


        // Now we process the event
        switch ( event.GetSocketEvent() ) {
        case wxSOCKET_INPUT: {
                // We disable input events, so that the test doesn't trigger
                // wxSocketEvent again.
                sock->SetNotify ( wxSOCKET_LOST_FLAG );

                // Which test are we going to run?
                unsigned char c;
                sock->Read ( &c, 1 );

                m_frame->m_text->AppendText ( wxChar ( c ) );

                // Enable input events again.
                sock->SetNotify ( wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG );
                break;
        }

        case wxSOCKET_LOST: {
                m_frame->m_text->AppendText ( s );
                m_numClients--;

                // Destroy() should be used instead of delete wherever possible,
                // due to the fact that wxSocket uses 'delayed events' (see the
                // documentation for wxQueueEvent) and we don't want an event to
                // arrive to the event handler (the frame, here) after the socket
                // has been deleted. Also, we might be doing some other thing with
                // the socket at the same time; for example, we might be in the
                // middle of a test or something. Destroy() takes care of all
                // this for us.

                m_frame->m_text->AppendText ( _ ( "Deleting socket.\n\n" ) );
                sock->Destroy();
                break;
        }

        default: ;
        }

        m_frame->UpdateStatusBar();
}

// convenience functions

void MyFrame::UpdateStatusBar()
{
#if wxUSE_STATUSBAR
        wxString s;
        if (m_testeur)
                s.Printf ( _ ( "%d clients connected" ), m_testeur->m_numClients );
        SetStatusText ( s, 1 );
#endif // wxUSE_STATUSBAR
}
