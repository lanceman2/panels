
#include "../include/panels.h"

#include "../lib/debug.h"


int main(void) {

    struct PnWindow *win = pnWindow_create(0, 10, 30,
            0/*x*/, 0/*y*/, 0/*gravity*/);
    ASSERT(win);
    pnWindow_show(win, true);

    struct PnWidget *w = pnWidget_create((struct PnSurface *) win/*parent*/,
            100/*width*/, 200/*height*/,
            0/*gravity*/, 0/*greed*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xFAA99999);
    pnWidget_show(w, true);

    w = pnWidget_create((struct PnSurface *) win/*parent*/,
            100/*width*/, 100/*height*/,
            0/*gravity*/, 0/*greed*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xFAA99999);
    pnWidget_show(w, true);


#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    return 0;
}
