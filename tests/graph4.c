#include <signal.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "run.h"
#include "rand.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static
bool Plot(struct PnWidget *g, struct PnPlot *p, void *userData,
        double xMin, double xMax, double yMin, double yMax) {

    const double tMax = 20 * M_PI;

    for(double t = 0.0; t <= 2*tMax + 10; t += 0.1) {
        double a = 1.0 - t/tMax;
        // inline function
        pnPlot_drawPoint(p, a * cos(t), a * sin(t));
    }
    return false;
}

static
bool Plot2(struct PnWidget *g, struct PnPlot *p, void *userData,
        double xMin, double xMax, double yMin, double yMax) {

    const double tMax = 20 * M_PI;

    for(double t = 0.02; t <= 2*tMax + 10; t += 0.1) {
        double a = 1.0 - t/tMax;
        // This in an inline function, so it can be faster then
        // if it was not an inline function.
        pnPlot_drawPoint(p, a * sin(t), a * cos(t));
    }
    return false;
}

static
bool Plot3(struct PnWidget *g, struct PnPlot *p, void *userData,
        double xMin, double xMax, double yMin, double yMax) {

    const double tMax = 20 * M_PI;

    for(double t = 0.02; t <= 2*tMax + 10; t += 0.1) {
        double a = 1.0 - t/tMax;
        // This in an inline function, so it can be faster then
        // if it was not an inline function.
        pnPlot_drawPoint(p, a * sin(t), a * cos(t));
    }
    return false;
}

static
bool Plot4(struct PnWidget *g, struct PnPlot *p, void *userData,
        double xMin, double xMax, double yMin, double yMax) {

    pnPlot_drawPoint(p, -1.0, -1.0);
    pnPlot_drawPoint(p, -0.5, -0.5);
    pnPlot_drawPoint(p, -0.5, 0.0);

    pnPlot_drawPoint(p, 0.0, 0.0);
    pnPlot_drawPoint(p, 0.5, 0.5);
    pnPlot_drawPoint(p, 1.0, 0.5);
    pnPlot_drawPoint(p, 1.0, 1.0);
     return false;
}


static struct PnWidget *win;

static uint32_t count = 0;


static struct PnWidget *MakeGraph(bool (*plot)(struct PnWidget *g,
            struct PnPlot *p, void *userData,
            double xMin, double xMax, double yMin, double yMax)) {

    ++count;

    // The auto 2D plotter grid (graph)
    struct PnWidget *w = pnGraph_create(
            win/*parent*/,
            400/*width*/, 400/*height*/, 0/*align*/,
            PnExpand_HV/*expand*/);
    ASSERT(w);
    //                  Color Bytes:  A R G B
    pnWidget_setBackgroundColor(w, Color(), 0);

    struct PnPlot *p = pnStaticPlot_create(w, plot, 0);
    ASSERT(p);
    // This plot, p, is owned by the graph, w.
    pnPlot_setLineColor(p, Color());
    pnPlot_setPointColor(p, Color());
    pnPlot_setLineWidth(p, Rand(1, 8.2));
    pnPlot_setPointSize(p, Rand(1, 8.2));
    pnGraph_setView(w, -1.05, 1.05, -1.05, 1.05);

    switch(count) {
        case 1:
            break;
        case 2:
            // No points
            pnPlot_setPointSize(p, 0);
            break;
        case 3:
            // No lines
            pnPlot_setLineWidth(p, 0);
            break;
        case 4:
            pnGraph_setView(w, -1.5, 1.5, -1.5, 1.5);
        default:
    }       

    return w;
}


int main(void) {

    srand(5);

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1650, 450);

    MakeGraph(Plot);
    MakeGraph(Plot2);
    MakeGraph(Plot3);
    MakeGraph(Plot4);

    pnWindow_show(win);

    Run(win);
    return 0;
}
