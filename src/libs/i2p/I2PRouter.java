/**
 *
 *                   RUN Router alone
 *
 */

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

public class I2PRouter extends Thread
{
	public static void main(String args[])
	{
		Router.main(args);
		if (args.length>10)
		{
			String addressbookpath[] = {"addressbook"};
			(new addressbook.DaemonThread ( addressbookpath ) ).start();
			TunnelControllerGroup.getInstance(); // have to call sth so that the class
					// is compiled and stays in the binary
		}
	}
}
