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

#ifndef KADC2CUDP_H
#define KADC2CUDP_H

enum KademliaV1OPcodes {
        KADEMLIA_BOOTSTRAP_REQ_DEPRECATED = 1,     // <PEER (sender) [25]>
        KADEMLIA_BOOTSTRAP_RES_DEPRECATED,     // <CNT [2]> <PEER [25]>*(CNT)

        KADEMLIA_HELLO_REQ_DEPRECATED,         // <PEER (sender) [25]>
        KADEMLIA_HELLO_RES_DEPRECATED,             // <PEER (receiver) [25]>

        KADEMLIA_REQ_DEPRECATED,               // <TYPE [1]> <HASH (target) [16]> <HASH (receiver) 16>
        KADEMLIA_RES_DEPRECATED,               // <HASH (target) [16]> <CNT> <PEER [25]>*(CNT)

        KADEMLIA_SEARCH_REQ,            // <HASH (key) [16]> <ext 0/1 [1]> <SEARCH_TREE>[ext]
        KADEMLIA_SEARCH_RES,            // <HASH (key) [16]> <CNT1 [2]> (<HASH (answer) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
	// UNUSED
        KADEMLIA_SEARCH_NOTES_REQ,     // <HASH (key) [16]>

        KADEMLIA_PUBLISH_REQ,       // <HASH (key) [16]> <CNT1 [2]> (<HASH (target) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
        KADEMLIA_PUBLISH_RES,       // <HASH (key) [16]>
	// UNUSED
        KADEMLIA_PUBLISH_NOTES_REQ_DEPRECATED,     // <HASH (key) [16]> <HASH (target) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
};

#endif // KADC2CUDP_H