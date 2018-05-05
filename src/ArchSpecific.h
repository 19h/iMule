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

#ifndef ARCHSPECIFIC_H
#define ARCHSPECIFIC_H

#include "Types.h"

#define ENDIAN_SWAP_16(x) (wxUINT16_SWAP_ON_BE(x))
#define ENDIAN_SWAP_I_16(x) x = wxUINT16_SWAP_ON_BE(x)
#define ENDIAN_SWAP_32(x) (wxUINT32_SWAP_ON_BE(x))
#define ENDIAN_SWAP_I_32(x) x = wxUINT32_SWAP_ON_BE(x)

#if ((defined __GNUC__) && __GNUC__ >= 2) || defined (_MSC_VER) || (defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x550))
#define ENDIAN_SWAP_64(x) (wxUINT64_SWAP_ON_BE(x))
#define ENDIAN_SWAP_I_64(x) x = wxUINT64_SWAP_ON_BE(x)
#endif

// ntohs
#define ENDIAN_NTOHS(x) ( wxUINT16_SWAP_ON_LE(x) )
// ntohl
#define ENDIAN_NTOHL(x) ( wxUINT32_SWAP_ON_LE(x) )
// new
#define ENDIAN_NTOHLL(x) ( wxUINT64_SWAP_ON_LE(x) )
// htons
#define ENDIAN_HTONS(x) ( wxUINT16_SWAP_ON_LE(x) )
// htonl
#define ENDIAN_HTONL(x) ( wxUINT32_SWAP_ON_LE(x) )
// new
#define ENDIAN_HTONLL(x) ( wxUINT64_SWAP_ON_LE(x) )


/**
 * Returns the value in the given bytestream.
 *
 * The value is returned exactly as it is found.
 */
// \{
inline uint16_t RawPeekUInt16(const void* p);
inline uint32_t RawPeekUInt32(const void* p);
inline uint64_t RawPeekUInt64(const void* p);
// \}



/**
 * Writes the specified value into the bytestream.
 *
 * The value is written exactly as it is.
 */
// \{
inline void RawPokeUInt16(void* p, uint16_t nVal);
inline void RawPokeUInt32(void* p, uint32_t nVal);
inline void RawPokeUInt64(void* p, uint64_t nVal);
// \}


/**
 * Returns the value in the given bytestream.
 *
 * The value is returned as little-endian.
 */
// \{
inline uint8_t PeekUInt8(const void* p);
inline uint16_t PeekUInt16(const void* p);
inline uint32_t PeekUInt32(const void* p);
inline uint64_t PeekUInt64(const void* p);
// \}


/**
 * Writes the specified value into the bytestream.
 *
 * The value is written as little-endian.
 */
// \{
inline void PokeUInt8(void* p, uint8_t nVal);
inline void PokeUInt16(void* p, uint16_t nVal);
inline void PokeUInt32(void* p, uint32_t nVal);
inline void PokeUInt64(void* p, uint64_t nVal);
// \}


#if defined(__arm__) || defined(__sparc__) || defined(__mips__)
#define ARM_OR_SPARC
#endif


///////////////////////////////////////////////////////////////////////////////
// Peek - helper functions for read-accessing memory without modifying the memory pointer

inline uint16_t RawPeekUInt16(const void* p)
{
#ifndef ARM_OR_SPARC
        return *((uint16_t*)p);
#else
        uint16_t value;
        memcpy( &value, p, sizeof( uint16_t ) );
        return value;
#endif
}


inline uint32_t RawPeekUInt32(const void* p)
{
#ifndef ARM_OR_SPARC
        return *((uint32_t*)p);
#else
        uint32_t value;
        memcpy( &value, p, sizeof( uint32_t ) );
        return value;
#endif
}


inline uint64_t RawPeekUInt64(const void* p)
{
#ifndef ARM_OR_SPARC
        return *((uint64_t*)p);
#else
        uint64_t value;
        memcpy( &value, p, sizeof( uint64_t ) );
        return value;
#endif
}


inline uint8_t PeekUInt8(const void* p)
{
        return *((uint8_t*)p);
}


inline uint16_t PeekUInt16(const void* p)
{
        return ENDIAN_SWAP_16( RawPeekUInt16( p ) );
}


inline uint32_t PeekUInt32(const void* p)
{
        return ENDIAN_SWAP_32( RawPeekUInt32( p ) );
}

inline uint64_t PeekUInt64(const void* p)
{
        return ENDIAN_SWAP_64( RawPeekUInt64( p ) );
}



///////////////////////////////////////////////////////////////////////////////
// Poke - helper functions for write-accessing memory without modifying the memory pointer


inline void RawPokeUInt16(void* p, uint16_t nVal)
{
#ifndef ARM_OR_SPARC
        *((uint16_t*)p) = nVal;
#else
        memcpy( p, &nVal, sizeof(uint16_t) );
#endif
}


inline void RawPokeUInt32(void* p, uint32_t nVal)
{
#ifndef ARM_OR_SPARC
        *((uint32_t*)p) = nVal;
#else
        memcpy( p, &nVal, sizeof(uint32_t) );
#endif
}


inline void RawPokeUInt64(void* p, uint64_t nVal)
{
#ifndef ARM_OR_SPARC
        *((uint64_t*)p) = nVal;
#else
        memcpy( p, &nVal, sizeof(uint64_t) );
#endif
}


inline void PokeUInt8(void* p, uint8_t nVal)
{
        *((uint8_t*)p) = nVal;
}


inline void PokeUInt16(void* p, uint16_t nVal)
{
        RawPokeUInt16( p, ENDIAN_SWAP_16( nVal ) );
}


inline void PokeUInt32(void* p, uint32_t nVal)
{
        RawPokeUInt32( p, ENDIAN_SWAP_32( nVal ) );
}

inline void PokeUInt64(void* p, uint64_t nVal)
{
        RawPokeUInt64( p, ENDIAN_SWAP_64( nVal ) );
}

#endif
// File_checked_for_headers
