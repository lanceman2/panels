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

static inline void
SetState(struct PnButton *b, enum PnButtonState state) {
    DASSERT(b);
    if(state == b->state) return;
    b->state = state;
    pnWidget_queueDraw(&b->widget);
}


static inline void Draw( cairo_t *cr, struct PnButton *b) {

    uint32_t color = b->colors[b->state];

    cairo_set_source_rgba(cr,
            PN_R_DOUBLE(color), PN_G_DOUBLE(color),
            PN_B_DOUBLE(color), PN_A_DOUBLE(color));
    cairo_paint(cr);


}

static void config(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h,
            uint32_t stride/*4 byte chunks*/,
            struct PnButton *b) {

    DASSERT(b);
    DASSERT(b == (void *) widget);
    if(b->frames) b->frames = 0;
    SetState(b, PnButtonState_Normal);
    DASSERT(w);
    DASSERT(h);
    b->width = w;
    b->height = h;
}



static int cairoDraw(struct PnWidget *w,
            cairo_t *cr, struct PnButton *b) {

    DASSERT(b);
    DASSERT(b->state < PnButtonState_NumRegularStates);

    if(b->state != PnButtonState_Active) {
        Draw(cr, b);
        if(b->frames)
            b->frames = 0;
        return 0;
    }

    DASSERT(b->frames);

    if(b->frames == NUM_FRAMES)
        // We draw just once.
        Draw(cr, b);

    --b->frames;

    if(!b->frames)
        SetState(b, PnButtonState_Hover);

    return 1;
}

static bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnButton *b) {
    if(which == 0) {
        SetState(b, PnButtonState_Pressed);
        return true;
    }
    return false;
}

static bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnButton *b) {
    if(which == 0) {
        struct PnAllocation a;
        pnWidget_getAllocation(w, &a);
        if(b->state == PnButtonState_Pressed &&
                a.x <= x && a.y <= y &&
                x < a.x + a.width &&
                y < a.y + a.height) {
            SetState(b, PnButtonState_Active);
            
            b->frames = NUM_FRAMES;
            return true;
        }
        SetState(b, PnButtonState_Normal);
    }
    return false;
}

static bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnButton *b) {
    DASSERT(b);
    if(b->state == PnButtonState_Normal)
        SetState(b, PnButtonState_Hover);
    return true; // take focus
}

static bool leave(struct PnWidget *w, struct PnButton *b) {
    DASSERT(b);
    if(b->state == PnButtonState_Hover)
        SetState(b, PnButtonState_Normal);
    return true; // take focus
}

static
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
}

struct PnWidget *pnButton_create(struct PnSurface *parent,
        const char *label, bool toggle, size_t size) {

    ASSERT(!toggle, "WRITE MORE CODE");

    // TODO: How many more function parameters should we add to the
    // button create function?
    //
    // We may need to unhide some of these widget create parameters, but
    // we're hoping to auto-generate these parameters as this button
    // abstraction evolves.
    //
    if(size < sizeof(struct PnButton))
        size = sizeof(struct PnButton);
    //
    struct PnButton *b = (void *) pnWidget_create(parent,
            50/*width*/, 35/*height*/,
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
    pnWidget_setLeave(&b->widget, (void *) leave, b);
    pnWidget_setPress(&b->widget, (void *) press, b);
    pnWidget_setRelease(&b->widget, (void *) release, b);
    pnWidget_setConfig(&b->widget, (void *) config, b);
    pnWidget_setCairoDraw(&b->widget, (void *) cairoDraw, b);
    pnWidget_addDestroy(&b->widget, (void *) destroy, b);

    b->colors = calloc(1, PnButtonState_NumRegularStates*sizeof(*b->colors));
    ASSERT(b->colors, "calloc(1,%zu) failed",
            PnButtonState_NumRegularStates*sizeof(*b->colors));
    // Default state colors:
    b->colors[PnButtonState_Normal] =  0xFFCDCDCD;
    b->colors[PnButtonState_Hover] =   0xFFBEE2F3;
    b->colors[PnButtonState_Pressed] = 0xFFD06AC7;
    b->colors[PnButtonState_Active] =  0xFF0BD109;
    pnWidget_setBackgroundColor(&b->widget, b->colors[b->state]);


    return &b->widget;
}
