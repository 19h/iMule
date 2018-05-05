//
// C++ Implementation: wxI2PEvents
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "wxI2PEvents.h"
#include "SamEvents.h"
#include "wxI2PSocketBase.h"

wxDEFINE_EVENT(wxSAM_EVENT, wxSamEvent);

wxI2PSocketBase * CI2PSocketEvent::GetSocket() {return dynamic_cast<wxI2PSocketBase *>(GetEventObject());}


/**
 * Converts a SAM error code into a short error message
 *
 * code - the error code
 *
 * Returns: error string
 */
wxString i2pstrerror ( int code )
{
        switch ( code ) {
        case SAM_OK:                /* Operation completed succesfully */
                return wxT ( "Ok" );

        case SAM_CANT_REACH_PEER:   /* The peer exists, but cannot be reached */
                return wxT ( "Can't reach peer" );

        case SAM_DUPLICATED_DEST:   /* The specified Destination is already in use by another SAM instance */
                return wxT ( "Duplicated destination" );

        case SAM_DUPLICATED_ID:     /* The specified nickname is already in use by another SAM instance */
                return wxT ( "Duplicated ID" );

        case SAM_I2P_ERROR:         /* A generic I2P error (e.g. I2CP disconnection, etc.) */
                return wxT ( "I2P error" );

        case SAM_INVALID_KEY:       /* The specified key is not valid (bad
                            format, etc.) */
                return wxT ( "Invalid key" );

        case SAM_KEY_NOT_FOUND:     /* The naming system can't resolve the given name */
                return wxT ( "Key not found" );

        case SAM_PEER_NOT_FOUND:    /* The peer cannot be found on the network*/
                return wxT ( "Peer not found" );

        case SAM_TIMEOUT:           /* Timeout while waiting for an event (e.g. peer answer) */
                return wxT ( "Timeout" );

                /*
                                            * SAM_UNKNOWN deliberately left out (goes to default)
                */
        case SAM_BAD_STYLE:         /* Style must be stream, datagram, or raw */
                return wxT ( "Bad connection style" );

        case SAM_BAD_VERSION:       /* sam_hello() had an unexpected reply */
                return wxT ( "Not a SAM port, or bad SAM version" );

        case SAM_CALLBACKS_UNSET:   /* Some callbacks are still set to NULL */
                return wxT ( "Callbacks unset" );

        case SAM_SOCKET_ERROR:      /* TCP/IP connection to the SAM host:port failed */
                return wxT ( "Socket error" );

        case SAM_TOO_BIG:           /* A function was passed too much data */
                return wxT ( "Data size too big" );

        case SAM_DISCONNECTED:
                return wxT ( "SAM socket has been disconnected" );
        case SAM_NONE:
                return wxT( "No SAM error" );
        default:                    /* An unexpected error happened */
                return wxT ( "Unknown error" );
        }
}

/**
 * Apparently Winsock does not have a strerror() equivalent for its functions
 *
 * code - code from WSAGetLastError()
 *
 * Returns: error string (from http://msdn.microsoft.com/library/default.asp?
 *      url=/library/en-us/winsock/winsock/windows_sockets_error_codes_2.asp)
 */
wxString wxstrerror ( int code )
{
        switch ( code ) {
        case wxSOCKET_NOERROR: return wxT ( "No error happened." );

        case wxSOCKET_INVOP: return wxT ( "Invalid operation." );

        case wxSOCKET_IOERR: return wxT ( "Input/Output error." );

        case wxSOCKET_INVADDR: return wxT ( "Invalid address passed to wxSocket." );

        case wxSOCKET_INVSOCK: return wxT ( "Invalid socket (uninitialized)." );

        case wxSOCKET_NOHOST: return wxT ( "No corresponding host." );

        case wxSOCKET_INVPORT: return wxT ( "Invalid port." );

        case wxSOCKET_WOULDBLOCK: return wxT ( "The socket is non-blocking and the operation would block." );

        case wxSOCKET_TIMEDOUT: return wxT ( "The timeout for this operation expired." );

        case wxSOCKET_MEMERR: return wxT ( "Memory exhausted." );

        default: return wxT ( "Unknown socket error." );
        }
}

