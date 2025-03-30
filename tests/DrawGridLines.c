#include <signal.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "../include/panels.h"

#include "../lib/debug.h"
#include "../lib/gridLines.h"

#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}



static inline void DestroyBGSurface(struct PnGraph *g) {
    if(g->bgSurface) {
        DASSERT(g->width);
        DASSERT(g->height);
        DASSERT(g->bgMemory);
        DZMEM(g->bgMemory, sizeof(*g->bgMemory)*g->width*g->height);
        free(g->bgMemory);
        cairo_surface_destroy(g->bgSurface);
        g->bgSurface = 0;
        g->bgMemory = 0;
        g->width = 0;
        g->height = 0;
    } else {
        DASSERT(!g->width);
        DASSERT(!g->height);
        DASSERT(!g->bgMemory);
    }
}

static inline void CreateBGSurface(struct PnGraph *g,
        uint32_t w, uint32_t h) {

    g->width = w;
    g->height = h;

    // Add the view box wiggle room, so that the user could pan the view
    // plus and minus the pad values (padX, panY), with the mouse pointer
    // or something.
    //
    // TODO: We could make the padX, and padY, a function of w, and h.
    //
    w += 2 * g->padX;
    h += 2 * g->padY;

    g->bgMemory = calloc(sizeof(*g->bgMemory), w * h);
    ASSERT(g->bgMemory, "calloc(%zu, %" PRIu32 "*%" PRIu32 ") failed",
            sizeof(*g->bgMemory), w, h);

    g->bgSurface = cairo_image_surface_create_for_data(
            (void *) g->bgMemory,
            CAIRO_FORMAT_ARGB32,
            w, h,
            w * 4/*stride in bytes*/);
    DASSERT(g->bgSurface);
}

// TODO: Add a zoom rescaler.
//
static void FreeZooms(struct PnGraph *g) {

    if(!g->zoom) return;

    DASSERT(g->top);

    // Free the zooms.
    while(pnGraph_popZoom(g));

    // Free the last zoom, that will not pop.  If it did pop, that would
    // suck for user's zooming ability.
    DZMEM(g->zoom, sizeof(*g->zoom));
    free(g->zoom);
    g->zoom = 0;
    g->top = 0;
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

        // Colors in:         A,R,G,B
        .subGridColor = { 1.0, 0.5, 0.8, 0.4 },
        .gridColor = { 1.0, 0.8, 0.8, 0.8 },
        .axesLabelColor = { 1.0, 1.0, 1.0, 1.0 }

        // The rest is zero.
    };

    // Could it pop this assertion(s) on an old compiler?
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
