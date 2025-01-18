
#include "../include/panels.h"

#include "../lib/debug.h"


int main(void) {

    struct PnWindow *win = pnWindow_create(0, 40, 40,
            0/*x*/, 0/*y*/, PnDirection_TB/*direction*/, 0);
    ASSERT(win);

    struct PnWidget *w = pnWidget_create((struct PnSurface *) win/*parent*/,
            100/*width*/, 100/*height*/,
            0/*direction*/, 0/*align*/, 0/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCCCF0000);

    w = pnWidget_create((struct PnSurface *) win/*parent*/,
            100/*width*/, 300/*height*/,
            0/*direction*/, 0/*align*/, 0/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCC00CF00);

    struct PnWidget *cw = pnWidget_create((struct PnSurface *) win/*parent*/,
            40/*width*/, 40/*height*/,
            PnDirection_RL/*direction*/, 0/*align*/, 0/*expand*/);
    ASSERT(cw);
    pnWidget_setBackgroundColor(cw, 0xCC0000CF);

    {
        w = pnWidget_create((struct PnSurface *) cw/*parent*/,
            130/*width*/, 100/*height*/,
            0/*direction*/, 0/*align*/, 0/*expand*/);
        ASSERT(w);
        pnWidget_setBackgroundColor(w, 0xCCCF0000);

        w = pnWidget_create((struct PnSurface *) cw/*parent*/,
            100/*width*/, 100/*height*/,
            0/*direction*/, 0/*align*/, 0/*expand*/);
        ASSERT(w);
        pnWidget_setBackgroundColor(w, 0xCC00CF00);
    }


    pnWindow_show(win, true);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    return 0;
}
