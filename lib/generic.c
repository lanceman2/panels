// This Generic libpanels widget may be a good starter code example
// to use to make your first libpanels widget.  Just copy this file
// to another file name (.c source file) and add that file name to
// Makefile (and/or any other build system file that needs it).
// You can maybe remove unneeded widget callback functions and/or
// add some.
//
// TODO: We not sure that this belongs in the release build of
// libpanels.so.  Time will tell.
//
// It may also be that you can "inherit" this generic widget "class"
// instead of copying this file; it depends I suppose.  I guess that's not
// likely.  Yes, not likely.
//
// Search and replace the word "generic", "Generic", and "g" with your
// widget name.
//
// You may want to add a PnSurfaceType (enum and CPP macro shit) to
// display.h.  Just search for "generic" (case insensitive) in display.h
// (very few matches) and you may get a clue.

// This file is linked into libpanels.so if WITH_CAIRO is defined in
// ../config.make, otherwise it's not.  libpanels.so links with
// libcairo.so if WITH_CAIRO is defined when this is compiled.
//
// If you wish to add code to the libpanels.so API please remove any
// comments that no longer make sense for your new widget (like this
// one).
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


// Our "generic" widget data structure:
//
struct PnGeneric {

    struct PnWidget widget; // inherit first

    // The x and y position of the mouse pointer when we record it.
    int32_t x, y;

    // Add more data types here...
};


// All these are optional common widget callbacks:
// config(), cairoDraw(), press(), release(), enter(), leave(), motion(),
// and destroy().

static void config(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h,
            uint32_t stride/*4 byte chunks*/,
            struct PnGeneric *g) {
    DASSERT(g);
    DASSERT(g == (void *) widget);
    DASSERT(IS_TYPE1(widget->type, PnWidgetType_generic));


    DSPEW();
}

static int cairoDraw(struct PnWidget *w,
            cairo_t *cr, struct PnGeneric *g) {
    DASSERT(g);
    DASSERT(g == (void *) w);
    DASSERT(IS_TYPE1(w->type, PnWidgetType_generic));
    DASSERT(cr);

    // This color may have been set by the API user with
    // pnWidget_setBackgroundColor(w, color), if not and if this widget
    // was created with a parent widget, we get the default color here
    // which is the parent widgets color when this widget was created.
    //
    uint32_t color = pnWidget_getBackgroundColor(w);

    // A massive Cairo drawing routine.
    SetColor(cr, color);
    cairo_paint(cr);

    return 0;
}


static bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnGeneric *g) {
    DASSERT(g);
    DASSERT(g == (void *) w);
    DASSERT(IS_TYPE1(w->type, PnWidgetType_generic));

    fprintf(stderr, "\n    press(%p)[%" PRIi32 ",%" PRIi32 "]\n",
            w, x, y);

    g->x = x;
    g->y = y;

    pnWidget_callAction(w, PN_GENERIC_CB_PRESS);

    return true; // true to grab
}

static bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnGeneric *g) {
    DASSERT(g);
    DASSERT(g == (void *) w);
    DASSERT(IS_TYPE1(w->type, PnWidgetType_generic));

    fprintf(stderr, "\n  release(%p)[%" PRIi32 ",%" PRIi32 "]\n",
            w, x, y);

    pnWidget_callAction(w, PN_GENERIC_CB_RELEASE);

    return true;
}

static bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnGeneric *g) {
    DASSERT(g);
    DASSERT(g == (void *) w);
    DASSERT(IS_TYPE1(w->type, PnWidgetType_generic));

    fprintf(stderr, "\n    enter(%p)[%" PRIi32 ",%" PRIi32 "]\n",
            w, x, y);

    return true; // take focus
}

static void leave(struct PnWidget *w, struct PnGeneric *g) {
    DASSERT(g);
    DASSERT(g == (void *) w);
    DASSERT(IS_TYPE1(w->type, PnWidgetType_generic));

    fprintf(stderr, "\n    leave(%p)[]\n", w);
}

static bool motion(struct PnWidget *w,
            int32_t x, int32_t y, struct PnGeneric *g) {
    DASSERT(g);
    DASSERT(g == (void *) w);
    DASSERT(IS_TYPE1(w->type, PnWidgetType_generic));

    fprintf(stderr, "\r   motion(%p)[%" PRIi32 ",%" PRIi32 "]    ",
            w, x, y);

    return true;
}

static void destroy(struct PnWidget *w, struct PnGeneric *g) {
    DASSERT(g);
    DASSERT(g == (void *) w);
    DASSERT(IS_TYPE1(w->type, PnWidgetType_generic));

    DSPEW();
}


// pressAction() and releaseAction() are two different widget type
// specific marshalling functions that setup a user callback interface API
// that a user can use with a given PnGeneric (type) widget.
//
//   pnWidget_addCallback(widget, index, userCallback, userdata)
//
// where index is PN_GENERIC_CB_PRESS and PN_GENERIC_CB_RELEASE
// respectively.  See test example ../tests/generic.c.
//
// The userCallback() user function prototype can be arbitrarily chosen to
// be whatever the widget creator chooses it to be.  Here we picked
//  void (*userCallback)(struct PnWidget *w, int32_t x, int32_t y)
//    and
//  bool (*userCallback)(struct PnGeneric *g, void *userData)
//    for
//  pressAction and releaseAction respectively.
//
static bool pressAction(struct PnGeneric *g, struct PnCallback *callback,
        // This is the user callback function prototype that we choose for
        // this "press" action.  The underlying API does not give a shit
        // what this (callback) void pointer is, but we do at this
        // point.
        void (*userCallback)(struct PnWidget *w, int32_t x,
            int32_t y, void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {

    DASSERT(g);
    DASSERT(actionData == 0);
    ASSERT(IS_TYPE1(g->widget.type, PnWidgetType_generic));
    DASSERT(actionIndex == PN_GENERIC_CB_PRESS);
    DASSERT(callback);
    DASSERT(userCallback);

    // userCallback() is the API user set callback.
    userCallback(&g->widget, g->x, g->y, userData);

    // return true will eat the event and stop this function from going
    // through (calling) all connected callbacks.
    return false;
}
//
static bool releaseAction(struct PnGeneric *g, struct PnCallback *callback,
        bool (*userCallback)(struct PnGeneric *g, void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {

    DASSERT(g);
    DASSERT(actionData == 0);
    ASSERT(IS_TYPE1(g->widget.type, PnWidgetType_generic));
    DASSERT(actionIndex == PN_GENERIC_CB_RELEASE);
    DASSERT(callback);
    DASSERT(userCallback);

    // userCallback() is the API user set callback.
    bool ret = userCallback(g, userData);

    // return true will eat the event and stop this function from going
    // through (calling) all connected callbacks.
    return  ret;
}


// This is the only required function.  The function prototype obviously may vary
// depending on your widget.
//
struct PnWidget *pnGeneric_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand) {

    struct PnGeneric *g = (void *) pnWidget_create(parent,
            width, height,
            layout, align, expand, sizeof(*g));
    if(!g)
        // A common error mode is that the parent cannot have children.
        // pnWidget_create() should spew for us.
        return 0; // Failure.

    // Setting the widget surface type.  We decrease the data, but
    // increase the complexity.  See enum PnSurfaceType in display.h.
    // It's so easy to forget about all these bits, but DASSERT() is my
    // hero.
    DASSERT(g->widget.type == PnWidgetType_widget);
    g->widget.type = PnWidgetType_generic;
    DASSERT(IS_TYPE1(g->widget.type, PnWidgetType_generic));

    // Add common widget callback functions:
    //
    pnWidget_setEnter(&g->widget, (void *) enter, g);
    pnWidget_setLeave(&g->widget, (void *) leave, g);
    pnWidget_setMotion(&g->widget, (void *) motion, g);
    pnWidget_setPress(&g->widget, (void *) press, g);
    pnWidget_setRelease(&g->widget, (void *) release, g);
    pnWidget_setConfig(&g->widget, (void *) config, g);
    pnWidget_setCairoDraw(&g->widget, (void *) cairoDraw, g);
    pnWidget_addDestroy(&g->widget, (void *) destroy, g);

    // Add PnGeneric specific user callback interfaces for use with
    // pnWidget_addCallback().  We call them action callbacks.
    //
    pnWidget_addAction(&g->widget, PN_GENERIC_CB_PRESS,
            (void *) pressAction, 0/*add()*/, 0/*actionData*/,
            0/*callbackSize*/);
    pnWidget_addAction(&g->widget, PN_GENERIC_CB_RELEASE,
            (void *) releaseAction, 0/*add()*/, 0/*actionData*/,
            0/*callbackSize*/);


    pnWidget_setBackgroundColor(&g->widget, 0xFFCDCDCD);

    return &g->widget;
}
