// This file is linked into libpanels.so if WITH_CAIRO is defined
// in ../config.make, otherwise it's not.
//
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <linux/input-event-codes.h>
#include <cairo/cairo.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"
#include "cairoWidget.h"


#define MIN_WIDTH   (9)
#define MIN_HEIGHT  (9)
#define LW          (2) // Line Width
#define R           (6) // Radius
#define PAD         (2)

static inline void
SetState(struct PnButton *b, enum PnButtonState state) {
    DASSERT(b);
    if(state == b->state) return;

    if(b->frames) b->frames = 0;

    b->state = state;
    pnWidget_queueDraw(&b->widget, false/*allocate*/);
}

static inline void DrawBorder(cairo_t *cr) {

    cairo_surface_t *crs = cairo_get_target(cr);
    int w = cairo_image_surface_get_width(crs);
    int h = cairo_image_surface_get_height(crs);
    if(w < MIN_WIDTH)
        w = MIN_WIDTH;
    if(h < MIN_HEIGHT)
        h = MIN_HEIGHT;
    // If w or h are small this may try to draw outside of the Cairo
    // drawing area, but Cairo can handle culling out extra pixels that
    // are outside the Cairo drawing area.

    cairo_set_line_width(cr, LW);
    w -= 2*(R+PAD);
    h -= 2*(R+PAD);
    double x = PAD + R + w, y = PAD;

    cairo_move_to(cr, x, y);
    cairo_arc(cr, x, y += R, R, -0.5*M_PI, 0);
    cairo_line_to(cr, x += R, y += h);
    cairo_arc(cr, x -= R, y, R, 0, 0.5*M_PI);
    cairo_line_to(cr, x -= w, y += R);
    cairo_arc(cr, x, y -= R, R, 0.5*M_PI, M_PI);
    cairo_line_to(cr, x -= R, y -= h);
    cairo_arc(cr, x += R, y, R, M_PI, 1.5*M_PI);
    cairo_close_path(cr);
}


static inline void Draw(cairo_t *cr, struct PnButton *b) {

    uint32_t color;

    color = b->widget.backgroundColor;
    cairo_set_source_rgba(cr,
            PN_R_DOUBLE(color), PN_G_DOUBLE(color),
            PN_B_DOUBLE(color), PN_A_DOUBLE(color));
    cairo_paint(cr);

    color = b->colors[b->state];
    cairo_set_source_rgba(cr,
            PN_R_DOUBLE(color), PN_G_DOUBLE(color),
            PN_B_DOUBLE(color), PN_A_DOUBLE(color));
    DrawBorder(cr);
    cairo_stroke(cr);
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
    DASSERT(w->type == PnSurfaceType_button);
    DASSERT(b == (void *) w);
    DASSERT(cr);
    DASSERT(b->state < PnButtonState_NumRegularStates);

    if(b->state != PnButtonState_Active) {
        Draw(cr, b);
        if(b->frames)
            b->frames = 0;
        return 0;
    }

    DASSERT(b->frames);

    if(b->frames == NUM_FRAMES)
        // First time drawing the active pixels for this button
        // pres/release cycle.
        //
        // We draw one active frame.  If we change pixel content we need
        // to draw more frames.  For now it's just the one set of "active"
        // pixels that is fixed.
        Draw(cr, b);

    --b->frames;

    if(!b->frames) {
        if(b->entered)
            SetState(b, PnButtonState_Hover);
        else
            SetState(b, PnButtonState_Normal);
        Draw(cr, b);
        return 0; // Done counting frames.
    }

    return 1;
}

#define CLICK_BUTTON  (BTN_LEFT)

static bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnButton *b) {
    if(which == CLICK_BUTTON) {
        SetState(b, PnButtonState_Pressed);
        pnWidget_callAction(w, PN_BUTTON_CB_PRESS);
        return true;
    }
    return false;
}

static bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnButton *b) {
    DASSERT(b == (void *) w);
    DASSERT(b);

    if(which == CLICK_BUTTON) {
        if(b->state == PnButtonState_Pressed &&
                pnWidget_isInSurface(w, x, y)) {
            SetState(b, PnButtonState_Active);
            pnWidget_callAction(w, PN_BUTTON_CB_CLICK);
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
    b->entered = true;
    return true; // take focus
}

static void leave(struct PnWidget *w, struct PnButton *b) {
    DASSERT(b);
    if(b->state == PnButtonState_Hover)
        SetState(b, PnButtonState_Normal);
    b->entered = false;
}

static
void destroy(struct PnWidget *w, struct PnButton *b) {

    DASSERT(b);
    DASSERT(b == (void *) w);

    if(w->type & W_BUTTON) {
        DASSERT(b->colors);
        DZMEM(b->colors,
                PnButtonState_NumRegularStates*sizeof(*b->colors));
        free(b->colors);
    } else {
        DASSERT(w->type & W_TOGGLE_BUTTON);
        ASSERT(0, "WRITE MORE CODE");
    }
}


// Press and no release yet.
//
static bool pressAction(struct PnWidget *b, struct PnCallback *callback,
        // This is the user callback function prototype that we choose for
        // this "press" action.  The underlying API does not give a shit
        // what this (callback) void pointer is, but we do at this
        // point.
        bool (*userCallback)(struct PnWidget *b, void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {

    DASSERT(b);
    DASSERT(actionData == 0);
    DASSERT(actionIndex == PN_BUTTON_CB_PRESS);
    ASSERT(GET_WIDGET_TYPE(b->type) == W_BUTTON);
    DASSERT(callback);
    DASSERT(userCallback);

    // callback() is the API user set callback.
    //
    // We let the user return the value.  true will eat the event and stop
    // this function from going through (calling) all connected
    // callbacks.
    return userCallback(b, userData);
}
// button "click" marshalling function gets the button API users callback
// to call.  Gets called indirectly from
// pnWidget_callAction(widget, PN_BUTTON_CB_CLICK).
//
// This is the most complex and dangerous thing in the libpanels widget
// API. And, it's not very complex. Passing a function to another function
// so it can call that function.  If the function prototypes do not line
// up at run-time, we're fucked.  This lacks the type safety stuff that is
// in GTK and Qt callbacks.  Ya, no fucking shit, apples and oranges.  I
// can't write a fucking book here to give all the needed context to make
// this a good comparison.  Unsafe but much faster than Qt.  I have not
// been able to unwind (in my head) the gobject CPP Macro madness in GTK,
// but this is butt head simple compared to connecting widget "signals"
// in GTK.  Is it faster than GTK?  I don't know, or have, a need to
// measure it.  I expect they are both fast enough.
//
// GTK and Qt are both: (1) bloated and (2) leak system resources into
// your process.  Both (1 and 2) are unacceptable for some applications.
// Bloated and leaky tend to go together.
//
static bool clickAction(struct PnWidget *b, struct PnCallback *callback,
        // This is the user callback function prototype that we choose for
        // this "click", PN_BUTTON_CB_CLICK, action.  The underlying API
        // does not give a shit what this (callback) void pointer is, but
        // we do at this point.
        bool (*userCallback)(struct PnWidget *b, void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {

    DASSERT(b);
    DASSERT(actionData == 0);
    ASSERT(GET_WIDGET_TYPE(b->type) == W_BUTTON);
    DASSERT(actionIndex == PN_BUTTON_CB_CLICK);
    DASSERT(callback);
    DASSERT(userCallback);

    // callback() is the libpanels API user set callback.
    //
    // We let the user return the value.  true will eat the event and stop
    // this function from going through (calling) all connected
    // callbacks.
    return userCallback(b, userData);
}


struct PnWidget *pnButton_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand,
        const char *label, bool toggle, size_t size) {

    ASSERT(!toggle, "WRITE MORE CODE");

    if(width < MIN_WIDTH)
        width = MIN_WIDTH;
    if(height < MIN_HEIGHT)
        height = MIN_HEIGHT;

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
            width, height,
            layout, align, expand, size);
    if(!b)
        // A common error mode is that the parent cannot have children.
        // pnWidget_create() should spew for us.
        return 0; // Failure.

    // Setting the widget surface type.  We decrease the data, but
    // increase the complexity.  See enum PnSurfaceType in display.h.
    // It's so easy to forget about all these bits, but DASSERT() is my
    // hero.
    DASSERT(b->widget.type == PnSurfaceType_widget);
    b->widget.type = PnSurfaceType_button;
    DASSERT(b->widget.type & WIDGET);

    // Add widget callback functions:
    pnWidget_setEnter(&b->widget, (void *) enter, b);
    pnWidget_setLeave(&b->widget, (void *) leave, b);
    pnWidget_setPress(&b->widget, (void *) press, b);
    pnWidget_setRelease(&b->widget, (void *) release, b);
    pnWidget_setConfig(&b->widget, (void *) config, b);
    pnWidget_setCairoDraw(&b->widget, (void *) cairoDraw, b);
    pnWidget_addDestroy(&b->widget, (void *) destroy, b);
    pnWidget_addAction(&b->widget, PN_BUTTON_CB_CLICK,
            (void *) clickAction, 0/*add()*/, 0/*actionData*/,
            0/*callbackSize*/);
    pnWidget_addAction(&b->widget, PN_BUTTON_CB_PRESS,
            (void *) pressAction, 0/*add()*/, 0/*actionData*/,
            0/*callbackSize*/);


    b->colors = calloc(1, PnButtonState_NumRegularStates*
            sizeof(*b->colors));
    ASSERT(b->colors, "calloc(1,%zu) failed",
            PnButtonState_NumRegularStates*sizeof(*b->colors));
    // Default state colors:
    //b->colors[PnButtonState_Normal] =  0xFFCDCDCD;
    b->colors[PnButtonState_Normal] = b->widget.backgroundColor;
    b->colors[PnButtonState_Hover] =   0xFF00EDFF;
    b->colors[PnButtonState_Pressed] = 0xFFD06AC7;
    b->colors[PnButtonState_Active] =  0xFF0BD109;
    pnWidget_setBackgroundColor(&b->widget, b->colors[b->state]);

    return &b->widget;
}
