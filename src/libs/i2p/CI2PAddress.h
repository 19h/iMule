//
// C++ Interface: CI2PAddress
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//


#ifndef __address_h__
#define __address_h__

#include <inttypes.h>
#include <iostream>
#include "common/Format.h"
#include "common/MuleDebug.h"
#include "b64codec.h"
#include "Logger.h"

#define PUBKEY_LEN 387
#define CERTIFICATE_BYTE_START 384
#define CERTIFICATE_LENGTH 3

/* The length of a base 64 public key - it's actually 516, but +1 for '\0' */
#define BASE64_PUBKEY_LEN 516
typedef char sam_pubkey_t[BASE64_PUBKEY_LEN+1];  /* base 64 public key, followed by '\0' */


class CI2PAddress
{
public:
        class DataFormatException : public CInvalidPacket
        {
        public:
                DataFormatException(wxString reason) : CInvalidPacket(wxT("CI2PAddress::DataFormatException : ") + reason) {}
        };
        inline static uint16_t validLength();

        inline void init();
        inline CI2PAddress();
        inline CI2PAddress(const uint8_t & type, bool empty);
        inline CI2PAddress(const sam_pubkey_t);
        inline CI2PAddress(const CI2PAddress & src);
        inline CI2PAddress(const wxString & src);
        inline CI2PAddress& operator=( const CI2PAddress & src );
        inline CI2PAddress& operator=( const wxString & src );

        inline bool isValid() const;
        inline bool operator!() const;

        inline const char * toBase64CharArray() const;
        inline const wxString toString() const {return wxString::FromAscii(m_b64);}
        inline static CI2PAddress fromString(wxString);

        inline wxString humanReadable() const;
        inline const wxString& getAlias() const {return m_alias;}
        inline void setAlias(wxString);

        inline bool operator== (const CI2PAddress &) const;
        inline bool operator!= (const CI2PAddress & other) const {return !(*this==other);}
        inline bool operator<  (const CI2PAddress &) const;

        inline wxUint32 size() const;
        inline wxUint32 writeBytes(uint8_t * buffer) const;
        inline wxUint32 readBytes(const uint8_t * buffer);
        inline uint32_t hashCode() const {
	  uint32_t r;
	  uint8_t * p = (uint8_t *)&r;
	  p[0]=m_bytes[0];
	  p[1]=m_bytes[1];
	  p[2]=m_bytes[2];
	  p[3]=m_bytes[3];
	  return r;
	}

        inline operator uint32_t() const {return hashCode();}

protected:

        inline void     check();

        sam_pubkey_t m_b64;
        uint8_t      m_bytes[PUBKEY_LEN];
        wxString     m_alias;
        char         m_type;  // special uses in imule

public:
        static const CI2PAddress null;
        static const CI2PAddress complete_file;
        static const CI2PAddress incomplete_file;

private:
        static uint16_t s_validLength;
        static const char INVALID_TAG 		;
        static const char VALID_TAG 		;
        static const char ALIAS_TAG 		;
        static const char COMPLETE_FILE_TAG 	;
        static const char INCOMPLETE_FILE_TAG 	;
};





CI2PAddress::CI2PAddress()
{
        init();
        m_type = INVALID_TAG;
}


CI2PAddress::CI2PAddress (const uint8_t & typetag, bool)
{
        init();
        m_type     = typetag;
}

void CI2PAddress::init()
{
        memset(m_b64, 0, BASE64_PUBKEY_LEN+1);
        memset(m_bytes, 0, PUBKEY_LEN);
        m_alias    = wxEmptyString;
        m_type     = INVALID_TAG;
}

CI2PAddress::CI2PAddress( const sam_pubkey_t dest )
{
        base64::decoder dec;
        init();
        if (!dest) {
                throw DataFormatException(wxT("Initializing CI2PAddress from NULL destination"));
        }
        memcpy( m_b64, dest, BASE64_PUBKEY_LEN );
        dec.decode((const char*)m_b64, BASE64_PUBKEY_LEN, (char*)m_bytes);
        check();
        return;
}

CI2PAddress::CI2PAddress(const CI2PAddress & src)
{
        (*this) = src;
}

CI2PAddress::CI2PAddress(const wxString & src)
{
        (*this) = src;
}

CI2PAddress& CI2PAddress::operator=( const CI2PAddress & src )
{
        init();
        memcpy( m_b64, src.m_b64, BASE64_PUBKEY_LEN );
        memcpy( m_bytes, src.m_bytes, PUBKEY_LEN );
        m_type = src.m_type;
        m_alias = src.m_alias.c_str();
        return (*this);
}

CI2PAddress& CI2PAddress::operator=( const wxString & src )
{
        return (*this) = fromString(src);
}

CI2PAddress CI2PAddress::fromString(wxString s)
{
        CI2PAddress ans = null;
        AddDebugLogLineN(logAddress, wxString(wxT("from string ")) << wxT(" <-- ") << s );
        if (s.Length()==BASE64_PUBKEY_LEN) {
	  ans = CI2PAddress( (const char*)s.mb_str() );
        } else {
                ans.setAlias(s);
        }
        return ans;
}

void CI2PAddress::setAlias(wxString alias)
{
        m_alias=alias.c_str();
        if (!m_alias.IsEmpty())
                m_type |= ALIAS_TAG ;
}



const char * CI2PAddress::toBase64CharArray() const
{
        return m_b64 ;
}

uint32_t CI2PAddress::writeBytes ( uint8_t * buffer ) const
{
        memcpy( buffer, m_bytes, PUBKEY_LEN );
        return PUBKEY_LEN;
}


uint32_t CI2PAddress::readBytes ( const uint8_t * buffer )
{
        base64::encoder enc;
        m_type = INVALID_TAG ;

        memcpy( m_bytes, buffer, PUBKEY_LEN );
        enc.encode((const char*)m_bytes, PUBKEY_LEN, (char*)m_b64);
        AddDebugLogLineN(logAddress, CFormat(wxT("read : dest=%x")) % hashCode());
        check();
        return PUBKEY_LEN;
}

void CI2PAddress::check()
{
        if (memcmp(m_bytes+CERTIFICATE_BYTE_START, null.m_bytes, CERTIFICATE_LENGTH)!=0)
	  {
	    uint16_t length; uint8_t * p = (uint8_t *) &length;
	    p[0]     = m_bytes[CERTIFICATE_BYTE_START+1];
	    p[1]     = m_bytes[CERTIFICATE_BYTE_START+2];
	    throw DataFormatException(CFormat(wxT("Invalid address : hashCode=%u certType=%u certLength=%u"))
                                          % hashCode() % *(uint8_t*)(m_bytes+CERTIFICATE_BYTE_START) % length
                                         );
	  }
        if (hashCode()!=0) m_type |= VALID_TAG ;
}


wxString CI2PAddress::humanReadable() const
{
        return CFormat(wxT("%x")) % (hashCode()&0xFFFF);
}

inline uint16_t CI2PAddress::validLength()
{
        return PUBKEY_LEN;
}

inline bool CI2PAddress::isValid() const
{
        return ( m_type & VALID_TAG );
}

inline bool CI2PAddress::operator!() const
{
        return !(isValid());
}



inline bool CI2PAddress::operator== (const CI2PAddress & other) const
{
        if (isValid()) {
                if (other.isValid())
                        return hashCode()==other.hashCode();
                else
                        return false;
        } else if (other.isValid()) {
                return false;
        } else if ( (m_type & ALIAS_TAG) & other.m_type ) {
                return m_alias==other.m_alias;
        } else {
                return m_type==other.m_type;
        }
}

inline bool CI2PAddress::operator<  (const CI2PAddress & other) const
{
        return (m_type<other.m_type
                || (m_type==other.m_type 
                        && (m_alias<other.m_alias
                                || (m_alias==other.m_alias 
                                        && (hashCode()<other.hashCode())))));
}

inline uint32_t CI2PAddress::size() const
{
        return PUBKEY_LEN;
}


#endif
