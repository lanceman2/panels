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

bool Plot(struct PnWidget *g, struct PnPlot *p, void *userData,
        double xMin, double xMax, double yMin, double yMax) {

    ASSERT(userData == (void *) catcher); // testing userData.

    const double tMax = 20 * M_PI;

    for(double t = 0.0; t <= 2*tMax + 10; t += 0.1) {
        double a = 1.0 - t/tMax;
        pnGraph_drawPoint(p, a * cos(t), a * sin(t));
    }
    return false;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWidget *win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1100, 900);

    // The auto 2D plotter grid (graph)
    struct PnWidget *w = pnGraph_create(
            win/*parent*/,
            90/*width*/, 70/*height*/, 0/*align*/,
            PnExpand_HV/*expand*/);
    ASSERT(w);
    //                  Color Bytes:  A R G B
    pnWidget_setBackgroundColor(w, 0xA0101010, 0);

    struct PnPlot *p = pnScopePlot_create(w, Plot, catcher);
    ASSERT(p);
    // This plot, p, is owned by the graph, w.
    pnPlot_setLineColor(p, 0xFFFF0000);
    pnPlot_setPointColor(p, 0xFF00FFFF);
    pnPlot_setLineWidth(p, 3.2);
    pnPlot_setPointSize(p, 12.5);

    pnGraph_setView(w, -1.05, 1.05, -1.05, 1.05);

    pnWindow_show(win);

    Run(win);
    return 0;
}
