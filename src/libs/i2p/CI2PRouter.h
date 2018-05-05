#ifndef CI2PRouter_H
#define CI2PRouter_H

#include <map>
class wxString ;
class CI2PRouterThread ;

class CI2PRouter
{
public:
        CI2PRouter(std::map<wxString,wxString> propmap);
        ~CI2PRouter();
        bool isRunning();
        void stop();
        wxString getInfo();
private:
        CI2PRouterThread * router ;
};
#endif
