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
#include  "display.h"


static inline void CreateCairo(struct PnBuffer *buffer,
        struct PnSurface *s) {

    DASSERT(s);
    DASSERT(buffer);
    DASSERT(!s->cairo_surface);
    DASSERT(!s->cr);

    if(s->culled || s->noDrawing ||
            // We use cairoDraw if we can we are not using the
            // non-cairo draw function.
            (!s->cairoDraw && s->draw)) return;

    s->cairo_surface = cairo_image_surface_create_for_data(
        (void *) (buffer->pixels +
            s->allocation.y * buffer->stride +
            s->allocation.x),
            CAIRO_FORMAT_ARGB32,
            s->allocation.width, s->allocation.height,
            buffer->stride*4);
    ASSERT(s->cairo_surface);
    s->cr = cairo_create(s->cairo_surface);
    ASSERT(s->cr);
    cairo_set_operator(s->cr, CAIRO_OPERATOR_SOURCE);
}

static void CreateCairos(struct PnBuffer *buffer,
        struct PnSurface *s) {

    DASSERT(s);

    CreateCairo(buffer, s);

    if(s->layout != PnLayout_Grid) {
        for(s = s->l.firstChild; s; s = s->pl.nextSibling)
            CreateCairos(buffer, s);
        return;
    }

    // s is a grid container.
    struct PnSurface ***child = s->g.grid->child;
    DASSERT(child);
    for(uint32_t y=s->g.numRows-1; y != -1; --y)
        for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
            struct PnSurface *c = child[y][x];
            if(!c || c->culled) continue;
            if(IsUpperLeftCell(child[y][x], child, x, y))
                CreateCairos(buffer, c);
        }
}

void pnSurface_setCairoDraw(struct PnSurface *s,
        int (*draw)(struct PnSurface *surface, cairo_t *cr, void *userData),
        void *userData) {
    DASSERT(s);
    DASSERT(s->window);
    s->cairoDraw = draw;
    s->cairoDrawData = userData;

    if(draw && !s->cr && s->window->buffer.wl_buffer)
        // This surface, "s", might need a Cairo surface (and Cairo
        // object).
        CreateCairo(&s->window->buffer, s);
    else if(!draw && s->draw)
        // This surface, "s", will not use Cairo to draw.  The user is
        // unsetting the Cairo draw callback.
        DestroyCairo(s);

    // We'll let the user queue the draw of this surface, if they need to.
    // Also it could be the window is not being shown now.
}


void RecreateCairos(struct PnWindow *win) {

    DASSERT(win);
    struct PnBuffer *buffer = &win->buffer;
    DASSERT(buffer->pixels);
    DASSERT(buffer->width);
    DASSERT(buffer->height);
    DASSERT(buffer->stride);
    DASSERT(buffer->wl_buffer);

    DestroyCairos(&win->widget.surface);
    CreateCairos(buffer, &win->widget.surface);
}

void DestroyCairo(struct PnSurface *s) {
    DASSERT(s);
    if(s->cr) {
        DASSERT(s->cairo_surface);
        cairo_destroy(s->cr);
        cairo_surface_destroy(s->cairo_surface);
        s->cr = 0;
        s->cairo_surface = 0;
    } else {
        DASSERT(!s->cairo_surface);
    }
}

void DestroyCairos(struct PnSurface *s) {

    DASSERT(s);
    DestroyCairo(s);

    // Now destroy the cairos in all children:
    //
    if(s->layout != PnLayout_Grid) {
        for(s = s->l.firstChild; s; s = s->pl.nextSibling)
            DestroyCairos(s);
        return;
    }
    //
    // s is a grid container.
    struct PnSurface ***child = s->g.grid->child;
    DASSERT(child);
    for(uint32_t y=s->g.numRows-1; y != -1; --y)
        for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
            struct PnSurface *c = child[y][x];
            if(!c || c->culled) continue;
            if(IsUpperLeftCell(c, child, x, y))
                DestroyCairos(c);
        }
}
