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


#include "Tag.h"				// Interface declarations

#include <common/Format.h>		// Needed for WXLONGLONGFMTSPEC

#include <Logger.h>
#include "SafeFile.h"		// Needed for CFileDataIO
#include "MemFile.h"
#include "MD4Hash.h"			// Needed for CMD4Hash

#ifdef __SUNPRO_CC
#define __FUNCTION__ __FILE__+__LINE__
#endif

#include "include/tags/FileTags.h"

#define _DEBUG_TAGS

///////////////////////////////////////////////////////////////////////////////
// CTag

const CTag CTag::null;

CTag::CTag()
{
        m_uType = TAGTYPE_INVALID;
}

bool CTag::isValid() const
{
        return m_uType!=TAGTYPE_INVALID;
}

CTag::CTag(uint8 uName)
{
        m_uType = TAGTYPE_INVALID;
        m_uName = uName;
        m_uVal = 0;
        m_nSize = 0;
}

CTag::CTag(uint8_t uName, uint64_t uVal)
{
        m_uType = TAGTYPE_UINT64;
        m_uName = uName;
        m_uVal = uVal;
        m_nSize = 0;
}

CTag::CTag(uint8_t uName, const wxString& rstrVal)
{
        m_uType = TAGTYPE_STRING;
        m_uName = uName;
        m_pstrVal = new wxString(rstrVal);
        m_nSize = 0;
}


CTag::CTag(uint8_t uName, const CI2PAddress& raddrVal)
{
        m_uType = TAGTYPE_ADDRESS;
        m_uName = uName;
        m_paddrVal = new CI2PAddress(raddrVal);
        m_nSize = 0;
}

CTag::CTag(uint8_t uName, const CMD4Hash& hash)
{
        m_uType = TAGTYPE_HASH16;
        m_uName = uName;
        m_hashVal = new CMD4Hash(hash);
        m_nSize = 0;
}

CTag::CTag(uint8_t uName, uint32_t nSize, const unsigned char* pucData)
{
        m_uType = TAGTYPE_BLOB;
        m_uName = uName;
        m_pData = new unsigned char[nSize];
        memcpy(m_pData, pucData, nSize);
        m_nSize = nSize;
}

CTag::CTag(uint8_t uName, uint64_t first, uint64_t last)
{
        m_uType = TAGTYPE_UINT64UINT64;
        m_uName = uName;
        m_u64u64.first = first;
        m_u64u64.last  = last;
}

CTag::CTag(const CTag& rTag)
{
        m_uType = TAGTYPE_INVALID;
        (*this) = rTag;
}

CTag::CTag(const CFileDataIO& data, bool bOptUTF8)
{
        uint32_t ntag = 0;
        // Zero variables to allow for safe deletion
        m_nSize = 0 ;
        m_uName = 0        ;
        m_uType = TAGTYPE_INVALID ;
        m_pData = NULL;

        try {
                m_uType = (Tag_Types) data.ReadUInt8();
                if (m_uType & 0x80) {
                        m_uType =  (Tag_Types) ( (uint32_t) m_uType & 0x7F);
                        m_uName = data.ReadUInt8();
                } else {
                        uint16 length = data.ReadUInt16();
                        if (length == 1) {
                                m_uName = data.ReadUInt8();
                        } else {
                                m_uName = 0;
                        }
                }

                // NOTE: It's very important that we read the *entire* packet data,
                // even if we do not use each tag. Otherwise we will get in trouble
                // when the packets are returned in a list - like the search results
                // from a server. If we cannot do this, then we throw an exception.
#ifdef _DEBUG_TAGS
                ntag = data.ReadUInt32() ;
#endif
                switch (m_uType) {
                case TAGTYPE_STRING:
                        m_pstrVal = new wxString(data.ReadString(bOptUTF8));
                        break;

                case TAGTYPE_UINT32:
                        m_uVal = data.ReadUInt32();
                        break;

                case TAGTYPE_UINT64:
                        m_uVal = data.ReadUInt64();
                        break;

                case TAGTYPE_UINT16:
                        m_uVal = data.ReadUInt16();
                        m_uType = TAGTYPE_UINT32;
                        break;

                case TAGTYPE_UINT8:
                        m_uVal = data.ReadUInt8();
                        m_uType = TAGTYPE_UINT32;
                        break;

                case TAGTYPE_FLOAT32:
                        //#warning Endianess problem?
                        data.Read(&m_fVal, 4);
                        break;

                case TAGTYPE_HASH16:
                        m_hashVal = new CMD4Hash(data.ReadHash());
                        break;

                case TAGTYPE_BOOL:
                        printf("***NOTE: %s; Reading BOOL tag\n", __FUNCTION__);
                        m_uVal = data.ReadUInt8();
                        break;

                case TAGTYPE_BOOLARRAY: {
                        printf("***NOTE: %s; Reading BOOL Array tag\n", __FUNCTION__);
                        uint16 len = data.ReadUInt16();

                        // 07-Apr-2004: eMule versions prior to 0.42e.29 used the formula "(len+7)/8"!
                        //TODO #warning This seems to be off by one! 8 / 8 + 1 == 2, etc.
                        //data.Seek((len / 8) + 1, wxFromCurrent);
                        data.Seek((len+7) / 8, wxFromCurrent);
                        break;
                }

                case TAGTYPE_BLOB:
                        // 07-Apr-2004: eMule versions prior to 0.42e.29 handled the "len" as int16!
                        m_nSize = data.ReadUInt32();

                        // Since the length is 32b, this check is needed to avoid
                        // huge allocations in case of bad tags.
                        if (m_nSize > data.GetLength() - data.GetPosition()) {
                                throw CInvalidPacket(wxT("Malformed tag"));
                        }

                        m_pData = new unsigned char[m_nSize];
                        data.Read(m_pData, m_nSize);
                        break;

                case TAGTYPE_ADDRESS:
                        m_paddrVal = new CI2PAddress(data.ReadAddress());
                        break;

                case TAGTYPE_UINT64UINT64:
                        m_u64u64.first = CTag(data).GetInt();
                        m_u64u64.last  = CTag(data).GetInt();
                        break;

                default:
                        if (m_uType >= TAGTYPE_STR1 && m_uType <= TAGTYPE_STR16) {
                                uint8 length = m_uType - TAGTYPE_STR1 + 1;
                                m_pstrVal = new wxString(data.ReadOnlyString(bOptUTF8, length));
                                m_uType = TAGTYPE_STRING;
                        } else {
                                // Since we cannot determine the length of this tag, we
                                // simply have to abort reading the file.
                                throw CInvalidPacket(CFormat(wxT("Unknown tag type encounted 0x%x, cannot proceed!")) % (int)m_uType);
                        }
                }
        } catch (...) {
                reinit() ;

                throw;
        }
        AddDebugLogLineN(logCTag, CFormat(wxT("Received #%d, type %s , name %s : %s "))
                         % ntag
                         % tagtypeStr(m_uType)
                         % tagnameStr(m_uName)
                         % GetFullInfo() );
}


void CTag::reinit()
{
        if (IsAddr()) {
                delete m_paddrVal;
        } else if (IsStr()) {
                delete m_pstrVal;
        } else if (IsHash()) {
                delete m_hashVal;
        } else if (IsBlob() || IsBsob()) {
                delete[] m_pData;
        }
        m_uType = TAGTYPE_INVALID;
}


CTag::~CTag()
{
        reinit();
}


CTag &CTag::operator=(const CTag& rhs)
{
        if (&rhs != this) {
                reinit();
                m_uType = rhs.m_uType;
                m_uName = rhs.m_uName;
                m_nSize = 0;

                if (rhs.IsStr()) {
                        m_pstrVal = new wxString(rhs.GetStr());
                } else if (rhs.IsInt()) {
                        m_uVal = rhs.GetInt();
                } else if (rhs.IsFloat()) {
                        m_fVal = rhs.GetFloat();
                } else if (rhs.IsHash()) {
                        m_hashVal = new CMD4Hash(rhs.GetHash());
                } else if (rhs.IsBlob()) {
                        m_nSize = rhs.GetBlobSize();
                        m_pData = new unsigned char[rhs.GetBlobSize()];
                        memcpy(m_pData, rhs.GetBlob(), rhs.GetBlobSize());
                } else if (rhs.IsBsob()) {
                        m_nSize = rhs.GetBsobSize();
                        m_pData = new unsigned char[rhs.GetBsobSize()];
                        memcpy(m_pData, rhs.GetBsob(), rhs.GetBsobSize());
                } else if (rhs.IsAddr()) {
                        m_paddrVal = new CI2PAddress(rhs.GetAddr());
                } else if (rhs.IsUint64Uint64()) {
                        m_u64u64.first = rhs.GetUint64Uint64().first;
                        m_u64u64.last = rhs.GetUint64Uint64().last;
                } else {
                        wxFAIL;
                        m_uVal = 0;
                }
        }
        return *this;
}





#define CHECK_TAG_TYPE(check, expected) \
	if (!(check)) { \
		throw CInvalidPacket(wxT(#expected) wxT(" tag expected, but found ") + GetFullInfo()); \
	}

uint64 CTag::GetInt() const
{
        CHECK_TAG_TYPE(IsInt(), Integer);

        return m_uVal;
}

const wxString& CTag::GetStr() const
{
        CHECK_TAG_TYPE(IsStr(), String);

        return *m_pstrVal;
}


float CTag::GetFloat() const
{
        CHECK_TAG_TYPE(IsFloat(), Float);

        return m_fVal;
}


const CMD4Hash& CTag::GetHash() const
{
        CHECK_TAG_TYPE(IsHash(), Hash);

        return *m_hashVal;
}


uint32 CTag::GetBlobSize() const
{
        CHECK_TAG_TYPE(IsBlob(), Blob);

        return m_nSize;
}


const byte* CTag::GetBlob() const
{
        CHECK_TAG_TYPE(IsBlob(), Blob);

        return m_pData;
}


uint32 CTag::GetBsobSize() const
{
        CHECK_TAG_TYPE(IsBsob(), Bsob);

        return m_nSize;
}


const byte* CTag::GetBsob() const
{
        CHECK_TAG_TYPE(IsBsob(), Bsob);

        return m_pData;
}

const CI2PAddress& CTag::GetAddr() const
{
        CHECK_TAG_TYPE(IsAddr(), Addr);

        return *m_paddrVal;
}

CTag::U64U64 CTag::GetUint64Uint64() const
{
        CHECK_TAG_TYPE(IsUint64Uint64(), Uint64Uint64);

        return m_u64u64;
}


bool CTag::WriteTo(CFileDataIO* data, EUtf8Str eStrEncode) const
{

        // Write tag type
        uint8 uType;

        if (IsInt()) {
                if (m_uVal <= 0xFF) {
                        uType = TAGTYPE_UINT8;
                } else if (m_uVal <= 0xFFFF) {
                        uType = TAGTYPE_UINT16;
                } else if (m_uVal <= 0xFFFFFFFF) {
                        uType = TAGTYPE_UINT32;
                } else  {
                        uType = TAGTYPE_UINT64;
                }
        } else if (IsStr()) {
                uint16 uStrValLen = GetRawSize(*m_pstrVal, eStrEncode);
                if (uStrValLen >= 1 && uStrValLen <= 16) {
                        uType = TAGTYPE_STR1 + uStrValLen - 1;
                } else {
                        uType = TAGTYPE_STRING;
                }
        } else {
                uType = m_uType;
        }

        // Write tag name
        if (false) {
                data->WriteUInt8(uType);
                data->WriteString(wxT("m_Name"),utf8strNone);
        } else {
                wxASSERT( m_uName != 0 );
                data->WriteUInt8(uType | 0x80);
                data->WriteUInt8(m_uName);
#ifdef _DEBUG_TAGS
                static uint32_t ntag = 0;
                ntag++;
                data->WriteUInt32(ntag);
                AddDebugLogLineN(logCTag, CFormat(wxT("Sent     #%d, type %s , name %s : %s"))
                                 % ntag
                                 % tagtypeStr(m_uType)
                                 % tagnameStr(m_uName)
                                 % GetFullInfo() ) ;
#endif
        }

        // Write tag data
        switch (uType) {
        case TAGTYPE_STRING:
                data->WriteString(*m_pstrVal,eStrEncode);
                break;
        case TAGTYPE_UINT64:
                data->WriteUInt64(m_uVal);
                break;
        case TAGTYPE_UINT32:
                data->WriteUInt32(m_uVal);
                break;
        case TAGTYPE_UINT16:
                data->WriteUInt16(m_uVal);
                break;
        case TAGTYPE_UINT8:
                data->WriteUInt8(m_uVal);
                break;
        case TAGTYPE_FLOAT32:
                //#warning Endianess problem?
                data->Write(&m_fVal, 4);
                break;
        case TAGTYPE_HASH16:
                data->WriteHash(*m_hashVal);
                break;
        case TAGTYPE_BLOB:
                data->WriteUInt32(m_nSize);
                data->Write(m_pData, m_nSize);
                break;
        case TAGTYPE_ADDRESS:
                data->WriteAddress(*m_paddrVal);
                break;
        case TAGTYPE_UINT64UINT64:
                CTag( m_uName, m_u64u64.first ).WriteTo(data);
                CTag( m_uName, m_u64u64.last  ).WriteTo(data);
                break;
        default:
                // See comment on the default: of CTag::CTag(const CFileDataIO& data, bool bOptUTF8)
                if (uType >= TAGTYPE_STR1 && uType <= TAGTYPE_STR16) {
                        // Sending '0' as len size makes it not write the len on the IO file.
                        // This is because this tag types send the len as their type.
                        data->WriteString(*m_pstrVal,eStrEncode,0);
                } else {
                        printf("%s; Unknown tag: type=0x%02X\n", __FUNCTION__, uType);
                        wxFAIL;
                        return false;
                }
                break;
        }

        return true;
}

wxString CTag::GetFullInfo() const
{
        wxString strTag;
        strTag = tagnameStr(m_uName);
        strTag += wxT("=");
        if (m_uType == TAGTYPE_ADDRESS) {
                strTag += wxT("\"");
                strTag += m_paddrVal->humanReadable();
                strTag += wxT("\"");
        } else if (m_uType == TAGTYPE_STRING) {
                strTag += wxT("\"");
                strTag += *m_pstrVal;
                strTag += wxT("\"");
        } else if (m_uType >= TAGTYPE_STR1 && m_uType <= TAGTYPE_STR16) {
                strTag += CFormat(wxT("(Str%u)\"")) % (m_uType - TAGTYPE_STR1 + 1)
                          +  *m_pstrVal + wxT("\"");
        } else if (m_uType == TAGTYPE_UINT64) {
                strTag += CFormat(wxT("(Int64)%u")) % m_uVal;
        } else if (m_uType == TAGTYPE_UINT32) {
                strTag += CFormat(wxT("(Int32)%u")) % m_uVal;
        } else if (m_uType == TAGTYPE_UINT16) {
                strTag += CFormat(wxT("(Int16)%u")) % m_uVal;
        } else if (m_uType == TAGTYPE_UINT8) {
                strTag += CFormat(wxT("(Int8)%u")) % m_uVal;
        } else if (m_uType == TAGTYPE_FLOAT32) {
                strTag += CFormat(wxT("(Float32)%f")) % m_fVal;
        } else if (m_uType == TAGTYPE_BLOB) {
                strTag += CFormat(wxT("(Blob)%u")) % m_nSize;
        } else if (m_uType == TAGTYPE_BSOB) {
                strTag += CFormat(wxT("(Bsob)%u")) % m_nSize;
        } else if (m_uType == TAGTYPE_UINT64UINT64) {
                strTag += CFormat(wxT("(couple)(%u,%u)")) % m_u64u64.first % m_u64u64.last;
        } else {
                strTag += CFormat(wxT("Type=%u")) % (int)m_uType;
        }
        return strTag;
}

CTagHash::CTagHash(uint8 name, const CMD4Hash& value)
        : CTag(name)
{
        m_hashVal = new CMD4Hash(value);
        m_uType = TAGTYPE_HASH16;
}

TagList::TagList(const CFileDataIO& data, bool bOptACP)
        : std::list<CTag>()
{
        uint64_t pos = data.GetPosition() ;
        uint8_t n = data.ReadUInt8() ;
        AddDebugLogLineN(logCTag, CFormat(wxT("TagList : pos = %llu, %d tags to read")) % pos % n ) ;
        while (n--) push_back( CTag(data, bOptACP) );
}

bool TagList::WriteTo(CFileDataIO* file) const
{
        CMemFile temp ;
        uint8_t n = 0;
        wxASSERT( size() < 0xFF );
        for (const_iterator it=begin(); it!=end(); it++)
                if (it->WriteTo(&temp, utf8strRaw)) n++ ;

        temp.Seek(0, wxFromStart);
        char buffer[ temp.GetLength() ];
        temp.Read( buffer, (size_t) temp.GetLength() );

        uint64_t pos = file->GetPosition() ;
        AddDebugLogLineN(logCTag, CFormat(wxT("TagList : pos = %llu, %d tags to write")) % pos % n ) ;
        file->WriteUInt8( n );
        file->Write( buffer, (size_t) temp.GetLength() );
        AddDebugLogLineN(logCTag, CFormat(wxT("TagList : %d tags written")) % n ) ;
        return true;
}

void TagList::deleteTags()
{
        clear() ;
}



// File_checked_for_headers