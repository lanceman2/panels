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
#include "SetColor.h"
#include "DrawHalo.h"


// button States
//
// Note: if it's not a toggle button it only has 4 usable states.  The
// toggle button adds another dimension, so it has 2 x 4 = 8 states.  At
// what point is a button a selector that has N * 4 states?  We know that
// you can say that button has 2 (remove the active state and just call it
// a transition, and remove the hover state), 3, (remove the active state
// and just call it a transition) or 5 (add an additional active state)
// states, but we had to start somewhere.
//
// The different between a toggle button and a check button:  In the
// implementation/code there is no difference except in the visuals or
// action animation.  In use the check button tends to change some future
// state that is (implied or explicit) to happen at a later event that
// will poll the state of the check button. Were as, a toggle button tends
// to make changes immediately when the toggle button is toggled.  Check
// button states tend to be batch processed in groups.  Toggle buttons
// event tend to cause immediate actions.
//
// There are also buttons that are in between toggle and check, like a
// toggle button with a LED (light emitting diode) light on it.  The LED
// light looks somewhat like a check, but it's just an indicator of
// whether the button is de-pressed or not.  The power button on an
// electronic device is a good example of a toggle, but if the LED light
// is changed just a little it can look like a check on top of a larger
// toggle.  There are two dynamical elements to this button, the LED
// light (or check) and the larger toggle button that it sits on.
//
#define NORMAL     (0)
#define HOVER      (1)
#define PRESSED    (2)
#define ACTIVE     (3) // animation, released
// TOGGLED is higher bits than the above.
#define TOGGLED    (1 << 2) // 4

// This must be true.  We cannot share bits between the first 4 state
// flags and TOGGLED.  In code we say:
//ASSERT( !( (NORMAL|HOVER|PRESSED|ACTIVE) & TOGGLED))

enum PnButtonState {

    PnButtonState_Normal   = NORMAL,
    PnButtonState_Hover    = HOVER,
    PnButtonState_Pressed  = PRESSED,
    PnButtonState_Active   = ACTIVE,
    PnButtonState_NumRegularStates,
    PnButtonState_ToggledNormal  = NORMAL  | TOGGLED,
    PnButtonState_ToggledHover   = HOVER   | TOGGLED,
    PnButtonState_ToggledPressed = PRESSED | TOGGLED,
    PnButtonState_ToggledActive  = ACTIVE  | TOGGLED,
    PnButtonState_NumToggleStates
};


// Number of drawing cycles for a button "click" animation.
//
// TODO: Instead of using the drawing cycle period we could set an
// interval timer to trigger that the animation is done.  That may use
// much less system resources, especially for the case that the animation
// does not change the pixels drawn every draw cycle.
//
#define NUM_FRAMES 11

struct PnButton {

    struct PnWidget widget; // inherit first

    enum PnButtonState state;
    uint32_t *colors;

    uint32_t width, height;

    uint32_t x, y;

    uint32_t frames; // an animation frame counter

    bool entered; // is it focused from mouse enter?
};


struct PnToggleButton {

    struct PnButton button; // inherit first
};


static inline void
SetState(struct PnButton *b, enum PnButtonState state) {
    DASSERT(b);
    if(state == b->state) return;

    if(b->frames) b->frames = 0;

    b->state = state;
    pnWidget_queueDraw(&b->widget, false/*allocate*/);
}


static inline void Draw(cairo_t *cr, struct PnButton *b) {

    SetColor(cr, b->widget.backgroundColor);
    cairo_paint(cr);

    DrawHalo(cr, MIN_WIDTH, MIN_HEIGHT, b->colors[b->state]);
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
    b->x = x;
    b->y = y;
    pnWidget_callAction(w, PN_BUTTON_CB_HOVER);
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
        // what this (userCallback) void pointer is, but we do at this
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
// in GTK.  Is it faster than GTK?  I don't know, or have a need to
// measure it.  I expect they are both fast enough.
//
// GTK and Qt are both: (1) bloated and (2) leak system resources into
// your process.  Both (1 and 2) are unacceptable for some applications.
// In the world of software, bloated and leaky tend to go together.  When
// things get bloated (large) one can lose track of all the stuff that it
// is, and so it leaks the stuff you lose track of.  In both Qt and GTK
// they didn't bother keeping "track" of their main loop class singleton
// object (and many other like things); I'd guess that they let that go
// and then piled more shit on it, and so, who knows how big that leaky
// (system resource) pile is.  I call it being lazy to an extreme.  Lazy
// cleanup can be good some times, but they have the extreme case of no
// cleanup at all.  That works great until it does not.
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

    return userCallback(b, userData);
}

static bool hoverAction(struct PnWidget *w, struct PnCallback *callback,
        bool (*userCallback)(struct PnWidget *b, uint32_t x, uint32_t y,
            void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {

    DASSERT(w);
    DASSERT(actionData == 0);
    ASSERT(GET_WIDGET_TYPE(w->type) == W_BUTTON);
    DASSERT(actionIndex == PN_BUTTON_CB_HOVER);
    DASSERT(callback);
    DASSERT(userCallback);

    struct PnButton *b = (void *) w;

    // callback() is the libpanels API user set callback.
    //
    // We let the user return the value.  true will eat the event and stop
    // this function from going through (calling) all connected
    // callbacks.
    return userCallback(w, b->x, b->y, userData);
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
    pnWidget_addAction(&b->widget, PN_BUTTON_CB_HOVER,
            (void *) hoverAction, 0/*add()*/, 0/*actionData*/,
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
