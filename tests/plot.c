#include <signal.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
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

    pnWindow_show(win, true);

    Run(win);
    return 0;
}
