#include "coroutine.h"
#include <wx/app.h>

CMuleCoroutine::CMuleCoroutine(Kind k, int delay) 
{
        __s = 0; 
        m_stop=false; 
        m_end=false; 
        m_delay = delay;
        _kind=k;
        sleep.Bind(wxEVT_TIMER, &CMuleCoroutine::RunTimer, this);
}

void CMuleCoroutine::Start()
{
        m_end = false;
        if (m_delay==0) {
                wxTheApp->Bind(wxEVT_IDLE, &CMuleCoroutine::RunIdle, this);
        }
        else if (m_delay>0) {
                sleep.StartOnce(m_delay);
        }
        else {
                Run();
        }
}

void CMuleCoroutine::RunTimer(wxTimerEvent &)
{
        if (Continue()) sleep.StartOnce();
        else OnExit();
}

void CMuleCoroutine::RunIdle(wxIdleEvent & /*evt*/)
{
        if (not Continue()) {
                wxTheApp->Unbind(wxEVT_IDLE, &CMuleCoroutine::RunIdle, this);
                OnExit();        
        }
        // for requesting a flow of idle events -> higher cpu
//         else evt.RequestMore();
}

void CMuleCoroutine::Run()
{
        if (not Continue()) {
                OnExit();        
        }
}

void CMuleCoroutine::OnExit() 
{
        m_end = true; 
        if (m_delay==0) {
        }
        if (_kind==DETACHED) delete this;
}

