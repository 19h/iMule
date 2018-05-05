#pragma GCC java_exceptions

#include <gcj/cni.h>
#include <java/lang/Throwable.h>
#include <java/util/Properties.h>
#include <java/io/StringWriter.h>
#include <java/util/Currency.h>
#include <java/lang/StackTraceElement.h>

#include <WxStringWriter.h>
#include <CI2PRouterThread.h>

#ifdef FALSE
#undef FALSE
#define FALSE 0
#endif
#ifdef TRUE
#undef TRUE
#define TRUE 1
#endif

#include <wx/wx.h>

#include <iostream>
#include <map>

//#include <OtherFunctions.h>	// Needed for GetConfigDir()
#include <Logger.h>
//#include <common/Format.h>
//#include <amule.h>
#include "JConversions.h"
#include "CI2PRouter.h"

#include "net/i2p/client/streaming/I2PSocketManagerFull.h"

#include "net/i2p/client/naming/MetaNamingService.h"

//#include "net/i2p/client/naming/FilesystemAddressDB.h"

#include "net/i2p/client/naming/HostsTxtNamingService.h"

#ifdef VERSION
#undef VERSION
#endif
#include "addressbook/Daemon.h"


#include "net/i2p/sam/SAMBridge.h"

#include "net/i2p/i2ptunnel/TunnelControllerGroup.h"

#include <config.h>

using java::lang::Throwable;
using java::util::Properties;


/**
 *
 *                   RUN Router THREAD CLASS
 *
 */

CI2PRouter::CI2PRouter ( std::map<wxString, wxString> propmap )
{
#ifdef INTERNAL_ROUTER

        router = NULL ;

        JvCreateJavaVM(NULL);

        try {
                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 0" ) );
                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 1" ) );

                JvAttachCurrentThread ( NULL, NULL );

                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 2" ) );

                Properties * props = new Properties();

                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 3" ) );
                for ( std::map<wxString, wxString>::iterator it = propmap.begin() ; it != propmap.end() ; it++ ) {
                        props->setProperty ( toJstring ( it->first ), toJstring ( it->second ) ) ;
                }

                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 4" ) );
                JvInitClass ( &CI2PRouterThread::class$ );

                // needed for the router
                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 5" ) );
                JvInitClass ( &net::i2p::client::streaming::I2PSocketManagerFull::class$ );

                /*
                 * The following initializations are here to force gcc keeping these classes
                 * in the binary, because they are loaded in I2P by the dynamic class loader
                 * according to their name.
                 * So, as they aren't any reference on them in the code, they would be
                 * stripped from the executable if we do nothing against that !
                 */

                // the class that launches tunnels
                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 6" ) );
                JvInitClass ( &net::i2p::i2ptunnel::TunnelControllerGroup::class$ );

                // needed for address resolution (launched by TunnelControllerGroup)
                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 7" ) );
                JvInitClass ( &addressbook::Daemon::class$ ) ;

                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 8" ) );
                JvInitClass ( &net::i2p::client::naming::MetaNamingService::class$ );

                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 9" ) );
                JvInitClass ( &net::i2p::client::naming::HostsTxtNamingService::class$ );

                // the sam bridge (launched by TunnelControllerGroup)
                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 10" ) );
                JvInitClass ( &net::i2p::sam::SAMBridge::class$ );

                // some try
                AddDebugLogLineN(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - 11" ) );
                JvInitClass ( &WxStringWriter::class$ ) ;

                // Router creation
                AddDebugLogLineN(logI2PRouter, wxT ( "new router" ) );
                router = new CI2PRouterThread( props, logI2PRouter) ;

                //router->startRouter( props, logI2PRouter );
                JvDetachCurrentThread();

        } catch ( Throwable * t ) {
                AddDebugLogLineC(logI2PRouter, wxT ( "CI2PRouter::CI2PRouter - Java Error" ) );

                while (t!=NULL) {
                        JArray<java::lang::StackTraceElement *> * stack = t->getStackTrace();
                        AddDebugLogLineC(logI2PRouter, toString( t->toString() ) );
                        t = t->getCause();

                        for (int i = 0; i < stack->length; i++) {
                                AddDebugLogLineC(logI2PRouter, toString( ((java::lang::StackTraceElement **)elements(stack))[i]->toString() ) );
                        }
                }

                JvDetachCurrentThread();
        }

#endif
}


CI2PRouter::~CI2PRouter()
{
        stop();
}

bool CI2PRouter::isRunning()
{
        bool ans = false;

#ifdef INTERNAL_ROUTER
        JvAttachCurrentThread ( NULL, NULL );

        if (router) ans = ( bool ) router->isRunning() ;
        JvDetachCurrentThread();
#endif
        return ans;
}

void CI2PRouter::stop()
{
#ifdef INTERNAL_ROUTER
        JvAttachCurrentThread ( NULL, NULL );

        if (router) router->stopRouter();
        JvDetachCurrentThread();
#endif
}

wxString CI2PRouter::getInfo()
{
        wxString ans ;
#ifdef INTERNAL_ROUTER
        JvAttachCurrentThread ( NULL, NULL );
        WxStringWriter * writer = new WxStringWriter((gnu::gcj::RawData *) &ans);

        if (router) router->getInfo(writer) ;
        JvDetachCurrentThread();
#endif
        return ans ;
}
