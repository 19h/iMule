//
// C++ Interface: CSamDefines
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef _CSAMDEFINES_
#define _CSAMDEFINES_
/*
* Lengths
*/

/* The maximum length a SAM command can be */
#define SAM_CMD_LEN 128
/* The maximum size of a single datagram packet */
//#define SAM_DGRAM_PAYLOAD_MAX (31 * 1024)
#define SAM_DGRAM_PAYLOAD_MAX (10 * 1024)
/* The maximum size of a single raw packet */
#define SAM_RAW_PAYLOAD_MAX (11 * 1024)
/* The longest log message */
#define SAM_LOGMSG_LEN 256
/* The longest `name' arg for the naming lookup callback */
#define SAM_NAME_LEN 256

/**
 * class for logging messages
 */

class CSamLogger
{
public:
        enum Level {logDEBUG, logINFO, logERROR, logCRITICAL};
        inline void log ( Level level, wxString m ) {if ( shouldLog ( level ) ) logMessage ( level, m );}
protected:
        virtual void logMessage ( Level, wxString ) {}
        virtual bool shouldLog ( Level ) {return false ;}
public:
        static CSamLogger none ;
};


class CSamLogged
{
private:
public:
        static void setLogger ( CSamLogger & logger ) ;
};


#endif
