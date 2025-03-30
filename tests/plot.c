#include <signal.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "../include/panels.h"
#include "../lib/debug.h"
#include "../lib/plot.h"

#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}



static void Config(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            struct PnGraph *g) {

    ASSERT(w);
    ASSERT(h);

    if(g->bgSurface && g->width == w && g->height == h)
        return;

    FreeZooms(g);
    DestroyBGSurface(g);

    CreateBGSurface(g, w, h);

    // Now we have the graph width and height we can
    // create a zoom.
    pnGraph_pushZoom(g, 0.0, 1.0, 0.0, 1.0);

    cairo_t *cr = cairo_create(g->bgSurface);
    pnGraph_drawGrids(g, cr, true/*show_subGrid*/);
    cairo_destroy(cr);
}

static int cairoDraw(struct PnWidget *w, cairo_t *cr,
            struct PnGraph *g) {

    //cairo_set_source_rgba(cr, 1, 0, 0.9, 0.5);
    //cairo_paint(cr);

    // Transfer the bgSurface to this cr (and its surface).
    cairo_set_source_surface(cr, g->bgSurface, g->padX, g->padY);
    cairo_rectangle(cr, g->padX, g->padY, g->width, g->height);
    cairo_fill(cr);

    return 0;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    // The auto 2D plotter grid (graph)
    //
    struct PnGraph graph = {

        // floating point scaled size exposed pixels without the padX and
        // padY added (not in number of pixels):
        .xMin=0.0,
        .xMax=1.0,
        .yMin=0.0,
        .yMax=1.0,

        // hidden edges of "zoom box" or "view box", in pixels.
        .padX=0,
        .padY=0,

        // Colors bytes in order: A,R,G,B
        .subGridColor = 0xFF70B070,
        .gridColor = 0xFFE0E0E0,
        .axesLabelColor = 0xFFFFFFFF

        // The rest is zero.
    };

    // Could it pop these assertion(s) on an old compiler?
    ASSERT(graph.bgSurface == 0);
    ASSERT(graph.bgMemory == 0);

    struct PnWidget *win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1100, 900);

    struct PnWidget *w = pnWidget_create(
            win/*parent*/,
            90/*width*/, 70/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCCCF0000);
    pnWidget_setCairoDraw(w, (void *) cairoDraw, &graph);
    pnWidget_setConfig(w, (void *) Config, &graph);

    pnWindow_show(win, true);

    Run(win);

    FreeZooms(&graph);
    DestroyBGSurface(&graph);

    return 0;
}
