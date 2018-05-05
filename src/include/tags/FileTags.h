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

#ifndef FILETAGS_H
#define FILETAGS_H

// ED2K search + known.met + .part.met
// #define	FT_FILENAME			0x01	// <string>
// #define	FT_FILESIZE			0x02	// <uint32>
// #define	FT_FILESIZE_HI			0x3A	// <uint32>
// #define	FT_FILETYPE			0x03	// <string> or <uint32>
// #define	FT_FILEFORMAT			0x04	// <string>
// #define	FT_LASTSEENCOMPLETE		0x05	// <uint32>
// #define	FT_TRANSFERRED			0x08	// <uint32>
// #define	FT_GAPSTART			0x09	// <uint32>
// #define	FT_GAPEND			0x0A	// <uint32>
// #define	FT_PARTFILENAME			0x12	// <string>
// #define	FT_OLDDLPRIORITY		0x13	// Not used anymore
// #define	FT_STATUS			0x14	// <uint32>
// #define	FT_SOURCES			0x15	// <uint32>
// #define	FT_PERMISSIONS			0x16	// <uint32>
// #define	FT_OLDULPRIORITY		0x17	// Not used anymore
// #define	FT_DLPRIORITY			0x18	// Was 13
// #define	FT_ULPRIORITY			0x19	// Was 17
// #define	FT_KADLASTPUBLISHKEY		0x20	// <uint32>
// #define	FT_KADLASTPUBLISHSRC		0x21	// <uint32>
// #define	FT_FLAGS			0x22	// <uint32>
// #define	FT_DL_ACTIVE_TIME		0x23	// <uint32>
// #define	FT_CORRUPTEDPARTS		0x24	// <string>
// #define	FT_DL_PREVIEW			0x25
// #define	FT_KADLASTPUBLISHNOTES		0x26	// <uint32>
// #define	FT_AICH_HASH			0x27
// #define	FT_COMPLETE_SOURCES		0x30	// nr. of sources which share a
// complete version of the
// associated file (supported
// by eserver 16.46+) statistic

// #define	FT_PUBLISHINFO			0x33	// <uint32>
// #define	FT_ATTRANSFERRED		0x50	// <uint32>
// #define	FT_ATREQUESTED			0x51	// <uint32>
// #define	FT_ATACCEPTED			0x52	// <uint32>
// #define	FT_CATEGORY			0x53	// <uint32>
// #define	FT_ATTRANSFERREDHI		0x54	// <uint32>
// #define	FT_MEDIA_ARTIST			0xD0	// <string>
// #define	FT_MEDIA_ALBUM			0xD1	// <string>
// #define	FT_MEDIA_TITLE			0xD2	// <string>
// #define	FT_MEDIA_LENGTH			0xD3	// <uint32> !!!
// #define	FT_MEDIA_BITRATE		0xD4	// <uint32>
// #define	FT_MEDIA_CODEC			0xD5	// <string>
// #define	FT_FILERATING			0xF7	// <uint8>


// Kad search + some unused tags to mirror the ed2k ones.
//file tags

enum file_tags {
        TAG_NONE     = 0,
        TAG_FILENAME = 35,                  // <string>
        TAG_FILESIZE,                   // <ULongLong>
        TAG_FILETYPE,                   // <string>
        TAG_FILEFORMAT,                 // <string>
        TAG_LASTSEENCOMPLETE,               // <uint32_t>
        TAG_PART_PATH,                  // <string>
        TAG_PART_HASH,
        TAG_TRANSFERRED,                 // <uint32_t>
        TAG_GAP_START,                  // <uint32_t>
        TAG_GAP_END,                    // <uint32_t>
        TAG_DESCRIPTION,                    // <string>
        TAG_PING,
        TAG_FAIL,
        TAG_PREFERENCE,
        TAG_VERSION,                    // <string>
        TAG_PARTFILENAME,               // <string>
        TAG_PRIORITY,                   // <uint32_t>
        TAG_STATUS,                 // <uint32_t>
        TAG_SOURCES,                    // <uint32_t>
        TAG_QTIME,
        TAG_PARTS,
        TAG_DLPRIORITY,                 // Was 13
        TAG_ULPRIORITY,                 // Was 17
        TAG_KADLASTPUBLISHKEY,              // <uint32_t>
        TAG_KADLASTPUBLISHSRC,              // <uint32_t>
        TAG_FLAGS,                      // <uint32_t>
#define TAG_FLAGS_COMPLETEFILE 0x01
        TAG_DL_ACTIVE_TIME,             // <uint32_t>
        TAG_CORRUPTEDPARTS,             // <string>
        TAG_DL_PREVIEW,
        TAG_KADLASTPUBLISHNOTES,                        // <uint32_t>
        TAG_AICH_HASH,
        TAG_COMPLETE_SOURCES,           // nr. of sources which share a complete version
        //of the associated file (supported by eserver 16.46+)
        TAG_COLLECTION,
        TAG_PERMISSIONS,

        // statistic
        TAG_ATTRANSFERRED,               // <wxULongLong>
        TAG_ATREQUESTED,                // <uint32_t>
        TAG_ATACCEPTED,             // <uint32_t>
        TAG_CATEGORY,               // <uint32_t>
        TAG_ATTRANSFERREDHI,         // <uint32_t> not used anymore
        TAG_MEDIA_ARTIST,           // <string>
        TAG_MEDIA_ALBUM,                // <string>
        TAG_MEDIA_TITLE,                // <string>
        TAG_MEDIA_LENGTH,           // <uint32_t> !!!
        TAG_MEDIA_BITRATE,          // <uint32_t>
        TAG_MEDIA_CODEC,                // <string>
        TAG_FILERATING,             // <uint8_t>

        TAG_SERVERDEST,             // <CI2PAddress>
        TAG_SOURCEUDEST,                // <CI2PAddress>
        TAG_SOURCEDEST,             // <CI2PAddress>
        TAG_SOURCETYPE,             // <uint8_t>
        TAG_PUBLISHINFO,

        TAG_COPIED,
        TAG_AVAILABILITY,
        TAG_KADMISCOPTIONS,
        TAG_ENCRYPTION,
};

class wxString;
wxString tagnameStr ( int tag );


// Media values for FT_FILETYPE
#define	ED2KFTSTR_AUDIO			wxT("Audio")
#define	ED2KFTSTR_VIDEO			wxT("Video")
#define	ED2KFTSTR_IMAGE			wxT("Image")
#define	ED2KFTSTR_DOCUMENT		wxT("Doc")
#define	ED2KFTSTR_PROGRAM		wxT("Pro")
#define	ED2KFTSTR_ARCHIVE		wxT("Arc")	// *Mule internal use only
#define	ED2KFTSTR_CDIMAGE		wxT("Iso")	// *Mule internal use only

// Additional media meta data tags from eDonkeyHybrid (note also the uppercase/lowercase)
#define	FT_ED2K_MEDIA_ARTIST	"Artist"	// <string>
#define	FT_ED2K_MEDIA_ALBUM		"Album"		// <string>
#define	FT_ED2K_MEDIA_TITLE		"Title"		// <string>
#define	FT_ED2K_MEDIA_LENGTH	"length"	// <string> !!!
#define	FT_ED2K_MEDIA_BITRATE	"bitrate"	// <uint32>
#define	FT_ED2K_MEDIA_CODEC		"codec"		// <string>

#endif // FILETAGS_H
