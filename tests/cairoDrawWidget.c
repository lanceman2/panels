#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include <cairo/cairo.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "rand.h"

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static cairo_surface_t *surface = 0;
static cairo_t *cr = 0;


static void config(struct PnSurface *s, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    if(cr)
        cairo_destroy(cr);

    if(surface)
        cairo_surface_destroy(surface);

    // We can't use cairo_format_stride_for_width() because the stride is
    // given by the window width and height, and not the widget width and
    // height.  It makes no sense to calculate stride at this point, the
    // widget doesn't need to know shit about how the pixel memory is
    // setup, the widget just uses the what it's given.  That's why
    // cairo_image_surface_create_for_data() has a stride parameter.  I'm
    // sure GTK uses it to, and I don't have to look at the GTK code to
    // know that.  Ya, I know the GTK widgets builders never see the
    // stride, that's because in GTK widgets have the cairo objects passed
    // to them in draw().
    //
    surface = cairo_image_surface_create_for_data((void *) pixels,
            CAIRO_FORMAT_ARGB32, w, h, stride*4);

    cr = cairo_create(surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
}


static
int draw(struct PnSurface *s, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    cairo_set_source_rgba(cr, 1, 0, 0, 0.5);
    cairo_paint(cr);

    return 0;
}


int main(int argc, char **argv) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWindow *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/, 0,
            PnExpand_HV);
    ASSERT(win);

    struct PnWidget *w = pnWidget_create(
            (struct PnSurface *) win/*parent*/,
            500/*width*/, 400/*height*/,
            0/*direction*/, 0/*align*/, PnExpand_HV/*expand*/);
    ASSERT(w);
    pnWidget_setDraw(w, draw, (void *) (uintptr_t)(argc - 1));
    pnWidget_setConfig(w, config, (void *) (uintptr_t)(argc - 1));


    w = pnWidget_create((struct PnSurface *) win/*parent*/,
            100/*width*/, 400/*height*/,
            0/*direction*/, 0/*align*/, PnExpand_None/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCC00CF00);

    pnWindow_show(win, true);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    if(cr)
        cairo_destroy(cr);

    if(surface)
        cairo_surface_destroy(surface);

    cairo_debug_reset_static_data();
    return 0;
}
