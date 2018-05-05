//
// C++ Implementation: conversions
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#pragma GCC java_exceptions

#include <gcj/cni.h>
#include "net/i2p/data/Destination.h"

#ifdef TRUE
#undef TRUE
#define TRUE 1
#endif

#ifdef FALSE
#undef FALSE
#define FALSE 1
#endif

#include <wx/wx.h>

#include "JConversions.h"








wxString toString(jstring orig)
{
        wxString dest;
        int size = JvGetStringUTFLength(orig);
        char * tmp = new char[size+1];
        JvGetStringUTFRegion(orig, 0, size, tmp);
        tmp[size] = '\0';
        dest = wxString(tmp,wxConvLocal);
        delete tmp;
        return dest;
}

wxString toString(net::i2p::data::Destination * d)
{
        jstring js = d->toString();
        return toString(js);
}

jstring toJstring( const wxString & s )
{
        return jstring( JvNewStringUTF ( ( const char * ) s.mb_str() ) );
}
