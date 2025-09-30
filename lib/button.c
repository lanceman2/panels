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
#include "check.h"


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


enum PnButtonState {

    // Regular Buttons have just 4 states.
    PnButtonState_Normal   = NORMAL,
    PnButtonState_Hover    = HOVER,
    PnButtonState_Pressed  = PRESSED,
    PnButtonState_Active   = ACTIVE,
    PnButtonState_NumStates,
    // Toggle Buttons have double the number of states
    // of Regular Buttons.
    PnToggledState_Normal  = NORMAL | (4),
    PnToggledState_Hover   = HOVER | (4),
    PnToggledState_Pressed = PRESSED | (4),
    PnToggledState_Active  = ACTIVE | (4),
    PnToggle_NumStates
};

// See state diagram (run in shell): % display buttonState.dot
// That's for both the button and the toggle button.
//

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

    // A singly linked list of checks that show the toggle state to the
    // user.
    struct PnCheck *checks;

    bool toggled; // used if it's a toggle button.
};


static inline void FlipToggle(struct PnToggleButton *t) {

    t->toggled = (t->toggled)?false:true;
    for(struct PnCheck *c = t->checks; c; c = c->next)
        pnCheck_set(&c->widget, t->toggled);
}

static inline void
SetState(struct PnButton *b, enum PnButtonState state) {
    DASSERT(b);

    DASSERT(state != b->state);
    ASSERT(IS_TYPE1(b->widget.type, PnWidgetType_button));

    // We have a state change.

    if(IS_TYPE2(b->widget.type, PnWidgetType_togglebutton) &&
            state == PnButtonState_Active)
        FlipToggle((void *) b);

    if(b->frames) b->frames = 0;

    b->state = state;
    pnWidget_queueDraw(&b->widget, false/*allocate*/);
}

static inline void DrawToggle(cairo_t *cr, struct PnToggleButton *t) {

    DASSERT(IS_TYPE1(t->button.widget.type, PnWidgetType_button));
    DASSERT(IS_TYPE2(t->button.widget.type, PnWidgetType_togglebutton));

    // Switch on 8 state values:
    switch(t->button.state | ((t->toggled)?4:0)) {

        case PnToggledState_Normal:
        case PnToggledState_Hover:
        case PnButtonState_Pressed:
        case PnToggledState_Active:
            DrawHalo2(cr, MIN_WIDTH, MIN_HEIGHT, 0xFF000000);

        case PnButtonState_Normal:
        case PnButtonState_Hover:
        case PnToggledState_Pressed:
        case PnButtonState_Active:
            DrawHalo(cr, MIN_WIDTH, MIN_HEIGHT,
                    t->button.colors[t->button.state]);
    }
}

static inline void Draw(cairo_t *cr, struct PnButton *b) {

    SetColor(cr, b->widget.backgroundColor);
    cairo_paint(cr);

    if(IS_TYPE2(b->widget.type, PnWidgetType_togglebutton))
        // It's a toggle button.
        DrawToggle(cr, (void *) b);
    else {
        // It's a regular button.
        DASSERT(IS_TYPE1(b->widget.type, PnWidgetType_button));
        DrawHalo(cr, MIN_WIDTH, MIN_HEIGHT, b->colors[b->state]);
    }
}

static void config(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h,
            uint32_t stride/*4 byte chunks*/,
            struct PnButton *b) {

    DASSERT(b);
    DASSERT(b == (void *) widget);
    if(b->frames) b->frames = 0;
    if(b->state != PnButtonState_Normal)
        SetState(b, PnButtonState_Normal);
    DASSERT(w);
    DASSERT(h);
    b->width = w;
    b->height = h;
}


#define ASSERT_BUTTON(w)  \
    ASSERT(IS_TYPE1(w->type, PnWidgetType_button))
#define DASSERT_BUTTON(w)  \
    DASSERT(IS_TYPE1(w->type, PnWidgetType_button))


static int cairoDraw(struct PnWidget *w,
            cairo_t *cr, struct PnButton *b) {
    DASSERT(b);
    DASSERT(b == (void *) w);
    DASSERT_BUTTON(w);
    DASSERT(cr);
    DASSERT(b->state < PnButtonState_NumStates);

    if(b->state != PnButtonState_Active) {
        Draw(cr, b);
        if(b->frames)
            b->frames = 0;
        return 0;
    }

    //b->state == PnButtonState_Active

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
    pnWidget_callAction(w, PN_BUTTON_CB_ENTER);
    return true; // take focus
}

static void leave(struct PnWidget *w, struct PnButton *b) {
    DASSERT(b);
    if(b->state == PnButtonState_Hover) {
        SetState(b, PnButtonState_Normal);
        pnWidget_callAction(w, PN_BUTTON_CB_LEAVE);
    }
    b->entered = false;
}

static
void destroy(struct PnWidget *w, struct PnButton *b) {

    DASSERT(b);
    DASSERT(b == (void *) w);

    DASSERT(b->colors);
    DZMEM(b->colors, PnButtonState_NumStates*sizeof(*b->colors));
    free(b->colors);
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
    ASSERT_BUTTON(b);
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
    ASSERT_BUTTON(b);
    DASSERT(actionIndex == PN_BUTTON_CB_CLICK);
    DASSERT(callback);
    DASSERT(userCallback);

    return userCallback(b, userData);
}

static bool enterAction(struct PnWidget *w, struct PnCallback *callback,
        bool (*userCallback)(struct PnWidget *b, uint32_t x, uint32_t y,
            void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {

    DASSERT(w);
    DASSERT(actionData == 0);
    ASSERT_BUTTON(w);
    DASSERT(actionIndex == PN_BUTTON_CB_ENTER);
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

static bool leaveAction(struct PnWidget *w, struct PnCallback *callback,
        bool (*userCallback)(struct PnWidget *b, void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {

    DASSERT(w);
    DASSERT(actionData == 0);
    ASSERT_BUTTON(w);
    DASSERT(actionIndex == PN_BUTTON_CB_LEAVE);
    DASSERT(callback);
    DASSERT(userCallback);

    // callback() is the libpanels API user set callback.
    //
    // We let the user return the value.  true will eat the event and stop
    // this function from going through (calling) all connected
    // callbacks.
    return userCallback(w, userData);
}

bool pnToggleButton_getToggled(struct PnWidget *w) {

    DASSERT(w);
    ASSERT(IS_TYPE1(w->type, PnWidgetType_button));
    ASSERT(IS_TYPE2(w->type, PnWidgetType_togglebutton));

    return ((struct PnToggleButton *) w)->toggled;
}

static inline void AddToggleCheck(struct PnToggleButton *t,
        struct PnCheck *check) {

    DASSERT(t);
    DASSERT(check);
    DASSERT(!check->next);

    if(t->checks) {
        struct PnCheck *last = t->checks;
        for(; last->next; last = last->next);
        last->next = check;
    } else
        t->checks = check;

    check->toggleButton = t;
}


void RemoveToggleCheck(struct PnCheck *check) {

    DASSERT(check);
    struct PnToggleButton *t = check->toggleButton;
    if(!t) {
        DASSERT(!check->next);
        return;
    }
    struct PnCheck *prev = 0;
    for(struct PnCheck *c = t->checks;
            c; c = c->next) {
        if(check == c) {
            if(prev)
                prev->next = check->next;
            else
                t->checks = check->next;
            check->toggleButton = 0;
            check->next = 0;
            break;
        }
        prev = c;
    }
}


void pnToggleButton_addCheck(struct PnWidget *w,
        struct PnWidget *check) {

    DASSERT(w);
    DASSERT(check);
    ASSERT(IS_TYPE1(check->type, PnWidgetType_check));
    ASSERT(IS_TYPE2(w->type, PnWidgetType_togglebutton));

    struct PnCheck *c = (void *) check;
    struct PnToggleButton *t = (void *) w;

    if(c->toggleButton)
        RemoveToggleCheck(c);

    pnCheck_set(check, t->toggled);
    AddToggleCheck(t, (void *) check);
}

void pnToggleButton_removeCheck(struct PnWidget *w,
        struct PnWidget *check) {

    DASSERT(w);
    DASSERT(check);
    ASSERT(IS_TYPE2(w->type, PnWidgetType_togglebutton));
    ASSERT(IS_TYPE1(check->type, PnWidgetType_check));

    RemoveToggleCheck((void *) check);
}


static void ToggleDestroy(struct PnWidget *w,
        struct PnToggleButton *t) {
    DASSERT(w);
    DASSERT(w = (void *) t);
    DASSERT(IS_TYPE2(w->type, PnWidgetType_togglebutton));

    while(t->checks)
        RemoveToggleCheck(t->checks);
}


struct PnWidget *pnToggleButton_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand,
        const char *label, bool toggled, size_t size) {

    struct PnToggleButton *t;

    if(size < sizeof(*t))
        size = sizeof(*t);

    t = (void *) pnButton_create(parent,
        width, height, layout, align, expand, label, size);

    if(!t) return 0; // fail.

    DASSERT(t->button.widget.type == PnWidgetType_button);
    // We have a different widget type that shares code with the button
    // widget, so we put it in the same source file: here.
    //
    // TODO: Will this idea hold up over time?  I'm not sure.
    t->button.widget.type = PnWidgetType_togglebutton;

    pnWidget_addDestroy(&t->button.widget, (void *) ToggleDestroy, t);

    // Initialize toggle:
    t->toggled = toggled;

    return &t->button.widget;
}


struct PnWidget *pnButton_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand,
        const char *label, size_t size) {

    if(width < MIN_WIDTH)
        width = MIN_WIDTH;
    if(height < MIN_HEIGHT)
        height = MIN_HEIGHT;

    struct PnButton *b;

    if(size < sizeof(*b))
        size = sizeof(*b);

    // TODO: How many more function parameters should we add to the
    // button create function?
    //
    // We may need to unhide some of these widget create parameters, but
    // we're hoping to auto-generate these parameters as this button
    // abstraction evolves.
    //
    b = (void *) pnWidget_create(parent,
            width, height,
            layout, align, expand, size);
    if(!b)
        // A common error mode is that the parent cannot have children.
        // pnWidget_create() should spew for us.
        return 0; // Failure.


    // Setting the widget surface type.  We decrease the data, but
    // increase the complexity.  See enum PnWidgetType in display.h.
    // It's so easy to forget about all these bits, but DASSERT() is my
    // hero.
    DASSERT(b->widget.type == PnWidgetType_widget);
    b->widget.type = PnWidgetType_button;

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
    pnWidget_addAction(&b->widget, PN_BUTTON_CB_ENTER,
            (void *) enterAction, 0/*add()*/, 0/*actionData*/,
            0/*callbackSize*/);
    pnWidget_addAction(&b->widget, PN_BUTTON_CB_LEAVE,
            (void *) leaveAction, 0/*add()*/, 0/*actionData*/,
            0/*callbackSize*/);


    b->colors = calloc(1, PnButtonState_NumStates*
            sizeof(*b->colors));
    ASSERT(b->colors, "calloc(1,%zu) failed",
            PnButtonState_NumStates*sizeof(*b->colors));
    // Default state colors:
    //b->colors[PnButtonState_Normal] =  0xFFCDCDCD;
    b->colors[PnButtonState_Normal] = b->widget.backgroundColor;
    b->colors[PnButtonState_Hover] =   0xFF00EDFF;
    b->colors[PnButtonState_Pressed] = 0xFFD06AC7;
    b->colors[PnButtonState_Active] =  0xFF0BD109;
    pnWidget_setBackgroundColor(&b->widget, b->colors[b->state], 0);

    if(label) {
        struct PnWidget *l = (void *) pnLabel_create(
            &b->widget/*parent*/,
            0/*width*/, 20/*height*/,
            4/*xPadding*/, 4/*yPadding*/,
            0/*align*/, expand/*expand*/, label);
        ASSERT(l);
        pnLabel_setFontColor(l, 0xF0FF0000);
    }

    return &b->widget;
}
