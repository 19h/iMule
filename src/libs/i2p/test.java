import net.i2p.client.*;
import net.i2p.client.streaming.*;
import java.io.*;
import java.lang.System;
import net.i2p.data.*;
import net.i2p.data.Base64;
import java.util.Properties;

public class test
{

		static String tcpKeyFileName;
		static String udpKeyFileName;
		static I2PSession _session;
		public test() {}
		
		
		public static void main(String args[])
		{
			try
			{
				//I2PClient theClient;
				//theClient = I2PClientFactory.createClient();

			    net.i2p.router.Router router = new net.i2p.router.Router();
			    //net.i2p.router.Router.main(args);
			    if (router==null) System.out.println( "Null router !" );
			    router.runRouter() ;
			    System.out.println ( router );
			    //while (true);

                I2PSocketManagerFull manager = (I2PSocketManagerFull) I2PSocketManagerFactory.createManager();
                if (manager==null) System.out.println( "Null manager !" );
                System.out.println ( manager.getSession().getMyDestination().toString() );
                I2PServerSocketFull server = (I2PServerSocketFull) manager.getServerSocket();
                if (server==null) System.out.println( "Null server !" );
                System.out.println ( manager.getSession().getMyDestination().toBase64() );
                server.accept();
                
				// Initialize keys when they do not exist

				// tcpKeyFileName = "/home/antoine/.i2pmule/tcpI2PdestKey";
				// udpKeyFileName = "/home/antoine/.i2pmule/udpI2PdestKey";


			}
			catch ( java.lang.Throwable t )
			{
				System.err.println ( "Unhandled Java exception:" );
				t.printStackTrace();
			}
			return;
		}

}
 
