// This test program does not use the Cairo draw callbacks from
// libpanels.so.  It creates it's own Cairo objects in this file.  So this
// links to libcairo.so seperately from libpanels.so.  libpanels.so does
// not need to be built with Cairo for this test program to work.

#include <signal.h>
#include <stdlib.h>
#include <math.h>

#include <cairo/cairo.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "run.h"

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static cairo_surface_t *surface = 0;
static cairo_t *cr = 0;


static void config(struct PnWidget *s, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    if(cr)
        cairo_destroy(cr);

    if(surface)
        cairo_surface_destroy(surface);

    // We can't use cairo_format_stride_for_width() because the stride is
    // given by the window width, and not the widget width.  It makes no
    // sense to calculate stride at this point, the widget doesn't need to
    // know shit about how the pixel memory is setup, the widget just uses
    // what it's given.  That's why
    // cairo_image_surface_create_for_data() has a stride parameter.  I'm
    // sure GTK uses it to, and I don't have to look at the GTK code to
    // know that.  Ya, I know the GTK widgets builders never see the
    // stride, that's because in GTK widgets have the Cairo objects passed
    // to them in draw().  libpanels lets the user draw how ever the fuck
    // they want; but ya, having a faster pixel memory via OpenGL and GPU
    // stuff should be added.  Yes, drawing with the CPU sucks.
    //
    // Cairo can be slow for many kinds of drawing, but is pretty good in
    // many common cases.
    //
    surface = cairo_image_surface_create_for_data((void *) pixels,
            CAIRO_FORMAT_ARGB32, w, h, stride*4);

    cr = cairo_create(surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
}


static
int draw(struct PnWidget *s, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            void *userData) {

    cairo_set_source_rgba(cr, 1, 0, 0.9, 0.5);
    cairo_paint(cr);

    // https://www.cairographics.org/samples/

    cairo_set_line_width (cr, 6);

    cairo_rectangle(cr, 12, 12, 232, 70);
    cairo_new_sub_path(cr);
    cairo_arc(cr, 64, 64, 40, 0, 2*M_PI);
    cairo_new_sub_path(cr);
    cairo_arc_negative(cr, 192, 64, 40, 0, -2*M_PI);

    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_set_source_rgb(cr, 0, 0.7, 0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_stroke(cr);

    cairo_translate(cr, 0, 128);
    cairo_rectangle(cr, 12, 12, 232, 70);
    cairo_new_sub_path(cr);
    cairo_arc(cr, 64, 64, 40, 0, 2*M_PI);
    cairo_new_sub_path(cr);
    cairo_arc_negative(cr, 192, 64, 40, 0, -2*M_PI);

    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
    cairo_set_source_rgb(cr, 0, 0, 0.9);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_stroke(cr);

    return 0;
}


int main(int argc, char **argv) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWindow *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnDirection_RL/*direction*/, PnAlign_RB,
            PnExpand_HV);
    ASSERT(win);

    struct PnWidget *w = pnWidget_create(
            (struct PnSurface *) win/*parent*/,
            300/*width*/, 300/*height*/,
            0/*direction*/, 0/*align*/,
            PnExpand_None/*expand*/, 0);
    ASSERT(w);
    pnWidget_setDraw(w, draw, (void *) (uintptr_t)(argc - 1));
    pnWidget_setConfig(w, config, (void *) (uintptr_t)(argc - 1));


    w = pnWidget_create((struct PnSurface *) win/*parent*/,
            100/*width*/, 400/*height*/,
            0/*direction*/, 0/*align*/,
            PnExpand_V/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCC00CF00);

    pnWindow_show(win, true);

    Run(win);

    if(cr)
        cairo_destroy(cr);

    if(surface)
        cairo_surface_destroy(surface);

    cairo_debug_reset_static_data();
    return 0;
}
