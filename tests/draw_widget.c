#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "rand.h"

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

#define EXPAND  (PnExpand_HV)

const static uint32_t halfPeriod = 422;
const static uint32_t period = halfPeriod * 2;
const static uint32_t thirdPeriod = period/3;
const static uint32_t thirdPeriod2 = (2*period)/3;


static uint32_t theta = 0;

uint32_t Saw(uint32_t x) {

    x = x % period;

    if(x <= halfPeriod)
        return 255 * x / halfPeriod;
    //else (x > 128)
    return 255 - (x - halfPeriod) * 255/halfPeriod;
}


static
int draw1(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {


    // stride is in pixels.

    DASSERT(stride >= w);
    stride -= w;

    uint32_t *pix = pixels;

    uint32_t phi = theta;

    for(uint32_t y = 0; y < h; ++y) {
        uint32_t c = 0xFF000000 |
            (Saw(phi) << 16) |
            (Saw(phi + thirdPeriod)) |
            (Saw(phi + thirdPeriod2) << 8);
        for(uint32_t x = 0; x < w; ++x)
            *pix++ = c;
        pix += stride;
        phi += 1;
    }

    theta += 4;

// Seem to print at about 60 Hz
//static uint32_t count = 0;
//fprintf(stderr, "  %" PRIu32, count++);

    if(theta > period) {
        theta = 0;
        return 0;
    }

    return 1;
}

static
int draw2(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    // stride is in pixels.

    DASSERT(stride >= w);
    stride -= w;

    uint32_t *pix = pixels;

    uint32_t color = 0xCC334499;

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
            500/*width*/, 400/*height*/,
            0/*direction*/, 0/*align*/, EXPAND/*expand*/);
    ASSERT(w);
    pnWidget_setDraw(w, draw1, 0);

    w = pnWidget_create((struct PnSurface *) win/*parent*/,
            100/*width*/, 400/*height*/,
            0/*direction*/, 0/*align*/, EXPAND/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCC00CF00);
    pnWidget_setDraw(w, draw2, 0);



    pnWindow_show(win, true);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    return 0;
}
