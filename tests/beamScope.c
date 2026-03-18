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


static volatile sig_atomic_t running = 1;

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static
void quit(int sig) {

    INFO("caught signal number %d.  Cleaning up.", sig);
    running = 0;
}


static double t = 0.0;

bool Plot(struct PnWidget *g, struct PnPlot *p, void *userData,
        double xMin, double xMax, double yMin, double yMax) {

#if 1
    for( uint32_t n = 5; n; t += 0.001, n--) {
        double a = cos(0.34 + t/(1.2 * M_PI));
        //double a = 0.9;
        pnPlot_drawPoint(p, a * cos(t*10.0), a * sin(t*10.0));
    }
#endif
    pnWidget_queueDraw(g, 0);

    return false;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    ASSERT(SIG_ERR != signal(SIGTERM, quit));
    ASSERT(SIG_ERR != signal(SIGINT, quit));

    struct PnWidget *win = pnWindow_create(0, 20, 20,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1100, 900);
    //pnWindow_setPreferredSize(win, 400, 380);

    // The auto 2D plotter grid (graph)
    struct PnWidget *w = pnGraph_create(
            win/*parent*/,
            90/*width*/, 70/*height*/, 0/*align*/,
            PnExpand_HV/*expand*/);
    ASSERT(w);
    //                  Color Bytes:  A R G B
    pnWidget_setBackgroundColor(w, 0xFF000000, 0);

    struct PnPlot *p = pnScopePlot_createWithBeam(w,
            3000/*maxPoints displayed, 0 is infinite*/,
            // NOTE: fade points are CPU usage intense.
            // Run htop and change the below and see for yourself:
            // 1000/*number of fade points*/,
            10/*number of fade points*/,
            Plot, 0);
    ASSERT(p);
    // This plot, p, is owned by the graph, w.
    pnPlot_setLineColor(p, 0xFFFF0000);
    pnPlot_setPointColor(p, 0xFF00FFFF);
    pnPlot_setLineWidth(p, 3.2);
    pnPlot_setPointSize(p, 4.5);

    pnGraph_setView(w, -1.05, 1.05, -1.05, 1.05);

    pnWindow_show(win);

    while(running && pnDisplay_dispatch());

    return 0;
}
