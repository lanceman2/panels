#include <signal.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "run.h"

uint32_t which = 0;

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

bool Plot(struct PnWidget *plot, cairo_t *cr, void *userData) {

    const double tMax = 20 * M_PI;

    for(double t = 0.0; t <= 2*tMax + 10; t += 0.1) {
        double a = 1.0 - t/tMax;
        pnPlot_drawPoint(plot, a * cos(t), a * sin(t));
    }
    return true;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWidget *win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1100, 900);

    // The auto 2D plotter grid (graph)
    struct PnWidget *w = pnPlot_create(
            win/*parent*/,
            90/*width*/, 70/*height*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0);
    ASSERT(w);
    //                  Color Bytes:  A R G B
    pnWidget_setBackgroundColor(w, 0xA0101010);

    pnWidget_addCallback(w, PN_PLOT_CB_STATIC_DRAW, Plot, 0);
    pnPlot_setView(w, -1.05, 1.05, -1.05, 1.05);

    pnWindow_show(win, true);

    Run(win);
    return 0;
}
