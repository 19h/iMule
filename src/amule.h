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


#ifndef AMULE_H
#define AMULE_H


#include <wx/app.h>		// Needed for wxApp
#include <wx/intl.h>		// Needed for wxLocale

#include "i2p/wxI2PEvents.h"
#include "i2p/CI2PAddress.h"

#include "Types.h"		// Needed for int32, uint16 and uint64
#include <map>
#ifndef __WXMSW__
#include <signal.h>
#if !wxCHECK_VERSION(3, 0, 0)
#include <wx/unix/execute.h>
#endif
#endif // __WXMSW__

#include <memory>

class CAbstractFile;
class CKnownFile;
class ExternalConn;
class CamuleDlg;
class CPreferences;
class CDownloadQueue;
class CUploadQueue;
class CServerConnect;
class CSharedFileList;
class CServer;
class CFriend;
class CMD4Hash;
class CServerList;
class CI2PRouter;
class CListenSocket;
class CClientList;
class CKnownFileList;
class CCanceledFileList;
class CSearchList;
class CClientCreditsList;
class CFriendList;
class CClientUDPSocket;
class CIPFilter;
class UploadBandwidthThrottler;
#ifdef ENABLE_UPNP
class CUPnPControlPoint;
class CUPnPPortMapping;
#endif
class CStatistics;
class wxSocketEvent;
class wxTimer;
class wxTimerEvent;
class wxCommandEvent;
class wxCloseEvent;
class wxFFileOutputStream;
class wxSingleInstanceChecker;
class CHashingEvent;
class CMuleInternalEvent;
class I2PConnectionManager ;
class CCompletionEvent;
class CAllocFinishedEvent;
class wxExecuteData;
class CLoggingEvent;

class wxCloseEvent;

namespace MuleNotify
{
class CMuleGUIEvent;
}


using MuleNotify::CMuleGUIEvent;


#ifdef AMULE_DAEMON
#define AMULE_APP_BASE wxAppConsole
//#define AMULE_APP_BASE wxApp
#define CORE_TIMER_PERIOD 300
#else
#define AMULE_APP_BASE wxApp
#define CORE_TIMER_PERIOD 100
#endif

#define CONNECTED_ED2K                  (0x01)
#define CONNECTED_KAD_NOT               (0x02)
#define CONNECTED_KAD_OK                (0x04)
#define CONNECTED_KAD_FIREWALLED        (0x08)
#define CONNECTED_KAD_RUNNING           (0x10)
#define CONNECTED_NETWORK_STARTING      (0x20)
#define CONNECTED_NETWORK_STARTED       (0x40)


// Base class common to amule, aamuled and amulegui
class CamuleAppCommon
{
private:
        // Used to detect a previous running instance of aMule
        wxSingleInstanceChecker*	m_singleInstance;

        bool		CheckPassedLink(const wxString &in, wxString &out, int cat);
protected:
        char		*strFullMuleVersion;
        char		*strOSDescription;
        wxString	OSType;
        bool		enable_daemon_fork;
        bool		ec_config;
        bool		m_skipConnectionDialog;
        bool		m_geometryEnabled;
        wxString	m_geometryString;
        wxString	m_logFile;
        wxString	m_appName;
        wxString	m_PidFile;

        bool		InitCommon(int argc, wxChar ** argv);
        void		RefreshSingleInstanceChecker();
        bool		CheckMuleDirectory(const wxString& desc, const class CPath& directory, const wxString& alternative, class CPath& outDir);
public:
        wxString	ConfigDir;
        wxString	m_configFile;

        CamuleAppCommon();
        ~CamuleAppCommon();
        void		AddLinksFromFile();
        // URL functions
        wxString	CreateMagnetLink(const CAbstractFile *f);
        wxString	CreateED2kLink(const CAbstractFile* f, bool add_source = false, bool use_hostname = false, bool addcryptoptions = false);
        wxString	CreateED2kAICHLink(const CKnownFile* f);
        // Who am I ?
#ifdef AMULE_DAEMON
        bool		IsDaemon() const { return true; }
#else
        bool		IsDaemon() const { return false; }
#endif

#ifdef CLIENT_GUI
        bool		IsRemoteGui() const { return true; }
#else
        bool		IsRemoteGui() const { return false; }
#endif

        const wxString&	GetMuleAppName() const { return m_appName; }
        const wxString	GetFullMuleVersion() const;
};

class CamuleApp : public AMULE_APP_BASE, public CamuleAppCommon
{
        friend class I2PConnectionManager ;
private:
        enum APPState {
                APP_STATE_RUNNING = 0,
                APP_STATE_SHUTTINGDOWN,
                APP_STATE_STARTING
        };

public:
        CamuleApp();
        virtual	 ~CamuleApp();

        virtual bool	OnInit();
        int		OnExit();
#if wxUSE_ON_FATAL_EXCEPTION
        void		OnFatalException();
#endif
        bool        InitializeNetwork(wxString *msg);

        void        StopNetwork() ;
        void        StartNetwork();
        void        RestartNetworkIfStarted();
        void        StartSearchNetwork();
        void        FetchIfNewVersionIsAvailable();
        wxString        GetKadContacts() const;

        // derived classes may override those
        virtual int InitGui(bool geometry_enable, wxString &geometry_string);

        // Socket handlers
        void ListenSocketHandler(CI2PSocketEvent& event);
        void ServerSocketHandler(CI2PSocketEvent& event);
        void UDPSocketHandler(CI2PSocketEvent& event);

        virtual int ShowAlert(wxString msg, wxString title, int flags) = 0;

        // Barry - To find out if app is running or shutting/shut down
        bool IsRunning() const { return (m_app_state == APP_STATE_RUNNING); }
        bool IsOnShutDown() const { return (m_app_state == APP_STATE_SHUTTINGDOWN); }

        bool NetworkStarting() {return networkStarting;}

        bool NetworkStarted() {return networkStarted;}

        // NOTE : i2P : no firewall, no LowID
        // Check ED2K and Kademlia state
        bool IsFirewalled() const;
        // Are we connected to at least one network?
        bool IsConnected() const;

        // Connection to ED2K
        bool IsConnectedED2K() const;

        // What about Kad? Is it running?
        bool IsKadRunning() const;
        // Connection to Kad
        bool IsConnectedKad() const;
        // Check Kad state (TCP)
        bool IsFirewalledKad() const;
        // Check Kad state (UDP)
        bool IsFirewalledKadUDP() const;
        // Check Kad state (LAN mode)
        bool IsKadRunningInLanMode() const;
        // Kad stats
        uint32	GetKadUsers() const;
        uint32	GetKadFiles() const;
        uint32	GetKadIndexedSources() const;
        uint32	GetKadIndexedKeywords() const;
        uint32	GetKadIndexedNotes() const;
        uint32	GetKadIndexedLoad() const;
        // True IP of machine
        CI2PAddress	GetKadIPAdress() const;

        // Misc functions
        void		OnlineSig(bool zero = false);
        void		Localize_mule();
        void		Trigger_New_version(wxString newMule);

        // shakraw - new EC code using wxSocketBase
        ExternalConn*	ECServerHandler;

        // return current (valid) public IP or 0 if unknown
        // If ignorelocal is true, don't use m_localip
        CI2PAddress		GetPublicDest ( bool ignorelocal = false ) const;
        void		SetPublicDest ( const CI2PAddress & dwDest );

        uint32	GetED2KID() const;
        uint32	GetID() const;
        CI2PAddress	GetUdpDest() const;
        CI2PAddress	GetTcpDest() const;

        // Other parts of the interface and such
        CPreferences*		glob_prefs;
        CDownloadQueue*		downloadqueue;
        CUploadQueue*		uploadqueue;
        // No server yet in imule
        CServerConnect*		serverconnect;
        CSharedFileList*	sharedfiles;
        CServerList*		serverlist;
#ifdef INTERNAL_ROUTER
        CI2PRouter*             i2prouter;
#endif
        CListenSocket*		listensocket;
        CClientList*		clientlist;
        CKnownFileList*		knownfiles;
        CCanceledFileList*	canceledfiles;
        CSearchList*		searchlist;
        CClientCreditsList*	clientcredits;
        CFriendList*		friendlist;
        CClientUDPSocket*	clientudp;
        CStatistics*		m_statistics;
        CIPFilter*		ipfilter;
        UploadBandwidthThrottler* uploadBandwidthThrottler;
#ifdef ENABLE_UPNP
        CUPnPControlPoint*	m_upnp;
        std::vector<CUPnPPortMapping> m_upnpMappings;
#endif
        wxLocale m_locale;

        void ShutDown();

        wxString GetLog(bool reset = false);
        wxString GetServerLog(bool reset = false);
        wxString GetDebugLog(bool reset = false);

        bool AddServer(CServer *srv, bool fromUser = false);
        void AddServerMessageLine(wxString &msg);
#ifdef INTERNAL_ROUTER
        virtual void UpdateRouterStatus( wxString & msg ) {}
        virtual void UpdateRouterStatus();
#endif
#ifdef __DEBUG__
        void AddSocketDeleteDebug(uint32 socket_pointer, uint32 creation_time);
#endif
        void SetOSFiles(const wxString new_path);


        const wxString& GetOSType() const { return OSType; }

        void ShowUserCount();

        void ShowConnectionState(bool forceUpdate = false);

        void StartKad();
        void StopKad();

        /** Bootstraps kad from the specified destination. */
        void BootstrapKad(const CI2PAddress & dest);
        void exportKadContactsOnFile(const wxString & filename) const;
        /** Updates the nodes.dat file from the specified url. */
        void UpdateNotesDat(const wxString& str);


        void DisconnectED2K();
        void SetNetworkStopped();
        void SetNetworkStarted();
        void SetNetworkStarting() {networkStarted=not(networkStarting=true);}
        
        bool CryptoAvailable() const;

protected:

#ifdef __WXDEBUG__
        /**
         * Handles asserts in a thread-safe manner.
         */
        virtual void OnAssertFailure(const wxChar* file, int line,
                                     const wxChar* func, const wxChar* cond, const wxChar* msg);
#endif


        void OnUDPDnsDone(CMuleInternalEvent& evt);
        void OnSourceDnsDone(CMuleInternalEvent& evt);
        void OnServerDnsDone(CMuleInternalEvent& evt);

        void OnTCPTimer(wxTimerEvent& evt);
        void OnCoreTimer(wxTimerEvent& evt);

        void OnFinishedHashing(CHashingEvent& evt);
        void OnFinishedAICHHashing(CHashingEvent& evt);
        void OnFinishedCompletion(CCompletionEvent& evt);
        void OnFinishedAllocation(CAllocFinishedEvent& evt);
        void OnFinishedHTTPDownload(CMuleInternalEvent& evt);
        void OnHashingShutdown(CMuleInternalEvent&);
        void OnNotifyEvent(CMuleGUIEvent& evt);

        void SetTimeOnTransfer();


        APPState m_app_state;

        std::unique_ptr<I2PConnectionManager> m_connection_manager ;
        wxString m_emulesig_path;
        wxString m_amulesig_path;


        CI2PAddress m_dwPublicDest;

        long webserver_pid;

        wxString server_msg;

        wxTimer* core_timer;

private:
        virtual void OnUnhandledException();

        void CheckNewVersion(uint32 result);

        CI2PAddress m_localdest;

        bool networkStarted ;

        bool networkStarting ;
};


#ifndef AMULE_DAEMON

class CamuleGuiBase
{
public:
        CamuleGuiBase();
        virtual	 ~CamuleGuiBase();

        wxString	m_FrameTitle;
        CamuleDlg*	amuledlg;
        int m_FileDetailDialogActive;

        bool CopyTextToClipboard( wxString strText );
        void ResetTitle();

        virtual int InitGui(bool geometry_enable, wxString &geometry_string);
        virtual int ShowAlert(wxString msg, wxString title, int flags);

        void AddGuiLogLine(const wxString& line);
protected:
        /**
         * This list is used to contain log messages that are to be displayed
         * on the GUI, when it is currently impossible to do so. This is in order
         * to allows us to queue messages till after the dialog has been created.
         */
        std::list<wxString> m_logLines;
};


#ifndef CLIENT_GUI


class CamuleGuiApp : public CamuleApp, public CamuleGuiBase
{

        virtual int InitGui(bool geometry_enable, wxString &geometry_string);

        int OnExit();
        bool OnInit();

public:

        virtual int ShowAlert(wxString msg, wxString title, int flags);

        void ShutDown(wxCloseEvent &evt);

        wxString GetLog(bool reset = false);
        wxString GetServerLog(bool reset = false);
        void AddServerMessageLine(wxString &msg);
#ifdef INTERNAL_ROUTER
        void UpdateRouterStatus( wxString & msg );
        void UpdateRouterStatus();
#endif
        DECLARE_EVENT_TABLE()
};

typedef CamuleGuiApp AMULE_APP;
DECLARE_APP(CamuleGuiApp)
extern CamuleGuiApp *theApp;


#else /* !CLIENT_GUI */


#include "amule-remote-gui.h"


#endif // CLIENT_GUI


#define CALL_APP_DATA_LOCK


#else /* ! AMULE_DAEMON */

// wxWidgets 2.8 requires special code for event handling and sockets.
// 2.9 doesn't, so standard event loop and sockets can be used
//
// Windows: aMuled compiles with 2.8 (without the special code),
// but works only with 2.9

#if !wxCHECK_VERSION(2, 9, 0)
#ifdef __WXMSW__
// MSW: can't run amuled with 2.8 anyway, just get it compiled
#define AMULED_DUMMY
#else
#define AMULED28
#error AMULED28 defined
#endif
#endif

#if wxCHECK_VERSION(3, 0, 0)
#define AMULED_NOAPPTRAIT
#endif
#ifdef __WXMSW__
#define AMULED_NOAPPTRAIT
#endif


#ifdef AMULED28
#include <wx/socket.h>


class CSocketSet;


class CAmuledGSocketFuncTable : public GSocketGUIFunctionsTable
{
private:
        CSocketSet *m_in_set, *m_out_set;

        wiMutex m_lock;
public:
        CAmuledGSocketFuncTable();

        void AddSocket(GSocket *socket, GSocketEvent event);
        void RemoveSocket(GSocket *socket, GSocketEvent event);
        void RunSelect();

        virtual bool OnInit();
        virtual void OnExit();
        virtual bool CanUseEventLoop();
        virtual bool Init_Socket(GSocket *socket);
        virtual void Destroy_Socket(GSocket *socket);
        virtual void Install_Callback(GSocket *socket, GSocketEvent event);
        virtual void Uninstall_Callback(GSocket *socket, GSocketEvent event);
        virtual void Enable_Events(GSocket *socket);
        virtual void Disable_Events(GSocket *socket);
};


#endif // AMULED28

// no AppTraits used on Windows
#ifndef AMULED_NOAPPTRAIT

typedef std::map<int, class wxEndProcessData *> EndProcessDataMap;

#include <wx/apptrait.h>

class CDaemonAppTraits : public wxConsoleAppTraits
{
private:
        struct sigaction m_oldSignalChildAction;
        struct sigaction m_newSignalChildAction;

#ifdef AMULED28
        CAmuledGSocketFuncTable *m_table;
        wiMutex m_lock;
        std::list<wxObject *> m_sched_delete;

public:
        CDaemonAppTraits(CAmuledGSocketFuncTable *table);
        virtual GSocketGUIFunctionsTable* GetSocketGUIFunctionsTable();
        virtual void ScheduleForDestroy(wxObject *object);
        virtual void RemoveFromPendingDelete(wxObject *object);

        void DeletePending();
#else	// AMULED28
public:
        CDaemonAppTraits();
#endif	// !AMULED28

        virtual int WaitForChild(wxExecuteData& execData);
#if defined(__WXMAC__) && !wxCHECK_VERSION(2, 9, 0)
        virtual wxStandardPathsBase& GetStandardPaths();
#endif
};


void OnSignalChildHandler(int signal, siginfo_t *siginfo, void *ucontext);
pid_t AmuleWaitPid(pid_t pid, int *status, int options, wxString *msg);
#endif // AMULED_NOAPPTRAIT


class CamuleDaemonApp : public CamuleApp
{
private:
#ifdef AMULED28
        bool m_Exit;
        CAmuledGSocketFuncTable *m_table;
#endif

        bool OnInit();
        int OnRun();
        int OnExit();
        void OnCoreTimer(wxTimerEvent& evt) {CamuleApp::OnCoreTimer(evt);}
        virtual int InitGui(bool geometry_enable, wxString &geometry_string);

#ifndef AMULED_NOAPPTRAIT
        struct sigaction m_oldSignalChildAction;
        struct sigaction m_newSignalChildAction;
public:
        wxAppTraits *CreateTraits();
#endif // AMULED_NOAPPTRAIT

public:

#ifdef AMULED28
        CamuleDaemonApp();

        void ExitMainLoop() { m_Exit = true; }
#endif

#ifdef AMULED_DUMMY
        void ExitMainLoop() {}
#endif

        bool CopyTextToClipboard(wxString strText);

        virtual int ShowAlert(wxString msg, wxString title, int flags);

        DECLARE_EVENT_TABLE()

};
typedef CamuleDaemonApp AMULE_APP;
DECLARE_APP(CamuleDaemonApp)
extern CamuleDaemonApp *theApp;

#endif /* ! AMULE_DAEMON */

#endif // AMULE_H
// File_checked_for_headers
