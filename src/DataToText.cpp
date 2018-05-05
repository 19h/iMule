//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "DataToText.h"

#include "KnownFile.h"		// Needed by PriorityToStr
#include "Constants.h"		// Needed by DownloadStateToStr and GetSoftName
#include "PartFileConvert.h"
#include <tags/FileTags.h>
#include <tags/TagTypes.h>
#include <tags/ServerTags.h>
#include <tags/ClientTags.h>
#include <protocol/Protocols.h>
#include <protocol/ed2k/ClientSoftware.h>
#include <protocol/ed2k/Client2Server/TCP.h>
#include <protocol/ed2k/Client2Server/UDP.h>
#include <protocol/ed2k/Client2Client/TCP.h>
#include <protocol/ed2k/Client2Client/UDP.h>
#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad2/Client2Client/UDP.h>
#include <protocol/kad2/Client2Client/TCP.h>
#include <wx/string.h>
#include <wx/intl.h>


wxString PriorityToStr( int priority, bool isAuto )
{
        if ( isAuto ) {
                switch ( priority ) {
                case PR_LOW:		return _("Auto [Lo]");
                case PR_NORMAL:		return _("Auto [No]");
                case PR_HIGH:		return _("Auto [Hi]");
                }
        } else {
                switch ( priority ) {
                case PR_VERYLOW:	return _("Very low");
                case PR_LOW:		return _("Low");
                case PR_NORMAL:		return _("Normal");
                case PR_HIGH:		return _("High");
                case PR_VERYHIGH:	return _("Very High");
                case PR_POWERSHARE:	return _("Release");
                }
        }

        wxFAIL;

        return _("Unknown");
}


wxString DownloadStateToStr( int state, bool queueFull )
{
        switch ( state ) {
        case DS_CONNECTING:		return  _("Connecting");
        case DS_CONNECTED:		return _("Asking");
                //case DS_WAITCALLBACK:		return _("Connecting via server");
        case DS_ONQUEUE:		return ( queueFull ? _("Queue Full") : _("On Queue") );
        case DS_DOWNLOADING:		return _("Downloading");
        case DS_REQHASHSET:		return _("Receiving hashset");
        case DS_NONEEDEDPARTS:		return _("No needed parts");
        case DS_LOWTOLOWIP:		return _("Cannot connect LowID to LowID");
        case DS_TOOMANYCONNS:		return _("Too many connections");
        case DS_NONE:			return _("Unknown");
                //case DS_WAITCALLBACKKAD: 	return _("Connecting via Kad");
        case DS_TOOMANYCONNSKAD:	return _("Too many Kad connections");
        case DS_BANNED:			return _("Banned");
        case DS_ERROR:			return _("Connection Error");
        case DS_REMOTEQUEUEFULL:	return _("Remote Queue Full");
        }

        wxFAIL;

        return _("Unknown");
}


const wxString GetSoftName(unsigned int software_ident)
{
        switch (software_ident) {
        case SO_OLDEMULE:
        case SO_EMULE:
                return wxT("eMule");
        case SO_CDONKEY:
                return wxT("cDonkey");
        case SO_LXMULE:
                return wxT("(l/x)Mule");
        case SO_AMULE:
                return wxT("aMule");
        case SO_IMULE:
                return wxT("iMule");
        case SO_SHAREAZA:
        case SO_NEW_SHAREAZA:
        case SO_NEW2_SHAREAZA:
                return wxT("Shareaza");
        case SO_EMULEPLUS:
                return wxT("eMule+");
        case SO_HYDRANODE:
                return wxT("HydraNode");
        case SO_MLDONKEY:
                return wxTRANSLATE("Old MLDonkey");
        case SO_NEW_MLDONKEY:
        case SO_NEW2_MLDONKEY:
                return wxTRANSLATE("New MLDonkey");
        case SO_LPHANT:
                return wxT("lphant");
        case SO_EDONKEYHYBRID:
                return wxT("eDonkeyHybrid");
        case SO_EDONKEY:
                return wxT("eDonkey");
        case SO_UNKNOWN:
                return wxTRANSLATE("Unknown");
        case SO_COMPAT_UNK:
                return wxTRANSLATE("eMule Compatible");
        default:
                return wxEmptyString;
        }
}


wxString OriginToText(unsigned int source_from)
{
        switch ((ESourceFrom)source_from) {
        case SF_LOCAL_SERVER:		return wxTRANSLATE("Local Server");
        case SF_REMOTE_SERVER:		return wxTRANSLATE("Remote Server");
        case SF_KADEMLIA:		return wxTRANSLATE("Kad");
        case SF_SOURCE_EXCHANGE: 	return wxTRANSLATE("Source Exchange");
        case SF_PASSIVE:		return wxTRANSLATE("Passive");
        case SF_LINK:			return wxTRANSLATE("Link");
        case SF_SOURCE_SEEDS:		return wxTRANSLATE("Source Seeds");
        case SF_SEARCH_RESULT:		return wxTRANSLATE("Search Result");
        case SF_NONE:
        default:		return wxTRANSLATE("Unknown");
        }
}


wxString GetConversionState(unsigned int state)
{
        switch (state) {
        case CONV_OK			: return _("Completed");
        case CONV_INPROGRESS		: return _("In progress");
        case CONV_OUTOFDISKSPACE	: return _("ERROR: Out of diskspace");
        case CONV_PARTMETNOTFOUND	: return _("ERROR: Partmet not found");
        case CONV_IOERROR		: return _("ERROR: IO error!");
        case CONV_FAILED		: return _("ERROR: Failed!");
        case CONV_QUEUE			: return _("Queued");
        case CONV_ALREADYEXISTS		: return _("Already downloading");
        case CONV_BADFORMAT		: return _("Unknown or bad tempfile format.");
        default: return wxT("?");
        }
}

wxString opstr ( int prot, int op )
{
        switch ( prot ) {
        case OP_EDONKEYHEADER :
        case OP_EDONKEYPROT :
        case OP_PACKEDPROT :
        case OP_EMULEPROT :
        case OP_MLDONKEYPROT :
                return opstr ( op );

        case OP_KADEMLIAHEADER :
        case OP_KADEMLIAPACKEDPROT :
                return kadopstr ( op );

        default:
                return wxT ( "unknown" );
        }
}

wxString kadopstr ( int op )
{
        if (op == KADEMLIA_BOOTSTRAP_REQ_DEPRECATED) return wxT ( "KADEMLIA_BOOTSTRAP_REQ_DEPRECATED" );
        if (op == KADEMLIA_BOOTSTRAP_RES_DEPRECATED) return wxT ( "KADEMLIA_BOOTSTRAP_RES_DEPRECATED" );

        if (op == KADEMLIA_HELLO_REQ_DEPRECATED) return wxT ( "KADEMLIA_HELLO_REQ_DEPRECATED" );
        if (op == KADEMLIA_HELLO_RES_DEPRECATED) return wxT ( "KADEMLIA_HELLO_RES_DEPRECATED" );

        if (op == KADEMLIA_REQ_DEPRECATED) return wxT ( "KADEMLIA_REQ_DEPRECATED" );
        if (op == KADEMLIA_RES_DEPRECATED) return wxT ( "KADEMLIA_RES_DEPRECATED" );

        if (op == KADEMLIA_SEARCH_REQ) return wxT ( "KADEMLIA_SEARCH_REQ" );
        if (op == KADEMLIA_SEARCH_RES) return wxT ( "KADEMLIA_SEARCH_RES" );

        if (op == KADEMLIA_SEARCH_NOTES_REQ) return wxT ( "KADEMLIA_SEARCH_NOTES_REQ" );

        if (op == KADEMLIA_PUBLISH_REQ) return wxT ( "KADEMLIA_PUBLISH_REQ" );
        if (op == KADEMLIA_PUBLISH_RES) return wxT ( "KADEMLIA_PUBLISH_RES" );

        if (op == KADEMLIA_PUBLISH_NOTES_REQ_DEPRECATED) return wxT ( "KADEMLIA_PUBLISH_NOTES_REQ_DEPRECATED" );


        if (op == KADEMLIA_FIND_NODE) return wxT ( "KADEMLIA_FIND_NODE" );
        if (op == KADEMLIA_FIND_VALUE) return wxT ( "KADEMLIA_FIND_VALUE" );
        if (op == KADEMLIA_STORE) return wxT ( "KADEMLIA_STORE" );

        if (op == KADEMLIA2_BOOTSTRAP_REQ) return wxT ( "KADEMLIA2_BOOTSTRAP_REQ" )     ;
        if (op == KADEMLIA2_BOOTSTRAP_RES) return wxT ( "KADEMLIA2_BOOTSTRAP_RES" )     ;
        if (op == KADEMLIA2_HELLO_REQ) return wxT ( "KADEMLIA2_HELLO_REQ" )         ;
        if (op == KADEMLIA2_HELLO_RES) return wxT ( "KADEMLIA2_HELLO_RES" )         ;
        if (op == KADEMLIA2_REQ) return wxT ( "KADEMLIA2_REQ" )               ;
        if (op == KADEMLIA2_HELLO_RES_ACK) return wxT ( "KADEMLIA2_HELLO_RES_ACK" )     ;   // <NodeID><uint8 tags>
        if (op == KADEMLIA2_RES) return wxT ( "KADEMLIA2_RES" )               ;
        if (op == KADEMLIA2_SEARCH_KEY_REQ) return wxT ( "KADEMLIA2_SEARCH_KEY_REQ" )    ;
        if (op == KADEMLIA2_SEARCH_SOURCE_REQ) return wxT ( "KADEMLIA2_SEARCH_SOURCE_REQ" ) ;
        if (op == KADEMLIA2_SEARCH_NOTES_REQ) return wxT ( "KADEMLIA2_SEARCH_NOTES_REQ" )  ;
        if (op == KADEMLIA2_SEARCH_RES) return wxT ( "KADEMLIA2_SEARCH_RES" )        ;
        if (op == KADEMLIA2_PUBLISH_KEY_REQ) return wxT ( "KADEMLIA2_PUBLISH_KEY_REQ" )   ;
        if (op == KADEMLIA2_PUBLISH_SOURCE_REQ) return wxT ( "KADEMLIA2_PUBLISH_SOURCE_REQ" );
        if (op == KADEMLIA2_PUBLISH_NOTES_REQ) return wxT ( "KADEMLIA2_PUBLISH_NOTES_REQ" ) ;
        if (op == KADEMLIA2_PUBLISH_RES) return wxT ( "KADEMLIA2_PUBLISH_RES" )       ;
        if (op == KADEMLIA2_PUBLISH_RES_ACK) return wxT ( "KADEMLIA2_PUBLISH_RES_ACK" )   ;   // (null)
        if (op == KADEMLIA_FIREWALLED2_REQ) return wxT ( "KADEMLIA_FIREWALLED2_REQ" )    ;   // <TCPPORT (sender) [2]><userhash><connectoptions 1>
        if (op == KADEMLIA2_PING) return wxT ( "KADEMLIA2_PING" )              ;   // (null)
        if (op == KADEMLIA2_PONG) return wxT ( "KADEMLIA2_PONG" )              ;   // (null)
        if (op == KADEMLIA2_FIREWALLUDP) return wxT ( "KADEMLIA2_FIREWALLUDP" )       ;    // <errorcode [1]><UDPPort_Used [2]>

        return (CFormat (wxT("unknown (%x)")) % op ).GetString();
}

wxString opstr ( int op )
{
        switch ( op ) {
        case OP_EDONKEYHEADER : return wxT ( "OP_EDONKEYHEADER" );

        case OP_EDONKEYPROT : return wxT ( "OP_EDONKEYPROT" );

        case OP_PACKEDPROT : return wxT ( "OP_PACKEDPROT" );

        case OP_EMULEPROT : return wxT ( "OP_EMULEPROT" );

        case OP_KADEMLIAHEADER : return wxT ( "OP_KADEMLIAHEADER" );

        case OP_KADEMLIAPACKEDPROT : return wxT ( "OP_KADEMLIAPACKEDPROT" );

        case OP_MLDONKEYPROT : return wxT ( "OP_MLDONKEYPROT" );

        case OP_LOGINREQUEST : return wxT ( "OP_LOGINREQUEST" );

        case OP_REJECT : return wxT ( "OP_REJECT" );

        case OP_GETSERVERLIST : return wxT ( "OP_GETSERVERLIST" );

        case OP_OFFERFILES : return wxT ( "OP_OFFERFILES" );

        case OP_SEARCHREQUEST : return wxT ( "OP_SEARCHREQUEST" );

        case OP_DISCONNECT : return wxT ( "OP_DISCONNECT" );

        case OP_GETSOURCES : return wxT ( "OP_GETSOURCES" );

        case OP_SEARCH_USER : return wxT ( "OP_SEARCH_USER" );

        case OP_CALLBACKREQUEST : return wxT ( "OP_CALLBACKREQUEST" );

        case OP_QUERY_MORE_RESULT : return wxT ( "OP_QUERY_MORE_RESULT" );

        case OP_SERVERLIST : return wxT ( "OP_SERVERLIST" );

        case OP_SEARCHRESULT : return wxT ( "OP_SEARCHRESULT" );

        case OP_SERVERSTATUS : return wxT ( "OP_SERVERSTATUS" );

        case OP_CALLBACKREQUESTED : return wxT ( "OP_CALLBACKREQUESTED" );

        case OP_CALLBACK_FAIL : return wxT ( "OP_CALLBACK_FAIL" );

        case OP_SERVERMESSAGE : return wxT ( "OP_SERVERMESSAGE" );

        case OP_IDCHANGE : return wxT ( "OP_IDCHANGE" );

        case OP_SERVERIDENT : return wxT ( "OP_SERVERIDENT" );

        case OP_FOUNDSOURCES : return wxT ( "OP_FOUNDSOURCES" );

        case OP_USERS_LIST : return wxT ( "OP_USERS_LIST" );

        case OP_GLOBGETSOURCES2 : return wxT ( "OP_GLOBGETSOURCES2" );

        case OP_GLOBSERVSTATREQ : return wxT ( "OP_GLOBSERVSTATREQ" );

        case OP_GLOBSERVSTATRES : return wxT ( "OP_GLOBSERVSTATRES" );

        case OP_GLOBSEARCHREQ : return wxT ( "OP_GLOBSEARCHREQ" );

        case OP_GLOBSEARCHRES : return wxT ( "OP_GLOBSEARCHRES" );

        case OP_GLOBGETSOURCES : return wxT ( "OP_GLOBGETSOURCES" );

        case OP_GLOBFOUNDSOURCES : return wxT ( "OP_GLOBFOUNDSOURCES" );

        case OP_GLOBCALLBACKREQ : return wxT ( "OP_GLOBCALLBACKREQ" );

        case OP_SERVER_LIST_REQ : return wxT ( "OP_SERVER_LIST_REQ" );

        case OP_SERVER_LIST_RES : return wxT ( "OP_SERVER_LIST_RES" );

        case OP_SERVER_DESC_REQ : return wxT ( "OP_SERVER_DESC_REQ" );

        case OP_SERVER_DESC_RES : return wxT ( "OP_SERVER_DESC_RES" );

        case OP_SERVER_LIST_REQ2 : return wxT ( "OP_SERVER_LIST_REQ2" );

        case INV_SERV_DESC_LEN : return wxT ( "INV_SERV_DESC_LEN" );

        case OP_HELLO : return wxT ( "OP_HELLO" );

        case OP_SENDINGPART : return wxT ( "OP_SENDINGPART" );

        case OP_REQUESTPARTS : return wxT ( "OP_REQUESTPARTS" );

        case OP_FILEREQANSNOFIL : return wxT ( "OP_FILEREQANSNOFIL" );

        case OP_END_OF_DOWNLOAD : return wxT ( "OP_END_OF_DOWNLOAD" );

        case OP_ASKSHAREDFILES : return wxT ( "OP_ASKSHAREDFILES" );

        case OP_ASKSHAREDFILESANSWER : return wxT ( "OP_ASKSHAREDFILESANSWER" );

        case OP_HELLOANSWER : return wxT ( "OP_HELLOANSWER" );

        case OP_CHANGE_CLIENT_ID : return wxT ( "OP_CHANGE_CLIENT_ID" );

        case OP_MESSAGE : return wxT ( "OP_MESSAGE" );

        case OP_SETREQFILEID : return wxT ( "OP_SETREQFILEID" );

        case OP_FILESTATUS : return wxT ( "OP_FILESTATUS" );

        case OP_HASHSETREQUEST : return wxT ( "OP_HASHSETREQUEST" );

        case OP_HASHSETANSWER : return wxT ( "OP_HASHSETANSWER" );

        case OP_STARTUPLOADREQ : return wxT ( "OP_STARTUPLOADREQ" );

        case OP_ACCEPTUPLOADREQ : return wxT ( "OP_ACCEPTUPLOADREQ" );

        case OP_CANCELTRANSFER : return wxT ( "OP_CANCELTRANSFER" );

        case OP_OUTOFPARTREQS : return wxT ( "OP_OUTOFPARTREQS" );

        case OP_REQUESTFILENAME : return wxT ( "OP_REQUESTFILENAME" );

        case OP_REQFILENAMEANSWER : return wxT ( "OP_REQFILENAMEANSWER" );

        case OP_CHANGE_SLOT : return wxT ( "OP_CHANGE_SLOT" );

        case OP_QUEUERANK : return wxT ( "OP_QUEUERANK" );

        case OP_ASKSHAREDDIRS : return wxT ( "OP_ASKSHAREDDIRS" );

        case OP_ASKSHAREDFILESDIR : return wxT ( "OP_ASKSHAREDFILESDIR" );

        case OP_ASKSHAREDDIRSANS : return wxT ( "OP_ASKSHAREDDIRSANS" );

        case OP_ASKSHAREDFILESDIRANS : return wxT ( "OP_ASKSHAREDFILESDIRANS" );

        case OP_ASKSHAREDDENIEDANS : return wxT ( "OP_ASKSHAREDDENIEDANS" );

        case OP_EMULEINFO : return wxT ( "OP_EMULEINFO" );

        case OP_EMULEINFOANSWER : return wxT ( "OP_EMULEINFOANSWER" );

        case OP_COMPRESSEDPART : return wxT ( "OP_COMPRESSEDPART" );

        case OP_QUEUERANKING : return wxT ( "OP_QUEUERANKING" );

        case OP_FILEDESC : return wxT ( "OP_FILEDESC" );

        case OP_VERIFYUPSREQ : return wxT ( "OP_VERIFYUPSREQ" );

        case OP_VERIFYUPSANSWER : return wxT ( "OP_VERIFYUPSANSWER" );

        case OP_UDPVERIFYUPREQ : return wxT ( "OP_UDPVERIFYUPREQ" );

        case OP_UDPVERIFYUPA : return wxT ( "OP_UDPVERIFYUPA" );

        case OP_REQUESTSOURCES : return wxT ( "OP_REQUESTSOURCES" );

        case OP_ANSWERSOURCES : return wxT ( "OP_ANSWERSOURCES" );

        case OP_PUBLICKEY : return wxT ( "OP_PUBLICKEY" );

        case OP_SIGNATURE : return wxT ( "OP_SIGNATURE" );

        case OP_SECIDENTSTATE : return wxT ( "OP_SECIDENTSTATE" );

        case OP_REQUESTPREVIEW : return wxT ( "OP_REQUESTPREVIEW" );

        case OP_PREVIEWANSWER : return wxT ( "OP_PREVIEWANSWER" );

        case OP_MULTIPACKET : return wxT ( "OP_MULTIPACKET" );

        case OP_MULTIPACKETANSWER : return wxT ( "OP_MULTIPACKETANSWER" );

//      case OP_PEERCACHE_QUERY : return wxT ( "OP_PEERCACHE_QUERY" );

//      case OP_PEERCACHE_ANSWER : return wxT ( "OP_PEERCACHE_ANSWER" );

//      case OP_PEERCACHE_ACK : return wxT ( "OP_PEERCACHE_ACK" );

        case OP_PUBLICIP_REQ : return wxT ( "OP_PUBLICIP_REQ" );

        case OP_PUBLICIP_ANSWER : return wxT ( "OP_PUBLICIP_ANSWER" );

        case OP_CALLBACK : return wxT ( "OP_CALLBACK" );

        case OP_AICHREQUEST : return wxT ( "OP_AICHREQUEST" );

        case OP_AICHANSWER : return wxT ( "OP_AICHANSWER" );

        case OP_AICHFILEHASHANS : return wxT ( "OP_AICHFILEHASHANS" );

        case OP_AICHFILEHASHREQ : return wxT ( "OP_AICHFILEHASHREQ" );

        case OP_REASKFILEPING : return wxT ( "OP_REASKFILEPING" );

        case OP_REASKACK : return wxT ( "OP_REASKACK" );

        case OP_FILENOTFOUND : return wxT ( "OP_FILENOTFOUND" );

        case OP_QUEUEFULL : return wxT ( "OP_QUEUEFULL" );

        case OP_PORTTEST : return wxT ( "OP_PORTTEST" );

        default:
                return wxString ( wxT ( "unknown(" ) ) << op << wxT ( ")" ) ;
        }
}



wxString tagnameStr ( int tag )
{
        switch ( tag ) {
        case ST_SERVERNAME : return wxT ( "ST_SERVERNAME" );

        case ST_DESCRIPTION : return wxT ( "ST_DESCRIPTION" );

        case ST_PING : return wxT ( "ST_PING" );

        case ST_PREFERENCE : return wxT ( "ST_PREFERENCE" );

        case ST_FAIL : return wxT ( "ST_FAIL" );

        case ST_USERS : return wxT ( "ST_USERS" );

        case ST_FILES : return wxT ( "ST_FILES" );

        case ST_DYNIP : return wxT ( "ST_DYNIP" );

        case ST_LASTPING : return wxT ( "ST_LASTPING" );

        case ST_MAXUSERS : return wxT ( "ST_MAXUSERS" );

        case ST_SOFTFILES : return wxT ( "ST_SOFTFILES" );

        case ST_HARDFILES : return wxT ( "ST_HARDFILES" );

        case ST_VERSION : return wxT ( "ST_VERSION" );

        case ST_UDPFLAGS : return wxT ( "ST_UDPFLAGS" );

        case ST_AUXPORTSLIST : return wxT ( "ST_AUXPORTSLIST" );



        case CT_NAME : return wxT ( "CT_NAME" );

        case CT_VERSION : return wxT ( "CT_VERSION" );

        case CT_SERVER_FLAGS : return wxT ( "CT_SERVER_FLAGS" );

        case CT_EMULECOMPAT_OPTIONS : return wxT ( "CT_EMULECOMPAT_OPTIONS" );

        case CT_EMULE_MISCOPTIONS1 : return wxT ( "CT_EMULE_MISCOPTIONS1" );

        case CT_EMULE_VERSION : return wxT ( "CT_EMULE_VERSION" );

        case CT_DEST : return wxT ( "CT_DEST" );

        case CT_EMULE_MISCOPTIONS2 : return wxT ( "CT_EMULE_MISCOPTIONS2" );



        case ET_COMPRESSION : return wxT ( "ET_COMPRESSION" );

        case ET_UDPDEST : return wxT ( "ET_UDPDEST" );

        case ET_UDPVER : return wxT ( "ET_UDPVER" );

        case ET_SOURCEEXCHANGE : return wxT ( "ET_SOURCEEXCHANGE" );

        case ET_COMMENTS : return wxT ( "ET_COMMENTS" );

        case ET_EXTENDEDREQUEST : return wxT ( "ET_EXTENDEDREQUEST" );

        case ET_COMPATIBLECLIENT : return wxT ( "ET_COMPATIBLECLIENT" );

        case ET_FEATURES : return wxT ( "ET_FEATURES" );

        case ET_MOD_VERSION : return wxT ( "ET_MOD_VERSION" );

        case ET_FEATURESET : return wxT ( "ET_FEATURESET" );

        case ET_OS_INFO : return wxT ( "ET_OS_INFO" );



        case TAG_FILENAME : return wxT ( "TAG_FILENAME" );

        case TAG_FILESIZE : return wxT ( "TAG_FILESIZE" );

        case TAG_FILETYPE : return wxT ( "TAG_FILETYPE" );

        case TAG_PUBLISHINFO : return wxT( "TAG_PUBLISHINFO" );

        case TAG_FILEFORMAT : return wxT ( "TAG_FILEFORMAT" );

        case TAG_LASTSEENCOMPLETE : return wxT ( "TAG_LASTSEENCOMPLETE" );

        case TAG_PART_PATH : return wxT ( "TAG_PART_PATH" );

        case TAG_PART_HASH : return wxT ( "TAG_PART_HASH" );

        case TAG_TRANSFERRED : return wxT ( "TAG_TRANSFERRED" );

        case TAG_GAP_START : return wxT ( "TAG_GAP_START" );

        case TAG_GAP_END : return wxT ( "TAG_GAP_END" );

        case TAG_DESCRIPTION : return wxT ( "TAG_DESCRIPTION" );

        case TAG_PING : return wxT ( "TAG_PING" );

        case TAG_FAIL : return wxT ( "TAG_FAIL" );

        case TAG_PREFERENCE : return wxT ( "TAG_PREFERENCE" );

        case TAG_VERSION : return wxT ( "TAG_VERSION" );

        case TAG_PARTFILENAME : return wxT ( "TAG_PARTFILENAME" );

        case TAG_PRIORITY : return wxT ( "TAG_PRIORITY" );

        case TAG_STATUS : return wxT ( "TAG_STATUS" );

        case TAG_SOURCES : return wxT ( "TAG_SOURCES" );

        case TAG_QTIME : return wxT ( "TAG_QTIME" );

        case TAG_PARTS : return wxT ( "TAG_PARTS" );

        case TAG_DLPRIORITY : return wxT ( "TAG_DLPRIORITY" );

        case TAG_ULPRIORITY : return wxT ( "TAG_ULPRIORITY" );

        case TAG_KADLASTPUBLISHKEY : return wxT ( "TAG_KADLASTPUBLISHKEY" );

        case TAG_KADLASTPUBLISHSRC : return wxT ( "TAG_KADLASTPUBLISHSRC" );

        case TAG_FLAGS : return wxT ( "TAG_FLAGS" );

        case TAG_DL_ACTIVE_TIME : return wxT ( "TAG_DL_ACTIVE_TIME" );

        case TAG_CORRUPTEDPARTS : return wxT ( "TAG_CORRUPTEDPARTS" );

        case TAG_DL_PREVIEW : return wxT ( "TAG_DL_PREVIEW" );

        case TAG_KADLASTPUBLISHNOTES : return wxT ( "TAG_KADLASTPUBLISHNOTES" );

        case TAG_AICH_HASH : return wxT ( "TAG_AICH_HASH" );

        case TAG_COMPLETE_SOURCES : return wxT ( "TAG_COMPLETE_SOURCES" );

        case TAG_COLLECTION : return wxT ( "TAG_COLLECTION" );

        case TAG_PERMISSIONS : return wxT ( "TAG_PERMISSIONS" );

        case TAG_ATTRANSFERRED : return wxT ( "TAG_ATTRANSFERRED" );

        case TAG_ATREQUESTED : return wxT ( "TAG_ATREQUESTED" );

        case TAG_ATACCEPTED : return wxT ( "TAG_ATACCEPTED" );

        case TAG_CATEGORY : return wxT ( "TAG_CATEGORY" );

        case TAG_MEDIA_ARTIST : return wxT ( "TAG_MEDIA_ARTIST" );

        case TAG_MEDIA_ALBUM : return wxT ( "TAG_MEDIA_ALBUM" );

        case TAG_MEDIA_TITLE : return wxT ( "TAG_MEDIA_TITLE" );

        case TAG_MEDIA_LENGTH : return wxT ( "TAG_MEDIA_LENGTH" );

        case TAG_MEDIA_BITRATE : return wxT ( "TAG_MEDIA_BITRATE" );

        case TAG_MEDIA_CODEC : return wxT ( "TAG_MEDIA_CODEC" );

        case TAG_FILERATING : return wxT ( "TAG_FILERATING" );

        case TAG_SERVERDEST : return wxT ( "TAG_SERVERDEST" );

        case TAG_SOURCEUDEST : return wxT ( "TAG_SOURCEUDEST" );

        case TAG_SOURCEDEST : return wxT ( "TAG_SOURCEDEST" );

        case TAG_SOURCETYPE : return wxT ( "TAG_SOURCETYPE" );

        case TAG_COPIED : return wxT( "TAG_COPIED" );
        case TAG_AVAILABILITY : return wxT( "TAG_AVAILABILITY" );
        case TAG_KADMISCOPTIONS : return wxT( "TAG_KADMISCOPTIONS" );
        case TAG_ENCRYPTION : return wxT( "TAG_ENCRYPTION" );


        default : return wxT ( "Unknown" );
        }
}


wxString tagtypeStr ( Tag_Types tag )
{
        switch ( tag ) {
        case TAGTYPE_INVALID : return wxT ( "TAGTYPE_INVALID" );

        case TAGTYPE_HASH16 : return wxT ( "TAGTYPE_HASH" );

        case TAGTYPE_STRING : return wxT ( "TAGTYPE_STRING" );

        case TAGTYPE_FLOAT32 : return wxT ( "TAGTYPE_FLOAT32" );

        case TAGTYPE_BOOL : return wxT ( "TAGTYPE_BOOL" );

        case TAGTYPE_BOOLARRAY : return wxT ( "TAGTYPE_BOOLARRAY" );

        case TAGTYPE_BLOB : return wxT ( "TAGTYPE_BLOB" );

        case TAGTYPE_UINT64 : return wxT ( "TAGTYPE_UINT64" );

        case TAGTYPE_UINT32 : return wxT ( "TAGTYPE_UINT32" );

        case TAGTYPE_UINT16 : return wxT ( "TAGTYPE_UINT16" );

        case TAGTYPE_UINT8 : return wxT ( "TAGTYPE_UINT8" );

        case TAGTYPE_BSOB : return wxT ( "TAGTYPE_BSOB" );

        case TAGTYPE_STR1 : return wxT ( "TAGTYPE_STR1" );

        case TAGTYPE_STR2 : return wxT ( "TAGTYPE_STR2" );

        case TAGTYPE_STR3 : return wxT ( "TAGTYPE_STR3" );

        case TAGTYPE_STR4 : return wxT ( "TAGTYPE_STR4" );

        case TAGTYPE_STR5 : return wxT ( "TAGTYPE_STR5" );

        case TAGTYPE_STR6 : return wxT ( "TAGTYPE_STR6" );

        case TAGTYPE_STR7 : return wxT ( "TAGTYPE_STR7" );

        case TAGTYPE_STR8 : return wxT ( "TAGTYPE_STR8" );

        case TAGTYPE_STR9 : return wxT ( "TAGTYPE_STR9" );

        case TAGTYPE_STR10 : return wxT ( "TAGTYPE_STR10" );

        case TAGTYPE_STR11 : return wxT ( "TAGTYPE_STR11" );

        case TAGTYPE_STR12 : return wxT ( "TAGTYPE_STR12" );

        case TAGTYPE_STR13 : return wxT ( "TAGTYPE_STR13" );

        case TAGTYPE_STR14 : return wxT ( "TAGTYPE_STR14" );

        case TAGTYPE_STR15 : return wxT ( "TAGTYPE_STR15" );

        case TAGTYPE_STR16 : return wxT ( "TAGTYPE_STR16" );

        case TAGTYPE_STR17 : return wxT ( "TAGTYPE_STR17" );

        case TAGTYPE_STR18 : return wxT ( "TAGTYPE_STR18" );

        case TAGTYPE_STR19 : return wxT ( "TAGTYPE_STR19" );

        case TAGTYPE_STR20 : return wxT ( "TAGTYPE_STR20" );

        case TAGTYPE_STR21 : return wxT ( "TAGTYPE_STR21" );

        case TAGTYPE_STR22 : return wxT ( "TAGTYPE_STR22" );

        case TAGTYPE_ADDRESS : return wxT ( "TAGTYPE_ADDRESS" );

        case TAGTYPE_UINT64UINT64 : return wxT ( "TAGTYPE_UINT64UINT64" );

        default : return wxT ( "TAGTYPE_Unknown" );
        }
}
// File checked for headers