#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

#define EXPAND  (PnExpand_HV)


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWindow *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/, 0,
            PnExpand_HV);
    ASSERT(win);

    struct PnWidget *w = pnWidget_create(
            (struct PnSurface *) win/*parent*/,
            100/*width*/, 200/*height*/,
            0/*direction*/, 0/*align*/, EXPAND/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCCCF0000);

    w = pnWidget_create((struct PnSurface *) win/*parent*/,
            100/*width*/, 300/*height*/,
            0/*direction*/, 0/*align*/, EXPAND/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCC00CF00);
    //pnWidget_show(w, false);

    struct PnWidget *cw = pnWidget_create(
            (struct PnSurface *) win/*parent*/,
            100/*width*/, 100/*height*/,
            PnDirection_LR/*direction*/, 0/*align*/,
            EXPAND/*expand*/);
    ASSERT(cw);
    pnWidget_setBackgroundColor(cw, 0xCC0000CF);

#if 1
    {
        w = pnWidget_create((struct PnSurface *) cw/*parent*/,
            130/*width*/, 100/*height*/,
            0/*direction*/, 0/*align*/, PnExpand_V & 0/*expand*/);
        ASSERT(w);
        pnWidget_setBackgroundColor(w, 0xCCCFFF00);
        //pnWidget_show(w, false);

        w = pnWidget_create((struct PnSurface *) cw/*parent*/,
            100/*width*/, 100/*height*/,
            0/*direction*/, 0/*align*/, EXPAND & 0/*expand*/);
        ASSERT(w);
        pnWidget_setBackgroundColor(w, 0xCC00CFFF);
        pnWidget_show(w, true);
    }
#endif

    pnWindow_show(win, true);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    return 0;
}
