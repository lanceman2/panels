#include <signal.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <math.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

double t = 0.0;

bool Plot(struct PnWidget *g, struct PnPlot *p, void *userData,
        double xMin, double xMax, double yMin, double yMax) {

    for( uint32_t n = 100; n; t += 0.1, n--) {
        //double a = 1.0 - t/tMax;
        double a = cos(0.34 + t/(540.2 * M_PI));
        pnGraph_drawPoint(p, a * cos(t), a * sin(t));
        t += 0.1;
    }
    pnWidget_queueDraw(g, 0);
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
    pnPlot_setPointSize(p, 4.5);

    pnGraph_setView(w, -1.05, 1.05, -1.05, 1.05);

    pnWindow_show(win);

    Run(win);
    return 0;
}
