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

static int cairoDraw(struct PnWidget *w, cairo_t *cr,
            struct PnPlot *p) {

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
WARN();
    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
  //cairo_translate(cr, p->slideX - p->padX, p->slideY - p->padY);
cairo_move_to (cr, 6, 6);
cairo_line_to (cr, 10000, 10000);
cairo_set_line_width(cr, 10);
cairo_stroke(cr);
  cairo_identity_matrix(cr);

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
    struct PnWidget *grid = pnPlot_create(
            win/*parent*/,
            90/*width*/, 70/*height*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0);
    ASSERT(grid);
    //                  Color Bytes:  A R G B
    pnWidget_setBackgroundColor(grid, 0xA0101010);

    struct PnWidget *plot = pnWidget_create(grid/*parent*/,
        0/*width*/, 0/*height*/, PnLayout_Cover,
        0/*align*/, PnExpand_HV, 0/*size*/);
    ASSERT(plot);
    pnWidget_setCairoDraw(plot, (void *) cairoDraw, grid);
    pnWidget_setBackgroundColor(grid, 0x00101010);



    pnWindow_show(win, true);

    Run(win);
    return 0;
}
