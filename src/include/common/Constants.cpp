//
// This file is part of the imule Project.
//
// Copyright (c) 2003-2006 imule Team ( mkvore@mail.i2p / http://www.imule.i2p )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include <wx/wx.h>
#include <common/Macros.h>
#include <common/Constants.h>

#include <inttypes.h>

// MOD Note: Do not change this part - Merkur
uint32_t MAX_RESULTS					= 100 ; 			// max global search results
uint32_t MAX_CLIENTCONNECTIONTRY	= 2 ;

//TODO #warning shorten for testing
uint32_t MIN_REQUESTTIME				= 0 ;        // 590000 =  MIN2MS(10) - SEC2MS(10) // clients requesting files more often will gain agressiveness, then be banned

//TODO #warning shorten for testing
uint32_t FILEREASKTIME					= MIN2MS ( 10 );	// 20mn before retrying to connect to a client for a downloading file
//, 1 minute less to contact him on his udp port if we already are on his queue but not connected.

uint32_t CONNECTION_TIMEOUT			= MIN2MS ( 10 )  ; 	/* 10mn. Sockets unused during this time are disconnected.
														(20mn if it participates in a chat session !)
														set this lower if you want less connections at once,
														set  it higher if you have enough sockets (edonkey has its own timout too,
														so a very high value won't effect this) */

//TODO #warning shorten for testing
uint32_t DOWNLOADTIMEOUT				= MIN2MS ( 11 ) ;  // (10mn) send "cancel transfer" if no download has COME for this time
// and consider myself on the other client's queue

uint32_t SERVERREASKTIME				= MIN2MS ( 15 );   // don't set this too low, it wont speed up anything, but it could kill imule or your internetconnection
uint32_t UDPSERVERREASKTIME			= MIN2MS ( 25 ) ; 	// 1300000 <- original value ***
uint32_t SOURCECLIENTREASKS			= MIN2MS ( 40 ) ; 	//40 mins
uint32_t SOURCECLIENTREASKF			= MIN2MS ( 5 ) ; 	//5 mins

uint32_t ED2KREPUBLISHTIME			= MIN2MS ( 1 ) ; 	//1 min
uint32_t MINCOMMONPENALTY			= 4 ;
uint32_t UDPSERVERSTATTIME			= SEC2MS ( 5 ) ; 	//5 secs
uint32_t UDPSERVSTATREASKTIME			= HR2MS ( 4 ) ; 	//4 hours - eMule uses HR2S, we are based on GetTickCount, hence MS
uint32_t UDPSERVSTATMINREASKTIME		= MIN2MS(20) ;
uint32_t UDPSERVERPORT				= 4665 ; 		//default udp port
uint32_t UDPMAXQUEUETIME			= SEC2MS ( 30 ) ; 	//30 Seconds
uint32_t RSAKEYSIZE				= 384 ; 		//384 bits
uint32_t MAX_SOURCES_FILE_SOFT			= 500 ;
uint32_t MAX_SOURCES_FILE_UDP			= 50 ;
double SESSIONMAXTRANS			= ( 9.3*1024*1024 ) ;   // 9.3 Mbytes. "Try to send complete chunks" always sends this amount of data
uint32_t SESSIONMAXTIME				= HR2MS ( 1 ) ; 	//1 hour
uint32_t MAXFILECOMMENTLEN			= 50 ;
uint32_t MIN_UP_CLIENTS_ALLOWED			= 2 ; 	                /* min. clients allowed to download regardless UPLOAD_CLIENT_DATARATE
                                                                   or any other factors. Don't set this too high */
// MOD Note: end

uint32_t MAXCONPER5SEC				= 20 ;
uint32_t MAXCON5WIN9X				= 10 ;
uint32_t UPLOAD_CHECK_CLIENT_DR			= 1000 ;     // not used
uint32_t UPLOAD_LOW_CLIENT_DR			= 2400 ;     /* not used - uploadspeed per client in bytes - you may want to adjust
							        this if you have a slow connection or T1-T3 ;) */
uint32_t UPLOAD_CLIENT_DATARATE			= 3072 ;     // min. client datarate. Used to compute the max. number of downloading peers
uint32_t MAX_UP_CLIENTS_ALLOWED			= 250 ;      // max. clients allowed regardless UPLOAD_CLIENT_DATARATE or any other factors. Don't set this too low, use DATARATE to adjust uploadspeed per client
uint32_t CONSERVTIMEOUT				= 120000 ;   // (2mn) agelimit for pending connection attempts towards ED2K server
uint32_t RARE_FILE				= 50 ;
uint32_t BADCLIENTBAN				= 4 ;        // not used

uint32_t MAX_PURGEQUEUETIME			= 3600000 ;
uint32_t PURGESOURCESWAPSTOP			= 900000 ;   // (15 mins), how long forbid swapping a source to a certain file (NNP,...)

//TODO #warning longed for testing
uint32_t CONNECTION_LATENCY			= MIN2MS ( 2 ); // (2mn) // 22050	// latency for responses

uint32_t SOURCESSLOTS				= 100 ;
uint32_t MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE	= 1500 ;
uint32_t MAXAVERAGETIME				= 40000 ;    // millisecs

//TODO #warning shorten for testing
uint32_t CLIENTBANTIME				= MIN2MS ( 1 ) ; // 1mn (7200000 // 2h)

uint32_t TRACKED_CLEANUP_TIME			= HR2MS ( 1 )  ; // 1 hour
uint32_t KEEPTRACK_TIME				= HR2MS ( 2 )  ; // 2h	//how long to keep track of clients which were once in the uploadqueue
uint32_t CLIENTLIST_CLEANUP_TIME		= MIN2MS ( 34 ); // 34 min

wxUint32 DISKSPACERECHECKTIME                   = 60000 ; // checkDiskspace
