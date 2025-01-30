#include <signal.h>
#include <stdlib.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "rand.h"

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

#define EXPAND  (PnExpand_HV)


int draw1(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    // stride is in pixels.

    DASSERT(stride >= w);
    stride -= w;

    uint32_t *pix = pixels;

    for(uint32_t y = 0; y < h; ++y) {
        for(uint32_t x = 0; x < w; ++x)
            *pix++ = 0x1C000000 | Rand(0, 0xFFFFFF);
        pix += stride;
    }

    return 0;
}


int draw2(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    // stride is in pixels.

    DASSERT(stride >= w);
    stride -= w;

    uint32_t *pix = pixels;

    uint32_t color = 0xCC000000 | Rand(0, 0xFFFFFF);

    for(uint32_t y = 0; y < h; ++y) {
        for(uint32_t x = 0; x < w; ++x)
            *pix++ = color;
        pix += stride;
    }

    return 0;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(2); // rand() seed

    struct PnWindow *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/, 0,
            PnExpand_HV);
    ASSERT(win);

    struct PnWidget *w = pnWidget_create(
            (struct PnSurface *) win/*parent*/,
            100/*width*/, 200/*height*/,
            0/*direction*/, 0/*align*/, EXPAND/*expand*/);
    ASSERT(w);
    pnWidget_setDraw(w, draw1, 0);

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
        pnWidget_setDraw(w, draw2, 0);

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
