/**
 *
 *                   RUN Router THREAD CLASS
 *
 */


import java.io.Writer;
import java.io.PrintWriter;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.lang.Thread ;
import java.lang.Throwable;
import java.io.File;
import java.util.Properties;
import java.lang.InterruptedException;

import net.i2p.router.Router ;
import net.i2p.i2ptunnel.TunnelControllerGroup;
import net.i2p.router.web.ReseedHandler ;

public class CI2PRouterThread extends Thread
{
		static final int NOT_STARTED = 1;

		static final int RUNNING     = 2;

		static final int STOPPED     = 3;

		int m_state = 1;

		int m_order = 1;

		CMuleLogger m_logger = null;

		Properties props = null;

		Router router    = null;

		//static CI2PRouterThread routerThread = null;

		/*public static void startRouter ( Properties p, int logId )
		{
			routerThread = new CI2PRouterThread ( p, logId ) ;
		}
		*/
		
		public CI2PRouterThread ( Properties p, int logId )

		{
			super ( "CI2PRouterThread" );

			m_logger = new CMuleLogger ( logId );

			m_state = NOT_STARTED    ;

			m_order = RUNNING    ;

			props = ( Properties ) p.clone() ;

			tryDeletingPingFile() ;

			m_logger.log ( "starting router thread" );

			setDaemon ( true ) ;

			start() ;

			m_logger.log ( "router thread started" );

		}



		private void makeTunnelConfigFile()
		{
			try
			{
				File tunnelConfigFile = new File ( "i2ptunnel.config" ) ;

				if ( ! tunnelConfigFile.exists() )
				{
					PrintWriter tunnelConfig = new PrintWriter ( new BufferedWriter ( new FileWriter ( "i2ptunnel.config" ) ) );

					tunnelConfig.write (
					    "tunnel.0.description=HTTP proxy for browsing eepsites and the web\n"
					    + "tunnel.0.i2cpHost=" + props.getProperty ( "i2cp.tcp.host" ) + "\n"
					    + "tunnel.0.i2cpPort=" + props.getProperty ( "i2cp.tcp.port" ) + "\n"
					    + "tunnel.0.interface=" + props.getProperty ( "i2cp.tcp.host" ) + "\n"
					    + "tunnel.0.listenPort=" + props.getProperty ( "eepProxy.port" ) + "\n"
					    + "tunnel.0.name=eepProxy\n"
					    + "tunnel.0.option.i2p.streaming.connectDelay=1000\n"
					    + "tunnel.0.option.inbound.nickname=i2p proxy\n"
					    + "tunnel.0.option.outbound.nickname=i2p proxy\n"
					    + "tunnel.0.proxyList=squid.i2p\n"
					    + "tunnel.0.sharedClient=true\n"
					    + "tunnel.0.startOnLoad=true\n"
					    + "tunnel.0.type=httpclient\n"
					);

					tunnelConfig.close();
				}
			}
			catch ( Throwable t ) {}}


		private void makeHostsFile()
		{
			try
			{
				File hostsFile = new File ( "userhosts.txt" ) ;

				if ( ! hostsFile.exists() )
				{
					PrintWriter hosts = new PrintWriter ( new BufferedWriter ( new FileWriter ( "userhosts.txt" ) ) );

					hosts.write (
					    "www.imule.i2p="
					    + "fDaVcuhylYrEq7aBNACuM~j3ymwZqS7ws-TsqNinsp2aO69mUVLY4On01r4cr4wBpWpe4e24aK~DBOIwsPb1M3WkS"
					    + "ZVoJX8zk8aGunG2X6mnhPTdT~HoCOhGi-Q8n27AoHrESa0MGTLRfw-UM9tePlgBOkqkbU98ROWj40hm3esD1V02Rnw"
					    + "vMrDHMIk69GVn8iKUTSEBDmHLFyaVpcQtFy7z09-Z45uqupd244vhKFZIjTTlPZFmI5KzAibBT-uoacdcbyomuPEjMI2a"
					    + "eK58uhPCdFW8-TinkTDdDq4oPrwqgvG1dAyfOqY9-~AS0Mc3-jrwXcdKh-iug2~mHc0hzmpvx~if6F5CeYJxYuivXMP"
					    + "RX0gSuLXK0er4tYkC0~wFLqBbyNr0MJKSnDKof885Fu9-tKhoAHf06PuPW7u104GjUbBKxS5D9JxdpX9DNhXBFa9l5"
					    + "mU9j7~gbUNzV3N4IAHkrD-GnYOZxWmSUqmbie6Lo5B8R7zqpKikGDlmmJtHAAAA\n" );

					hosts.close() ;
				}
			}
			catch ( Throwable t ) {}
		}

		private void makeLogConfigFile()
		{
			try
			{
				File logConfigFile = new File ( net.i2p.util.LogManager.CONFIG_LOCATION_DEFAULT ) ;

				if ( ! logConfigFile.exists() )
				{
					PrintWriter logConfigWriter = new PrintWriter
							( new BufferedWriter
							( new FileWriter
							( net.i2p.util.LogManager.CONFIG_LOCATION_DEFAULT ) ) );

					logConfigWriter.write ( net.i2p.util.LogManager.PROP_DEFAULTLEVEL
//							+ "=" + net.i2p.util.Log.STR_DEBUG + "\n");
//                          + "=" + net.i2p.util.Log.STR_INFO + "\n");
                            + "=" + net.i2p.util.Log.STR_ERROR + "\n");

					logConfigWriter.close() ;
				}
			}
			catch ( Throwable t ) {}
		}


		private void callPeerReseeder()
		{
			// reseeding peers if needed

			try
			{
				File netDb = new File ( "netDb" );

				String names[] = ( netDb.exists() ? netDb.list() : null );

				if ( ( names == null ) || ( names.length < 15 ) )
				{
					if ( names==null ) {
						m_logger.logCritical ( "CI2PRouterThread: Request Reseeding, because :" );
					
						m_logger.logCritical ( "netDb list does not exist" );
					} else {
						m_logger.logCritical ( "CI2PRouterThread: Request Reseeding, because :" );
					
						m_logger.logCritical ( "length of netDb list is " + names.length );
					}
					ReseedHandler reseedHandler = new ReseedHandler();
					reseedHandler.requestReseed();
				}
			}
			catch ( Throwable t ) {}}


		private void makeAppsConfigFile()
		{
			try
			{
				PrintWriter appsConfig = new PrintWriter ( new BufferedWriter ( new FileWriter ( "clients.config" ) ) );

				appsConfig.write (
				    "# start up the SAM bridge so other client apps can connect\n"
				    + "clientApp.0.main=net.i2p.sam.SAMBridge\n"
				    + "clientApp.0.name=SAMBridge\n"
				    + "clientApp.0.args=sam.keys "
				    + props.getProperty ( "i2cp.tcp.host" ) + " "
				    + props.getProperty ( "sam.port" )
				    + " i2cp.tcp.host=" + props.getProperty ( "i2cp.tcp.host" )
				    + " i2cp.tcp.port=" + props.getProperty ( "i2cp.tcp.port" )
				    + "\n\n\n"

				    + "# poke the i2ptunnels defined in i2ptunnel.config\n"
				    + "clientApp.1.main=net.i2p.i2ptunnel.TunnelControllerGroup\n"
				    + "clientApp.1.name=Tunnels\n"
				    + "clientApp.1.args=i2ptunnel.config\n\n\n"

				    + "# start up addressbook daemon\n"
				    + "clientApp.2.main=addressbook.Daemon\n"
				    + "clientApp.2.name=AddressBookDaemon\n"
				    + "clientApp.2.args=addressbook\n\n\n"
				);

				appsConfig.close();
			}
			catch ( Throwable t ) {}}

		public void run()

		{
			m_logger.log ( "routerThread::run - SetPriority" );
			setPriority ( MIN_PRIORITY );

			try
			{
				synchronized ( this )
				{
					// default config file for tunnels : proxy 4444

					m_logger.log ( "routerThread::run - makeLogConfigFile" );
					makeLogConfigFile();

					m_logger.log ( "routerThread::run - makeTullelConfigFile" );
					makeTunnelConfigFile() ;

					m_logger.log ( "routerThread::run - makeHostsFile" );
					makeHostsFile()        ;

					m_logger.log ( "routerThread::run - makeAppsConfig" );
					makeAppsConfigFile()   ;


					// create router
					
					//net.i2p.I2PAppContext.getGlobalContext().
					//		logManager().setDefaultLimit ( "DEBUG" );

					m_logger.log ( "routerThread::run - new Router" );
					router = new Router ( props );

					if ( router != null )
						m_logger.log ( "CI2PRouterThread: Router has been created" );
					else
					{
						m_logger.log ( "CI2PRouterThread: Router init failed" );
						m_state = STOPPED;
						notifyAll() ;
						return;
					}

					if ( m_order == STOPPED )
					{
						m_state = STOPPED;
						return;
					}

					m_logger.log ( "routerThread::run - runRouter" );
					router.runRouter() ;

					m_state = RUNNING ;

					notifyAll() ;

					m_logger.log ( "CI2PRouterThread: Router is running" );

					callPeerReseeder()     ;

					while ( m_order == RUNNING ) 
					{
						wait();
					}

					m_logger.log ( "CI2PRouterThread: calling router.shutdown(0)" );

					router.shutdown ( 0 ) ;

					m_logger.log ( "CI2PRouterThread: exit thread" );
				}

				//router.getContext().logManager().saveConfig();
			}
			catch ( Throwable t )
			{
				StackTraceElement[] stack = t.getStackTrace();
				String message = "";
				while (t!=null) {
					message += t.toString()+"\n";
					t = t.getCause();
				}
				
				message += "\nstacktrace : ";
				for (int i = 0; i < stack.length; i++) {
					message += "\n" + stack[i].toString();
				}
				m_logger.logCritical ( message );
			}

			synchronized ( this )
			{
				m_state = STOPPED;
				notifyAll() ;
			}

		}

		public boolean isRunning()

		{
			return m_state == RUNNING ;
		}


		public  void stopRouter()

		{
			m_logger.log ( "CI2PRouterThread: stopRouter: entering" );

			//if ( routerThread != null )
				synchronized ( this )
				{
					m_logger.log ( "CI2PRouterThread: stopRouter: ordering to stop" );

					//if ( isRunning() ) router.shutdownGracefully() ;

					if ( isRunning() )
					{
						m_order = STOPPED ;
						/*routerThread.*/notify() ;
					}

					m_logger.log ( "CI2PRouterThread: stopRouter: removing file lock" );

					String pingFile = props.getProperty ( "router.pingFile" ) ;
					File file = new File ( pingFile );

					if ( file.exists() ) file.delete();

					/*while ( file.exists() )
					{
						m_logger.log ( "CI2PRouterTHread: stopRouter: Waiting for ping file deletion" );

						try
						{
							sleep ( 1000 );
						}
						catch ( InterruptedException e )
						{
							m_logger.log ( "CI2PRouterTHread: stopRouter: Interrupted while sleeping" );
							break;
						}
					}*/

					m_logger.log ( "CI2PRouterThread: stopRouter: leaving" );
				}
		}


		public  void tryDeletingPingFile()

		{
			String pingFile = props.getProperty ( "router.pingFile" ) ;

			File file = new File ( pingFile );

			if ( file.exists() )
			{
				if ( file.delete() )
				{
					m_logger.log ( "I2P Router already launched or not cleanly shut down. This ping file has been removed : "
					               + pingFile );
				}
				else
				{
					m_logger.log ( "I2P Router already launched or not cleanly shut down : the followin ping file already exists"
					               + "and I cannot delete it : " + pingFile );
					m_logger.log ( "Delete the ping file and restart iMule, or disable the internal I2P Router in preferences if you are running I2P independantly of iMule." ) ;
				}

			}
		}
		
		public void getInfo(Writer w)
		{
			if (router==null) return ;
			
			try {
				router.renderStatusHTML( w );
			} catch (Throwable e)
			{
			}
		}
}


/*
JvInitClass ( &System::class$ );

JvInitClass ( &net::i2p::client::naming::HostsTxtNamingService::class$ );

System::setProperty ( toJstring ( wxT ( "i2p.naming.impl" ) ),
		      toJstring ( wxT ( "net.i2p.client.naming.HostsTxtNamingService" ) ) );

m_javaContext = net::i2p::I2PAppContext::getGlobalContext();
#ifdef __DEBUG__
m_javaContext->logManager()->setDefaultLimit ( toJstring ( wxT ( "DEBUG" ) ) );
#endif

JvInitClass ( &I2PClient::class$ );

JvInitClass ( &I2PClientImpl::class$ );

JvInitClass ( &net::i2p::util::LogRecordFormatter::class$ );

JvInitClass ( &net::i2p::client::streaming::I2PSocketManagerFull::class$ );

JvInitClass ( &net::i2p::client::naming::FilesystemAddressDB::class$ );

JvInitClass ( &net::i2p::client::naming::PetNameNamingService::class$ );

JvInitClass ( &net::i2p::client::naming::MetaNamingService::class$ );

JvInitClass ( &net::i2p::i2ptunnel::TunnelControllerGroup::class$ );

JvInitClass ( &addressbook::Daemon::class$ );

JvInitClass ( &I2PSocketManagerFactory::class$ );

JvInitClass ( &ConnectionOptions::class$ );

JvInitClass ( &LogManager::class$ );

JvInitClass ( &Router::class$ );

JvInitClass ( &FIFOBandwidthRefiller::class$ );

JvInitClass ( &TCPTransport::class$ );

JvInitClass ( &UDPTransport::class$ );

*/
