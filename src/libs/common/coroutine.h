#ifndef __COROUTINE__
#define __COROUTINE__

#include <wx/timer.h>

/**
 * contexts for coroutines
 *
 */

#define cr_context(Ctx)   int __s; Ctx() {__s = 0;}
#define cr_begin(ctx)   switch (ctx.__s) { case 0:
#define cr_redo(ctx)    { ctx.__s = __LINE__; case __LINE__: ; }
#define cr_return(x,ctx) { ctx.__s = __LINE__; return x; case __LINE__: ; }
#define cr_exit(ctx)    { ctx.__s = 0; return false; }
#define cr_end(ctx)     { default: break; } } ctx.__s = 0;
#define cr_reinit(ctx)     ctx.__s = 0;

#define CR_BEGIN        switch (__s) { case 0:
#define CR_CALL(cond)   { __s = __LINE__; case __LINE__: if (cond) return true; }
#define CR_RETURN       { __s = __LINE__; return true; case __LINE__: ; }
#define CR_EXIT         { __s = 0; return false; }
#define CR_END          { default: break; } } __s = 0;
#define CR_REINIT       __s = 0;

class CMuleCoroutine : public wxEvtHandler
{
public:
        enum Kind {DETACHED, JOINABLE};
        
        CMuleCoroutine(Kind k = DETACHED, int delay = 0);
        virtual ~CMuleCoroutine() {}
        void Stop() {m_stop = true;}
        void Start();
        bool TestDestroy() const {return m_stop;}
        bool IsAlive() {return __s!=0 and not m_end;}
        virtual void reinit() {__s = 0;}
        void Create() {}
        int __s;
protected:
        virtual bool Continue() = 0;
        virtual void OnExit();
        void Run();
        
        wxTimer sleep;
        bool m_stop;
private:
        Kind _kind;
        bool m_end;
        int  m_delay;
        void RunIdle (wxIdleEvent  & evt);
        void RunTimer(wxTimerEvent & evt);
};

#endif
