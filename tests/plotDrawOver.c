#include <signal.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "../include/panels.h"

#include "../lib/xdg-shell-protocol.h"
#include "../lib/xdg-decoration-protocol.h"

#include "../lib/debug.h"
#include "../lib/display.h"
#include "../lib/plot.h"


#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static int cairoDraw(struct PnWidget *w, cairo_t *cr, void *userData) {

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);

    cairo_move_to(cr, 36, 36);
    cairo_line_to(cr, 500, 500);
    cairo_line_to(cr, 36, 500);
    cairo_line_to(cr, 500, 36);
    cairo_line_to(cr, 36, 36);
    cairo_line_to(cr, 44, 44);
    cairo_set_line_width(cr, 18);
    cairo_stroke(cr);

    return 0;
}

int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWidget *win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1100, 900);

    // The auto 2D plotter grid (graph)
    struct PnWidget *plot = pnPlot_create(
            win/*parent*/,
            90/*width*/, 70/*height*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0);
    ASSERT(plot);
    //                  Color Bytes:  A R G B
    pnWidget_setBackgroundColor(plot, 0xA0101010);

    struct PnWidget *over = pnWidget_create(plot/*parent*/,
        0/*width*/, 0/*height*/, PnLayout_Cover,
        0/*align*/, PnExpand_HV, 0/*size*/);
    ASSERT(over);
    pnWidget_setCairoDraw(over, cairoDraw, 0);


    pnWindow_show(win, true);

    Run(win);
    return 0;
}
