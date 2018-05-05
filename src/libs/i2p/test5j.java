import java.lang.Thread;
import net.i2p.router.Router;
import java.io.File;

public class test5j extends Thread
{
    Router routeur;
    int m_order;
    
    public void run()
    {
	
	File netDb = new File ( "netDb" );
	String names[] = ( netDb.exists() ? netDb.list() : null );

	if ( ( names == null ) || ( names.length < 15 ) )
	{
	    System.out.println( "CI2PRouterTHread: Request Reseeding, because :" );
	    System.out.println( "netDb list = " + names );


	    //I2PPeersReseeder.requestReseed();
	}


	routeur = new Router();

	System.out.println("Routeur cree\n" );

	routeur.setKillVMOnEnd ( false );

	System.out.println("router->setKillVMOnEnd ( false );" );

	System.out.println( "" + Thread.activeCount()+ " threads" );
	

	routeur.runRouter() ;

	System.out.println( "Routeur demarre" );

	try {
	    synchronized (this)
	    {
		while (m_order!=1) wait();
	    }
	} catch (java.lang.InterruptedException e) {
	    e.printStackTrace();
	}

	routeur.shutdownGracefully() ;
    }
    
    
    public test5j()
    {
	int i;
	m_order = 0;
	start();
	try {
	    for (i=0; i<600; i++)
	    {
		System.out.println( ""+i );
		sleep(1000);
	    }
	} catch (java.lang.InterruptedException e) {
	    e.printStackTrace();
	}

	synchronized (this)
	{
	    m_order = 1;
	    notifyAll();
	}
    }
}
