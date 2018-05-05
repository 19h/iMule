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

#include "SafeFile.h"				// Interface declarations.
#include "MD4Hash.h"				// Needed for CMD4Hash
#include "kademlia/utils/UInt128.h"	// Needed for CUInt128
#include "Logger.h"
#include <common/Format.h>	// Needed for CFormat
#include "i2p/CI2PAddress.h"			// Needed for CI2PAddress
#include "MemFile.h"
#include <memory>

#if defined(__SUNPRO_CC)
#define __FUNCTION__ __FILE__+__LINE__
#endif

#define CHECK_BOM(size, x) ((size >= 3)  && (x[0] == (char)0xEF) && (x[1] == (char)0xBB) && (x[2] == (char)0xBF))

const char BOMHeader[3] = { '\xEF', '\xBB', '\xBF'};


CSafeIOException::CSafeIOException(const wxString& type, const wxString& desc)
        : CMuleException(wxT("SafeIO::") + type, desc) {}


CEOFException::CEOFException(const wxString& desc)
        : CSafeIOException(wxT("EOF"), desc) {}


CIOFailureException::CIOFailureException(const wxString& desc)
        : CSafeIOException(wxT("IOFailure"), desc) {}

CIOFailureException::CIOFailureException(const wxString& type, const wxString& desc)
        : CSafeIOException(wxT("IOFailure::") + type, desc) {}


///////////////////////////////////////////////////////////////////////////////
// CFileDataIO


CFileDataIO::~CFileDataIO()
{
}


bool CFileDataIO::Eof() const
{
        return GetPosition() >= GetLength();
}


void CFileDataIO::Read(void *buffer, size_t count) const
{
        MULE_VALIDATE_PARAMS(buffer, wxT("Attempting to write to NULL buffer."));

        // Check that we read everything we wanted.
        if (doRead(buffer, count) == (signed)count) {
                return;
        }

        // To reduce potential system calls, we only do EOF checks when reads fail.
        if (Eof()) {
                throw CEOFException(wxT("Attempt to read past end of file."));
        } else {
                throw CIOFailureException(wxT("Read error, failed to read from file."));
        }
}


void CFileDataIO::Write(const void* buffer, size_t count)
{
        MULE_VALIDATE_PARAMS(buffer, wxT("Attempting to read from NULL buffer."));

        if (doWrite(buffer, count) != (signed)count) {
                throw CIOFailureException(wxT("Write error, failed to write to file."));
        }
}


uint64 CFileDataIO::Seek(sint64 offset, wxSeekMode from) const
{
        sint64 newpos = 0;
        switch (from) {
        case wxFromStart:
                newpos = offset;
                break;

        case wxFromCurrent:
                newpos = GetPosition() + offset;
                break;

        case wxFromEnd:
                newpos = GetLength() + offset;
                break;

        default:
                MULE_VALIDATE_PARAMS(false, wxT("Invalid seek-mode specified."));
        }

        MULE_VALIDATE_PARAMS(newpos >= 0, wxT("Position after seeking would be less than zero!"));

        sint64 result = doSeek(newpos);
        MULE_VALIDATE_STATE(result >= 0, wxT("Seeking resulted in invalid offset."));
        MULE_VALIDATE_STATE(result == newpos, wxT("Target position and actual position disagree."));

        return result;
}


uint8 CFileDataIO::ReadUInt8() const
{
        uint8 value = 0;
        Read(&value, sizeof(uint8));

        return value;
}


uint16 CFileDataIO::ReadUInt16() const
{
        uint16 value = 0;
        Read(&value, sizeof(uint16));

        return ENDIAN_SWAP_16(value);
}

CI2PAddress CFileDataIO::ReadAddress() const
{
        CI2PAddress value;
        byte bytes[CI2PAddress::validLength()];
        Read(bytes, CI2PAddress::validLength());
        value.readBytes( bytes );
        return value;
}

void CFileDataIO::WriteAddress(const CI2PAddress& value)
{
        uint16_t writeLen  = CI2PAddress::validLength();
        byte bytes[writeLen];
        value.writeBytes(bytes);
        Write(bytes, writeLen);
}


uint32 CFileDataIO::ReadUInt32() const
{
        uint32 value = 0;
        Read(&value, sizeof(uint32));

        return ENDIAN_SWAP_32(value);
}


uint64 CFileDataIO::ReadUInt64() const
{
        uint64 value = 0;
        Read(&value, sizeof(uint64));

        return ENDIAN_SWAP_64(value);
}


// UInt128 values are stored a little weird way...
// Four little-endian 32-bit numbers, stored in
// big-endian order
CUInt128 CFileDataIO::ReadUInt128() const
{
        CUInt128 value;
        for (int i = 0; i < 4; i++) {
                // Four 32bits chunks
                value.Set32BitChunk(i, ReadUInt32());
        }

        return value;
}


CMD4Hash CFileDataIO::ReadHash() const
{
        CMD4Hash value;
        Read(value.GetHash(), MD4HASH_LENGTH);

        return value;
}


float CFileDataIO::ReadFloat() const
{
        float retVal;
        Read(&retVal, sizeof(float));
        return retVal;
}


unsigned char* CFileDataIO::ReadBsob(uint8* puSize) const
{
        MULE_VALIDATE_PARAMS(puSize, wxT("NULL pointer argument in ReadBsob"));

        *puSize = ReadUInt8();

        std::unique_ptr<unsigned char[]> bsob(new unsigned char[*puSize]);
        Read(bsob.get(), *puSize);

        return bsob.release();
}


wxString CFileDataIO::ReadString(bool bOptUTF8, uint8 SizeLen, bool SafeRead) const
{
        uint32 readLen;
        switch (SizeLen) {
        case sizeof(uint16):	readLen = ReadUInt16();	break;
        case sizeof(uint32):	readLen = ReadUInt32();	break;

        default:
                MULE_VALIDATE_PARAMS(false, wxT("Invalid SizeLen value in ReadString"));
        }

        if (SafeRead) {
                readLen = std::min<uint64>(readLen, GetLength() - GetPosition());
        }

        return ReadOnlyString(bOptUTF8, readLen);
}


wxString CFileDataIO::ReadOnlyString(bool bOptUTF8, uint16 raw_len) const
{
        // We only need to set the the NULL terminator, since we know that
        // reads will either succeed or throw an exception, in which case
        // we wont be returning anything
        std::vector<char> val_array(raw_len + 1);
        val_array[raw_len] = 0;

        char* val = &(val_array[0]);

        Read(val, raw_len);
        wxString str;

        if (CHECK_BOM(raw_len, val)) {
                // This is a UTF8 string with a BOM header, skip header.
                str = UTF82unicode(val + 3);
        } else if (bOptUTF8) {
                str = UTF82unicode(val);
                if (str.IsEmpty()) {
                        // Fallback to Latin-1
                        str = wxString(val, wxConvISO8859_1, raw_len);
                }
        } else {
                // Raw strings are written as Latin-1 (see CFileDataIO::WriteStringCore)
                str = wxString(val, wxConvISO8859_1, raw_len);
        }

        return str;
}


void CFileDataIO::WriteUInt8(uint8 value)
{
        Write(&value, sizeof(uint8));
}


void CFileDataIO::WriteUInt16(uint16 value)
{
        ENDIAN_SWAP_I_16(value);

        Write(&value, sizeof(uint16));
}


void CFileDataIO::WriteUInt32(uint32 value)
{
        ENDIAN_SWAP_I_32(value);

        Write(&value, sizeof(uint32));
}


void CFileDataIO::WriteUInt64(uint64 value)
{
        ENDIAN_SWAP_I_64(value);

        Write(&value, sizeof(uint64));
}


// UInt128 values are stored a little weird way...
// Four little-endian 32-bit numbers, stored in
// big-endian order
void CFileDataIO::WriteUInt128(const Kademlia::CUInt128& value)
{
        for (int i = 0; i < 4; i++) {
                // Four 32bits chunks
                WriteUInt32(value.Get32BitChunk(i));
        }
}


void CFileDataIO::WriteHash(const CMD4Hash& value)
{
        Write(value.GetHash(), MD4HASH_LENGTH);
}


void CFileDataIO::WriteFloat(float value)
{
        Write(&value, sizeof(float));
}


void CFileDataIO::WriteBsob(const unsigned char* value, uint8 size)
{
        WriteUInt8(size);
        Write(value, size);
}


void CFileDataIO::WriteString(const wxString& str, EUtf8Str eEncode, uint8 SizeLen)
{
        switch (eEncode) {
        case utf8strRaw:
        case utf8strOptBOM: {
                Unicode2CharBuf s(unicode2UTF8(str));
                //                 printf("CFileDataIO::WriteString %s\n", (const char *) s);
                if (s) {
                        WriteStringCore(s, eEncode, SizeLen);
                        break;
                }
        }
        default: {
                // Non UTF-8 strings are saved as Latin-1
                wxCharBuffer s1 = wxConvISO8859_1.cWC2MB(str.c_str());
                WriteStringCore(s1, utf8strNone, SizeLen);
        }
        }
}


void CFileDataIO::WriteStringCore(const char *s, EUtf8Str eEncode, uint8 SizeLen)
{
        uint32 sLength = s ? strlen(s) : 0;
        uint32 real_length = 0;
        if (eEncode == utf8strOptBOM) {
                real_length = sLength + 3; // For BOM header.
        } else {
                real_length = sLength;
        }

        switch (SizeLen) {
        case 0:
                // Don't write size.
                break;

        case sizeof(uint16):
                // We must not allow too long strings to be written,
                // as this would allow for a buggy clients to "poison"
                // us, by sending ISO8859-1 strings that expand to a
                // greater than 16b length when converted as UTF-8.
                if (real_length > 0xFFFF) {
                        wxFAIL_MSG(wxT("String is too long to be saved"));

                        real_length = std::min<uint32>(real_length, 0xFFFF);
                        if (eEncode == utf8strOptBOM) {
                                sLength = real_length - 3;
                        } else {
                                sLength = real_length;
                        }
                }

                WriteUInt16(real_length);
                break;

        case sizeof(uint32):
                WriteUInt32(real_length);
                break;

        default:
                MULE_VALIDATE_PARAMS(false, wxT("Invalid length for string-length field."));
        }

        // The BOM header must be written even if the string is empty.
        if (eEncode == utf8strOptBOM) {
                Write(BOMHeader, 3);
        }

        // Only attempt to write non-NULL strings.
        if (sLength) {
                // No NULL terminator is written since we explicitly specify the length
                Write(s, sLength);
        }
}


CTag *CFileDataIO::ReadTag(bool bOptACP) const
{
        CTag *retVal = NULL;
        try {
                retVal = new CTag(*this, bOptACP);
        } catch(const CMuleException& e) {
                AddLogLineN(e.what());
                delete retVal;
                throw;
        } catch(const wxString& e) {
                AddLogLineN(e);
                throw;
        }

        return retVal;
}


void CFileDataIO::ReadTagList(TagList* taglist, bool bOptACP) const
{
        MULE_VALIDATE_PARAMS(taglist, wxT("NULL pointer argument in ReadTagPtrList"));
        TagList tl(*this, bOptACP);
        *taglist = tl ;
}


void CFileDataIO::WriteTag(const CTag& tag)
{
        try {
                tag.WriteTo(this,utf8strRaw);
        } catch (...) {
                AddLogLineNS(wxT("Exception in CDataIO:WriteTag"));
                throw;
        }
}


void CFileDataIO::WriteTagList(const TagList& tagList)
{
        tagList.WriteTo(this);
}

uint64 CFileDataIO::GetIntTagValue() const
{

        uint8 type = ReadUInt8();

        ReadString(false);

        switch (type) {

        case TAGTYPE_UINT64:
                return ReadUInt64();
                break;

        case TAGTYPE_UINT32:
                return ReadUInt32();
                break;

        case TAGTYPE_UINT16:
                return ReadUInt16();
                break;

        case TAGTYPE_UINT8:
                return ReadUInt8();
                break;

        default:
                throw wxString(wxT("Wrong tag type reading int tag"));
        }
}
// File_checked_for_headers