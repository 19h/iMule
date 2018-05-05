AM_CPPFLAGS = $(MULECPPFLAGS)
AM_CFLAGS = $(MULECFLAGS)
AM_CXXFLAGS = $(MULECXXFLAGS)
AM_LDFLAGS = $(MULELDFLAGS)

GCJCOMPILE = $(GCJ)   $(GCJFLAGS)
GCJLD      = $(GCJ)
GCJLINK    = $(GCJLD) $(AM_LDFLAGS) $(LDFLAGS) -o $@

SUBDIRS = libs utils pixmaps kademlia


if INSTALL_SKINS
SUBDIRS += skins
endif

EXTRA_DIST = \
	iMule.xpm \
	Parser.y \
	Scanner.h.in \
	Scanner.l \
	IPFilterScanner.l \
	muuli.wdr 

bin_PROGRAMS = 

if ROUTER
bin_PROGRAMS += i2prouter
endif
if MONOLITHIC
bin_PROGRAMS += imule
endif

if COMPILE_CMD
bin_PROGRAMS += imulecmd
endif

if ED2K
bin_PROGRAMS += ed2k
endif

if AMULE_GUI
bin_PROGRAMS += imulegui
endif

if AMULE_DAEMON
bin_PROGRAMS +=imuled
endif

# Sources
noinst_LIBRARIES =
if MONOLITHIC
noinst_LIBRARIES += libmuleappcore.a libmuleappgui.a libmuleappcommon.a
else
if AMULE_GUI
noinst_LIBRARIES += libmuleappcore.a libmuleappgui.a libmuleappcommon.a
else
if AMULE_DAEMON
noinst_LIBRARIES += libmuleappcore.a libmuleappcommon.a
endif
endif
endif
if WEB
#noinst_LIBRARIES += libmuleweb.a
SUBDIRS += webserver
endif


# Common to core/gui/monolithic

libmuleappcommon_a_SOURCES = \
	CFile.cpp \
	ClientCredits.cpp \
	ED2KLink.cpp \
	Friend.cpp \
	GapList.cpp \
	MagnetURI.cpp \
	MemFile.cpp \
	NetworkFunctions.cpp \
	OtherFunctions.cpp \
	Packet.cpp \
	RLE.cpp \
	SafeFile.cpp \
	SHA.cpp \
	Tag.cpp \
	TerminationProcess.cpp \
	kademlia/utils/UInt128.cpp

libmuleappcommon_a_CPPFLAGS = $(AM_CPPFLAGS) $(WXBASE_CPPFLAGS) -I$(srcdir)/libs -I$(srcdir)/include $(CRYPTOPP_CPPFLAGS) $(LIBUPNP_CPPFLAGS)


# Common to core/monolithic

libmuleappcore_a_SOURCES = \
	AsyncDNS.cpp \
	DeadSourceList.cpp \
	FileArea.cpp \
	FileAutoClose.cpp \
	IPFilterScanner.cpp \
	Scanner.cpp \
	Parser.cpp \
	PlatformSpecific.cpp \
	RandomFunctions.cpp \
	RC4Encrypt.cpp \
	StateMachine.cpp \
	TerminationProcessAmuleweb.cpp \
	ThreadScheduler.cpp \
	UPnPBase.cpp \
	kademlia/kademlia/Entry.cpp \
	kademlia/kademlia/SearchManager.cpp \
	kademlia/routing/RoutingBin.cpp \
	kademlia/routing/Maps.cpp

libmuleappcore_a_CPPFLAGS = $(AM_CPPFLAGS) $(WXBASE_CPPFLAGS) -I$(srcdir)/libs -I$(srcdir)/include $(CRYPTOPP_CPPFLAGS) $(LIBUPNP_CPPFLAGS)


# Common to gui/monolithic

libmuleappgui_a_SOURCES = \
	BarShader.cpp \
	ColorFrameCtrl.cpp \
	EditServerListDlg.cpp \
	extern/wxWidgets/listctrl.cpp \
	FileDetailListCtrl.cpp \
	IP2Country.cpp \
	MuleColour.cpp \
	MuleGifCtrl.cpp \
	MuleListCtrl.cpp \
	MuleNotebook.cpp \
	MuleTextCtrl.cpp \
	MuleCollection.cpp \
	muuli_wdr.cpp 

libmuleappgui_a_CPPFLAGS = $(AM_CPPFLAGS) $(WX_CPPFLAGS) -I$(srcdir)/libs -I$(srcdir)/include $(LIBUPNP_CPPFLAGS) $(GEOIP_CPPFLAGS)

core_sources = \
	amule.cpp \
	BaseClient.cpp \
	CanceledFileList.cpp \
	ClientList.cpp \
	ClientCreditsList.cpp \
	ClientTCPSocket.cpp \
	ClientUDPSocket.cpp \
	CorruptionBlackBox.cpp \
	DownloadClient.cpp \
	DownloadQueue.cpp \
	ECSpecialCoreTags.cpp \
	EMSocket.cpp \
	EncryptedStreamSocket.cpp \
	EncryptedDatagramSocket.cpp \
	ExternalConn.cpp \
	FriendList.cpp \
	IPFilter.cpp \
	KnownFileList.cpp \
	ListenSocket.cpp \
	MuleUDPSocket.cpp \
	SearchFile.cpp \
	SearchList.cpp \
	ServerConnect.cpp \
	ServerList.cpp \
	ServerSocket.cpp \
	ServerUDPSocket.cpp \
	SHAHashSet.cpp \
	SharedFileList.cpp \
	ThreadTasks.cpp \
	UploadBandwidthThrottler.cpp \
	UploadClient.cpp \
	UploadQueue.cpp \
	I2PConnectionManager.cpp \
	kademlia/kademlia/Indexed.cpp \
	kademlia/kademlia/Kademlia.cpp \
	kademlia/kademlia/Prefs.cpp \
	kademlia/kademlia/Search.cpp \
	kademlia/kademlia/UDPFirewallTester.cpp \
	kademlia/net/KademliaUDPListener.cpp \
	kademlia/net/PacketTracking.cpp \
	kademlia/routing/RoutingZone.cpp


gui_sources = \
	amule-gui.cpp \
	amuleDlg.cpp \
	AddFriend.cpp \
	CatDialog.cpp \
	ChatSelector.cpp \
	ChatWnd.cpp \
	CommentDialog.cpp \
	CommentDialogLst.cpp \
	GenericClientListCtrl.cpp \
	ClientDetailDialog.cpp \
	DirectoryTreeCtrl.cpp \
	FileDetailDialog.cpp \
	KadDlg.cpp \
	kadNodesListView.cpp \
	OScopeCtrl.cpp \
	PrefsUnifiedDlg.cpp \
	SearchDlg.cpp \
	ServerWnd.cpp \
	SharedFilesWnd.cpp \
	StatisticsDlg.cpp \
	SearchListCtrl.cpp \
	DownloadListCtrl.cpp \
	SourceListCtrl.cpp \
	SharedFilePeersListCtrl.cpp \
	FriendListCtrl.cpp \
	ServerListCtrl.cpp \
	SharedFilesCtrl.cpp \
	MuleTrayIcon.cpp \
	TransferWnd.cpp 


common_sources = \
	kademlia/routing/Contact.cpp \
	amuleAppCommon.cpp \
	ClientRef.cpp \
	DataToText.cpp \
	ECSpecialMuleTags.cpp \
	KnownFile.cpp \
	GetTickCount.cpp \
	GuiEvents.cpp \
	HTTPDownload.cpp \
	Logger.cpp \
	PartFile.cpp \
	Preferences.cpp \
	Proxy.cpp \
	Server.cpp \
	Statistics.cpp \
	StatTree.cpp \
	UserEvents.cpp

# patch for static linking with libgcj.a
.gnujavautilregex_resource.o:
	$(GCJ) --resource gnu/java/util/regex/$(*) -c $< -o $@

# java resources for static linking : it's better to specify that in libgcj.spec
# (see java_resources.txt in this folder)
java_resources_patch_sources =
java_resources_patch_objs =
if STATIC_LIBGCJ
java_resources_patch_sources += $(property_files)
java_resources_patch_objs    += $(propertyo_files)
#java_resources_patch_sources += \
#	MessagesBundle.gnujavautilregex_resource \
#	MessagesBundle.properties.gnujavautilregex_resource
endif
# Libs

core_libs = -L. -lmuleappcore $(LIBUPNP_LDFLAGS) $(LIBUPNP_LIBS)
gui_libs = -L. -lmuleappgui $(WX_LIBS) $(GEOIP_LDFLAGS) $(GEOIP_LIBS)
remote_common_libs = -Llibs/common -Llibs/ec/cpp -lmulecommon -lec $(BFD_LIBS) $(ZLIB_LDFLAGS) $(ZLIB_LIBS) $(RESOLV_LIB)
common_libs = $(remote_common_libs) -L. -lmuleappcommon  $(CRYPTOPP_LDFLAGS) $(CRYPTOPP_LIBS)
i2p_libs = -Llibs/i2p -lmulei2p
if IROUTER
i2p_libs += -lcorei2p -lstdc++
endif

# dependencies

core_deps = libmuleappcore.a
gui_deps = libmuleappgui.a
remote_common_deps = libs/common/libmulecommon.a libs/ec/cpp/libec.a
common_deps = libmuleappcommon.a $(remote_common_deps)
i2p_deps = libs/i2p/libmulei2p.a
if IROUTER
i2p_deps += libs/i2p/libcorei2p.a
endif

if SYS_WIN32
core_libs += -lshlwapi
endif

#
# Flags
#
i2p_flags = -Ilibs/i2p -I$(srcdir)/libs/i2p
core_flags = $(LIBUPNP_CPPFLAGS) $(LIBUPNP_CFLAGS)
gui_flags = $(WX_CPPFLAGS) $(GEOIP_CPPFLAGS)
common_flags = -I$(srcdir)/libs -Ilibs -I$(srcdir)/include $(CRYPTOPP_CPPFLAGS)

# --------- Apps ---------

imulegui_SOURCES =                       \
	amule-remote-gui.cpp \
	$(gui_sources) \
	$(ec_sources) \
	$(common_sources)

imule_SOURCES = \
	$(java_resources_patch_sources) \
	CaptchaDialog.cpp \
	CaptchaGenerator.cpp \
	PartFileConvert.cpp \
	PartFileConvertDlg.cpp \
	$(core_sources) \
	$(gui_sources) \
	$(ec_sources) \
	$(common_sources)

imuled_SOURCES =                         \
	amuled.cpp \
		$(java_resources_patch_sources) \
	$(core_sources) \
	$(ec_sources) \
	$(common_sources)



#i2ptest_SOURCES = $(common_sources) $(core_sources) $(ec_sources) $(gui_sources)

#i2ptest_LINK          = $(GCJLINK)
#i2ptest_LDADD         = $(core_libs) $(common_libs)  $(i2p_libs) $(gui_libs)
#i2ptest_DEPENDENCIES  = $(core_deps)  $(gui_deps)    $(common_deps)  $(i2p_deps)
#i2ptest_CFLAGS        = $(core_flags) $(gui_flags)   $(common_flags)
#i2ptest_CXXFLAGS      = $(core_flags) $(gui_flags)   $(common_flags)



i2prouter_SOURCES      = libs/i2p/I2PRouter.java
i2prouter_GCJFLAGS     = -I./libs/i2p
i2prouter_LINK         = $(GCJLINK) $(i2prouter_LDFLAGS)
i2prouter_LDADD        = libs/i2p/libcorei2p.a $(WXBASE_LIBS)
i2prouter_LDFLAGS      = --main=I2PRouter


if IROUTER
imule_LINK            = $(GCJLINK)
imuled_LINK	      = $(GCJLINK)
else
imule_LINK            = $(CXXLINK)
imuled_LINK           = $(CXXLINK)
endif

imule_ldflags         =
if WINDOWS
win_static_ld_flags = -Wl,--allow-multiple-definition
else
win_static_ld_flags = 
endif
if STATIC_LIBGCJ
imule_ldflags         += -static-libgcj $(win_static_ld_flags)
endif
imule_DEPENDENCIES = $(common_deps) $(core_deps) $(gui_deps) $(i2p_deps)
imule_CPPFLAGS        = $(AM_CPPFLAGS) $(core_flags) $(gui_flags) $(common_flags)
imule_CXXFLAGS        = $(AM_CXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CPPFLAGS_ONLY)
imule_LDADD           = $(common_libs) $(core_libs) $(gui_libs) $(i2p_libs) $(java_resources_patch_objs)
imule_LDFLAGS         = $(imule_ldflags)

imulegui_DEPENDENCIES = $(core_deps) $(common_deps) $(core_deps) $(gui_deps) $(i2p_deps)
imulegui_CPPFLAGS     = $(AM_CPPFLAGS) $(gui_flags) $(core_flags) $(common_flags) -DCLIENT_GUI
imulegui_CXXFLAGS     = $(AM_CXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)
imulegui_LDADD        = $(gui_libs) $(common_libs) $(core_libs) $(i2p_libs) $(WXBASE_LIBS)

imuled_DEPENDENCIES   = $(core_deps) $(common_deps) $(i2p_deps)
imuled_CPPFLAGS       = $(AM_CPPFLAGS) $(WXBASE_CPPFLAGS) $(core_flags) $(common_flags) -DAMULE_DAEMON
imuled_CXXFLAGS       = $(AM_CXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)
imuled_LDADD          = $(common_libs) $(core_libs) $(i2p_libs) $(WXBASE_LIBS)
imuled_LDFLAGS        = $(imule_ldflags)

ed2k_SOURCES = \
	ED2KLinkParser.cpp \
	MagnetURI.cpp \
	MuleCollection.cpp	

ed2k_CPPFLAGS = $(AM_CPPFLAGS) $(WXBASE_CPPFLAGS) -DUSE_STD_STRING

# on Win32
if SYS_WIN32
ed2k_LDADD = -lshlwapi
endif
# on Mac
if NEED_CORESERVICES
ed2k_LDFLAGS = $(AM_LDFLAGS) -framework CoreServices
endif

imulecmd_SOURCES = \
	TextClient.cpp \
	ExternalConnector.cpp \
	LoggerConsole.cpp \
	OtherFunctions.cpp \
	NetworkFunctions.cpp \
	libs/i2p/CI2PAddress.cpp

imulecmd_DEPENDENCIES = $(remote_common_deps)
imulecmd_CPPFLAGS = $(AM_CPPFLAGS) $(WXBASE_CPPFLAGS) $(common_flags)
imulecmd_CXXFLAGS = $(AM_CXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)
#amulecmd_LDADD = $(WXBASE_LIBS) $(READLINE_LIBS) $(remote_common_libs)
imulecmd_LDADD = -Llibs/common -Llibs/ec/cpp -lmulecommon -lec \
	$(WXBASE_LIBS) $(READLINE_LIBS) $(BFD_LIBS) $(ZLIB_LDFLAGS) $(ZLIB_LIBS) $(RESOLV_LIB)

noinst_HEADERS = \
		AddFriend.h \
		AsyncDNS.h \
		amule-remote-gui.h \
		amuleDlg.h \
		amule.h \
		amuleIPV4Address.h \
		ArchSpecific.h \
		BarShader.h \
		BitVector.h \
		CanceledFileList.h \
		CaptchaDialog.h \
		CaptchaGenerator.h \
		CatDialog.h \
		CFile.h \
		ChatSelector.h \
		ChatWnd.h \
		ClientCredits.h \
		ClientCreditsList.h \
		ClientDetailDialog.h \
		SharedFilePeersListCtrl.h \
		ClientList.h \
		ClientRef.h \
		ClientTCPSocket.h \
		ClientUDPSocket.h \
		ColorFrameCtrl.h \
		CommentDialog.h \
		CommentDialogLst.h \
		Constants.h \
		CorruptionBlackBox.h \
		CryptoPP_Inc.h \
		DataToText.h \
		DeadSourceList.h \
		DirectoryTreeCtrl.h \
		DownloadListCtrl.h \
		DownloadQueue.h \
		ED2KLink.h \
		EditServerListDlg.h \
		EMSocket.h \
		EncryptedDatagramSocket.h \
		EncryptedStreamSocket.h \
		ExternalConnector.h \
		ExternalConn.h \
		FileArea.h \
		FileAutoClose.h \
		FileDetailDialog.h \
		FileDetailListCtrl.h \
		FileLock.h \
		Friend.h \
		FriendListCtrl.h \
		FriendList.h \
		GapList.h \
		GetTickCount.h \
		GenericClientListCtrl.h \
		GuiEvents.h \
		HTTPDownload.h \
	I2PConnectionManager.h \
		inetdownload.h \
		InternalEvents.h \
		IP2Country.h \
		IPFilter.h \
		IPFilterScanner.h \
		KadDlg.h \
		KnownFile.h \
		KnownFileList.h \
		ListenSocket.h \
		Logger.h \
		MagnetURI.h \
		MD4Hash.h \
		MemFile.h \
		MuleCollection.h \
		MuleColour.h \
		MuleGifCtrl.h \
		MuleListCtrl.h \
		MuleNotebook.h \
		MuleTextCtrl.h \
		MuleThread.h \
		MuleTrayIcon.h \
		MuleUDPSocket.h \
		muuli_wdr.h \
		NetworkFunctions.h \
		OScopeCtrl.h \
		Observable.h \
		ObservableQueue.h \
		OtherFunctions.h \
		OtherStructs.h \
		Packet.h \
		Parser.hpp \
		PartFileConvert.h \
		PartFileConvertDlg.h \
		PartFile.h \
		PlatformSpecific.h \
		Preferences.h \
		PrefsUnifiedDlg.h \
		Proxy.h \
		RangeMap.h \
		RC4Encrypt.h \
		RLE.h \
		RandomFunctions.h \
		SafeFile.h \
		Scanner.h \
		SearchDlg.h \
		SearchExpr.h \
		SearchFile.h \
		SearchListCtrl.h \
		SearchList.h \
		ServerConnect.h \
		Server.h \
		ServerListCtrl.h \
		ServerList.h \
		ServerSocket.h \
		ServerUDPSocket.h \
		ServerWnd.h \
		SHA.h \
		SHAHashSet.h \
		SharedFileList.h \
		SharedFilesCtrl.h \
		SharedFilesWnd.h \
		SourceListCtrl.h \
		StateMachine.h \
		StatisticsDlg.h \
		Statistics.h \
		StatTree.h \
		Tag.h \
		TerminationProcess.h \
		TerminationProcessAmuleweb.h \
		TextClient.h \
		ThreadScheduler.h \
		ThreadTasks.h \
		ThrottledSocket.h \
		TransferWnd.h \
		Types.h \
		updownclient.h \
		UpDownClientEC.h \
		UploadBandwidthThrottler.h \
		UploadQueue.h \
		UPnPBase.h \
		UPnPCompatibility.h \
		UserEvents.h \
		kadNodesListView.h \
		extern/wxWidgets/listctrl.h

MAINTAINERCLEANFILES = Makefile.in


BUILT_SOURCES = Parser.cpp Parser.hpp Scanner.cpp Scanner.h IPFilterScanner.cpp

$(srcdir)/Parser.cpp $(srcdir)/Parser.hpp: Parser.y
	$(BISON) --debug -t -d -v -o $(srcdir)/Parser.cpp $(srcdir)/Parser.y

$(srcdir)/Scanner.cpp $(srcdir)/Scanner.h: Scanner.l
if GENERATE_FLEX_HEADER
		$(LEX) --header-file=$(srcdir)/Scanner.h -o $(srcdir)/Scanner.cpp $(srcdir)/Scanner.l
else
		$(LEX) -o $(srcdir)/Scanner.cpp $(srcdir)/Scanner.l
		echo "// Empty file generated by a flex version unable to create headers" > $(srcdir)/Scanner.h
endif

$(srcdir)/IPFilterScanner.cpp: IPFilterScanner.l
		$(LEX) -Pyyip -o $@ $(srcdir)/IPFilterScanner.l;
# Resources

if NEED_RC

SUFFIXES = .rc

.rc.$(OBJEXT):
	$(RC) $(MULERCFLAGS) -I$(top_srcdir) $(RCFLAGS) -O COFF -i "$<" -o "$@"

nodist_imuled_SOURCES = imuled-version.rc
nodist_imulecmd_SOURCES = imulecmd-version.rc
nodist_ed2k_SOURCES = ed2k-version.rc
imule_LDADD += imulerc.$(OBJEXT)
imule_DEPENDENCIES += imulerc.$(OBJEXT)
imulegui_LDADD += imuleguirc.$(OBJEXT)
imulegui_DEPENDENCIES += imuleguirc.$(OBJEXT)

imulerc.$(OBJEXT): $(srcdir)/../imule.rc imule-version.rc
	cat $^ | \
	( \
		abs_builddir=`pwd` ; \
		abs_top_srcdir=`cd $(top_srcdir) ; pwd` ; \
		cd $(<D) ; \
		$(RC) $(MULERCFLAGS) -I$${abs_top_srcdir} $(RCFLAGS) -O COFF -o "$${abs_builddir}/$@" ; \
	)

imuleguirc.$(OBJEXT): $(srcdir)/../imule.rc imulegui-version.rc
	cat $^ | \
	( \
		abs_builddir=`pwd` ; \
		abs_top_srcdir=`cd $(top_srcdir) ; pwd` ; \
		cd $(<D) ; \
		$(RC) $(MULERCFLAGS) -I$${abs_top_srcdir} $(RCFLAGS) -O COFF -o "$${abs_builddir}/$@" ; \
	)

imule-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"All-Platform P2P Client Based on eMule"/' \
	     -e 's/VER_INTERNALNAME_STR/"imule"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"imule$(EXEEXT)"/' "$<" > "$@"

imuled-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"imule Daemon"/' \
	     -e 's/VER_INTERNALNAME_STR/"imuled"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"imuled$(EXEEXT)"/' "$<" > "$@"

imulecmd-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"imule Text Client"/' \
	     -e 's/VER_INTERNALNAME_STR/"imulecmd"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"imulecmd$(EXEEXT)"/' "$<" > "$@"

imulegui-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"imule Remote GUI (Experimental)"/' \
	     -e 's/VER_INTERNALNAME_STR/"imulegui"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"imulegui$(EXEEXT)"/' "$<" > "$@"

ed2k-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"ED2K Links Handler"/' \
	     -e 's/VER_INTERNALNAME_STR/"ed2k"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"ed2k$(EXEEXT)"/' "$<" > "$@"

CLEANFILES = *-version.rc

endif


gmpsolibs = libnonegmp.so

gmpdlllibs = nonegmp.dll
#pentiumgmp.dll pentiummmxgmp.dll pentium2gmp.dll pentium3gmp.dll pentium4gmp.dll k6gmp.dll k62gmp.dll k63gmp.dll athlongmp.dll amd64gmp.dll

gmpjnilibs = libnonegmp.jnilib
#libpentiumgmp.jnilib libpentiummmxgmp.jnilib libpentium2gmp.jnilib libpentium3gmp.jnilib libpentium4gmp.jnilib libk6gmp.jnilib libk62gmp.jnilib libk63gmp.jnilib libathlongmp.jnilib libamd64gmp.jnilib


EXTRA_SCRIPTS = $(gmpsolibs) $(gmpdlllibs) mingwm10.dll
libexec_SCRIPTS =

if IROUTER
libexec_SCRIPTS += $(router_scripts)
BUILD = $(HOST_BUILD)
endif

if ALLGMP
router_scripts = $(allgmp_scripts)
else
router_scripts = $(onegmp_scripts)
endif

if GMP
onegmp_scripts = $(system_gmp_scripts)
else
onegmp_scripts =
endif

if WINDOWS
allgmp_scripts = $(gmpdlllibs)
system_gmp_scripts = nonegmp.dll
libexec_SCRIPTS += mingwm10.dll
endif

if LINUX
allgmp_scripts = $(gmpsolibs)
system_gmp_scripts = libnonegmp.so
endif

if MAC
allgmp_scripts = $(gmpjnilibs)
system_gmp_scripts = libnonegmp.jnilib
endif

#mingwm10.dll: /usr/share/doc/mingw32-runtime/mingwm10.dll.gz
#	cp $< $@.gz && gunzip -f $@.gz
#mingwm10.dll: /usr/local/mingw32/i586-pc-mingw32/bin/mingwm10.dll
#	cp $< $@
mingwm10.dll:
	bash -c '( locate mingwm10.dll && cp `locate mingwm10.dll | grep /usr` $@ ) || ( cp /usr/share/doc/mingw32-runtime/mingwm10.dll.gz . && gunzip -f $@.gz )'

libgmp.so : libthiscpugmp.so
	cp $< $@
gmp.dll : thiscpugmp.dll
	cp $< $@
libgmp.jnilib : libthiscpugmp.jnilib
	cp $< $@

lib%gmp.so : $(srcdir)/libs/i2p/gmp/gmp.tgz $(srcdir)/libs/i2p/gmp/build_gmp.sh \
	   $(srcdir)/libs/i2p/gmp/forlib.c $(@D)
	rm -rf libgmp
	mkdir -p libgmp
	cp $(srcdir)/libs/i2p/gmp/gmp.tgz $(srcdir)/libs/i2p/gmp/build_gmp.sh \
	   $(srcdir)/libs/i2p/gmp/forlib.c libgmp
	cd  libgmp && CC="$(CC)" BUILD=$(BUILD) TARGET=$(target) MAKE=$(MAKE) sh build_gmp.sh
	mv libgmp/libgmp.so $@



%gmp.dll : $(srcdir)/libs/i2p/gmp/gmp.tgz $(srcdir)/libs/i2p/gmp/build_gmp.sh \
	   $(srcdir)/libs/i2p/gmp/forlib.c $(@D)
	rm -rf libgmp
	mkdir -p libgmp
	cp $(srcdir)/libs/i2p/gmp/gmp.tgz $(srcdir)/libs/i2p/gmp/build_gmp.sh \
	   $(srcdir)/libs/i2p/gmp/forlib.c libgmp
	cd  libgmp && CC="$(CC)" CPATH= MAKE=$(MAKE) BUILD=$(BUILD) TARGET=$(target) sh build_gmp.sh
	mv libgmp/gmp.dll $@

%gmp.jnilib : $(srcdir)/libs/i2p/gmp/gmp.tgz $(srcdir)/libs/i2p/gmp/build_gmp.sh \
	   $(srcdir)/libs/i2p/gmp/forlib.c $(@D)
	rm -rf libgmp
	mkdir -p libgmp
	cp $(srcdir)/libs/i2p/gmp/gmp.tgz $(srcdir)/libs/i2p/gmp/build_gmp.sh \
	   $(srcdir)/libs/i2p/gmp/forlib.c libgmp
	cd  libgmp && CC="$(CC)" CPATH= BUILD=$(BUILD) TARGET=$(target) MAKE=$(MAKE) sh build_gmp.sh $(*F) $(BUILD) $(HOST)
	mv libgmp/libgmp.jnilib $@
#lib_LTLIBRARIES = libgmp.la
#libgmp_la_SOURCES = forlib.c
#libgmp_la_LDFLAGS = -module

propertyo_files = $(patsubst classpath/resource/%,%,$(addsuffix .o,$(property_files)))

$(propertyo_files): %.o: classpath/resource/%
	$(mkinstalldirs) `dirname $@` && \
	$(GCJ) --resource $(@:.o=) -c $< -o $@

# this list is obtained with the following commands :
#  cd classpath/resource
#  svn export svn://gcc.gnu.org/svn/gcc/trunk/libjava/classpath/resource .
#  rm Makefile.*
#  cd ../..
#  find classpath  ! -path "*svn*" -type f | sed 's/^/\t/' | sed 's/$/\t\\/'

property_files = \
        classpath/resource/java/util/logging/logging.properties \
        classpath/resource/java/util/weeks.properties   \
        classpath/resource/java/util/iso4217.properties \
        classpath/resource/java/text/metazones.properties       \
        classpath/resource/sun/rmi/rmic/messages.properties     \
        classpath/resource/org/ietf/jgss/MessagesBundle.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_bg_BG.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_nb.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sr_BA_Latn.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_es_VE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_th_TH.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_fr.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_kn.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_aa_ET.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_sk_SK.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_fr_CA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_de_LI.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_uz_Latn.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_dz.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_fr_BE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_ZW.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_cy_GB.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ca.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_es_UY.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_pt_PT.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_be.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ga.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_kl_GL.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ka.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_fa_AF.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_BZ.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_GT.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_gu_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_sa.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ar_TN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_mr_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_hu_HU.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_NZ.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_byn_ER.properties  \
        classpath/resource/gnu/java/locale/LocaleInformation_af_ZA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_et_EE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_sq.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ru.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_el.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_nl_NL.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_pt_BR.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ml.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_id_ID.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_zh.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_gv_GB.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_haw_US.properties  \
        classpath/resource/gnu/java/locale/LocaleInformation_km_KH.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_PH.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_AR.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_MT.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_eo.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_aa_ER_SAAHO.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_it_IT.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_so_ET.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ca_ES.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_wal_ET.properties  \
        classpath/resource/gnu/java/locale/LocaleInformation_gez_ET.properties  \
        classpath/resource/gnu/java/locale/LocaleInformation.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_kw.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ps.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_zh_TW_Hant.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_HK.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_bn_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ps_AF.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_sw.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ar_JO.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_PK.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_te_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_bn.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_de_DE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ar.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_dv.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_GB.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_nn.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_syr.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_mr.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_lt_LT.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_nl_BE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_PY.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_lv.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_so_SO.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ur.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_fa.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_de_LU.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_fo_FO.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ja.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ar_DZ.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_hi.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_cs_CZ.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ga_IE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_km.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_eu.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_kok_IN.properties  \
        classpath/resource/gnu/java/locale/LocaleInformation_om.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_es_MX.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ti.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_de_AT.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_iu.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_hy.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_fa_IR.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_wal.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_ar_SA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_DO.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_el_CY.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_GU.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_aa_DJ.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_so_KE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_VI.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_UM.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_zh_CN_Hans.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sw_KE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_pl.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_lv_LV.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_kok.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_it_CH.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_bg.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_nb_NO.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_zh_Hant.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_en_MP.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_sq_AL.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_cs.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_mk.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_es_PR.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_lo.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ru_UA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_uk.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_es_SV.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_tig.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_ja_JP.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_AS.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_hi_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_PE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ti_ER.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_dz_BT.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_az_Cyrl.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_uk_UA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_sv_FI.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_pl_PL.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_sl_SI.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_hr.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_US.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_mn.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_gv.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sv_SE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_aa.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ms_MY.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_tr.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_am_ET.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_CL.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_sv.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_el_GR.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_de.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_he.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_so_DJ.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_de_CH.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_CA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_fr_LU.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_te.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_BE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ro_RO.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_cy.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_hu.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sid.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_zh_HK_Hant.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_zh_SG_Hans.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_he_IL.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ky.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_uz_AF_Arab.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_es_CO.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_gl_ES.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_id.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ar_YE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_eu_ES.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_gl.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sw_TZ.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_kl.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_is_IS.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_aa_ER.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_et.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_th.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_dv_MV.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_it.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sr_Cyrl.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_sl.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_mt.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_be_BY.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_uz_Arab.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_byn.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_as_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_tr_TR.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_CR.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_HN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ms_BN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_kn_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_om_ET.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_bs.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sid_ET.properties  \
        classpath/resource/gnu/java/locale/LocaleInformation_ko.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_haw.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_so.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_es_PA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ta_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_SG.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_NI.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_de_BE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_az.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ar_LB.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_or.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sr.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_da.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_lo_LA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_sa_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_kw_GB.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_zh_MO_Hant.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_hy_AM.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_am.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ml_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_fi.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_uz.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_pa.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ta.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ko_KR.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_kk_KZ.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_gez_ER.properties  \
        classpath/resource/gnu/java/locale/LocaleInformation_gu.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_es_EC.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_vi.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ar_MA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_IE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_fr_CH.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ar_QA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_AU.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_BO.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_om_KE.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_en_ZA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_ES.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_sr_Latn.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_da_DK.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ti_ET.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_nl.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_fi_FI.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ar_SY.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es_US.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_lt.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_pt.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_MH.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_or_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_tt.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_as.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_US_POSIX.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_fo.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_kk.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sr_BA_Cyrl.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_nn_NO.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_es.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_tig_ER.properties  \
        classpath/resource/gnu/java/locale/LocaleInformation_is.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sk.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ku_Arab.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_hy_AM_REVISED.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ms.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ro.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_mt_MT.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_gez.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_pa_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_af.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_syr_SY.properties  \
        classpath/resource/gnu/java/locale/LocaleInformation_en_BW.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ru_RU.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_tt_RU.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_nr.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_kpe.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_ii.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ne.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_af_NA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ve.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_my.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_JM.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_zu.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_st.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_kaj.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_sr_RS_Latn.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ts.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_yo.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_Dsrt.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_kam.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_pa_Arab.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_ee.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_cop.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_ku_Latn.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_kfo.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_ak.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_TT.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_to.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ss.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_rw.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_fil.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_ia.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ur_IN.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_trv.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_si.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ssy.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_se_FI.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_nso.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_kcg.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_xh.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_fur.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_ig.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ug.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_Shaw.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_wo.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_gaa.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_ln.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ha_Arab.properties \
        classpath/resource/gnu/java/locale/LocaleInformation_tn.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ha.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_sr_ME_Latn.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_cch.properties     \
        classpath/resource/gnu/java/locale/LocaleInformation_se.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_ku.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_en_NA.properties   \
        classpath/resource/gnu/java/locale/LocaleInformation_ny.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_bo.properties      \
        classpath/resource/gnu/java/locale/LocaleInformation_tg.properties      \
        classpath/resource/gnu/java/awt/peer/gtk/font.properties        \
        classpath/resource/gnu/java/awt/peer/x/xfonts.properties        \
        classpath/resource/gnu/java/awt/peer/x/fonts.properties \
        classpath/resource/gnu/java/awt/font/fonts.properties   \
        classpath/resource/gnu/java/util/regex/MessagesBundle_fr.properties     \
        classpath/resource/gnu/java/util/regex/MessagesBundle_it.properties     \
        classpath/resource/gnu/java/util/regex/MessagesBundle.properties        \
        classpath/resource/gnu/classpath/tools/rmiregistry/messages.properties  \
        classpath/resource/gnu/classpath/tools/native2ascii/messages.properties \
        classpath/resource/gnu/classpath/tools/getopt/Messages.properties       \
        classpath/resource/gnu/classpath/tools/jarsigner/MessageBundle.properties       \
        classpath/resource/gnu/classpath/tools/jarsigner/messages.properties    \
        classpath/resource/gnu/classpath/tools/tnameserv/messages.properties    \
        classpath/resource/gnu/classpath/tools/keytool/MessageBundle.properties \
        classpath/resource/gnu/classpath/tools/keytool/messages.properties      \
        classpath/resource/gnu/classpath/tools/orbd/messages.properties \
        classpath/resource/gnu/classpath/tools/appletviewer/messages.properties \
        classpath/resource/gnu/classpath/tools/common/Messages.properties       \
        classpath/resource/gnu/classpath/tools/rmic/messages.properties \
        classpath/resource/gnu/classpath/tools/rmid/messages.properties \
        classpath/resource/gnu/classpath/tools/jar/messages.properties  \
        classpath/resource/gnu/classpath/tools/serialver/messages.properties    \
        classpath/resource/gnu/javax/security/auth/callback/MessagesBundle.properties   \
        classpath/resource/gnu/javax/print/PrinterDialog_de.properties  \
        classpath/resource/gnu/javax/print/PrinterDialog.properties     \
        classpath/resource/javax/imageio/plugins/jpeg/MessagesBundle.properties \
        classpath/resource/com/sun/tools/javac/messages.properties

        
#  classpath/resource/gnu/java/util/regex/MessagesBundle
EXTRA__DIST__SUBDIRS = include kademlia
include $(top_srcdir)/automake/dist-hook.am
