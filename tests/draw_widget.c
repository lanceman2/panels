#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "rand.h"
#include "run.h"

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

#define EXPAND  (PnExpand_HV)

const static uint32_t halfPeriod = 300;
const static uint32_t period = halfPeriod * 2;
const static uint32_t thirdPeriod = period/3;
const static uint32_t thirdPeriod2 = (2*period)/3;


// Saw Tooth function.
uint32_t Saw(uint32_t x) {

    x = x % period;

    if(x <= halfPeriod)
        return 255 * x / halfPeriod;
    //else (x > halfPeriod)
    return 255 - (x - halfPeriod) * 255/halfPeriod;
}


static void config(struct PnWidget *surface, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    fprintf(stderr, "config()\n");
}


static
int draw1(struct PnWidget *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    // Some sort of phase angle memory for between draw calls.
    static uint32_t theta = 0;

    // stride is in pixels.

    DASSERT(stride >= w);
    stride -= w;

    uint32_t *pix = pixels;

    // Phase angle for this draw call.
    uint32_t phi = theta;

    // Draw a rainbow of colors. Red, Green, Blue each varying in a saw
    // tooth wave with each color wave one third out of phase of each
    // other.  We tried sine waves but that seemed to be more CPU
    // intensive.

    // We checked this by running htop with and without this for loop,
    // and the CPU usage for this process is mostly from this loop
    // as you'd expect.  So that's a good sign for libpanels.so.

    for(uint32_t y = 0; y < h; ++y) {
        uint32_t c = 0xFF000000 | // alpha
            (Saw(phi) << 16) | // red
            (Saw(phi + thirdPeriod2) << 8) | // green
            (Saw(phi + thirdPeriod)); // blue

        for(uint32_t x = 0; x < w; ++x) {
            // Just wanted to see a vertical strip of the color
            // of the parent widget below.
            if(x < 130 || x > 170)
                *pix = c;
            ++pix;
        }
        pix += stride;
        phi += 1;
    }

    theta += 4;

    // Seems to draw at about 60 Hz and that's good.
    static uint32_t count = 0;
    ++count;
    if(!(count % 60)) {
        if(count > 6000)
            count = 0;
        fprintf(stderr, "  %" PRIu32 " draws\n", count);
    }

    if(userData)
        theta %= period;
    else if(theta > 2*period) {
        theta = 0;
        return 0;
    }

    return 1;
}

static
int draw2(struct PnWidget *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    // stride is in pixels.

    DASSERT(stride >= w);
    stride -= w;

    uint32_t *pix = pixels;

    uint32_t color = 0xCC55AA99;

    for(uint32_t y = 0; y < h; ++y) {
        for(uint32_t x = 0; x < w; ++x)
            *pix++ = color;
        pix += stride;
    }

    return 0;
}


int main(int argc, char **argv) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(2); // rand() seed

    struct PnWidget *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);

    struct PnWidget *w = pnWidget_create(
            win/*parent*/,
            150/*width*/, 40/*height*/,
            0/*layout*/, 0/*align*/,
            EXPAND/*expand*/, 0);
    ASSERT(w);
    pnWidget_setDraw(w, draw1, (void *) (uintptr_t)(argc - 1));
    pnWidget_setConfig(w, config, (void *) (uintptr_t)(argc - 1));


    w = pnWidget_create(win/*parent*/,
            100/*width*/, 100/*height*/,
            0/*layout*/, 0/*align*/,
            EXPAND/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCC00CF00);
    pnWidget_setDraw(w, draw2, 0);

    pnWindow_setPreferredSize(win, 1100, 900);
    pnWindow_show(win);

    Run(win);

    return 0;
}
