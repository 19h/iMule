#include <config.h>

#pragma GCC java_exceptions


#include <gcj/cni.h>
#include <java/lang/Throwable.h>

#ifdef FALSE
#undef FALSE
#define FALSE 0
#endif
#ifdef TRUE
#undef TRUE
#define TRUE 1
#endif

#include <wx/wx.h>

#include <map>

#include <Logger.h>
#include "JConversions.h"

#include "net/i2p/client/streaming/I2PSocketManagerFull.h"


#ifdef VERSION
#undef VERSION
#endif


#include "net/i2p/sam/SAMBridge.h"



using java::lang::Throwable;


#include "CSamLauncher.h"

/**
 *
 *                   RUN SAM
 *
 */

void CSamLauncher::start( std::map<wxString, wxString> propmap )
{
        JvCreateJavaVM(NULL);

        try {
                JvAttachCurrentThread ( NULL, NULL );

                // needed for the router
                JvInitClass ( &net::i2p::sam::SAMBridge::class$ );

                JvInitClass ( &net::i2p::client::streaming::I2PSocketManagerFull::class$ );

                /*
                 * The following initializations are here to force gcc keeping these classes
                 * in the binary, because they are loaded in I2P by the dynamic class loader
                 * according to their name.
                 * So, as they aren't any reference on them in the code, they would be
                 * stripped from the executable if we do nothing against that !
                 */

                using namespace java::lang;

                JArray<String *> * args
                = (JArray<String *> *) JvNewObjectArray( 5 , &String::class$, NULL);
                wxString i2phost;
                wxString i2pport;
                i2phost << wxT("i2cp.tcp.host=") << propmap[wxT("i2cp.tcp.host")];
                i2pport << wxT("i2cp.tcp.port=") << propmap[wxT("i2cp.tcp.port")];

                elements(args)[0] = JvNewStringLatin1("sam.keys") ;
                elements(args)[1] = JvNewStringLatin1("localhost") ;
                elements(args)[2] = toJstring(propmap[wxT("sam.port")]);
                elements(args)[3] = toJstring(i2phost);
                elements(args)[4] = toJstring(i2pport);


                // SAM creation

                AddDebugLogLineC(logSamLauncher, wxT ( "launch Sam bridge" ) );

                net::i2p::sam::SAMBridge::main(args) ;

                JvDetachCurrentThread();

        } catch ( Throwable * t ) {
                AddDebugLogLineC(logSamLauncher, wxT ( "CSamLauncher::CSamLauncher - Java Error" ) );
                JvDetachCurrentThread();
        }
}


