//
// This file is part of the imule Project.
//
// Copyright (c) 2004-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2006 imule Team ( mkvore@mail.i2p / http://www.imule.i2p )
// Copyright (c) 2003 Barry Dunne (http://www.emule-project.net)
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

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "LittleEndian.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////
namespace Kademlia
{
////////////////////////////////////////

//NOTE: avoid those function whenever possible -> terribly slow
uint16_t le(uint16_t val)
{
        uint32_t b0 = (val      ) & 0xFF;
        uint32_t b1 = (val >>  8) & 0xFF;
        return (uint16_t) ((b0 << 8) | b1);
}

//NOTE: avoid those function whenever possible -> terribly slow
uint32_t le(uint32_t val)
{
        uint32_t b0 = (val      ) & 0xFF;
        uint32_t b1 = (val >>  8) & 0xFF;
        uint32_t b2 = (val >> 16) & 0xFF;
        uint32_t b3 = (val >> 24) & 0xFF;
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

//NOTE: avoid those function whenever possible -> terribly slow
wxULongLong le(wxULongLong val)
{
        uint32_t lo = val.GetLo();
        uint32_t hi = val.GetHi();

        uint8_t b0 = (uint8_t)((lo      ) & 0xFF);
        uint8_t b1 = (uint8_t)((lo >>  8) & 0xFF);
        uint8_t b2 = (uint8_t)((lo >> 16) & 0xFF);
        uint8_t b3 = (uint8_t)((lo >> 24) & 0xFF);
        uint8_t b4 = (uint8_t)((hi      ) & 0xFF);
        uint8_t b5 = (uint8_t)((hi >>  8) & 0xFF);
        uint8_t b6 = (uint8_t)((hi >> 16) & 0xFF);
        uint8_t b7 = (uint8_t)((hi >> 24) & 0xFF);
        return ((wxULongLong)b0 << 56) | ((wxULongLong)b1 << 48) | ((wxULongLong)b2 << 40) | ((wxULongLong)b3 << 32) |
               ((wxULongLong)b4 << 24) | ((wxULongLong)b5 << 16) | ((wxULongLong)b6 << 8) | (wxULongLong)b7;
}

} // End namespace
