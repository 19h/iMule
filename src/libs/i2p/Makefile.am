#%.Po::
#	echo \#dummy > $@
#.java.h:
#	$(GCJ) -C $< $(BUILT_SOURCES_INCLUDES) -I$
#	sh -c 'basetargetclassname=$(*F) ; \
#		basetargetdir=$(*D) ; \
#		basesourcedir=`echo $< | sed 's/[^/]*\.java//g'` ; \
#		for fc in $$basesourcedir$$basetargetclassname*.class ; do \
#			childsourcepath=$${fc%%.class} ; \
#			childclassname=$${childsourcepath##.*/} ; \
#			childtargetpath=$$basetargetdir/$$childclassname ; \
#			$(GCJ)h $(BUILT_SOURCES_INCLUDES) $$childtargetpath ; \
#		done'
#AM_GCJFLAGS = $(INCLUDES)
#AM_CFLAGS   = $(WX_CFLAGS) $(GCJ_FLAGS) fdsqoucdsqof
#CXXFLAGS = $(WX_CFLAGS) $(GCJ_FLAGS) rezrezvsg
#CXX         = $(GCJ) -c
#CXXCOMPILE  = $(GCJCOMPILE) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS)
GCJLINK     = $(GCJ) -o $@ $(LDADD)
AM_CPPFLAGS = $(MULECPPFLAGS) $(WXBASE_CPPFLAGS)
AM_CXXFLAGS = $(MULECXXFLAGS) $(WXBASE_CFLAGS_ONLY) $(WXBASE_CXXFLAGS_ONLY)
AM_GCJFLAGS = $(WX_GCJFLAGS)

EXTRA_PROGRAMS = test1 test2 test3 test4 test5 test7
noinst_PROGRAMS =

test1_LINK    = $(GCJLINK)
test1_SOURCES = test.java
test1_LDADD   = $(top_builddir)/src/libs/i2p/libcorei2p.a $(WXBASE_LIBS) -lstdc++
test1_LDFLAGS = --main=test

test2_LINK    = $(GCJLINK)
test2_SOURCES = test2.cpp JConversions.cpp
test2_CXXFLAGS= $(WXBASE_CXXFLAGS)
test2_LDADD   = $(WXBASE_LIBS) -lstdc++ $(top_builddir)/src/libs/i2p/libcorei2p.a

test3_LINK    = $(GCJLINK)
test3_SOURCES = test3.cpp JConversions.cpp
test3_CXXFLAGS= $(WXBASE_CXXFLAGS)
test3_LDADD   = $(WXBASE_LIBS) -lstdc++ $(top_builddir)/src/libs/i2p/libcorei2p.a

test4_LINK    = $(GCJLINK)
test4_SOURCES = test4j.java test4.cpp JConversions.cpp
test4_CXXFLAGS= $(WXBASE_CXXFLAGS) -I$(srcdir) -I.
test4_LDADD   = $(WXBASE_LIBS) -lstdc++ $(top_builddir)/src/libs/i2p/libcorei2p.a
test4_LDFLAGS = -static-libgcj
test4_DEPENDENCIES = test4j.h test4j.java test4.cpp JConversions.cpp

test5_LINK    = $(GCJLINK)
test5_SOURCES = test5j.java test5.cpp JConversions.cpp
test5_CXXFLAGS= $(WX_CXXFLAGS) -I$(srcdir) -I.
test5_LDADD   = $(WX_LIBS) -lstdc++ $(top_builddir)/src/libs/i2p/libcorei2p.a
test5_LDFLAGS = -static-libgcj
test5_DEPENDENCIES = test5j.h test5j.java test5.cpp JConversions.cpp

test7_SOURCES = test7-socket.cpp
test7_CXXFLAGS= $(WX_CXXFLAGS) -I$(srcdir) -I.
test7_LDADD   = $(WX_LIBS) -L. -lmulei2p
test7_DEPENDENCIES = libmulei2p.a

#.java.h:
#	$(GCJ) -C $< -I$(srcdir) -I. -d . && touch $(*).class && $(GCJ)h $(*) -I$(srcdir) -I. && touch $(*).h


#.java.class:
#	$(GCJ) -C $< -d $(srcdir) $(BUILT_SOURCES_INCLUDES)

AM_CPPFLAGS +=  -I$(top_srcdir)/src \
		-I$(top_srcdir)/src/libs \
		-I$(top_srcdir)/src/libs/i2p

ARFLAGS = cq
libcorei2p_a_AR = sh -c ' \
	nbSources=$$\# ; \
	i=0 ; \
	sourcesList= ; \
	for c in $$* ; do \
		i=$$(($$i+1)) ; \
		sourcesList="$$sourcesList $$c" ; \
		if [ $$(($$i%100)) -eq 0 -o $$(($$i)) -eq $$\# ] ;\
		then \
			echo $$i $$sourcesList ; \
			$(AR) $(ARFLAGS) $$0 $$sourcesList ; \
			sourcesList= ; \
		fi ; \
	done'


javasources.headers: javasources.jar
	$(GJAVAH) -cni -all . && touch javasources.headers

javasources.jar: javasources.classes_stamp
	find . -name "*.class" | $(JAR) -@ -c -f javasources1.jar && mv javasources1.jar javasources.jar

javasources.classes_stamp: javasources.list
	gcj -C -d . @javasources.list && touch $@

javasources.list: $(javasources)
	rm -f javasources.list
	touch javasources.list
	for c in $^ ; do echo $$c >> javasources.list ; done

$(javasources):
	./link_coreI2P.sh


EXTRA_LIBRARIES  = libcorei2p.a
noinst_LIBRARIES = libmulei2p.a
noinst_HEADERS = $(corei2pheaders) $(mulei2pheaders) CSamDefines.h

if COREI2P
noinst_LIBRARIES += libcorei2p.a
BUILT_SOURCES     = javasources.headers
endif


javasourceso_files = $(patsubst %.java,%.o, $(javasources))

$(javasourceso_files): %.o: %.java
	find . -path "./$(@:.o=)\$$*.class" -o -path ./$(@:.o=.class) \
	| xargs $(GCJCOMPILE) $(GCJFLAGS) -c -o $@

libmulei2p_a_SOURCES = $(mulei2psources)
libcorei2p_a_SOURCES = $(corei2pCsources)
libcorei2p_a_LIBADD = $(javasourceso_files)

#libcorei2p_a_SOURCES = javasources.jar $(corei2pCsources)
libcorei2p_a_DEPENDENCIES = javasources.headers $(javasourceso_files)




mulei2pheaders = \
	coroutine.h \
	b64codec.h \
	CI2PAddress.h \
	CI2PRouter.h \
	CSamDefines.h \
	CSamLauncher.h \
	CSamSession.h \
	JConversions.h \
	myMutexLocker.h \
	SamEvents.h \
	wxI2PDatagramSocket.h \
	wxI2PEvents.h \
	wxI2PSocketBase.h \
	wxI2PSocketClient.h \
	wxI2PSocketServer.h 

corei2pheaders = 

mulei2psources = \
	CI2PAddress.cpp \
	CSamSession.cpp \
	myMutexLocker.cpp \
	wxI2PDatagramSocket.cpp \
	wxI2PEvents.cpp \
	wxI2PSocketBase.cpp \
	wxI2PSocketClient.cpp \
	wxI2PSocketServer.cpp 

corei2pCsources = \
	freenet/support/CPUInformation/CPUIDImpl.cpp \
	net/i2p/util/NativeBigIntegerImpl.cpp \
	CI2PRouter.cpp \
	CMuleLoggerImpl.cpp \
	CSamLauncher.cpp \
	JConversions.cpp \
	wxStringWriterImpl.cpp 

javasources = \
	addressbook/AddressBook.java \
	addressbook/ConfigParser.java \
	addressbook/Daemon.java \
	addressbook/DaemonThread.java \
	addressbook/Log.java \
	addressbook/SubscriptionIterator.java \
	addressbook/Subscription.java \
	addressbook/SubscriptionList.java \
	com/nettgryppa/security/HashCash.java \
	freenet/support/CPUInformation/AMDCPUInfo.java \
	freenet/support/CPUInformation/CPUID.java \
	freenet/support/CPUInformation/CPUInfo.java \
	freenet/support/CPUInformation/IntelCPUInfo.java \
	freenet/support/CPUInformation/UnknownCPUException.java \
	gnu/crypto/hash/BaseHashStandalone.java \
	gnu/crypto/hash/IMessageDigestStandalone.java \
	gnu/crypto/hash/Sha256Standalone.java \
	gnu/crypto/prng/AsyncFortunaStandalone.java \
	gnu/crypto/prng/BasePRNGStandalone.java \
	gnu/crypto/prng/FortunaStandalone.java \
	gnu/crypto/prng/IRandomStandalone.java \
	gnu/crypto/prng/LimitReachedExceptionStandalone.java \
	gnu/crypto/prng/RandomEventListenerStandalone.java \
	gnu/crypto/prng/RandomEventStandalone.java \
	net/i2p/client/BWLimitsMessageHandler.java \
	net/i2p/client/datagram/I2PDatagramDissector.java \
	net/i2p/client/datagram/I2PDatagramMaker.java \
	net/i2p/client/datagram/I2PInvalidDatagramException.java \
	net/i2p/client/DestReplyMessageHandler.java \
	net/i2p/client/DisconnectMessageHandler.java \
	net/i2p/client/HandlerImpl.java \
	net/i2p/client/I2CPMessageHandler.java \
	net/i2p/client/I2CPMessageProducer.java \
	net/i2p/client/I2PClientFactory.java \
	net/i2p/client/I2PClientImpl.java \
	net/i2p/client/I2PClient.java \
	net/i2p/client/I2PClientMessageHandlerMap.java \
	net/i2p/client/I2PSessionDemultiplexer.java \
	net/i2p/client/I2PSessionException.java \
	net/i2p/client/I2PSessionImpl2.java \
	net/i2p/client/I2PSessionImpl.java \
	net/i2p/client/I2PSession.java \
	net/i2p/client/I2PSessionListener.java \
	net/i2p/client/I2PSessionMuxedImpl.java \
	net/i2p/client/I2PSessionMuxedListener.java \
	net/i2p/client/I2PSimpleClient.java \
	net/i2p/client/I2PSimpleSession.java \
	net/i2p/client/MessagePayloadMessageHandler.java \
	net/i2p/client/MessageState.java \
	net/i2p/client/MessageStatusMessageHandler.java \
	net/i2p/client/naming/AddressDB.java \
	net/i2p/client/naming/AddressDBNamingService.java \
	net/i2p/client/naming/DummyAddressDB.java \
	net/i2p/client/naming/DummyNamingService.java \
	net/i2p/client/naming/EepGetNamingService.java \
	net/i2p/client/naming/ExecNamingService.java \
	net/i2p/client/naming/FilesystemAddressDB.java \
	net/i2p/client/naming/HostsTxtNamingService.java \
	net/i2p/client/naming/LookupDest.java \
	net/i2p/client/naming/MetaNamingService.java \
	net/i2p/client/naming/NamingService.java \
	net/i2p/client/naming/PetNameDB.java \
	net/i2p/client/naming/PetName.java \
	net/i2p/client/naming/PetNameNamingService.java \
	net/i2p/client/RequestLeaseSetMessageHandler.java \
	net/i2p/client/SessionIdleTimer.java \
	net/i2p/client/SessionStatusMessageHandler.java \
	net/i2p/client/SetDateMessageHandler.java \
	net/i2p/client/streaming/ByteCollector.java \
	net/i2p/client/streaming/ConnectionDataReceiver.java \
	net/i2p/client/streaming/ConnectionHandler.java \
	net/i2p/client/streaming/Connection.java \
	net/i2p/client/streaming/ConnectionManager.java \
	net/i2p/client/streaming/ConnectionOptions.java \
	net/i2p/client/streaming/ConnectionPacketHandler.java \
	net/i2p/client/streaming/I2PServerSocketFull.java \
	net/i2p/client/streaming/I2PServerSocketImpl.java \
	net/i2p/client/streaming/I2PServerSocket.java \
	net/i2p/client/streaming/I2PSocketFull.java \
	net/i2p/client/streaming/I2PSocketImpl.java \
	net/i2p/client/streaming/I2PSocket.java \
	net/i2p/client/streaming/I2PSocketManagerFactory.java \
	net/i2p/client/streaming/I2PSocketManagerFull.java \
	net/i2p/client/streaming/I2PSocketManagerImpl.java \
	net/i2p/client/streaming/I2PSocketManager.java \
	net/i2p/client/streaming/I2PSocketOptionsImpl.java \
	net/i2p/client/streaming/I2PSocketOptions.java \
	net/i2p/client/streaming/MessageHandler.java \
	net/i2p/client/streaming/MessageInputStream.java \
	net/i2p/client/streaming/MessageOutputStream.java \
	net/i2p/client/streaming/PacketHandler.java \
	net/i2p/client/streaming/Packet.java \
	net/i2p/client/streaming/PacketLocal.java \
	net/i2p/client/streaming/PacketQueue.java \
	net/i2p/client/streaming/RetransmissionTimer.java \
	net/i2p/client/streaming/SchedulerChooser.java \
	net/i2p/client/streaming/SchedulerClosed.java \
	net/i2p/client/streaming/SchedulerClosing.java \
	net/i2p/client/streaming/SchedulerConnectedBulk.java \
	net/i2p/client/streaming/SchedulerConnecting.java \
	net/i2p/client/streaming/SchedulerDead.java \
	net/i2p/client/streaming/SchedulerHardDisconnected.java \
	net/i2p/client/streaming/SchedulerImpl.java \
	net/i2p/client/streaming/SchedulerPreconnect.java \
	net/i2p/client/streaming/SchedulerReceived.java \
	net/i2p/client/streaming/StreamSinkClient.java \
	net/i2p/client/streaming/StreamSinkSend.java \
	net/i2p/client/streaming/StreamSinkServer.java \
	net/i2p/client/streaming/TaskScheduler.java \
	net/i2p/client/streaming/TCBShare.java \
	net/i2p/client/streaming/TestSwarm.java \
	net/i2p/client/streaming/TooManyStreamsException.java \
	net/i2p/CoreVersion.java \
	net/i2p/crypto/AESEngine.java \
	net/i2p/crypto/AESInputStream.java \
	net/i2p/crypto/AESOutputStream.java \
	net/i2p/crypto/CryptixAESEngine.java \
	net/i2p/crypto/CryptixAESKeyCache.java \
	net/i2p/crypto/CryptixRijndael_Algorithm.java \
	net/i2p/crypto/CryptoConstants.java \
	net/i2p/crypto/DHSessionKeyBuilder.java \
	net/i2p/crypto/DSAEngine.java \
	net/i2p/crypto/DummyDSAEngine.java \
	net/i2p/crypto/DummyElGamalEngine.java \
	net/i2p/crypto/DummyPooledRandomSource.java \
	net/i2p/crypto/ElGamalAESEngine.java \
	net/i2p/crypto/ElGamalEngine.java \
	net/i2p/crypto/EntropyHarvester.java \
	net/i2p/crypto/HMAC256Generator.java \
	net/i2p/crypto/HMACGenerator.java \
	net/i2p/crypto/KeyGenerator.java \
	net/i2p/crypto/PersistentSessionKeyManager.java \
	net/i2p/crypto/SessionKeyManager.java \
	net/i2p/crypto/SHA1.java \
	net/i2p/crypto/SHA1Test.java \
	net/i2p/crypto/SHA256Generator.java \
	net/i2p/crypto/TransientSessionKeyManager.java \
	net/i2p/crypto/TrustedUpdate.java \
	net/i2p/crypto/YKGenerator.java \
	net/i2p/data/Address.java \
	net/i2p/data/Base32.java \
	net/i2p/data/Base64.java \
	net/i2p/data/ByteArray.java \
	net/i2p/data/Certificate.java \
	net/i2p/data/DataFormatException.java \
	net/i2p/data/DataHelper.java \
	net/i2p/data/DataStructureImpl.java \
	net/i2p/data/DataStructure.java \
	net/i2p/data/Destination.java \
	net/i2p/data/Hash.java \
	net/i2p/data/i2cp/AbuseReason.java \
	net/i2p/data/i2cp/AbuseSeverity.java \
	net/i2p/data/i2cp/BandwidthLimitsMessage.java \
	net/i2p/data/i2cp/CreateLeaseSetMessage.java \
	net/i2p/data/i2cp/CreateSessionMessage.java \
	net/i2p/data/i2cp/DestLookupMessage.java \
	net/i2p/data/i2cp/DestReplyMessage.java \
	net/i2p/data/i2cp/DestroySessionMessage.java \
	net/i2p/data/i2cp/DisconnectMessage.java \
	net/i2p/data/i2cp/GetBandwidthLimitsMessage.java \
	net/i2p/data/i2cp/GetDateMessage.java \
	net/i2p/data/i2cp/I2CPMessageException.java \
	net/i2p/data/i2cp/I2CPMessageHandler.java \
	net/i2p/data/i2cp/I2CPMessageImpl.java \
	net/i2p/data/i2cp/I2CPMessage.java \
	net/i2p/data/i2cp/I2CPMessageReader.java \
	net/i2p/data/i2cp/MessageId.java \
	net/i2p/data/i2cp/MessagePayloadMessage.java \
	net/i2p/data/i2cp/MessageStatusMessage.java \
	net/i2p/data/i2cp/ReceiveMessageBeginMessage.java \
	net/i2p/data/i2cp/ReceiveMessageEndMessage.java \
	net/i2p/data/i2cp/ReconfigureSessionMessage.java \
	net/i2p/data/i2cp/ReportAbuseMessage.java \
	net/i2p/data/i2cp/RequestLeaseSetMessage.java \
	net/i2p/data/i2cp/SendMessageExpiresMessage.java \
	net/i2p/data/i2cp/SendMessageMessage.java \
	net/i2p/data/i2cp/SessionConfig.java \
	net/i2p/data/i2cp/SessionId.java \
	net/i2p/data/i2cp/SessionStatusMessage.java \
	net/i2p/data/i2cp/SetDateMessage.java \
	net/i2p/data/i2np/BuildRequestRecord.java \
	net/i2p/data/i2np/BuildResponseRecord.java \
	net/i2p/data/i2np/DatabaseLookupMessage.java \
	net/i2p/data/i2np/DatabaseSearchReplyMessage.java \
	net/i2p/data/i2np/DatabaseStoreMessage.java \
	net/i2p/data/i2np/DataMessage.java \
	net/i2p/data/i2np/DateMessage.java \
	net/i2p/data/i2np/DeliveryInstructions.java \
	net/i2p/data/i2np/DeliveryStatusMessage.java \
	net/i2p/data/i2np/EndPointPrivateKey.java \
	net/i2p/data/i2np/EndPointPublicKey.java \
	net/i2p/data/i2np/GarlicClove.java \
	net/i2p/data/i2np/GarlicMessage.java \
	net/i2p/data/i2np/I2NPMessageException.java \
	net/i2p/data/i2np/I2NPMessageHandler.java \
	net/i2p/data/i2np/I2NPMessageImpl.java \
	net/i2p/data/i2np/I2NPMessage.java \
	net/i2p/data/i2np/I2NPMessageReader.java \
	net/i2p/data/i2np/TunnelBuildMessage.java \
	net/i2p/data/i2np/TunnelBuildReplyMessage.java \
	net/i2p/data/i2np/TunnelCreateMessage.java \
	net/i2p/data/i2np/TunnelCreateStatusMessage.java \
	net/i2p/data/i2np/TunnelDataMessage.java \
	net/i2p/data/i2np/TunnelGatewayMessage.java \
	net/i2p/data/i2np/TunnelSessionKey.java \
	net/i2p/data/i2np/TunnelSigningPrivateKey.java \
	net/i2p/data/i2np/TunnelSigningPublicKey.java \
	net/i2p/data/i2np/TunnelVerificationStructure.java \
	net/i2p/data/Lease.java \
	net/i2p/data/LeaseSet.java \
	net/i2p/data/Payload.java \
	net/i2p/data/PrivateKeyFile.java \
	net/i2p/data/PrivateKey.java \
	net/i2p/data/PublicKey.java \
	net/i2p/data/RouterAddress.java \
	net/i2p/data/RouterIdentity.java \
	net/i2p/data/RouterInfo.java \
	net/i2p/data/RoutingKeyGenerator.java \
	net/i2p/data/SessionKey.java \
	net/i2p/data/SessionTag.java \
	net/i2p/data/Signature.java \
	net/i2p/data/SigningPrivateKey.java \
	net/i2p/data/SigningPublicKey.java \
	net/i2p/data/TunnelId.java \
	net/i2p/data/VerifiedDestination.java \
	net/i2p/I2PAppContext.java \
	net/i2p/I2PException.java \
	net/i2p/i2ptunnel/BufferLogger.java \
	net/i2p/i2ptunnel/HTTPResponseOutputStream.java \
	net/i2p/i2ptunnel/I2Ping.java \
	net/i2p/i2ptunnel/I2PTunnelClientBase.java \
	net/i2p/i2ptunnel/I2PTunnelClient.java \
	net/i2p/i2ptunnel/I2PTunnelConnectClient.java \
	net/i2p/i2ptunnel/I2PTunnelGUI.java \
	net/i2p/i2ptunnel/I2PTunnelHTTPClient.java \
	net/i2p/i2ptunnel/I2PTunnelHTTPClientRunner.java \
	net/i2p/i2ptunnel/I2PTunnelHTTPServer.java \
	net/i2p/i2ptunnel/I2PTunnelIRCClient.java \
	net/i2p/i2ptunnel/I2PTunnelIRCServer.java \
	net/i2p/i2ptunnel/I2PTunnel.java \
	net/i2p/i2ptunnel/I2PTunnelRunner.java \
	net/i2p/i2ptunnel/I2PTunnelServer.java \
	net/i2p/i2ptunnel/I2PTunnelTask.java \
	net/i2p/i2ptunnel/Logging.java \
	net/i2p/i2ptunnel/socks/I2PSOCKSTunnel.java \
	net/i2p/i2ptunnel/socks/MultiSink.java \
	net/i2p/i2ptunnel/socks/ReplyTracker.java \
	net/i2p/i2ptunnel/socks/SOCKS4aServer.java \
	net/i2p/i2ptunnel/socks/SOCKS5Server.java \
	net/i2p/i2ptunnel/socks/SOCKSException.java \
	net/i2p/i2ptunnel/socks/SOCKSHeader.java \
	net/i2p/i2ptunnel/socks/SOCKSServerFactory.java \
	net/i2p/i2ptunnel/socks/SOCKSServer.java \
	net/i2p/i2ptunnel/socks/SOCKSUDPPort.java \
	net/i2p/i2ptunnel/socks/SOCKSUDPTunnel.java \
	net/i2p/i2ptunnel/socks/SOCKSUDPUnwrapper.java \
	net/i2p/i2ptunnel/socks/SOCKSUDPWrapper.java \
	net/i2p/i2ptunnel/streamr/MultiSource.java \
	net/i2p/i2ptunnel/streamr/Pinger.java \
	net/i2p/i2ptunnel/streamr/StreamrConsumer.java \
	net/i2p/i2ptunnel/streamr/StreamrProducer.java \
	net/i2p/i2ptunnel/streamr/Subscriber.java \
	net/i2p/i2ptunnel/TunnelControllerGroup.java \
	net/i2p/i2ptunnel/TunnelController.java \
	net/i2p/i2ptunnel/udp/I2PSinkAnywhere.java \
	net/i2p/i2ptunnel/udp/I2PSink.java \
	net/i2p/i2ptunnel/udp/I2PSource.java \
	net/i2p/i2ptunnel/udp/Sink.java \
	net/i2p/i2ptunnel/udp/Source.java \
	net/i2p/i2ptunnel/udp/Stream.java \
	net/i2p/i2ptunnel/udpTunnel/I2PTunnelUDPClientBase.java \
	net/i2p/i2ptunnel/udpTunnel/I2PTunnelUDPServerBase.java \
	net/i2p/i2ptunnel/udp/UDPSink.java \
	net/i2p/i2ptunnel/udp/UDPSource.java \
	net/i2p/i2ptunnel/web/EditBean.java \
	net/i2p/i2ptunnel/web/IndexBean.java \
	net/i2p/router/admin/AdminListener.java \
	net/i2p/router/admin/AdminManager.java \
	net/i2p/router/admin/AdminRunner.java \
	net/i2p/router/admin/StatsGenerator.java \
	net/i2p/router/Blocklist.java \
	net/i2p/router/client/ClientConnectionRunner.java \
	net/i2p/router/client/ClientListenerRunner.java \
	net/i2p/router/client/ClientManagerFacadeImpl.java \
	net/i2p/router/client/ClientManager.java \
	net/i2p/router/client/ClientMessageEventListener.java \
	net/i2p/router/client/ClientWriterRunner.java \
	net/i2p/router/client/CreateSessionJob.java \
	net/i2p/router/client/LeaseRequestState.java \
	net/i2p/router/client/LookupDestJob.java \
	net/i2p/router/ClientManagerFacade.java \
	net/i2p/router/ClientMessage.java \
	net/i2p/router/ClientMessagePool.java \
	net/i2p/router/client/MessageReceivedJob.java \
	net/i2p/router/client/ReportAbuseJob.java \
	net/i2p/router/client/RequestLeaseSetJob.java \
	net/i2p/router/ClientTunnelSettings.java \
	net/i2p/router/CommSystemFacade.java \
	net/i2p/router/DummyClientManagerFacade.java \
	net/i2p/router/DummyCommSystemFacade.java \
	net/i2p/router/DummyNetworkDatabaseFacade.java \
	net/i2p/router/DummyPeerManagerFacade.java \
	net/i2p/router/DummyTunnelManagerFacade.java \
	net/i2p/router/HandlerJobBuilder.java \
	net/i2p/router/InNetMessagePool.java \
	net/i2p/router/JobImpl.java \
	net/i2p/router/Job.java \
	net/i2p/router/JobQueue.java \
	net/i2p/router/JobQueueRunner.java \
	net/i2p/router/JobStats.java \
	net/i2p/router/JobTiming.java \
	net/i2p/router/KeyManager.java \
	net/i2p/router/LeaseSetKeys.java \
	net/i2p/router/LoadTestManager.java \
	net/i2p/router/message/BuildTestMessageJob.java \
	net/i2p/router/message/CloveSet.java \
	net/i2p/router/message/GarlicConfig.java \
	net/i2p/router/message/GarlicMessageBuilder.java \
	net/i2p/router/message/GarlicMessageHandler.java \
	net/i2p/router/message/GarlicMessageParser.java \
	net/i2p/router/message/GarlicMessageReceiver.java \
	net/i2p/router/message/HandleGarlicMessageJob.java \
	net/i2p/router/MessageHistory.java \
	net/i2p/router/message/OutboundClientMessageJobHelper.java \
	net/i2p/router/message/OutboundClientMessageOneShotJob.java \
	net/i2p/router/message/OutboundClientMessageStatus.java \
	net/i2p/router/message/PayloadGarlicConfig.java \
	net/i2p/router/MessageReceptionInfo.java \
	net/i2p/router/MessageSelector.java \
	net/i2p/router/message/SendGarlicJob.java \
	net/i2p/router/message/SendMessageDirectJob.java \
	net/i2p/router/MessageStateMonitor.java \
	net/i2p/router/MessageValidator.java \
	net/i2p/router/MultiRouterBuilder.java \
	net/i2p/router/MultiRouter.java \
	net/i2p/router/NetworkDatabaseFacade.java \
	net/i2p/router/networkdb/DatabaseLookupMessageHandler.java \
	net/i2p/router/networkdb/DatabaseStoreMessageHandler.java \
	net/i2p/router/networkdb/HandleDatabaseLookupMessageJob.java \
	net/i2p/router/networkdb/HandleDatabaseStoreMessageJob.java \
	net/i2p/router/networkdb/kademlia/DataStore.java \
	net/i2p/router/networkdb/kademlia/ExpireLeasesJob.java \
	net/i2p/router/networkdb/kademlia/ExpireRoutersJob.java \
	net/i2p/router/networkdb/kademlia/ExploreJob.java \
	net/i2p/router/networkdb/kademlia/ExploreKeySelectorJob.java \
	net/i2p/router/networkdb/kademlia/FloodfillDatabaseLookupMessageHandler.java \
	net/i2p/router/networkdb/kademlia/FloodfillDatabaseStoreMessageHandler.java \
	net/i2p/router/networkdb/kademlia/FloodfillMonitorJob.java \
	net/i2p/router/networkdb/kademlia/FloodfillNetworkDatabaseFacade.java \
	net/i2p/router/networkdb/kademlia/FloodfillPeerSelector.java \
	net/i2p/router/networkdb/kademlia/FloodfillStoreJob.java \
	net/i2p/router/networkdb/kademlia/FloodfillVerifyStoreJob.java \
	net/i2p/router/networkdb/kademlia/FloodOnlyLookupMatchJob.java \
	net/i2p/router/networkdb/kademlia/FloodOnlyLookupSelector.java \
	net/i2p/router/networkdb/kademlia/FloodOnlyLookupTimeoutJob.java \
	net/i2p/router/networkdb/kademlia/FloodOnlySearchJob.java \
	net/i2p/router/networkdb/kademlia/HandleFloodfillDatabaseLookupMessageJob.java \
	net/i2p/router/networkdb/kademlia/HandleFloodfillDatabaseStoreMessageJob.java \
	net/i2p/router/networkdb/kademlia/HarvesterJob.java \
	net/i2p/router/networkdb/kademlia/KademliaNetworkDatabaseFacade.java \
	net/i2p/router/networkdb/kademlia/KBucketImpl.java \
	net/i2p/router/networkdb/kademlia/KBucket.java \
	net/i2p/router/networkdb/kademlia/KBucketSet.java \
	net/i2p/router/networkdb/kademlia/OnRepublishFailure.java \
	net/i2p/router/networkdb/kademlia/OnRepublishSuccess.java \
	net/i2p/router/networkdb/kademlia/PeerSelector.java \
	net/i2p/router/networkdb/kademlia/PersistentDataStore.java \
	net/i2p/router/networkdb/kademlia/ReplyNotVerifiedJob.java \
	net/i2p/router/networkdb/kademlia/ReplyVerifiedJob.java \
	net/i2p/router/networkdb/kademlia/RepublishLeaseSetJob.java \
	net/i2p/router/networkdb/kademlia/RouterGenerator.java \
	net/i2p/router/networkdb/kademlia/SearchJob.java \
	net/i2p/router/networkdb/kademlia/SearchMessageSelector.java \
	net/i2p/router/networkdb/kademlia/SearchReplyJob.java \
	net/i2p/router/networkdb/kademlia/SearchState.java \
	net/i2p/router/networkdb/kademlia/SearchUpdateReplyFoundJob.java \
	net/i2p/router/networkdb/kademlia/SelectionCollector.java \
	net/i2p/router/networkdb/kademlia/SingleLookupJob.java \
	net/i2p/router/networkdb/kademlia/SingleSearchJob.java \
	net/i2p/router/networkdb/kademlia/StartExplorersJob.java \
	net/i2p/router/networkdb/kademlia/StoreJob.java \
	net/i2p/router/networkdb/kademlia/StoreMessageSelector.java \
	net/i2p/router/networkdb/kademlia/StoreState.java \
	net/i2p/router/networkdb/kademlia/TransientDataStore.java \
	net/i2p/router/networkdb/kademlia/XORComparator.java \
	net/i2p/router/networkdb/PublishLocalRouterInfoJob.java \
	net/i2p/router/OutNetMessage.java \
	net/i2p/router/OutNetMessagePool.java \
	net/i2p/router/peermanager/Calculator.java \
	net/i2p/router/peermanager/CapacityCalculator.java \
	net/i2p/router/peermanager/DBHistory.java \
	net/i2p/router/PeerManagerFacade.java \
	net/i2p/router/peermanager/IntegrationCalculator.java \
	net/i2p/router/peermanager/InverseCapacityComparator.java \
	net/i2p/router/peermanager/PeerManagerFacadeImpl.java \
	net/i2p/router/peermanager/PeerManager.java \
	net/i2p/router/peermanager/PeerProfile.java \
	net/i2p/router/peermanager/PeerTestJob.java \
	net/i2p/router/peermanager/PersistProfileJob.java \
	net/i2p/router/peermanager/PersistProfilesJob.java \
	net/i2p/router/peermanager/ProfileManagerImpl.java \
	net/i2p/router/peermanager/ProfileOrganizer.java \
	net/i2p/router/peermanager/ProfileOrganizerRenderer.java \
	net/i2p/router/peermanager/ProfilePersistenceHelper.java \
	net/i2p/router/peermanager/SpeedCalculator.java \
	net/i2p/router/peermanager/TunnelHistory.java \
	net/i2p/router/PeerSelectionCriteria.java \
	net/i2p/router/PersistentKeyRing.java \
	net/i2p/router/ProfileManager.java \
	net/i2p/router/ReplyJob.java \
	net/i2p/router/RouterClock.java \
	net/i2p/router/RouterContext.java \
	net/i2p/router/RouterDoSThrottle.java \
	net/i2p/router/Router.java \
	net/i2p/router/RouterLaunch.java \
	net/i2p/router/RouterThrottleImpl.java \
	net/i2p/router/RouterThrottle.java \
	net/i2p/router/RouterVersion.java \
	net/i2p/router/RouterWatchdog.java \
	net/i2p/router/Service.java \
	net/i2p/router/SessionKeyPersistenceHelper.java \
	net/i2p/router/Shitlist.java \
	net/i2p/router/SSUDemo.java \
	net/i2p/router/startup/BootCommSystemJob.java \
	net/i2p/router/startup/BootNetworkDbJob.java \
	net/i2p/router/startup/BuildTrustedLinksJob.java \
	net/i2p/router/startup/ClientAppConfig.java \
	net/i2p/router/startup/CreateRouterInfoJob.java \
	net/i2p/router/startup/LoadClientAppsJob.java \
	net/i2p/router/startup/LoadRouterInfoJob.java \
	net/i2p/router/startup/ReadConfigJob.java \
	net/i2p/router/startup/RebuildRouterInfoJob.java \
	net/i2p/router/startup/StartAcceptingClientsJob.java \
	net/i2p/router/startup/StartupJob.java \
	net/i2p/router/StatisticsManager.java \
	net/i2p/router/SubmitMessageHistoryJob.java \
	net/i2p/router/transport/BandwidthLimitedInputStream.java \
	net/i2p/router/transport/BandwidthLimitedOutputStream.java \
	net/i2p/router/transport/CommSystemFacadeImpl.java \
	net/i2p/router/transport/FIFOBandwidthLimiter.java \
	net/i2p/router/transport/FIFOBandwidthRefiller.java \
	net/i2p/router/transport/GetBidsJob.java \
	net/i2p/router/transport/ntcp/EstablishState.java \
	net/i2p/router/transport/ntcp/EventPumper.java \
	net/i2p/router/transport/ntcp/NTCPAddress.java \
	net/i2p/router/transport/ntcp/NTCPConnection.java \
	net/i2p/router/transport/ntcp/NTCPSendFinisher.java \
	net/i2p/router/transport/ntcp/NTCPTransport.java \
	net/i2p/router/transport/ntcp/Reader.java \
	net/i2p/router/transport/ntcp/Writer.java \
	net/i2p/router/transport/OutboundMessageRegistry.java \
	net/i2p/router/transport/TransportBid.java \
	net/i2p/router/transport/TransportEventListener.java \
	net/i2p/router/transport/TransportImpl.java \
	net/i2p/router/transport/Transport.java \
	net/i2p/router/transport/TransportManager.java \
	net/i2p/router/transport/udp/ACKBitfield.java \
	net/i2p/router/transport/udp/ACKSender.java \
	net/i2p/router/transport/udp/EstablishmentManager.java \
	net/i2p/router/transport/udp/InboundEstablishState.java \
	net/i2p/router/transport/udp/InboundMessageFragments.java \
	net/i2p/router/transport/udp/InboundMessageState.java \
	net/i2p/router/transport/udp/IntroductionManager.java \
	net/i2p/router/transport/udp/MessageQueue.java \
	net/i2p/router/transport/udp/MessageReceiver.java \
	net/i2p/router/transport/udp/OutboundEstablishState.java \
	net/i2p/router/transport/udp/OutboundMessageFragments.java \
	net/i2p/router/transport/udp/OutboundMessageState.java \
	net/i2p/router/transport/udp/OutboundRefiller.java \
	net/i2p/router/transport/udp/PacketBuilder.java \
	net/i2p/router/transport/udp/PacketHandler.java \
	net/i2p/router/transport/udp/PacketPusher.java \
	net/i2p/router/transport/udp/PeerState.java \
	net/i2p/router/transport/udp/PeerTestManager.java \
	net/i2p/router/transport/udp/PeerTestState.java \
	net/i2p/router/transport/udp/RemoteHostId.java \
	net/i2p/router/transport/udp/TimedWeightedPriorityMessageQueue.java \
	net/i2p/router/transport/udp/UDPAddress.java \
	net/i2p/router/transport/udp/UDPEndpoint.java \
	net/i2p/router/transport/udp/UDPEndpointTest.java \
	net/i2p/router/transport/udp/UDPFlooder.java \
	net/i2p/router/transport/udp/UDPPacket.java \
	net/i2p/router/transport/udp/UDPPacketReader.java \
	net/i2p/router/transport/udp/UDPReceiver.java \
	net/i2p/router/transport/udp/UDPSender.java \
	net/i2p/router/transport/udp/UDPTransport.java \
	net/i2p/router/transport/VMCommSystem.java \
	net/i2p/router/tunnel/BatchedPreprocessor.java \
	net/i2p/router/tunnel/BatchedRouterPreprocessor.java \
	net/i2p/router/tunnel/BloomFilterIVValidator.java \
	net/i2p/router/tunnel/BuildMessageGenerator.java \
	net/i2p/router/tunnel/BuildMessageProcessor.java \
	net/i2p/router/tunnel/BuildMessageTest.java \
	net/i2p/router/tunnel/BuildReplyHandler.java \
	net/i2p/router/tunnel/DummyValidator.java \
	net/i2p/router/tunnel/FlushTimer.java \
	net/i2p/router/tunnel/FragmentedMessage.java \
	net/i2p/router/tunnel/FragmentHandler.java \
	net/i2p/router/tunnel/HashSetIVValidator.java \
	net/i2p/router/tunnel/HopConfig.java \
	net/i2p/router/tunnel/HopProcessor.java \
	net/i2p/router/tunnel/InboundEndpointProcessor.java \
	net/i2p/router/tunnel/InboundGatewayProcessor.java \
	net/i2p/router/tunnel/InboundGatewayReceiver.java \
	net/i2p/router/tunnel/InboundMessageDistributor.java \
	net/i2p/router/tunnel/InboundSender.java \
	net/i2p/router/TunnelInfo.java \
	net/i2p/router/tunnel/IVValidator.java \
	net/i2p/router/TunnelManagerFacade.java \
	net/i2p/router/tunnel/OutboundGatewayProcessor.java \
	net/i2p/router/tunnel/OutboundMessageDistributor.java \
	net/i2p/router/tunnel/OutboundReceiver.java \
	net/i2p/router/tunnel/OutboundSender.java \
	net/i2p/router/tunnel/OutboundTunnelEndpoint.java \
	net/i2p/router/tunnel/pool/BuildExecutor.java \
	net/i2p/router/tunnel/pool/BuildHandler.java \
	net/i2p/router/tunnel/pool/BuildRequestor.java \
	net/i2p/router/tunnel/pool/ClientPeerSelector.java \
	net/i2p/router/tunnel/pool/ExpireJob.java \
	net/i2p/router/tunnel/pool/ExploratoryPeerSelector.java \
	net/i2p/router/tunnel/pool/PooledTunnelCreatorConfig.java \
	net/i2p/router/tunnel/pool/SendGarlicMessageJob.java \
	net/i2p/router/TunnelPoolSettings.java \
	net/i2p/router/tunnel/pool/TestJob.java \
	net/i2p/router/tunnel/pool/TunnelPeerSelector.java \
	net/i2p/router/tunnel/pool/TunnelPool.java \
	net/i2p/router/tunnel/pool/TunnelPoolManager.java \
	net/i2p/router/tunnel/PumpedTunnelGateway.java \
	net/i2p/router/tunnel/RouterFragmentHandler.java \
	net/i2p/router/TunnelSelectionCriteria.java \
	net/i2p/router/TunnelSettings.java \
	net/i2p/router/tunnel/TrivialPreprocessor.java \
	net/i2p/router/tunnel/TrivialRouterPreprocessor.java \
	net/i2p/router/tunnel/TunnelCreatorConfig.java \
	net/i2p/router/tunnel/TunnelDispatcher.java \
	net/i2p/router/tunnel/TunnelGateway.java \
	net/i2p/router/tunnel/TunnelGatewayPumper.java \
	net/i2p/router/tunnel/TunnelGatewayZeroHop.java \
	net/i2p/router/tunnel/TunnelParticipant.java \
	net/i2p/router/web/ContextHelper.java \
	net/i2p/router/web/ReseedHandler.java \
	net/i2p/sam/client/SAMClientEventListenerImpl.java \
	net/i2p/sam/client/SAMEventHandler.java \
	net/i2p/sam/client/SAMReader.java \
	net/i2p/sam/client/SAMStreamSend.java \
	net/i2p/sam/client/SAMStreamSink.java \
	net/i2p/sam/SAMBridge.java \
	net/i2p/sam/SAMDatagramReceiver.java \
	net/i2p/sam/SAMDatagramSession.java \
	net/i2p/sam/SAMException.java \
	net/i2p/sam/SAMHandlerFactory.java \
	net/i2p/sam/SAMHandler.java \
	net/i2p/sam/SAMInvalidDirectionException.java \
	net/i2p/sam/SAMMessageSession.java \
	net/i2p/sam/SAMRawReceiver.java \
	net/i2p/sam/SAMRawSession.java \
	net/i2p/sam/SAMStreamReceiver.java \
	net/i2p/sam/SAMStreamSession.java \
	net/i2p/sam/SAMUtils.java \
	net/i2p/sam/SAMv1Handler.java \
	net/i2p/sam/SAMv2Handler.java \
	net/i2p/sam/SAMv2StreamSession.java \
	net/i2p/sam/SAMv3DatagramSession.java \
	net/i2p/sam/SAMv3Handler.java \
	net/i2p/sam/SAMv3RawSession.java \
	net/i2p/sam/SAMv3StreamSession.java \
	net/i2p/stat/BufferedStatLog.java \
	net/i2p/stat/Frequency.java \
	net/i2p/stat/FrequencyStat.java \
	net/i2p/stat/PersistenceHelper.java \
	net/i2p/stat/Rate.java \
	net/i2p/stat/RateStat.java \
	net/i2p/stat/RateSummaryListener.java \
	net/i2p/stat/SimpleStatDumper.java \
	net/i2p/stat/SizeTest.java \
	net/i2p/stat/StatLog.java \
	net/i2p/stat/StatLogSplitter.java \
	net/i2p/stat/StatManager.java \
	net/i2p/time/NtpClient.java \
	net/i2p/time/NtpMessage.java \
	net/i2p/time/Timestamper.java \
	net/i2p/util/BufferedRandomSource.java \
	net/i2p/util/ByteCache.java \
	net/i2p/util/CachingByteArrayOutputStream.java \
	net/i2p/util/Clock.java \
	net/i2p/util/ConcurrentHashSet.java \
	net/i2p/util/ConvertToHash.java \
	net/i2p/util/Copy.java \
	net/i2p/util/DecayingBloomFilter.java \
	net/i2p/util/Delete.java \
	net/i2p/util/EepGet.java \
	net/i2p/util/EepGetScheduler.java \
	net/i2p/util/EepPost.java \
	net/i2p/util/EventDispatcherImpl.java \
	net/i2p/util/EventDispatcher.java \
	net/i2p/util/Exec.java \
	net/i2p/util/Executor.java \
	net/i2p/util/FileUtil.java \
	net/i2p/util/FortunaRandomSource.java \
	net/i2p/util/HexDump.java \
	net/i2p/util/HTTPSendData.java \
	net/i2p/util/I2PAppThread.java \
	net/i2p/util/I2PThread.java \
	net/i2p/util/KeyRing.java \
	net/i2p/util/LogConsoleBuffer.java \
	net/i2p/util/Log.java \
	net/i2p/util/LogLimit.java \
	net/i2p/util/LogManager.java \
	net/i2p/util/LogRecordFormatter.java \
	net/i2p/util/LogRecord.java \
	net/i2p/util/LogWriter.java \
	net/i2p/util/LookaheadInputStream.java \
	net/i2p/util/NativeBigInteger.java \
	net/i2p/util/OrderedProperties.java \
	net/i2p/util/PooledRandomSource.java \
	net/i2p/util/RandomSource.java \
	net/i2p/util/ResettableGZIPInputStream.java \
	net/i2p/util/ResettableGZIPOutputStream.java \
	net/i2p/util/ReusableGZIPInputStream.java \
	net/i2p/util/ReusableGZIPOutputStream.java \
	net/i2p/util/ShellCommand.java \
	net/i2p/util/SimpleScheduler.java \
	net/i2p/util/SimpleStore.java \
	net/i2p/util/SimpleTimer2.java \
	net/i2p/util/SimpleTimer.java \
	net/i2p/util/SocketTimeout.java \
	org/bouncycastle/crypto/Digest.java \
	org/bouncycastle/crypto/digests/GeneralDigest.java \
	org/bouncycastle/crypto/digests/MD5Digest.java \
	org/bouncycastle/crypto/Mac.java \
	org/bouncycastle/crypto/macs/HMac.java \
	org/xlattice/crypto/filters/BloomSHA1.java \
	org/xlattice/crypto/filters/KeySelector.java \
	CI2PRouterThread.java \
	CMuleLogger.java \
	I2PRouter.java \
	WxStringWriter.java 

