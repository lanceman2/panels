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


// PlotPoint() and PlotPoints() do the same thing, just using different
// pnGraph functions.
//
void PlotPoint(struct PnWidget *graph) {

    const double tMax = 20 * M_PI;

    for(double t = 0.0; t <= 2*tMax + 10; t += 0.1) {
        double a = 1.0 - t/tMax;
        pnGraph_drawPoint(graph, a * cos(t), a * sin(t));
    }
}

// Very stupid, but it's just a test.
//
void PlotPoints(struct PnWidget *graph) {

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
            pnGraph_drawPoints(graph, x, y, Len);
            i = 0;
        }
    }
    if(i)
        pnGraph_drawPoints(graph, x, y, i);
}

bool Plot(struct PnWidget *graph, cairo_t *cr, void *userData) {

    if(which % 2)
        PlotPoint(graph);
    else
        PlotPoints(graph);

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
