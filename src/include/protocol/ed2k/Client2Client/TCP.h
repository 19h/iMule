//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef ED2KC2CTCP_H
#define ED2KC2CTCP_H

// Client <-> Client
enum ED2KStandardClientTCP {
        OP_HELLO					= 0x29,	// 0x10<HASH 16><ID 4><PORT 2><1 Tag_set>
        OP_SENDINGPART				= 0x2A,	// <HASH 16><von 4><bis 4><Daten len:(von-bis)>
        OP_REQUESTPARTS				= 0x2B,	// <HASH 16><von[3] 4*3><bis[3] 4*3>
        OP_FILEREQANSNOFIL			= 0x2C,	// <HASH 16>
        OP_END_OF_DOWNLOAD     		= 0x2D,	// <HASH 16> // Unused for sending
        OP_ASKSHAREDFILES			= 0x2E,	// (null)
        OP_ASKSHAREDFILESANSWER 	= 0x2F,	// <count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]
        OP_HELLOANSWER				= 0x30,	// <HASH 16><ID 4><PORT 2><1 Tag_set><SERVER_IP 4><SERVER_PORT 2>
        OP_CHANGE_CLIENT_ID 		= 0x31,	// <ID_old 4><ID_new 4> // Unused for sending
        OP_MESSAGE					= 0x32,	// <len 2><Message len>
        OP_SETREQFILEID				= 0x33,	// <HASH 16>
        OP_FILESTATUS				= 0x34,	// <HASH 16><count 2><status(bit array) len:((count+7)/8)>
        OP_HASHSETREQUEST			= 0x35,	// <HASH 16>
        OP_HASHSETANSWER			= 0x36,	// <count 2><HASH[count] 16*count>
        OP_STARTUPLOADREQ			= 0x37,	// <HASH 16>
        OP_ACCEPTUPLOADREQ			= 0x38,	// (null)
        OP_CANCELTRANSFER			= 0x39,	// (null)
        OP_OUTOFPARTREQS			= 0x3A,	// (null)
        OP_REQUESTFILENAME			= 0x3B,	// <HASH 16>	(more correctly file_name_request)
        OP_REQFILENAMEANSWER		= 0x3C,	// <HASH 16><len 4><NAME len>
        OP_CHANGE_SLOT				= 0x3D,	// <HASH 16> // Not used for sending
        OP_QUEUERANK				= 0x3E,	// <wert  4> (slot index of the request) // Not used for sending
        OP_ASKSHAREDDIRS			= 0x3F,	// (null)
        OP_ASKSHAREDFILESDIR		= 0x40,	// <len 2><Directory len>
        OP_ASKSHAREDDIRSANS			= 0x41,	// <count 4>(<len 2><Directory len>)[count]
        OP_ASKSHAREDFILESDIRANS		= 0x42,	// <len 2><Directory len><count 4>(<HASH 16><ID 4><PORT 2><1 T
        OP_ASKSHAREDDENIEDANS		= 0x43	// (null)
};

// Extended prot client <-> Extended prot client
enum ED2KExtendedClientTCP {
        OP_EMULEINFO				= 0x44,	//
        OP_EMULEINFOANSWER			= 0x45,	//
        OP_COMPRESSEDPART			= 0x46,	//
        OP_QUEUERANKING				= 0x47,	// <RANG 2>
        OP_FILEDESC					= 0x48,	// <len 2><NAME len>
        OP_VERIFYUPSREQ				= 0x49,	// (never used)
        OP_VERIFYUPSANSWER			= 0x4A,	// (never used)
        OP_UDPVERIFYUPREQ			= 0x4b,	// (never used)
        OP_UDPVERIFYUPA				= 0x4c,	// (never used)
        OP_REQUESTSOURCES			= 0x4d,	// <HASH 16>
        OP_ANSWERSOURCES			= 0x4e,	//
        OP_REQUESTSOURCES2			= 0x66,	// <HASH 16>
        OP_ANSWERSOURCES2			= 0x67,	//
        OP_PUBLICKEY				= 0x4f,	// <len 1><pubkey len>
        OP_SIGNATURE				= 0x50,	// v1: <len 1><signature len>
        // v2:<len 1><signature len><sigIPused 1>
        OP_SECIDENTSTATE			= 0x51,	// <state 1><rndchallenge 4>
        OP_REQUESTPREVIEW			= 0x52,	// <HASH 16> // Never used for sending on aMule
        OP_PREVIEWANSWER			= 0x53,	// <HASH 16><frames 1>{frames * <len 4><frame len>} // Never used for sending on aMule
        OP_MULTIPACKET				= 0x54,
        OP_MULTIPACKETANSWER		= 0x55,
//	OP_PEERCACHE_QUERY			= 0x56, // Unused on aMule - no PeerCache
//	OP_PEERCACHE_ANSWER			= 0x57, // Unused on aMule - no PeerCache
//	OP_PEERCACHE_ACK			= 0x58, // Unused on aMule - no PeerCache
        OP_PUBLICIP_REQ				= 0x59,
        OP_PUBLICIP_ANSWER			= 0x5a,
        OP_CALLBACK					= 0x5b,	// <HASH 16><HASH 16><uint 16>
//	OP_REASKCALLBACKTCP			= 0x,
        OP_AICHREQUEST				= 0x5c,	// <HASH 16><uint16><HASH aichhashlen>
        OP_AICHANSWER				= 0x5d,	// <HASH 16><uint16><HASH aichhashlen> <data>
        OP_AICHFILEHASHANS			= 0x5e,
        OP_AICHFILEHASHREQ			= 0x5f,
//	OP_BUDDYPING				= 0x,
//	OP_BUDDYPONG				= 0x,
//	OP_COMPRESSEDPART_I64		= 0xA1,	// <HASH 16><von 8><size 4><Data len:size>
//	OP_SENDINGPART_I64			= 0xA2,	// <HASH 16><start 8><end 8><Data len:(end-start)>
//	OP_REQUESTPARTS_I64			= 0xA3,	// <HASH 16><start[3] 8*3><end[3] 8*3>
        OP_MULTIPACKET_EXT			= 0x65,
	OP_CHATCAPTCHAREQ			= 0xA5,
	OP_CHATCAPTCHARES			= 0xA6,
};

#endif // ED2KC2CTCP_H
