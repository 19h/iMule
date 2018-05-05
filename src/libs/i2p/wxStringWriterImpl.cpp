#pragma GCC java_exceptions

#include <gcj/cni.h>
#include <WxStringWriter.h>

#ifdef FALSE
#undef FALSE
#define FALSE 0
#endif
#ifdef TRUE
#undef TRUE
#define TRUE 1
#endif

#include <wx/string.h>
#include "JConversions.h"


void WxStringWriter::wxWrite( jstring s )
{
        *( (wxString *) _wxstring ) << ::toString(s) ;
}

