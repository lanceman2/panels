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
    DASSERT(b);

    b->state = PnButtonState_Hover;
    pnWidget_queueDraw((void *)b);
    return true; // take focus
}


static inline void SetDefaultColors(struct PnButton *b) {

    // These numbers have to be somewhere in the code, so here they are:
    DASSERT(b);

    b->colors[PnButtonState_Normal] =  0xFFCDCDCD;
    b->colors[PnButtonState_Hover] =   0xFFBEE2F3;
    b->colors[PnButtonState_Pressed] = 0xFFD06AC7;
    b->colors[PnButtonState_Active] =  0xFF0BD109;

    if(b->widget.surface.type & W_TOGGLE_BUTTON) {
        ASSERT(0, "WRITE MORE CODE HERE");
    }
}

void destroy(struct PnWidget *w, struct PnButton *b) {

    DASSERT(b);
    DASSERT(b = (void *) w);

    if(w->surface.type & W_BUTTON) {
        DASSERT(b->colors);
        DZMEM(b->colors,
                PnButtonState_NumRegularStates*sizeof(*b->colors));
        free(b->colors);
    } else {
        DASSERT(w->surface.type & W_TOGGLE_BUTTON);
        ASSERT(0, "WRITE MORE CODE");
    }
    //DSPEW();
}

struct PnWidget *pnButton_create(struct PnSurface *parent,
        const char *label, bool toggle) {

    ASSERT(!toggle, "WRITE MORE CODE");

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

    // Setting the widget surface type.  We decrease the data, but
    // increase the complexity.  See enum PnSurfaceType in display.h.
    // It's so easy to forget about all these bits, but DASSERT() is my
    // hero.
    DASSERT(b->widget.surface.type == PnSurfaceType_widget);
    b->widget.surface.type = PnSurfaceType_button;
    DASSERT(b->widget.surface.type & WIDGET);

    pnWidget_setEnter(&b->widget, (void *) enter, b);
    pnWidget_setCairoDraw((void *) b, (void *) cairoDraw, b);
    pnWidget_setDestroy((void *)b, (void *) destroy, b);
    b->colors = calloc(1, PnButtonState_NumRegularStates*sizeof(*b->colors));
    ASSERT(b->colors, "calloc(1,%zu) failed",
            PnButtonState_NumRegularStates*sizeof(*b->colors));
    SetDefaultColors(b);

    return (void *) b;
}
