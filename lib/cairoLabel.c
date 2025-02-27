// This file is linked into libpanels.so if WITH_CAIRO is defined
// in ../config.make, otherwise it's not.
//
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cairo/cairo.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"
#include "cairoWidget.h"


#define MIN_WIDTH   (30)
#define MIN_HEIGHT  (30)
#define PAD         (2)


static inline void Draw(struct PnLabel *l, cairo_t *cr) {

    const uint32_t color = 0xFFFFFFFF;

    cairo_set_source_rgba(cr,
            PN_R_DOUBLE(color), PN_G_DOUBLE(color),
            PN_B_DOUBLE(color), PN_A_DOUBLE(color));
    cairo_paint(cr);
}

static void config(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h,
            uint32_t stride/*4 byte chunks*/,
            struct PnLabel *l) {
    DASSERT(l);
    DASSERT(l == (void *) widget);
    DASSERT(w);
    DASSERT(h);
    //l->width = w;
    //l->height = h;
}

static int cairoDraw(struct PnWidget *w,
            cairo_t *cr, struct PnLabel *l) {
    DASSERT(l);
    DASSERT(l = (void *) w);
    DASSERT(cr);

    Draw(l, cr);

    return 0;
}


static
void destroy(struct PnWidget *w, struct PnLabel *l) {

    DASSERT(l);
    DASSERT(l = (void *) w);
}


struct PnWidget *pnLabel_create(struct PnSurface *parent,
        uint32_t width, uint32_t height,
        enum PnExpand expand,
        const char *text, size_t size) {

    if(size < sizeof(struct PnLabel))
        size = sizeof(struct PnLabel);
    //
    struct PnLabel *l = (void *) pnWidget_create(parent,
            MIN_WIDTH, MIN_HEIGHT,
            PnDirection_None, 0/*align*/,
            expand, size);
    if(!l)
        // TODO: Not sure that this can fail, other than memory allocation
        // failure.  For now, propagate the failure from
        // pnWidget_create() (which will spew for us).
        return 0; // Failure.

    // Setting the widget surface type.  We decrease the data (just one
    // enum), but increase the complexity (with bit flags in the enum).
    // See enum PnSurfaceType in display.h.  It's so easy to forget about
    // all these bits (in type), but DASSERT() is my hero.
    //
    // It starts out life as a widget:
    DASSERT(l->widget.surface.type == PnSurfaceType_widget);
    // And now it becomes a label:
    l->widget.surface.type = PnSurfaceType_label;
    // which is also a widget too (and more bits):
    DASSERT(l->widget.surface.type & WIDGET);

    pnWidget_setConfig(&l->widget, (void *) config, l);
    pnWidget_setCairoDraw(&l->widget, (void *) cairoDraw, l);
    pnWidget_addDestroy(&l->widget, (void *) destroy, l);

    return &l->widget; // We inherited PnWidget.
}
