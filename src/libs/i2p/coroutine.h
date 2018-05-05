#ifndef __COROUTINE__
#define __COROUTINE__

/**
 * contexts for coroutines
 *
 */

#define cr_context(Ctx)   int __s; Ctx() {__s = 0;}
#define cr_start(ctx)   switch (ctx.__s) { case 0:
#define cr_redo(ctx)    { ctx.__s = __LINE__; case __LINE__: ; }
#define cr_return(x,ctx) { ctx.__s = __LINE__; return x; case __LINE__: ; }
#define cr_end(ctx)     { break; default: for (;;) ; } } ctx.__s = 0;
#define cr_reinit(ctx)     ctx.__s = 0;

#endif
