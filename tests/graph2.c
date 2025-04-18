#include <signal.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "run.h"


static
void catcher(int sig) {
    ASSERT(0, "caught signal number %d", sig);
}


// PlotPoint() and PlotPoints() do the same thing, just using different
// pnGraph functions.
//
static
void PlotPoint(struct PnPlot *p) {

    const double tMax = 20 * M_PI;

    for(double t = 0.0; t <= 2*tMax + 10; t += 0.1) {
        double a = 1.0 - t/tMax;
        pnGraph_drawPoint(p, a * cos(t), a * sin(t));
    }
}

// Very stupid, but it's just a test.
//
static
void PlotPoints(struct PnPlot *p) {

    const uint32_t Len = 10;
    double x[Len];
    double y[Len];
    uint32_t i = 0;

    const double tMax = 20 * M_PI;

    for(double t = 0.0; t <= 2*tMax + 10; t += 0.1) {
        double a = 1.0 - t/tMax;
        x[i] = a * cos(t);
        y[i++] = a * sin(t);
        if(i == Len) {
            pnGraph_drawPoints(p, x, y, Len);
            i = 0;
        }
    }
    if(i)
        pnGraph_drawPoints(p, x, y, i);
}

static
bool Plot(struct PnWidget *graph, struct PnPlot *p, void *userData) {

    static uint32_t which = 0;

    if(which % 2)
        PlotPoint(p);
    else
        PlotPoints(p);

    ++which;
    return true;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWidget *win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1100, 900);

    // The auto 2D graphter grid (graph)
    struct PnWidget *w = pnGraph_create(
            win/*parent*/,
            90/*width*/, 70/*height*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0);
    ASSERT(w);
    //                  Color Bytes:  A R G B
    pnWidget_setBackgroundColor(w, 0xA0101010);

    pnWidget_addCallback(w, PN_GRAPH_CB_STATIC_DRAW, Plot, 0);
    pnGraph_setView(w, -1.05, 1.05, -1.05, 1.05);

    pnWindow_show(win, true);

    Run(win);
    return 0;
}
