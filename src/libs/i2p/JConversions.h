//
// C++ Interface: JConversions
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef JCONVERSIONS_H

extern "Java" {
        namespace net
        {
        namespace i2p
        {
        namespace data
        {
        class Destination;
        }
        }
        }
        namespace java
        {
        namespace lang
        {
        class String;
        }
        }
}


extern "C++" {
        class wxString ;
}

wxString toString(java::lang::String* orig);
wxString toString(net::i2p::data::Destination * d);
java::lang::String* toJstring( const wxString & s );

#endif
