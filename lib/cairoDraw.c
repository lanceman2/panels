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


void pnSurface_setCairoDraw(struct PnSurface *s,
        int (*draw)(struct PnSurface *surface, cairo_t *cr, void *userData),
        void *userData) {
    DASSERT(s);
    s->cairoDraw = draw;
    s->cairoDrawData = userData;
}
