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


// This bgSurface uses a locally allocated memory buffer; and is not the
// mmap(2) pixel buffer that is created for us by libpanels that the Cairo
// object is tied to in our draw function (Draw()).  We use it just to
// store a background image of grid lines that we "paste" to the Cairo
// object is tied to in our draw function (Draw()).
//
static cairo_surface_t *bgSurface = 0;
static uint32_t *bgMemory = 0; // This is the memory for the Cairo
                               // surface.


// A 2D plotter has a lot of parameters.  This is just the parameters that
// we choose to draw the background line grid of a 2D graph (plotter).  It
// has a "zoom" object in it that is parametrization of the 2D (linear)
// transformation for the window (widget/pixels) view.  This is the
// built-in stuff.  Of course a user could draw arbitrarily pixels on top
// of this.  And so, yes, this is not natively 2D vector graphics, but one
// could over-lay a reduction of 2D vector graphics on top of this.  I
// don't think that computers are fast enough to make real-time
// (operator-in-the-loop) 2D vector graphics (at 60 Hz or faster), we must
// use pixels as the basis, though the grid uses Cairo, a floating point
// calculation of pixels values for the very static (not changing fast)
// background plotter grid lines.
//
// Note: we could speed up the background grid drawing by not using Cairo,
// because the background grid drawing requires just horizontal and
// vertical line drawing which is easy to calculate the anti-aliasing then
// Cairo's general anti-aliased line (lines at all angles).  But, the
// grid background drawing is not a performance bottleneck, so fuck it.
//
// The foreground points plotted may just directly set the pixel colors as
// a uint32_t (32 bit int), or a slower Cairo based foreground drawing.
// Cairo drawing can be about 10 to 1000 times slower than just changing a
// few individual pixels numbers in the mapped memory.  It's just that we
// are comparing the speed of doing something (memory copying all the
// fucking widget pixels for any one frame, after floating point fucking
// all of them) to the speed of doing next to nothing (like 1 out of a
// 1000 pixels being changed).  It depends where the bottleneck is.  If
// you do not need fast pixel changes, Cairo drawing could be fine.  Cairo
// drawing will likely look better than just integer pixel diddling.
//
// Purpose slowly draw background once (one frame), then quickly draw on
// top of it, 20 million points over many many frames.
//
// We already have a layout widget called PnGrid, so:
//
// The auto 2D plotter grid (graph)
//
struct PnGraph g = {
    .top=0, .zoom=0,

    // floating point scaled size exposed pixels without the padX and padY
    // added (not in number of pixels):
    .xMin=0.0, .xMax=1.0, .yMin=0.0, .yMax=1.0,

    // hidden edges of "zoom box" or "view box".
    .padX=0, .padY=0, // in pixels

    // Colors in:         A,R,G,B
    .subGridColor =   { 1.0, 0.9, 0.4, 0.4 },
    .gridColor =      { 1.0, 0.9, 0.9, 0.9 },
    .axesLabelColor = { 1.0, 1.0, 1.0, 1.0 },

    // width and height of the drawing area in pixels.
    .width=0, .height=0,
    .zoomCount=0
};


static inline void DestroyBGSurface(void) {
    if(bgSurface) {
        DASSERT(g.width);
        DASSERT(g.height);
        DASSERT(bgMemory);
        DZMEM(bgMemory, sizeof(*bgMemory)*g.width*g.height);
        free(bgMemory);
        cairo_surface_destroy(bgSurface);
        bgSurface = 0;
        bgMemory = 0;
        g.width = 0;
        g.height = 0;
    } else {
        DASSERT(!g.width);
        DASSERT(!g.height);
        DASSERT(!bgMemory);
    }
}

static inline void CreateBGSurface(uint32_t w, uint32_t h) {

    g.width = w;
    g.height = h;

    // Add the view box wiggle room, so that the user could pan the view
    // plus and minus the pad values (padX, panY), with the mouse pointer
    // or something.
    //
    // TODO: We could make the padX, and padY, a function of w, and h.
    //
    w += 2 * g.padX;
    h += 2 * g.padY;

    bgMemory = calloc(sizeof(*bgMemory), w * h);
    ASSERT(bgMemory, "calloc(%zu, %" PRIu32 "*%" PRIu32 ") failed",
            sizeof(*bgMemory), w, h);

    bgSurface = cairo_image_surface_create_for_data(
            (void *) bgMemory,
            CAIRO_FORMAT_ARGB32,
            w, h,
            w * 4/*stride in bytes*/);
    DASSERT(bgSurface);
}


static void Config(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    ASSERT(w);
    ASSERT(h);

    if(bgSurface && g.width == w && g.height == h)
        return;

    DestroyBGSurface();
    CreateBGSurface(w, h);

    INFO();
}

static
int Draw(struct PnWidget *w, cairo_t *pn_cr,
            void *userData) {

    cairo_set_source_rgba(pn_cr, 1, 0, 0.9, 0.5);
    cairo_paint(pn_cr);

    return 0;
}



int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    pnGraph_pushZoom(&g, 0.0, 1.0, 0.0, 1.0);

    struct PnWidget *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);

    struct PnWidget *w = pnWidget_create(
            win/*parent*/,
            500/*width*/, 450/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCCCF0000);
    pnWidget_setCairoDraw(w, Draw, 0);
    pnWidget_setConfig(w, Config, 0);


    pnWindow_show(win, true);

    Run(win);

    DestroyBGSurface();
    while(pnGraph_popZoom(&g));

    // Free the last zoom, that will not pop.  If it did pop, that would
    // suck for user's zooming ability.
    ASSERT(g.zoom);
    DZMEM(g.zoom, sizeof(*g.zoom));
    free(g.zoom);

    return 0;
}
