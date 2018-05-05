#ifndef SAMLAUNCHER_H
#define SAMLAUNCHER_H

#include <wx/wx.h>
#include <map>

class CSamLauncher
{
public:
        static void start( std::map<wxString, wxString> propmap );
};

#endif


