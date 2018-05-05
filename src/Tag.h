//
// This file is part of the imule Project.
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

#ifndef TAG_H
#define TAG_H


#include <common/StringFunctions.h>	// Needed for EUtf8Str

#include <tags/TagTypes.h>

#include "OtherFunctions.h"

class CMD4Hash;
class CFileDataIO;
class CI2PAddress;

///////////////////////////////////////////////////////////////////////////////
// CTag

class CTag
{
        friend class TagList;
public:
        typedef struct {uint64_t first; uint64_t last;} U64U64;
        CTag();
        bool isValid() const;
        //CTag(char* pszName, uint32_t uVal);
        CTag(uint8_t uName, uint64_t uVal);
        //CTag(char* pszName, const wxString& rstrVal);
        CTag(uint8_t uName, const wxString& rstrVal/*comment to see where it is used, EUtf8Str eStrEncode = utf8strRaw*/);
        //CTag(char* pszName, const CI2PAddress& raddrVal); //I2P
        CTag(uint8_t uName, const CI2PAddress& raddrVal); //I2P
        CTag(uint8_t uName, const CMD4Hash& hash);
        CTag(uint8_t uName, uint32_t nSize, const unsigned char* pucData);
        //CTag(uint8_t uName, uint16_t nI16, uint32_t nI32);
        CTag(uint8_t uName, uint64_t first, uint64_t last);
        CTag(const CTag& rTag);
        CTag& operator=(const CTag&);
        CTag(const CFileDataIO& data, bool bOptACP = true);
        void reinit();
        ~CTag();

        Tag_Types GetType() const			{ return m_uType; }
        uint8_t  GetID() const			{ return m_uName; }

        bool IsStr() const		{ return m_uType == TAGTYPE_STRING; }
        bool IsInt() const		{
                return
                        (m_uType == TAGTYPE_UINT64) ||
                        (m_uType == TAGTYPE_UINT32) ||
                        (m_uType == TAGTYPE_UINT16) ||
                        (m_uType == TAGTYPE_UINT8);
        }
        bool IsFloat() const		{ return m_uType == TAGTYPE_FLOAT32; }
        bool IsHash() const		{ return m_uType == TAGTYPE_HASH16; }
        bool IsBlob() const		{ return m_uType == TAGTYPE_BLOB; }
        bool IsBsob() const		{ return m_uType == TAGTYPE_BSOB; }
        bool IsAddr() const				{ return m_uType == TAGTYPE_ADDRESS; }
        bool IsUint64Uint64() const                     { return m_uType == TAGTYPE_UINT64UINT64; }

        uint64 GetInt() const;

        const wxString& GetStr() const;

        const CI2PAddress& GetAddr() const;
        float GetFloat() const;

        const CMD4Hash& GetHash() const;
        const byte* GetBlob() const;
        uint32 GetBlobSize() const;

        const byte* GetBsob() const;
        uint32 GetBsobSize() const;

        U64U64 GetUint64Uint64() const;

        CTag* CloneTag()		{ return new CTag(*this); }

        bool WriteTo(CFileDataIO* file, EUtf8Str eStrEncode = utf8strRaw) const;

        wxString GetFullInfo() const;

        static const CTag null;

protected:
        CTag(uint8 uName);

        Tag_Types   m_uType;
        union {
                CMD4Hash*	m_hashVal;
                wxString*	m_pstrVal;
                uint64		m_uVal;
                float		m_fVal;
                unsigned char*	m_pData;
                CI2PAddress*	m_paddrVal;
                U64U64          m_u64u64;
        };
        uint32		m_nSize;

private:
        uint8		m_uName;
};


class CTagIntSized : public CTag
{
public:
        CTagIntSized(uint8 name, uint64 value, uint8 bitsize)
                : CTag(name) {
                Init(value, bitsize);
        }

protected:
        CTagIntSized(uint8 name) : CTag(name) {}

        void Init(uint64 value, uint8 bitsize) {
                switch (bitsize) {
                case 64:
                        wxASSERT(value <= ULONGLONG(0xFFFFFFFFFFFFFFFF));
                        m_uVal = value;
                        m_uType = TAGTYPE_UINT64;
                        break;
                case 32:
                        wxASSERT(value <= 0xFFFFFFFF);
                        m_uVal = value;
                        m_uType = TAGTYPE_UINT32;
                        break;
                case 16:
                        wxASSERT(value <= 0xFFFF);
                        m_uVal = value;
                        m_uType = TAGTYPE_UINT16;
                        break;
                case 8:
                        wxASSERT(value <= 0xFF);
                        m_uVal = value;
                        m_uType = TAGTYPE_UINT8;
                        break;
                default:
                        throw wxString(wxT("Invalid bitsize on int tag"));
                }
        }
};

class CTagVarInt : public CTagIntSized
{
public:
        CTagVarInt(uint8 name, uint64 value, uint8 forced_bits = 0)
                : CTagIntSized(name) {
                SizedInit(value, forced_bits);
        }
private:
        void SizedInit(uint64 value, uint8 forced_bits) {
                if (forced_bits) {
                        // The bitsize was forced.
                        Init(value,forced_bits);
                } else {
                        m_uVal = value;
                        if (value <= 0xFF) {
                                m_uType = TAGTYPE_UINT8;
                        } else if (value <= 0xFFFF) {
                                m_uType = TAGTYPE_UINT16;
                        } else if (value <= 0xFFFFFFFF) {
                                m_uType = TAGTYPE_UINT32;
                        } else {
                                m_uType = TAGTYPE_UINT64;
                        }
                }
        }
};

class CTagInt64 : public CTagIntSized
{
public:
        CTagInt64(uint8 name, uint64 value)
                : CTagIntSized(name, value, 64) { }
};

class CTagInt32 : public CTagIntSized
{
public:
        CTagInt32(uint8 name, uint64 value)
                : CTagIntSized(name, value, 32) { }
};

class CTagInt16 : public CTagIntSized
{
public:
        CTagInt16(uint8 name, uint64 value)
                : CTagIntSized(name, value, 16) { }
};

class CTagInt8 : public CTagIntSized
{
public:
        CTagInt8(uint8 name, uint64 value)
                : CTagIntSized(name, value, 8) { }
};

class CTagFloat : public CTag
{
public:
        CTagFloat(uint8 name, float value)
                : CTag(name) {
                m_fVal = value;
                m_uType = TAGTYPE_FLOAT32;
        }
};

class CTagString : public CTag
{
public:
        CTagString(uint8 name, const wxString& value)
                : CTag(name) {
                m_pstrVal = new wxString(value);
                m_uType = TAGTYPE_STRING;
        }
};

class CTagHash : public CTag
{
public:
        // Implementation on .cpp to allow forward declaration of CMD4Hash
        CTagHash(uint8 name, const CMD4Hash& value);
};

class CTagBsob : public CTag
{
public:
        CTagBsob(uint8 name, const byte* value, uint8 nSize)
                : CTag(name) {
                m_uType = TAGTYPE_BSOB;
                m_pData = new byte[nSize];
                memcpy(m_pData, value, nSize);
                m_nSize = nSize;
        }
};

class CTagBlob : public CTag
{
public:
        CTagBlob(uint8 name, const byte* value, uint8 nSize)
                : CTag(name) {
                m_uType = TAGTYPE_BLOB;
                m_pData = new byte[nSize];
                memcpy(m_pData, value, nSize);
                m_nSize = nSize;
        }
};

struct TagList : public std::list<CTag> {
        TagList() : std::list<CTag>() {};
        /**
         * Creates a tag list from a file
         * @param data the file to be read
         * @param bOptACP whether the strings should be converted to local codepage or not
         */
        TagList(const CFileDataIO& data, bool bOptACP = true);
        bool WriteTo(CFileDataIO* file) const;
        void deleteTags() ;
};


#endif // TAG_H
// File_checked_for_headers