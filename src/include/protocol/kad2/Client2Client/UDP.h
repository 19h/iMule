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

#ifndef KAD2C2CUDP_H
#define KAD2C2CUDP_H

enum Ed2kUDPOpcodesForKademliaV2 {
        OP_DIRECTCALLBACKREQ		= 0x95	// <TCPPort 2><Userhash 16><ConnectionOptions 1>
};

enum Kademlia2Opcodes {
        KADEMLIA2_BOOTSTRAP_REQ		= 0x0D,
        KADEMLIA2_BOOTSTRAP_RES     ,
        KADEMLIA2_HELLO_REQ         ,
        KADEMLIA2_HELLO_RES         ,
        KADEMLIA2_REQ               ,
        KADEMLIA2_HELLO_RES_ACK     ,   // <NodeID><uint8 tags>
        KADEMLIA2_RES               ,
        KADEMLIA2_SEARCH_KEY_REQ    ,
        KADEMLIA2_SEARCH_SOURCE_REQ ,
        KADEMLIA2_SEARCH_NOTES_REQ  ,
        KADEMLIA2_SEARCH_RES        ,
        KADEMLIA2_PUBLISH_KEY_REQ   ,
        KADEMLIA2_PUBLISH_SOURCE_REQ,
        KADEMLIA2_PUBLISH_NOTES_REQ ,
        KADEMLIA2_PUBLISH_RES       ,
        KADEMLIA2_PUBLISH_RES_ACK   ,   // (null)
        KADEMLIA_FIREWALLED2_REQ    ,   // <TCPPORT (sender) [2]><userhash><connectoptions 1>
        KADEMLIA2_PING              ,   // (null)
        KADEMLIA2_PONG              ,   // (null)
        KADEMLIA2_FIREWALLUDP           // <errorcode [1]><UDPPort_Used [2]>
};

#endif // KAD2C2CUDP_H
