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


static int cairoDraw(struct PnSurface *surface,
            cairo_t *cr, struct PnButton *b) {



    return 0;
}

static bool enter(struct PnSurface *surface,
            uint32_t x, uint32_t y, struct PnButton *b) {

    uint32_t color = pnWidget_getBackgroundColor((void *)b);
    color &=  0x00FFFFFF; // change the Alpha
    pnWidget_setBackgroundColor((void *)b, color);

    pnWidget_queueDraw((void *)b);
 
    return true; // take focus
}


struct PnWidget *pnButton_create(struct PnSurface *parent,
        const char *label) {

    // TODO: How many more function parameters should we add to the
    // button create function?
    //
    // We may need to unhide some of these widget create parameters, but
    // we're hoping to auto-generate these parameters as this button
    // abstraction evolves.
    struct PnButton *b = (void *) pnWidget_create(parent,
              200/*width*/, 100/*height*/,
            0/*direction*/, 0/*align*/,
            PnExpand_HV/*expand*/, sizeof(*b));
    if(!b)
        // A common error mode is that the parent cannot have children.
        // pnWidget_create() should spew for us.
        return 0; // Failure.

    pnWidget_setEnter((void *) b, (void *) enter, b);
    pnWidget_setCairoDraw((void *) b, (void *) cairoDraw, b);


    return (void *) b;
}
