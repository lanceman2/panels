#include <signal.h>
#include <inttypes.h>
#include <stdlib.h>

#include "../include/panels.h"
#include "../include/panels_drawingUtils.h"
#include "../lib/debug.h"

#include "run.h"


static void catcher(int sig) {
    ASSERT(0, "caught signal number %d", sig);
}

#define LEN       (100)
#define REC_SIZE  (50) // half the width and height


static struct PnWidget *w;
static uint32_t x0, y0;
static uint32_t num = 0;
static int32_t x[LEN], y[LEN];

static bool motion(struct PnWidget *widget,
            int32_t xi, int32_t yi, void *userData) {
    ASSERT(widget == w);

    //fprintf(stderr, "  %" PRIi32 ",%" PRIi32 "\n", xi, yi);

    if(num < LEN) {
        // xi is the pointer x position in the window.
        // x0 is the x position of the widget in the window.
        // x[num] is a pointer x position relative to the widget.
        x[num] = xi - x0;
        // Ya, y stuff like x above.
        y[num] = yi - y0;
        if(!userData) {
            x[num] -= REC_SIZE;
            y[num] -= REC_SIZE;
        }
        ++num;
        if(num == LEN)
            WARN("Got %" PRIu32 " positions", num);
    }

    pnWidget_queueDraw(w, 0);

    return true;
}

static void config(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    //fprintf(stderr, "config()\n");

    x0 = x;
    y0 = y;

    // stride is in pixels.
    DASSERT(stride >= w);
    // We'll need a value of stride minus the width, w.
    stride -= w;

    uint32_t c = 0xFFFFFFFF;
    for(uint32_t y = 0; y < h; ++y) {
        for(uint32_t x = 0; x < w; ++x)
            *pixels++ = c;
        pixels += stride;
    }
}


static
int draw(struct PnWidget *widget, uint32_t *pixels,
            uint32_t width, uint32_t height, uint32_t stride/*4 bytes*/,
            void *userData) {

    if(!num) return 0;


    if(!userData)
        for(uint32_t i = 0; i < num; ++i) {
            int32_t p = x[i], q = y[i];
            // We must cull out the points that are not in the surface.
            uint32_t w = 2*REC_SIZE, h = 2*REC_SIZE;
            if(p < -2*REC_SIZE || q < -2*REC_SIZE) continue;
            if(p > 0 && p >= width) continue;
            if(q > 0 && q >= height) continue;

            // We must keep the position p,q in the surface and the width,
            // w, and height, h from being too large to fit in the
            // surface.
            if(p < 0) {
                w += p;
                p = 0;
            }
            if(p + w >= width)
                w = width - p;

            if(q < 0) {
                h += q;
                q = 0;
            }
            if(q + h >= height)
                h = height - q;

            pn_drawFilledRectangle(pixels, p, q, w, h,
                    stride, 0x80565151);
        }
    else
        for(uint32_t i = 0; i < num; ++i) {
            int32_t p = x[i], q = y[i];
            // We must cull out the points that are not in the surface.
            if(p < 0 || q < 0) continue;
            if(p < width && q < height)
                pn_drawPixel(pixels, p, q, stride, 0xA0FF0000);
        }

    // Reset the x[], y[] element counter.
    num = 0;

    return 0;
}


int main(int argc, char **argv) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWidget *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);

    w = pnWidget_create(
            win/*parent*/,
            500/*width*/, 550/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0/*size*/);
    ASSERT(w);
    pnWidget_setDraw(w, draw, (void *) (uintptr_t)(argc - 1));
    pnWidget_setConfig(w, config, 0);
    pnWidget_setMotion(w, motion, (void *) (uintptr_t)(argc - 1));

    pnWindow_show(win);

    Run(win);

    return 0;
}
