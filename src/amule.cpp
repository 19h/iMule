//
// This file is part of the imule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//




#include "amule.h"			// Interface declarations.

#include <memory>
#include <csignal>
#include <cstring>
#include <wx/process.h>
#include <wx/sstream.h>
#include "i2p/CI2PAddress.h"
#include "MuleThread.h"

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for HAVE_GETRLIMIT, HAVE_SETRLIMIT,
//   HAVE_SYS_RESOURCE_H, HAVE_SYS_STATVFS_H, VERSION
//   and ENABLE_NLS
#endif

#include <common/ClientVersion.h>

#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/config.h>			// Do_not_auto_remove (win32)
#include <wx/fileconf.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>


#include <common/Format.h>		// Needed for CFormat
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Prefs.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "kademlia/routing/RoutingZone.h"

#include "CanceledFileList.h"
#include "ClientCreditsList.h"		// Needed for CClientCreditsList
#include "ClientList.h"			// Needed for CClientList
#include "ClientUDPSocket.h"		// Needed for CClientUDPSocket & CMuleUDPSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include <common/FileFunctions.h>	// Needed for CDirIterator
#include "FriendList.h"			// Needed for CFriendList
#include "HTTPDownload.h"		// Needed for CHTTPDownloadThread
#include "InternalEvents.h"		// Needed for CMuleInternalEvent
#include "IPFilter.h"			// Needed for CIPFilter
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "ListenSocket.h"		// Needed for CListenSocket
#include "Logger.h"			// Needed for CLogger // Do_not_auto_remove
#include "MagnetURI.h"			// Needed for CMagnetURI
#include "OtherFunctions.h"
#include "PartFile.h"			// Needed for CPartFile
#include "PlatformSpecific.h"   // Needed for PlatformSpecific::AllowSleepMode();
#include "Preferences.h"		// Needed for CPreferences
#include "SearchList.h"			// Needed for CSearchList
#include "Server.h"			// Needed for GetListName
#include "ServerList.h"			// Needed for CServerList
#include "ServerConnect.h"              // Needed for CServerConnect
#include "ServerUDPSocket.h"		// Needed for CServerUDPSocket
#include "Statistics.h"			// Needed for CStatistics
#include "TerminationProcessAmuleweb.h"	// Needed for CTerminationProcessAmuleweb
#include "ThreadTasks.h"
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "UploadBandwidthThrottler.h"
#include "UserEvents.h"

#include "i2p/CI2PRouter.h"             // Needed for CI2PRouter
#include "i2p/CSamLauncher.h"           // Needed for CSamLauncher
#include "I2PConnectionManager.h"

#ifdef ENABLE_UPNP
#include "UPnPBase.h"			// Needed for UPnP
#endif

#ifdef __WXMAC__
#include <wx/sysopt.h>			// Do_not_auto_remove
#endif

#ifndef AMULE_DAEMON
#ifdef __WXMAC__
#include <CoreFoundation/CFBundle.h>  // Do_not_auto_remove
#if wxCHECK_VERSION(2, 9, 0)
#include <wx/osx/core/cfstring.h>  // Do_not_auto_remove
#else
#include <wx/mac/corefoundation/cfstring.h>  // Do_not_auto_remove
#endif
#endif
#include <wx/msgdlg.h>

#include "amuleDlg.h"
#endif


#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>  // Do_not_auto_remove
#endif


#ifdef __GLIBC__
# define RLIMIT_RESOURCE __rlimit_resource
#else
# define RLIMIT_RESOURCE int
#endif

#ifdef AMULE_DAEMON
CamuleDaemonApp *theApp;
#else
CamuleGuiApp *theApp;
#endif

#if defined(HAVE_GETRLIMIT) && defined(HAVE_SETRLIMIT)
static void UnlimitResource(RLIMIT_RESOURCE resType)
{
        struct rlimit rl;
        getrlimit(resType, &rl);
        rl.rlim_cur = rl.rlim_max;
        setrlimit(resType, &rl);
}
#else
static void UnlimitResource(RLIMIT_RESOURCE) {}
#endif


static void SetResourceLimits()
{
#ifdef HAVE_SYS_RESOURCE_H
        UnlimitResource(RLIMIT_DATA);
#ifndef __UCLIBC__
        UnlimitResource(RLIMIT_FSIZE);
#endif
        UnlimitResource(RLIMIT_NOFILE);
#ifdef RLIMIT_RSS
        UnlimitResource(RLIMIT_RSS);
#endif
#endif
}

// We store the received signal in order to avoid race-conditions
// in the signal handler.
bool g_shutdownSignal = false;

void OnShutdownSignal( int /* sig */ )
{
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);

        g_shutdownSignal = true;

#ifdef AMULE_DAEMON
        //theApp->ShutDown();
        //theApp->ExitMainLoop();
#endif
}

wxDEFINE_EVENT(ID_LISTENSOCKET_EVENT, CI2PSocketEvent);
wxDEFINE_EVENT(ID_ED2KSERVER_UDPSOCKET_EVENT, CI2PSocketEvent);
wxDEFINE_EVENT(ID_KAD_UDPSOCKET_EVENT, CI2PSocketEvent);

CamuleApp::CamuleApp()
{
        // Madcat - Initialize timer as the VERY FIRST thing to avoid any issues later.
        // Kry - I love to init the vars on init, even before timer.
        StartTickTimer();

        // Initialization
        m_app_state = APP_STATE_STARTING;

        m_connection_manager.reset(new I2PConnectionManager());
        theApp = &wxGetApp();

        clientlist	= NULL;
        searchlist	= NULL;
        knownfiles	= NULL;
        canceledfiles	= NULL;
        serverlist	= NULL;
        serverconnect	= NULL;
        sharedfiles	= NULL;
#ifdef INTERNAL_ROUTER
        i2prouter       = NULL;
#endif
        listensocket	= NULL;
        clientudp	= NULL;
        clientcredits	= NULL;
        friendlist	= NULL;
        downloadqueue	= NULL;
        uploadqueue	= NULL;
        ipfilter	= NULL;
        ECServerHandler = NULL;
        glob_prefs	= NULL;
        m_statistics	= NULL;
        uploadBandwidthThrottler = NULL;
#ifdef ENABLE_UPNP
        m_upnp		= NULL;
        m_upnpMappings.resize(4);
#endif
        core_timer	= NULL;

        m_localdest		= CI2PAddress::null;
        m_dwPublicDest	= CI2PAddress::null;
        webserver_pid	= 0;

        enable_daemon_fork = false;

        strFullMuleVersion = NULL;
        strOSDescription = NULL;

        // Apprently needed for *BSD
        SetResourceLimits();
#ifdef _MSC_VER
        _CrtSetDbgFlag(0);		// Disable useless memleak debugging
#endif        
}

CamuleApp::~CamuleApp()
{
        // Closing the log-file as the very last thing, since
        // wxWidgets log-events are saved in it as well.
        theLogger.CloseLogfile();

        if (strFullMuleVersion) {
                free(strFullMuleVersion);
        }
        if (strOSDescription) {
                free(strOSDescription);
        }
}

int CamuleApp::OnExit()
{
        if (m_app_state!=APP_STATE_STARTING) {
                AddLogLineNS(_("Now, exiting main app..."));
        }

        // From wxWidgets docs, wxConfigBase:
        // ...
        // Note that you must delete this object (usually in wxApp::OnExit)
        // in order to avoid memory leaks, wxWidgets won't do it automatically.
        //
        // As it happens, you may even further simplify the procedure described
        // above: you may forget about calling Set(). When Get() is called and
        // there is no current object, it will create one using Create() function.
        // To disable this behaviour DontCreateOnDemand() is provided.
        delete wxConfigBase::Set((wxConfigBase *)NULL);

        // Save credits
        clientcredits->SaveList();

        // Kill amuleweb if running
        if (webserver_pid) {
                AddLogLineNS(CFormat(_("Terminating imuleweb instance with pid '%ld' ... ")) % webserver_pid);
                wxKillError rc;
                if (wxKill(webserver_pid, wxSIGTERM, &rc) == -1) {
                        AddLogLineNS(CFormat(_("Killing imuleweb instance with pid '%ld' ... ")) % webserver_pid);
                        if (wxKill(webserver_pid, wxSIGKILL, &rc) == -1) {
                                AddLogLineNS(_("Failed"));
                        }
                }
        }

        if (m_app_state!=APP_STATE_STARTING) {
                AddLogLineNS(_("iMule OnExit: Terminating core."));
        }

        delete serverlist;
        serverlist = NULL;

        delete searchlist;
        searchlist = NULL;

        delete clientcredits;
        clientcredits = NULL;

        delete friendlist;
        friendlist = NULL;

        // Destroying CDownloadQueue calls destructor for CPartFile
        // calling CSharedFileList::SafeAddKFile occasionally.
        delete sharedfiles;
        sharedfiles = NULL;

        delete serverconnect;
        serverconnect = NULL;

        delete listensocket;
        listensocket = NULL;

        delete clientudp;
        clientudp = NULL;

        delete knownfiles;
        knownfiles = NULL;

        delete canceledfiles;
        canceledfiles = NULL;

        delete clientlist;
        clientlist = NULL;

        delete uploadqueue;
        uploadqueue = NULL;

        delete downloadqueue;
        downloadqueue = NULL;

        delete ipfilter;
        ipfilter = NULL;

#ifdef ENABLE_UPNP
        delete m_upnp;
        m_upnp = NULL;
#endif

        delete ECServerHandler;
        ECServerHandler = NULL;

        delete m_statistics;
        m_statistics = NULL;

        delete glob_prefs;
        glob_prefs = NULL;
        CPreferences::EraseItemList();

        delete uploadBandwidthThrottler;
        uploadBandwidthThrottler = NULL;

#ifdef INTERNAL_ROUTER
        if ( i2prouter != NULL ) delete i2prouter;
        i2prouter = NULL;
#endif

        if (m_app_state!=APP_STATE_STARTING) {
                AddLogLineNS(_("iMule shutdown completed."));
        }

#if wxUSE_MEMORY_TRACING
        AddLogLineNS(_("Memory debug results for iMule exit:"));
        // Log mem debug mesages to wxLogStderr
        wxLog* oldLog = wxLog::SetActiveTarget(new wxLogStderr);
        //AddLogLineNS(wxT("**************Classes**************");
        //wxDebugContext::PrintClasses();
        //AddLogLineNS(wxT("***************Dump***************");
        //wxDebugContext::Dump();
        AddLogLineNS(wxT("***************Stats**************"));
        wxDebugContext::PrintStatistics(true);

        // Set back to wxLogGui
        delete wxLog::SetActiveTarget(oldLog);
#endif

        StopTickTimer();

        // Return 0 for succesful program termination
        return AMULE_APP_BASE::OnExit();
}


int CamuleApp::InitGui(bool, wxString &)
{
        return 0;
}


//
// Application initialization
//
bool CamuleApp::OnInit()
{
        wxSocketBase::Initialize();
        AMULE_APP_BASE::OnInit();
#if wxUSE_MEMORY_TRACING
        // any text before call of Localize_mule needs not to be translated.
        AddLogLineNS(wxT("Checkpoint set on app init for memory debug"));	// debug output
        wxDebugContext::SetCheckpoint();
#endif

        // Forward wxLog events to CLogger
        wxLog::SetActiveTarget(new CLoggerTarget);

#ifndef __WXMSW__
        // get rid of sigpipe
        signal(SIGPIPE, SIG_IGN);
#else
        // Handle CTRL-Break
        signal(SIGBREAK, OnShutdownSignal);
#endif
        // Handle sigint and sigterm
        signal(SIGINT, OnShutdownSignal);
        signal(SIGTERM, OnShutdownSignal);

#ifdef __WXMAC__
        // For listctrl's to behave on Mac
        wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), 1);
#endif


        // Handle uncaught exceptions
        InstallMuleExceptionHandler();

        if (!InitCommon(AMULE_APP_BASE::argc, AMULE_APP_BASE::argv)) {
                return false;
        }


        glob_prefs = new CPreferences();

        CPath outDir;
        if (CheckMuleDirectory(wxT("temp"), thePrefs::GetTempDir(), ConfigDir + wxT("Temp"), outDir)) {
                thePrefs::SetTempDir(outDir);
        } else {
                return false;
        }

        if (CheckMuleDirectory(wxT("incoming"), thePrefs::GetIncomingDir(), ConfigDir + wxT("Incoming"), outDir)) {
                thePrefs::SetIncomingDir(outDir);
        } else {
                return false;
        }

        // Some sanity check
        if (!thePrefs::UseTrayIcon()) {
                thePrefs::SetMinToTray(false);
        }

        // Build the filenames for the two OS files
        SetOSFiles(thePrefs::GetOSDir().GetRaw());

#ifdef ENABLE_NLS
        // Load localization settings
        Localize_mule();
#endif

        // Configure EC for amuled when invoked with ec-config
        if (ec_config) {
                AddLogLineNS(_("\nEC configuration"));
                thePrefs::SetECPass(GetPassword());
                thePrefs::EnableExternalConnections(true);
                AddLogLineNS(_("Password set and external connections enabled."));
        }

#ifndef __WXMSW__
        if (getuid() == 0) {
                wxString msg =
                        wxT ( "Warning! You are running imule as root.\n" )
                        wxT("Doing so is not recommended for security reasons,\n")
                        wxT ( "and you are advised to run imule as an normal\n" )
                        wxT("user instead.");

                ShowAlert(msg, _("WARNING"), wxCENTRE | wxOK | wxICON_ERROR);

                fprintf(stderr, "\n--------------------------------------------------\n");
                fprintf(stderr, "%s", (const char*)unicode2UTF8(msg));
                fprintf(stderr, "\n--------------------------------------------------\n\n");
        }
#endif

        // Display notification on new version or first run
        wxTextFile vfile( ConfigDir + wxT("lastversion") );
        wxString newMule(wxT( VERSION ));

        if ( !wxFileExists( vfile.GetName() ) ) {
                vfile.Create();
        }

        if ( vfile.Open() ) {
                // Check if this version has been run before
                bool found = false;
                for ( size_t i = 0; i < vfile.GetLineCount(); i++ ) {
                        // Check if this version has been run before
                        if ( vfile.GetLine(i) == newMule ) {
                                found = true;
                                break;
                        }
                }

                // We havent run this version before?
                if ( !found ) {
                        // Insert new at top to provide faster searches
                        vfile.InsertLine( newMule, 0 );

                        Trigger_New_version( newMule );
                }

                // Keep at most 10 entires
                while ( vfile.GetLineCount() > 10 )
                        vfile.RemoveLine( vfile.GetLineCount() - 1 );

                vfile.Write();
                vfile.Close();
        }

        // Check if we have the old style locale config
        wxString langId = thePrefs::GetLanguageID();
        if (!langId.IsEmpty() && (langId.GetChar(0) >= '0' && langId.GetChar(0) <= '9')) {
                wxString info(_("Your locale has been changed to System Default due to a configuration change. Sorry."));
                thePrefs::SetLanguageID(wxLang2Str(wxLANGUAGE_DEFAULT));
                ShowAlert(info, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);
        }

        m_statistics = new CStatistics();

        clientlist	= new CClientList();
        friendlist	= new CFriendList();
        searchlist	= new CSearchList();
        knownfiles	= new CKnownFileList();
        canceledfiles	= new CCanceledFileList;
        serverlist	= new CServerList();

        sharedfiles	= new CSharedFileList(knownfiles);
        clientcredits	= new CClientCreditsList();

        // bugfix - do this before creating the uploadqueue
        downloadqueue	= new CDownloadQueue();
        uploadqueue	= new CUploadQueue();
        ipfilter	= new CIPFilter();

        // Creates all needed listening sockets
        wxString msg;
        if (!InitializeNetwork(&msg)) {
                AddLogLineNS(wxT("\n"));
                AddLogLineNS(msg);
        }

        // Create main dialog, or fork to background (daemon).
        InitGui(m_geometryEnabled, m_geometryString);

#ifdef AMULE_DAEMON
        // Need to refresh wxSingleInstanceChecker after the daemon fork() !
        if (enable_daemon_fork) {
                RefreshSingleInstanceChecker();
                // No need to check IsAnotherRunning() - we've done it before.
        }
#endif

        // Has to be created after the call to InitGui, as fork
        // (when using posix threads) only replicates the mainthread,
        // and the UBT constructor creates a thread.
        uploadBandwidthThrottler = new UploadBandwidthThrottler();

        // Start performing background tasks
        // This will start loading the IP filter. It will start right away.
        // Log is confusing, because log entries from background will only be printed
        // once foreground becomes idle, and that will only be after loading
        // of the partfiles has finished.
        CThreadScheduler::Start();
        // These must be initialized after the gui is loaded.
        if (thePrefs::GetNetworkED2K()) {
                serverlist->Init();
        }
        downloadqueue->LoadMetFiles(thePrefs::GetTempDir());
        sharedfiles->Reload();



        // Ensure that the up/down ratio is used
        CPreferences::CheckUlDlRatio();

        // Load saved friendlist (now, so it can update in GUI right away)
        friendlist->LoadList();

        // The user can start pressing buttons like mad if he feels like it.
        m_app_state = APP_STATE_RUNNING;

        if (!serverlist->GetServerCount() && thePrefs::GetNetworkED2K()) {
                // There are no servers and ED2K active -> ask for download.
                // As we cannot ask in amuled, we just update there
                // Kry TODO: Store server.met URL on preferences and use it here and in GUI.
#ifndef AMULE_DAEMON
                if (wxYES == wxMessageBox(
                                        wxString(
                                                _("You don't have any server in the server list.\nDo you want aMule to download a new list now?")),
                                        wxString(_("Server list download")),
                                        wxYES_NO,
                                        static_cast<wxWindow*>(theApp->amuledlg)))
#endif
                {
                        // workaround amuled crash
#ifndef AMULE_DAEMON
                        serverlist->UpdateServerMetFromURL(
                                wxT("http://gruk.org/server.met.gz"));
#endif
                }
        }

        // Autoconnect if that option is enabled // handled by I2PConnectionManager in iMule
        /*
        if (thePrefs::DoAutoConnect()) {
                // IP filter is still loading and will be finished on event.
                // Tell it to autoconnect.
                if (thePrefs::GetNetworkED2K()) {
                        ipfilter->ConnectToAnyServerWhenReady();
                }
                if (thePrefs::GetNetworkKademlia()) {
                        ipfilter->StartKADWhenReady();
                }
        }
        */

        // Enable GeoIP
#ifdef ENABLE_IP2COUNTRY
        theApp->amuledlg->EnableIP2Country();
#endif

        // Run webserver?
        if (thePrefs::GetWSIsEnabled()) {
                wxString aMuleConfigFile = ConfigDir + m_configFile;
                wxString amulewebPath = thePrefs::GetWSPath();

#if defined(__WXMAC__) && !defined(AMULE_DAEMON)
                // For the Mac GUI application, look for amuleweb in the bundle
                CFURLRef amulewebUrl = CFBundleCopyAuxiliaryExecutableURL(
                                               CFBundleGetMainBundle(), CFSTR("imuleweb"));

                if (amulewebUrl) {
                        CFURLRef absoluteUrl = CFURLCopyAbsoluteURL(amulewebUrl);
                        CFRelease(amulewebUrl);

                        if (absoluteUrl) {
                                CFStringRef amulewebCfstr = CFURLCopyFileSystemPath(absoluteUrl, kCFURLPOSIXPathStyle);
                                CFRelease(absoluteUrl);
#if wxCHECK_VERSION(2, 9, 0)
                                amulewebPath = wxCFStringRef(amulewebCfstr).AsString(wxLocale::GetSystemEncoding());
#else
                                amulewebPath = wxMacCFStringHolder(amulewebCfstr).AsString(wxLocale::GetSystemEncoding());
#endif
                        }
                }
#endif

#ifdef __WXMSW__
#	define QUOTE	wxT("\"")
#else
#	define QUOTE	wxT("\'")
#endif

                wxString cmd =
                        QUOTE +
                        amulewebPath +
                        QUOTE wxT(" ") QUOTE wxT("--imule-config-file=") +
                        aMuleConfigFile +
                        QUOTE;
                CTerminationProcessAmuleweb *p = new CTerminationProcessAmuleweb(cmd, &webserver_pid);
                webserver_pid = wxExecute(cmd, wxEXEC_ASYNC, p);
                bool webserver_ok = webserver_pid > 0;
                if (webserver_ok) {
                        AddLogLineC(CFormat(_("web server running on pid %d")) % webserver_pid);
                } else {
                        delete p;
                        ShowAlert(_(
                                          "You requested to run web server on startup, but the imuleweb binary cannot be run. Please install the package containing iMule web server, or compile iMule using --enable-webserver and run make install"),
                                  _("ERROR"), wxOK | wxICON_ERROR);
                }
        }
        return true;
}

bool CamuleApp::InitializeNetwork ( wxString* msg )
{
        bool ok = true;

        static bool firstTime = true;

        if ( !firstTime ) {
                // TODO: Destroy previously created sockets
        }

        firstTime = false;

        // Some sanity checks first
        if ( thePrefs::ECPort() == thePrefs::GetI2PServerPort()
                        || thePrefs::ECPort() == thePrefs::GetI2PServerI2PTcpPort()
                        || thePrefs::ECPort() == thePrefs::GetI2PServerI2PUdpPort()
           ) {
                // Select a random usable port in the range 1025 ... 2^16 - 1
                uint16 port = thePrefs::ECPort();

                while ( port < 1024
                                || port == thePrefs::GetI2PServerPort()
                                || port == thePrefs::GetI2PServerI2PUdpPort()
                                || port == thePrefs::GetI2PServerI2PTcpPort()
                      ) {
                        port = (uint16)rand();
                }

                thePrefs::SetECPort ( port );

                wxString err = wxT ( "Network configuration failed! You cannot use the same port\n"
                                     "for the I2P server ports and the External Connections port.\n"
                                     "The EC port has been changed to avoid conflict, see the\n"
                                     "preferences for the new value.\n" );

                *msg << err;

                AddLogLineN(wxEmptyString );
                AddLogLineC(err );
                AddLogLineN(wxEmptyString );

                ok = false;
        }


        // Create the address where we are going to listen
        // TODO: read this from configuration file
        amuleIPV4Address myaddr[1];

        // Create the External Connections Socket.
        // Default is 4712.
        // Get ready to handle connections from apps like amulecmd
        if (thePrefs::GetECAddress().IsEmpty() || !myaddr[0].Hostname(thePrefs::GetECAddress())) {
                myaddr[0].AnyAddress();
        }
        myaddr[0].Service(thePrefs::ECPort());
        ECServerHandler = new ExternalConn(myaddr[0], msg);

        // create sockets I2P
        StartNetwork();

#ifdef ENABLE_UPNP
        if (thePrefs::GetUPnPEnabled()) {
                try {
                        m_upnpMappings[0] = CUPnPPortMapping(
                                                    myaddr[0].Service(),
                                                    "TCP",
                                                    thePrefs::GetUPnPECEnabled(),
                                                    "iMule TCP External Connections Socket");
                        m_upnp = new CUPnPControlPoint(thePrefs::GetUPnPTCPPort());
                        m_upnp->AddPortMappings(m_upnpMappings);
                } catch(CUPnPException &e) {
                        wxString error_msg;
                        error_msg << e.what();
                        AddLogLineC(error_msg);
                        fprintf(stderr, "%s\n", (const char *)unicode2char(error_msg));
                }
        }
#endif

        return ok;
}


/* called by I2PConnectionManager when connected to the I2P router */
void CamuleApp::SetNetworkStarted() {
        if ( listensocket->IsOk() ) {
                Bind(ID_LISTENSOCKET_EVENT, &CamuleApp::ListenSocketHandler, this);
                listensocket->SetEventHandler( *this, ID_LISTENSOCKET_EVENT );
                listensocket->SetNotify( wxSOCKET_CONNECTION_FLAG | wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG );
                listensocket->Notify( true );
                listensocket->StartListening();
        }
        if ( clientudp->Ok() ) {
                Bind(ID_ED2KSERVER_UDPSOCKET_EVENT, &CamuleApp::UDPSocketHandler, this);
                Bind(ID_KAD_UDPSOCKET_EVENT, &CamuleApp::UDPSocketHandler, this);                
                clientudp->SetEventHandler ( *this, ID_KAD_UDPSOCKET_EVENT );
                clientudp->SetNotify ( wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_LOST_FLAG );
                clientudp->Notify ( true );
        }
        networkStarting=false;
        networkStarted=true;
}

/* called by I2PConnectionManager when disconnected to the I2P router */
void CamuleApp::SetNetworkStopped() {
        Unbind(ID_LISTENSOCKET_EVENT, &CamuleApp::ListenSocketHandler, this);
        Unbind(ID_ED2KSERVER_UDPSOCKET_EVENT, &CamuleApp::UDPSocketHandler, this);
        Unbind(ID_KAD_UDPSOCKET_EVENT, &CamuleApp::UDPSocketHandler, this);        

        networkStarting=false;
        networkStarted=false;
}

/* to be removed
// Returns a ed2k link with AICH info if available
wxString CamuleApp::CreateED2kAICHLink(const CKnownFile* f)
{
        // Create the first part of the URL
        wxString strURL = CreateED2kLink(f);
        // Append the AICH info
        if (f->HasProperAICHHashSet()) {
                strURL.RemoveLast();		// remove trailing '/'
                strURL << wxT("h=") << f->GetAICHMasterHash() << wxT("|/");
        }

        // Result is "ed2k://|file|<filename>|<size>|<hash>|h=<AICH master hash>|/"
        return strURL;
}
*/

/* Original implementation by Bouc7 of the eMule Project.
   aMule Signature idea was designed by BigBob and implemented
   by Un-Thesis, with design inputs and suggestions from bothie.
*/
void CamuleApp::OnlineSig(bool zero /* reset stats (used on shutdown) */)
{
        // Do not do anything if online signature is disabled in Preferences
        if (!thePrefs::IsOnlineSignatureEnabled() || m_emulesig_path.IsEmpty()) {
                // We do not need to check m_amulesig_path because if m_emulesig_path is empty,
                // that means m_amulesig_path is empty too.
                return;
        }

        // Remove old signature files
        if ( wxFileExists( m_emulesig_path ) ) { wxRemoveFile( m_emulesig_path ); }
        if ( wxFileExists( m_amulesig_path ) ) { wxRemoveFile( m_amulesig_path ); }


        wxTextFile amulesig_out;
        wxTextFile emulesig_out;

        // Open both files if needed
        if ( !emulesig_out.Create( m_emulesig_path) ) {
                AddLogLineC(_("Failed to create OnlineSig File"));
                // Will never try again.
                m_amulesig_path.Clear();
                m_emulesig_path.Clear();
                return;
        }

        if ( !amulesig_out.Create(m_amulesig_path) ) {
                AddLogLineC(_("Failed to create iMule OnlineSig File"));
                // Will never try again.
                m_amulesig_path.Clear();
                m_emulesig_path.Clear();
                return;
        }

        wxString emulesig_string;
        wxString temp;

        if (zero) {
                emulesig_string = wxT("0\xA0.0|0.0|0");
                amulesig_out.AddLine(wxT("0\n0\n0\n0\n0\n0\n0.0\n0.0\n0\n0"));
        } else {
                if (IsConnectedED2K()) {

                        temp = serverconnect->GetCurrentServer()->GetDest().toString();

                        // We are online
                        emulesig_string =
                                // Connected
                                wxT("1|")
                                //Server name
                                + serverconnect->GetCurrentServer()->GetListName()
                                + wxT("|")
                                // IP and port of the server
                                + serverconnect->GetCurrentServer()->GetDest().toString() //I2P : GetFullIP()
                                + wxT("|")
                                + temp;


                        // Now for amule sig

                        // Connected. State 1, full info
                        amulesig_out.AddLine(wxT("1"));
                        // Server Name
                        amulesig_out.AddLine(serverconnect->GetCurrentServer()->GetListName());
                        // Server IP
                        amulesig_out.AddLine ( serverconnect->GetCurrentServer()->GetDest().toString() );//I2P:IP());
                        // Server Port
                        amulesig_out.AddLine(temp);

                        if (serverconnect->IsLowID()) {
                                amulesig_out.AddLine(wxT("L"));
                        } else {
                                amulesig_out.AddLine(wxT("H"));
                        }

                } else if (serverconnect->IsConnecting()) {
                        emulesig_string = wxT("0");

                        // Connecting. State 2, No info.
                        amulesig_out.AddLine(wxT("2\n0\n0\n0\n0"));
                } else {
                        // Not connected to a server
                        emulesig_string = wxT("0");

                        // Not connected, state 0, no info
                        amulesig_out.AddLine(wxT("0\n0\n0\n0\n0"));
                }
                if (IsConnectedKad()) {
                        if(Kademlia::CKademlia::IsFirewalled()) {
                                // Connected. Firewalled. State 1.
                                amulesig_out.AddLine(wxT("1"));
                        } else {
                                // Connected. State 2.
                                amulesig_out.AddLine(wxT("2"));
                        }
                } else {
                        // Not connected.State 0.
                        amulesig_out.AddLine(wxT("0"));
                }
                emulesig_string += wxT("\xA");

                // Datarate for downloads
                temp = CFormat(wxT("%.1f")) % (theStats::GetDownloadRate() / 1024.0);

                emulesig_string += temp + wxT("|");
                amulesig_out.AddLine(temp);

                // Datarate for uploads
                temp = CFormat(wxT("%.1f")) % (theStats::GetUploadRate() / 1024.0);

                emulesig_string += temp + wxT("|");
                amulesig_out.AddLine(temp);

                // Number of users waiting for upload
                temp = CFormat(wxT("%d")) % theStats::GetWaitingUserCount();

                emulesig_string += temp;
                amulesig_out.AddLine(temp);

                // Number of shared files (not on eMule)
                amulesig_out.AddLine(CFormat(wxT("%d")) % theStats::GetSharedFileCount());
        }

        // eMule signature finished here. Write the line to the wxTextFile.
        emulesig_out.AddLine(emulesig_string);

        // Now for aMule signature extras

        // Nick on the network
        amulesig_out.AddLine(thePrefs::GetUserNick());

        // Total received in bytes
        amulesig_out.AddLine(CFormat(wxT("%llu")) % theStats::GetTotalReceivedBytes());

        // Total sent in bytes
        amulesig_out.AddLine(CFormat(wxT("%llu")) % theStats::GetTotalSentBytes());

        // amule version
#ifdef SVNDATE
        amulesig_out.AddLine(wxT(VERSION) wxT(" ") wxT(SVNDATE));
#else
        amulesig_out.AddLine(wxT(VERSION));
#endif

        if (zero) {
                amulesig_out.AddLine(wxT("0"));
                amulesig_out.AddLine(wxT("0"));
                amulesig_out.AddLine(wxT("0"));
        } else {
                // Total received bytes in session
                amulesig_out.AddLine( CFormat( wxT("%llu") ) %
                                      theStats::GetSessionReceivedBytes() );

                // Total sent bytes in session
                amulesig_out.AddLine( CFormat( wxT("%llu") ) %
                                      theStats::GetSessionSentBytes() );

                // Uptime
                amulesig_out.AddLine(CFormat(wxT("%llu")) % theStats::GetUptimeSeconds());
        }

        // Flush the files
        emulesig_out.Write();
        amulesig_out.Write();
} //End Added By Bouc7


#if wxUSE_ON_FATAL_EXCEPTION
// Gracefully handle fatal exceptions and print backtrace if possible
void CamuleApp::OnFatalException()
{
        /* Print the backtrace */
        fprintf(stderr, "\n--------------------------------------------------------------------------------\n");
        fprintf ( stderr, "A fatal error has occurred and imule has crashed.\n" );
        fprintf(stderr, "Please assist us in fixing this problem by posting the backtrace below in our\n");
        fprintf ( stderr, "'imule Crashes' forum and include as much information as possible regarding the\n" );
        fprintf(stderr, "circumstances of this crash. The forum is located here:\n");
        fprintf(stderr, "    http://forum.amule.org/index.php?board=67.0\n");
        fprintf(stderr, "If possible, please try to generate a real backtrace of this crash:\n");
        fprintf ( stderr, "    http://www.imule.i2p/wiki/index.php/Backtraces\n\n" );
        fprintf(stderr, "----------------------------=| BACKTRACE FOLLOWS: |=----------------------------\n");
        fprintf(stderr, "Current version is: %s\n", strFullMuleVersion);
        fprintf(stderr, "Running on: %s\n\n", strOSDescription);

        print_backtrace(1); // 1 == skip this function.

        fprintf(stderr, "\n--------------------------------------------------------------------------------\n");
}
#endif


// Sets the localization of aMule
void CamuleApp::Localize_mule()
{
        InitCustomLanguages();
        InitLocale(m_locale, StrLang2wx(thePrefs::GetLanguageID()));
        if (!m_locale.IsOk()) {
                AddLogLineN(_("The selected locale seems not to be installed on your box. (Note: I'll try to set it anyway)"));
        }
}


// Displays information related to important changes in aMule.
// Is called when the user runs a new version of aMule
void CamuleApp::Trigger_New_version(wxString new_version)
{
        wxString info = wxT ( " --- " ) + CFormat ( _ ( "This is the first time you run imule %s" ) ) % new_version + wxT ( " ---\n\n" );

        info += wxT ( "\n" );

#ifdef INTERNAL_ROUTER
        info += _ (
                        "If you are running the java I2P router,\n"
                        "\n"
                        "(1) PLEASE DISABLE the \"internal I2P router\"\n"
                        "     in iMule Preferences tab !\n"
                        "\n"
                        "(2) ENABLE the SAMBridge client in your router configuration tab\n"
                        "     (probably by browsing http://localhost:7657/configclients.jsp)\n"
                        "     (you may have to restart your I2P router after this change)\n" );
#else
        info += _ (
                        "This version does NOT include an internal I2P router.\n"
                        "In order to run iMule, you have to install and run the java I2P router.\n"
                        "You can fetch it here :\n"
                        "\n"
                        " http://www.i2p2.de/download\n"
                        "\n"
                        "PLEASE ENABLE the SAMBridge client in your router configuration tab\n"
                        "     (probably by browsing http://localhost:7657/configclients.jsp)\n"
                        "     (you may have to restart your I2P router after this change)\n" );
#endif
        if (new_version == wxT("SVN")) {
                info += _("This version is a testing version, updated daily, and\n");
                info += _("we give no warranty it won't break anything, burn your house,\n");
                info += _("or kill your dog. But it *should* be safe to use anyway.\n");
        }

        // General info
        info += wxT("\n");
        info += _("More information, support and new releases can found at our homepage,\n");
        info += _ ( "at www.imule.i2p.\n" );
        info += wxT("\n");
        info += _ ( "Feel free to report any bugs to http://forum.i2p/viewforum.php?f=30" );

        ShowAlert(info, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);
}


void CamuleApp::SetOSFiles(const wxString new_path)
{
        if ( thePrefs::IsOnlineSignatureEnabled() ) {
                if ( ::wxDirExists(new_path) ) {
                        m_emulesig_path = JoinPaths(new_path, wxT("onlinesig.dat"));
                        m_amulesig_path = JoinPaths(new_path, wxT("imulesig.dat"));
                } else {
                        ShowAlert(_("The folder for Online Signature files you specified is INVALID!\n OnlineSignature will be DISABLED until you fix it on preferences."), _("ERROR"), wxOK | wxICON_ERROR);
                        m_emulesig_path.Clear();
                        m_amulesig_path.Clear();
                }
        } else {
                m_emulesig_path.Clear();
                m_amulesig_path.Clear();
        }
}


#ifdef __WXDEBUG__
#ifndef wxUSE_STACKWALKER
#define wxUSE_STACKWALKER 0
#endif
void CamuleApp::OnAssertFailure(const wxChar* file, int line,
                                const wxChar* func, const wxChar* cond, const wxChar* msg)
{
        if (!wxUSE_STACKWALKER || !wiThread::IsMain() || !IsRunning()) {
                wxString errmsg = CFormat( wxT("%s:%s:%d: Assertion '%s' failed. %s") )
                                  % file % func % line % cond % ( msg ? msg : wxT("") );

                fprintf(stderr, "Assertion failed: %s\n", (const char*)unicode2char(errmsg));

                // Skip the function-calls directly related to the assert call.
                fprintf(stderr, "\nBacktrace follows:\n");
                print_backtrace(3);
                fprintf(stderr, "\n");
        }

        if (wiThread::IsMain() && IsRunning()) {
                AMULE_APP_BASE::OnAssertFailure(file, line, func, cond, msg);
        } else {
                // Abort, allows gdb to catch the assertion
                raise( SIGABRT );
        }
}
#endif


void CamuleApp::OnUDPDnsDone(CMuleInternalEvent& evt)
{
        CServerUDPSocket* socket =(CServerUDPSocket*)evt.GetClientData();
        socket->OnHostnameResolved(CI2PAddress::fromString(evt.GetString()));
}


void CamuleApp::OnSourceDnsDone(CMuleInternalEvent& evt)
{
        downloadqueue->OnHostnameResolved(CI2PAddress::fromString(evt.GetString()));
}


void CamuleApp::OnServerDnsDone(CMuleInternalEvent& evt)
{
        AddLogLineNS(_("Server hostname notified"));
        serverconnect->OnServerHostnameResolved(evt.GetClientData(), CI2PAddress::fromString(evt.GetString()));
}


void CamuleApp::OnTCPTimer(wxTimerEvent& WXUNUSED(evt))
{
        if(!IsRunning()) {
                return;
        }
        serverconnect->StopConnectionTry();
        if (IsConnectedED2K() ) {
                return;
        }
        serverconnect->ConnectToAnyServer();
}

#ifdef INTERNAL_ROUTER
void CamuleApp::UpdateRouterStatus()
{
        wxString info ;
        if ( i2prouter != NULL && networkStarted ) {
                info =  i2prouter->getInfo() ;
        } else if (!thePrefs::GetI2PServerInternal()) {

                info = _("<center><b>You are not running the internal I2P router</b></center>") ;
        } else {
                info = _("<center><b>The network is not ready.</b></center>");
        }
        UpdateRouterStatus ( info );
}

#ifndef CLIENT_GUI
void CamuleGuiApp::UpdateRouterStatus()
{
        CamuleApp::UpdateRouterStatus();
}
#endif // CLIENT_GUI

#endif // INTERNAL_ROUTER

void CamuleApp::OnCoreTimer(wxTimerEvent& WXUNUSED(evt))
{
        // Former TimerProc section
        static uint64 msPrev1, msPrev5, msPrevSave, msPrevHist, msPrevOS, msPrevKnownMet;
        uint64 msCur = theStats::GetUptimeMillis();
        TheTime = msCur / 1000;

        if (!IsRunning()) {
                return;
        }

#ifndef AMULE_DAEMON
        // Check if we should terminate the app
        if ( g_shutdownSignal ) {
                wxWindow* top = GetTopWindow();

                if ( top ) {
                        top->Close(true);
                } else {
                        // No top-window, have to force termination.
                        wxExit();
                }
        }
#else
        if ( g_shutdownSignal ) {
                ShutDown();
                //                 ProcessPendingEvents();
                wxExit();
        }
#endif

        // There is a theoretical chance that the core time function can recurse:
        // if an event function gets blocked on a mutex (communicating with the
        // UploadBandwidthThrottler) wx spawns a new event loop and processes more events.
        // If CPU load gets high a new core timer event could be generated before the last
        // one was finished and so recursion could occur, which would be bad.
        // Detect this and do an early return then.
        static bool recurse = false;
        if (recurse) {
                return;
        }
        recurse = true;

        uploadqueue->Process();
        downloadqueue->Process();
        //theApp->clientcredits->Process();
        theStats::CalculateRates();

        if (msCur-msPrevHist > 1000) {
                // unlike the other loop counters in this function this one will sometimes
                // produce two calls in quick succession (if there was a gap of more than one
                // second between calls to TimerProc) - this is intentional!  This way the
                // history list keeps an average of one node per second and gets thinned out
                // correctly as time progresses.
                msPrevHist += 1000;

                m_statistics->RecordHistory();

        }


        if ( networkStarted && ( msCur - msPrev1 > 1000 ) ) {
                // approximately every second
                msPrev1 = msCur;
                clientcredits->Process();
                clientlist->Process();

                // Publish files to server if needed.
                sharedfiles->Process();

                if( Kademlia::CKademlia::IsRunning() ) {
                        Kademlia::CKademlia::Process();
                        if(Kademlia::CKademlia::GetPrefs()->HasLostConnection()) {
                                StopKad();
                                clientudp->Close();
                                clientudp->Open();
                                if (thePrefs::Reconnect()) {
                                        StartKad();
                                }
                        }
                }

                listensocket->UpdateConnectionsStatus();

        }


        if (msCur-msPrev5 > 5000) {  // every 5 seconds
                msPrev5 = msCur;
                if ( networkStarted ) {
                        listensocket->Process();
                }
        }

        if (msCur-msPrevSave >= 60000) {
                msPrevSave = msCur;
                theStats::Save();

        }

        // Special
        if (msCur - msPrevOS >= thePrefs::GetOSUpdate() * 1000ull) {
                OnlineSig(); // Added By Bouc7
                msPrevOS = msCur;
        }

        if (msCur - msPrevKnownMet >= 30*60*1000/*There must be a prefs option for this*/) {
                // Save Shared Files data
                knownfiles->Save();
                msPrevKnownMet = msCur;
        }


        // Recomended by lugdunummaster himself - from emule 0.30c
        if ( networkStarted )
                serverconnect->KeepConnectionAlive();

        // Disarm recursion protection
        recurse = false;
}


void CamuleApp::OnFinishedHashing(CHashingEvent& evt)
{
        wxCHECK_RET(evt.GetResult(), wxT("No result of hashing"));

        CKnownFile* owner = const_cast<CKnownFile*>(evt.GetOwner());
        CKnownFile* result = evt.GetResult();

        if (owner) {
                // Check if the partfile still exists, as it might have
                // been deleted in the mean time.
                if (downloadqueue->IsPartFile(owner)) {
                        // This cast must not be done before the IsPartFile
                        // call, as dynamic_cast will barf on dangling pointers.
                        dynamic_cast<CPartFile*>(owner)->PartFileHashFinished(result);
                }
        } else {
                static uint64 bytecount = 0;

                if (knownfiles->SafeAddKFile(result, true)) {
                        AddDebugLogLineN(logKnownFiles,
                                         CFormat(wxT("Safe adding file to sharedlist: %s")) % result->GetFileName());
                        sharedfiles->SafeAddKFile(result);

                        bytecount += result->GetFileSize();
                        // If we have added files with a total size of ~300mb
                        if (bytecount >= 314572800) {
                                AddDebugLogLineN(logKnownFiles, wxT("Failsafe for crash on file hashing creation"));
                                if ( m_app_state != APP_STATE_SHUTTINGDOWN ) {
                                        knownfiles->Save();
                                        bytecount = 0;
                                }
                        }
                } else {
                        AddDebugLogLineN(logKnownFiles,
                                         CFormat(wxT("File not added to sharedlist: %s")) % result->GetFileName());
                        delete result;
                }
        }
}


void CamuleApp::OnFinishedAICHHashing(CHashingEvent& evt)
{
        wxCHECK_RET(evt.GetResult(), wxT("No result of AICH-hashing"));

        CKnownFile* owner = const_cast<CKnownFile*>(evt.GetOwner());
        std::unique_ptr<CKnownFile> result(evt.GetResult());

        if (result->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE) {
                CAICHHashSet* oldSet = owner->GetAICHHashset();
                CAICHHashSet* newSet = result->GetAICHHashset();

                owner->SetAICHHashset(newSet);
                newSet->SetOwner(owner);

                result->SetAICHHashset(oldSet);
                oldSet->SetOwner(result.get());
        }
}


void CamuleApp::OnFinishedCompletion(CCompletionEvent& evt)
{
        CPartFile* completed = const_cast<CPartFile*>(evt.GetOwner());
        wxCHECK_RET(completed, wxT("Completion event sent for unspecified file"));
        wxASSERT_MSG(downloadqueue->IsPartFile(completed), wxT("CCompletionEvent for unknown partfile."));

        completed->CompleteFileEnded(evt.ErrorOccured(), evt.GetFullPath());
        if (evt.ErrorOccured()) {
                CUserEvents::ProcessEvent(CUserEvents::ErrorOnCompletion, completed);
        }

        // Check if we should execute an script/app/whatever.
        CUserEvents::ProcessEvent(CUserEvents::DownloadCompleted, completed);
}

void CamuleApp::OnFinishedAllocation(CAllocFinishedEvent& evt)
{
        CPartFile *file = evt.GetFile();
        wxCHECK_RET(file, wxT("Allocation finished event sent for unspecified file"));
        wxASSERT_MSG(downloadqueue->IsPartFile(file), wxT("CAllocFinishedEvent for unknown partfile"));

        file->SetStatus(PS_EMPTY);

        if (evt.Succeeded()) {
                if (evt.IsPaused()) {
                        file->StopFile();
                } else {
                        file->ResumeFile();
                }
        } else {
                AddLogLineN(CFormat(_("Disk space preallocation for file '%s' failed: %s")) % file->GetFileName() % wxString(UTF82unicode(std::strerror(evt.GetResult()))));
                file->StopFile();
        }

        file->AllocationFinished();
};

void CamuleApp::OnNotifyEvent(CMuleGUIEvent& evt)
{
#ifdef AMULE_DAEMON
        evt.Notify();
#else
        if (theApp->amuledlg) {
                evt.Notify();
        }
#endif
}


void CamuleApp::ShutDown()
{
        // Log
        AddLogLineNS(_("iMule shutdown initiated."));

        // Just in case
        PlatformSpecific::AllowSleepMode();

        // Signal the hashing thread to terminate
        m_app_state = APP_STATE_SHUTTINGDOWN;

        StopKad();

        // Kry - Save the sources seeds on app exit
        if (thePrefs::GetSrcSeedsOn()) {
                downloadqueue->SaveSourceSeeds();
        }

        OnlineSig(true); // Added By Bouc7

        // Exit HTTP downloads
        CHTTPDownloadCoroutine::StopAll();

        // Exit thread scheduler and upload thread
        CThreadScheduler::Terminate();

        AddDebugLogLineN(logGeneral, wxT("Terminate upload thread."));
        uploadBandwidthThrottler->EndThread();
        // Close sockets to avoid new clients coming in
        if (listensocket) {
                listensocket->Close();
                listensocket->KillAllSockets();
        }

        if (serverconnect) {
                serverconnect->Disconnect();
        }

        ECServerHandler->KillAllSockets();

#ifdef ENABLE_UPNP
        if (thePrefs::GetUPnPEnabled()) {
                if (m_upnp) {
                        m_upnp->DeletePortMappings(m_upnpMappings);
                }
        }
#endif

        // saving data & stuff
        if (knownfiles) {
                knownfiles->Save();
        }

        theStats::Save();

        CPath configFileName = CPath(ConfigDir + m_configFile);
        CPath::BackupFile(configFileName, wxT(".bak"));

        if (clientlist) {
                clientlist->DeleteAll();
        }


        // Log
        AddDebugLogLineN(logGeneral, wxT("CamuleApp::ShutDown() has ended."));
}


bool CamuleApp::AddServer(CServer *srv, bool fromUser)
{
        if ( serverlist->AddServer(srv, fromUser) ) {
                Notify_ServerAdd(srv);
                return true;
        }
        return false;
}


CI2PAddress CamuleApp::GetPublicDest ( bool ignorelocal ) const
{
        if ( !m_dwPublicDest ) {
                if ( Kademlia::CKademlia::IsConnected() && Kademlia::CKademlia::GetIPAddress().isValid() ) {
                        return Kademlia::CKademlia::GetIPAddress();
                } else {
                        return ignorelocal ? CI2PAddress::null : m_localdest;
                }
        }

        return m_dwPublicDest;
}


void CamuleApp::SetPublicDest ( const CI2PAddress & dwDest )
{
        wxASSERT ( dwDest.isValid() );
        m_dwPublicDest = dwDest;
	m_localdest = dwDest; // iMule. I don't know where else to initialize this
}


wxString CamuleApp::GetLog(bool reset)
{
        ConfigDir = CConfigDir::GetConfigDir();
        wxFile logfile;
        logfile.Open(ConfigDir + wxT("logfile"));
        if ( !logfile.IsOpened() ) {
                return _("ERROR: can't open logfile");
        }
        int len = logfile.Length();
        if ( len == 0 ) {
                return _("WARNING: logfile is empty. Something is wrong.");
        }
        char *tmp_buffer = new char[len + sizeof(wxChar)];
        logfile.Read(tmp_buffer, len);
        memset(tmp_buffer + len, 0, sizeof(wxChar));

        // try to guess file format
        wxString str;
        if (tmp_buffer[0] && tmp_buffer[1]) {
                str = wxString(UTF82unicode(tmp_buffer));
        } else {
                str = wxWCharBuffer((wchar_t *)tmp_buffer);
        }

        delete [] tmp_buffer;
        if ( reset ) {
                theLogger.CloseLogfile();
                if (theLogger.OpenLogfile(ConfigDir + wxT("logfile"))) {
                        AddLogLineN(_("Log has been reset"));
                }
                ECServerHandler->ResetAllLogs();
        }
        return str;
}


wxString CamuleApp::GetServerLog(bool reset)
{
        wxString ret = server_msg;
        if ( reset ) {
                server_msg.Clear();
        }
        return ret;
}

wxString CamuleApp::GetDebugLog(bool reset)
{
        return GetLog(reset);
}


void CamuleApp::AddServerMessageLine(wxString &msg)
{
        server_msg += msg + wxT("\n");
        AddLogLineN(CFormat(_("ServerMessage: %s")) % msg);
}



void CamuleApp::OnFinishedHTTPDownload(CMuleInternalEvent& event)
{
        switch (event.GetInt()) {
        case HTTP_IPFilter:
                ipfilter->DownloadFinished(event.GetExtraLong());
                break;
        case HTTP_ServerMet:
                serverlist->DownloadFinished(event.GetExtraLong());
                break;
        case HTTP_ServerMetAuto:
                serverlist->AutoDownloadFinished(event.GetExtraLong());
                break;
        case HTTP_VersionCheck:
                CheckNewVersion(event.GetExtraLong());
                break;
        case HTTP_NodesDat:
                if (event.GetExtraLong() == HTTP_Success) {

                        wxString file = ConfigDir + wxT("nodes.dat");
                        if (wxFileExists(file)) {
                                wxRemoveFile(file);
                        }

                        if ( Kademlia::CKademlia::IsRunning() ) {
                                Kademlia::CKademlia::Stop();
                        }

                        wxRenameFile(file + wxT(".download"),file);

                        Kademlia::CKademlia::Start();
                        theApp->ShowConnectionState();

                } else if (event.GetExtraLong() == HTTP_Skipped) {
                        AddLogLineN(CFormat(_("Skipped download of %s, because requested file is not newer.")) % wxT("nodes.dat"));
                } else {
                        AddLogLineC(_("Failed to download the nodes list."));
                }
                break;
#ifdef ENABLE_IP2COUNTRY
        case HTTP_GeoIP:
                theApp->amuledlg->IP2CountryDownloadFinished(event.GetExtraLong());
                // If we updated, the dialog is already up. Redraw it to show the flags.
                theApp->amuledlg->Refresh();
                break;
#endif
        }
}

void CamuleApp::CheckNewVersion(uint32 result)
{
        if (result == HTTP_Success) {
                wxString filename = ConfigDir + wxT("last_version_check");
                wxTextFile file;

                if (!file.Open(filename)) {
                        AddLogLineC(_("Failed to open the downloaded version check file") );
                        return;
                } else if (!file.GetLineCount()) {
                        AddLogLineC(_("Corrupted version check file"));
                } else {
                        wxString versionLine = file.GetFirstLine();
                        wxStringTokenizer tkz(versionLine, wxT("."));

                        AddDebugLogLineN(logGeneral, wxString(wxT("Running: ")) + wxT(VERSION) + wxT(", Version check: ") + versionLine);

                        long fields[] = {0, 0, 0, 0};
                        for (int i = 0; i < 4; ++i) {
                                if (!tkz.HasMoreTokens()) {
                                        AddLogLineC(_("Corrupted version check file"));
                                        return;
                                } else {
                                        wxString token = tkz.GetNextToken();

                                        if (!token.ToLong(&fields[i])) {
                                                AddLogLineC(_("Corrupted version check file"));
                                                return;
                                        }
                                }
                        }

                        long curVer = get_full_ed2k_version();
                        long newVer = make_full_ed2k_version(fields[0], fields[1], fields[2], fields[3]);

                        if (curVer < newVer) {
                                AddLogLineC(_("You are using an outdated version of iMule!"));
                                AddLogLineN(CFormat(_("Your iMule version is %s and the latest version is %s")) % wxT(VERSION) % versionLine);
                                AddLogLineN(_("The latest version can always be found at http://www.imule.i2p"));
#ifdef AMULE_DAEMON
                                printf("%s\n", (const char*)unicode2UTF8(wxString::Format(
                                                        _("WARNING: Your iMuled version is outdated: %s < %s"),
                                                        VERSION, versionLine.c_str())));
#endif
                        } else {
                                AddLogLineN(_("Your copy of iMule is up to date."));
                        }
                }

                file.Close();
                wxRemoveFile(filename);
        } else {
                AddLogLineC(_("Failed to download the version check file"));
        }

}


bool CamuleApp::IsConnected() const
{
        return (IsConnectedED2K() || IsConnectedKad());
}


bool CamuleApp::IsConnectedED2K() const
{
        return serverconnect && serverconnect->IsConnected();
}


bool CamuleApp::IsConnectedKad() const
{
        return Kademlia::CKademlia::IsConnected();
}


bool CamuleApp::IsFirewalled() const
{
        if (theApp->IsConnectedED2K() && !theApp->serverconnect->IsLowID()) {
                return false; // we have an eD2K HighID -> not firewalled
        }

        return IsFirewalledKad(); // If kad says ok, it's ok.
}

bool CamuleApp::IsFirewalledKad() const
{
        return !Kademlia::CKademlia::IsConnected()		// not connected counts as firewalled
               || Kademlia::CKademlia::IsFirewalled();
}

bool CamuleApp::IsFirewalledKadUDP() const
{
        return !Kademlia::CKademlia::IsConnected()		// not connected counts as firewalled
               || Kademlia::CUDPFirewallTester::IsFirewalledUDP(true);
}

bool CamuleApp::IsKadRunning() const
{
        return Kademlia::CKademlia::IsRunning();
}

bool CamuleApp::IsKadRunningInLanMode() const
{
        return Kademlia::CKademlia::IsRunningInLANMode();
}

// Kad stats
uint32 CamuleApp::GetKadUsers() const
{
        return Kademlia::CKademlia::GetKademliaUsers();
}

uint32 CamuleApp::GetKadFiles() const
{
        return Kademlia::CKademlia::GetKademliaFiles();
}

uint32 CamuleApp::GetKadIndexedSources() const
{
        return Kademlia::CKademlia::GetIndexed()->m_totalIndexSource;
}

uint32 CamuleApp::GetKadIndexedKeywords() const
{
        return Kademlia::CKademlia::GetIndexed()->m_totalIndexKeyword;
}

uint32 CamuleApp::GetKadIndexedNotes() const
{
        return Kademlia::CKademlia::GetIndexed()->m_totalIndexNotes;
}

uint32 CamuleApp::GetKadIndexedLoad() const
{
        return Kademlia::CKademlia::GetIndexed()->m_totalIndexLoad;
}


// True IP of machine
CI2PAddress CamuleApp::GetKadIPAdress() const
{
        return Kademlia::CKademlia::GetIPAddress();
}


void CamuleApp::ShowUserCount()
{
        uint32 totaluser = 0, totalfile = 0;

        theApp->serverlist->GetUserFileStatus( totaluser, totalfile );

        wxString buffer;

        static const wxString s_singlenetstatusformat = _("Users: %s | Files: %s");
        static const wxString s_bothnetstatusformat = _("Users: E: %s K: %s | Files: E: %s K: %s");

        if (thePrefs::GetNetworkED2K() && thePrefs::GetNetworkKademlia()) {
                buffer = CFormat(s_bothnetstatusformat) % CastItoIShort(totaluser) % CastItoIShort(Kademlia::CKademlia::GetKademliaUsers()) % CastItoIShort(totalfile) % CastItoIShort(Kademlia::CKademlia::GetKademliaFiles());
        } else if (thePrefs::GetNetworkED2K()) {
                buffer = CFormat(s_singlenetstatusformat) % CastItoIShort(totaluser) % CastItoIShort(totalfile);
        } else if (thePrefs::GetNetworkKademlia()) {
                buffer = CFormat(s_singlenetstatusformat) % CastItoIShort(Kademlia::CKademlia::GetKademliaUsers()) % CastItoIShort(Kademlia::CKademlia::GetKademliaFiles());
        } else {
                buffer = _("No networks selected");
        }

        Notify_ShowUserCount(buffer);
}


void CamuleApp::ListenSocketHandler ( CI2PSocketEvent& event )
{
        { wxCHECK_RET(listensocket, wxT("Connection-event for NULL'd listen-socket")); }
        {
                wxCHECK_RET(event.GetSocketEvent() == wxSOCKET_CONNECTION || event.GetSocketEvent() == wxSOCKET_LOST,
                            wxT("Invalid event received for listen-socket"));
        }

        { wxCHECK_RET ( dynamic_cast<CListenSocket *>(event.GetEventObject()) == listensocket, wxT( "CamuleApp::ListenSocketHandler received an unattended event" ) ); }


        switch ( event.GetSocketEvent() ) {
        case wxSOCKET_CONNECTION :
                if (m_app_state == APP_STATE_RUNNING) {
                        listensocket->OnAccept(0);
                } else if (m_app_state == APP_STATE_STARTING) {
                        // When starting up, connection may be made before we are able
                        // to handle them. However, if these are ignored, no futher
                        // connection-events will be triggered, so we have to accept it.
                        wxI2PSocketClient * socket = listensocket->Accept ( false );

                        wxCHECK_RET(socket, wxT("NULL returned by Accept() during startup"));

                        socket->Destroy();
                }
                break ;

        case wxSOCKET_LOST :
                if ( m_app_state == APP_STATE_RUNNING ) {
                        AddLogLineC( _("iMule has just been disconnected from the I2P router. Restarting network...") );
                        StopNetwork() ;
                        StartNetwork() ;
                }

        default:
                break;
        }
}


void CamuleApp::ShowConnectionState(bool forceUpdate)
{
        static uint8 old_state = (1<<7); // This flag doesn't exist

        uint8 state = 0;

        if (theApp->serverconnect->IsConnected()) {
                state |= CONNECTED_ED2K;
        }

        if (Kademlia::CKademlia::IsRunning()) {
                if (Kademlia::CKademlia::IsConnected()) {
                        if (!Kademlia::CKademlia::IsFirewalled()) {
                                state |= CONNECTED_KAD_OK;
                        } else {
                                state |= CONNECTED_KAD_FIREWALLED;
                        }
                } else {
                        state |= CONNECTED_KAD_NOT;
                }
        }


        if (old_state != state) {
                // Get the changed value
                int changed_flags = old_state ^ state;

                if (changed_flags & CONNECTED_ED2K) {
                        // ED2K status changed
                        wxString connected_server;
                        CServer* ed2k_server = theApp->serverconnect->GetCurrentServer();
                        if (ed2k_server) {
                                connected_server = ed2k_server->GetListName();
                        }
                        if (state & CONNECTED_ED2K) {
                                // We connected to some server
                                const wxString id = theApp->serverconnect->IsLowID() ? _("with LowID") : _("with HighID");

                                AddLogLineC(CFormat(_("Connected to %s %s")) % connected_server % id);
                        } else {
                                if ( theApp->serverconnect->IsConnecting() ) {
                                        AddLogLineC(CFormat(_("Connecting to %s")) % connected_server);
                                } else {
                                        AddLogLineC(_("Disconnected from eD2k"));
                                }
                        }
                }

                if (changed_flags & CONNECTED_KAD_NOT) {
                        if (state & CONNECTED_KAD_NOT) {
                                AddLogLineC(_("Kad started."));
                        } else {
                                AddLogLineC(_("Kad stopped."));
                        }
                }

                if (changed_flags & (CONNECTED_KAD_OK | CONNECTED_KAD_FIREWALLED)) {
                        if (state & (CONNECTED_KAD_OK | CONNECTED_KAD_FIREWALLED)) {
                                if (state & CONNECTED_KAD_OK) {
                                        AddLogLineC(_("Connected to Kad (ok)"));
                                } else {
                                        AddLogLineC(_("Connected to Kad (firewalled)"));
                                }
                        } else {
                                AddLogLineC(_("Disconnected from Kad"));
                        }
                }

                old_state = state;

                theApp->downloadqueue->OnConnectionState(IsConnected());
        }

        ShowUserCount();
        Notify_ShowConnState(forceUpdate);
}


void CamuleApp::UDPSocketHandler ( CI2PSocketEvent& event )
{
        CMuleUDPSocket* socket = (CMuleUDPSocket*)(event.GetClientData());
        wxCHECK_RET(socket, wxT("No socket owner specified."));

        if (IsOnShutDown() || thePrefs::IsUDPDisabled()) return;

        if (!IsRunning()) {
                if (event.GetSocketEvent() == wxSOCKET_INPUT) {
                        // Back to the queue!
                        theApp->AddPendingEvent(event);
                        return;
                }
        }

        switch (event.GetSocketEvent()) {
        case wxSOCKET_INPUT:
                socket->OnReceive(0);
                break;

        case wxSOCKET_OUTPUT:
                socket->OnSend(0);
                break;

        case wxSOCKET_LOST:
                AddDebugLogLineC(logGeneral, wxT("UDP socket lost. Restarting network.") ) ;
                theApp->RestartNetworkIfStarted() ;
                break;

        default:
                wxFAIL;
                break;
        }
}


void CamuleApp::OnUnhandledException()
{
        // Call the generic exception-handler.
        fprintf(stderr, "\tiMule Version: %s\n", (const char*)unicode2char(GetFullMuleVersion()));
        ::OnUnhandledException();
}

void CamuleApp::StartKad()
{
        if (!Kademlia::CKademlia::IsRunning() && thePrefs::GetNetworkKademlia()) {
                // Kad makes no sense without the Client-UDP socket.
                if (!thePrefs::IsUDPDisabled()) {
                        if (ipfilter->IsReady()) {
                                Kademlia::CKademlia::Start();
                                ShowConnectionState();
                        } else {
                                ipfilter->StartKADWhenReady();
                        }
                } else {
                        AddLogLineC(_("Kad network cannot be used if UDP port is disabled on preferences, not starting."));
                }
        } else if (!thePrefs::GetNetworkKademlia()) {
                AddLogLineC(_("Kad network disabled on preferences, not connecting."));
        }
}

void CamuleApp::StopKad()
{
        // Stop Kad if it's running
        if (Kademlia::CKademlia::IsRunning()) {
                Kademlia::CKademlia::Stop();
        }
}


void CamuleApp::BootstrapKad(const CI2PAddress & dest)
{
        if (NetworkStarted()) {
                if (!Kademlia::CKademlia::IsRunning()) {
                        Kademlia::CKademlia::Start();
                        theApp->ShowConnectionState();
                }

                Kademlia::CKademlia::Bootstrap(dest);
        }
}

void CamuleApp::exportKadContactsOnFile(const wxString & filename) const
{
        CFile f(filename);
        f.Open(filename, CFile::write);
        f.WriteString(Kademlia::CKademlia::GetRoutingZone()->exportContacts());
        f.Close();
}

void CamuleApp::UpdateNotesDat(const wxString& url)
{
        wxString strTempFilename(theApp->ConfigDir + wxT("nodes.dat.download"));

        CHTTPDownloadCoroutine *downloader = new CHTTPDownloadCoroutine(url, strTempFilename, theApp->ConfigDir + wxT("nodes.dat"), HTTP_NodesDat, true, false);
        downloader->Create();
        downloader->Start();
}


void CamuleApp::DisconnectED2K()
{
        // Stop ED2K if it's running
        if (IsConnectedED2K()) {
                serverconnect->Disconnect();
        }
}

bool CamuleApp::CryptoAvailable() const
{
        return clientcredits && clientcredits->CryptoAvailable();
}

uint32 CamuleApp::GetED2KID() const
{
        return serverconnect ? serverconnect->GetClientID() : 0;
}

uint32 CamuleApp::GetID() const
{
        uint32 ID;

        if( Kademlia::CKademlia::IsConnected() && !Kademlia::CKademlia::IsFirewalled() ) {
                // We trust Kad above ED2K
                ID = ENDIAN_NTOHL(Kademlia::CKademlia::GetIPAddress());
        } else if( theApp->serverconnect->IsConnected() ) {
                ID = theApp->serverconnect->GetClientID();
        } else if ( Kademlia::CKademlia::IsConnected() && Kademlia::CKademlia::IsFirewalled() ) {
                // A firewalled Kad client get's a "1"
                ID = 1;
        } else {
                ID = 0;
        }

        return ID;
}

CI2PAddress CamuleApp::GetUdpDest() const
{
        CI2PAddress dest;
        if (clientudp)
                clientudp->GetLocal ( dest );
        return dest;
}

CI2PAddress CamuleApp::GetTcpDest() const

{
        CI2PAddress dest;
        if (listensocket)
                listensocket->GetLocal ( dest );
        return dest;
}

void CamuleApp::StartNetwork()
{
        m_connection_manager->start() ;
        theApp->ShowConnectionState();
}

void CamuleApp::StopNetwork()
{        
        //disconnect the ED2K servers
        if ( thePrefs::GetNetworkED2K() ) {
                if ( theApp->serverconnect->IsConnecting() ) {
                        theApp->serverconnect->StopConnectionTry();
                } else {
                        theApp->serverconnect->Disconnect();
                }
        }

        // Disconnect Kad
        if ( thePrefs::GetNetworkKademlia() ) {
                theApp->StopKad();
        }

        // stop and kill connections
        m_connection_manager->stop() ;

//	clientlist->Process();

        // update display
        theApp->ShowConnectionState();
}

void CamuleApp::RestartNetworkIfStarted()
{
        bool restart = NetworkStarted() or NetworkStarting() ;
        StopNetwork();
        if (restart)
                StartNetwork();
}

void CamuleApp::StartSearchNetwork()
{
        if (thePrefs::DoAutoConnect()) {
                // Tell it to autoconnect.
                if ( thePrefs::GetNetworkED2K() ) {
                        AddLogLineN (_ ( "Autoconnect to the ED2K servers (see Preferences if you want to disable it)." ) );

                        theApp->serverconnect->ConnectToAnyServer();
                }
                if ( thePrefs::GetNetworkKademlia() ) {
                        AddLogLineN (_ ( "Autoconnect to the Kad indexing network (see Preferences if you want to disable it)." ) );

                        theApp->StartKad();
                }
        }
}

void CamuleApp::FetchIfNewVersionIsAvailable()
{
        // Test if there's any new version
        if ( thePrefs::GetCheckNewVersion() ) {
                // We use the thread base because I don't want a dialog to pop up.
                CHTTPDownloadCoroutine* version_check =
                        new CHTTPDownloadCoroutine ( wxT ( "http://www.imule.i2p/lastversion" ),
                                                  theApp->ConfigDir + wxT ( "last_version_check" ), theApp->ConfigDir + wxT ( "last_version_check" ), HTTP_VersionCheck, false, true );
                version_check->Create();
                version_check->Start();
        }
}

wxString CamuleApp::GetKadContacts() const
{
        return Kademlia::CKademlia::GetRoutingZone()->exportContacts();
}

DEFINE_LOCAL_EVENT_TYPE ( wxEVT_MULE_NOTIFY_EVENT )

DEFINE_LOCAL_EVENT_TYPE ( wxEVT_CORE_FILE_HASHING_FINISHED )
DEFINE_LOCAL_EVENT_TYPE ( wxEVT_CORE_FILE_HASHING_SHUTDOWN )
DEFINE_LOCAL_EVENT_TYPE ( wxEVT_CORE_FINISHED_FILE_COMPLETION )
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_UDP_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SERVER_DNS_DONE)
// File_checked_for_headers
