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


static
int cairoDraw(struct PnWidget *s, cairo_t *cr,
            void *userData) {

    cairo_set_source_rgba(cr, 1, 0, 0.9, 0.5);
    cairo_paint(cr);

    // from https://www.cairographics.org/samples/

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


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWidget *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnLayout_RL/*layout*/, PnAlign_RB,
            PnExpand_HV);
    ASSERT(win);

    struct PnWidget *w = pnWidget_create(
            win/*parent*/,
            300/*width*/, 300/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_None/*expand*/, 0);
    ASSERT(w);
    pnWidget_setCairoDraw(w, cairoDraw, 0);

    w = pnWidget_create(win/*parent*/,
            100/*width*/, 400/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_V/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCC00CF00, 0);

    pnWindow_show(win);

    Run(win);

    return 0;
}
