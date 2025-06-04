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
#include "SetColor.h"


struct PnMenuItem {

    struct PnWidget widget; // inherit first
};


static bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnMenuItem *m) {
    DASSERT(m);
    DASSERT(m == (void *) w);
    DASSERT(w->type == PnSurfaceType_menuitem);
    DASSERT(GET_WIDGET_TYPE(w->type) == W_MENUITEM);

    fprintf(stderr, "\n    press(%p)[%" PRIi32 ",%" PRIi32 "]\n",
            w, x, y);

    return true; // true to grab
}

static bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnMenuItem *m) {
    DASSERT(m);
    DASSERT(m == (void *) w);
    DASSERT(w->type == PnSurfaceType_menuitem);
    DASSERT(GET_WIDGET_TYPE(w->type) == W_MENUITEM);

    fprintf(stderr, "\n  release(%p)[%" PRIi32 ",%" PRIi32 "]\n",
            w, x, y);

    pnWidget_callAction(w, PN_MENUITEM_CB_CLICK);

    return true;
}

static bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnMenuItem *m) {
    DASSERT(m);
    DASSERT(m == (void *) w);
    DASSERT(w->type == PnSurfaceType_menuitem);
    DASSERT(GET_WIDGET_TYPE(w->type) == W_MENUITEM);

    fprintf(stderr, "\n    enter(%p)[%" PRIi32 ",%" PRIi32 "]\n",
            w, x, y);

    return true; // take focus
}

static void leave(struct PnWidget *w, struct PnMenuItem *m) {
    DASSERT(m);
    DASSERT(m == (void *) w);
    DASSERT(w->type == PnSurfaceType_menuitem);
    DASSERT(GET_WIDGET_TYPE(w->type) == W_MENUITEM);

    fprintf(stderr, "\n    leave(%p)[]\n", w);
}


static bool clickAction(struct PnMenuItem *m, struct PnCallback *callback,
        // This is the user callback function prototype that we choose for
        // this "press" action.  The underlying API does not give a shit
        // what this (callback) void pointer is, but we do at this
        // point.
        void (*userCallback)(struct PnWidget *w, void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {

    DASSERT(m);
    DASSERT(actionData == 0);
    ASSERT(GET_WIDGET_TYPE(m->widget.type) == W_MENUITEM);
    DASSERT(actionIndex == PN_MENUITEM_CB_CLICK);
    DASSERT(callback);
    DASSERT(userCallback);

    userCallback(&m->widget, userData);

    // return true will eat the event and stop this function from going
    // through (calling) all connected callbacks.
    return false;
}


struct PnWidget *pnMenuItem_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand,
        size_t size) {

    if(size < sizeof(struct PnMenuItem))
        size = sizeof(struct PnMenuItem);
    //
    struct PnMenuItem *m = (void *) pnWidget_create(parent,
            width, height,
            layout, align, expand, size);
    if(!m)
        return 0; // Failure.

    DASSERT(m->widget.type == PnSurfaceType_widget);
    m->widget.type = PnSurfaceType_menuitem;
    DASSERT(m->widget.type & WIDGET);

    pnWidget_setEnter(&m->widget, (void *) enter, m);
    pnWidget_setLeave(&m->widget, (void *) leave, m);
    pnWidget_setPress(&m->widget, (void *) press, m);
    pnWidget_setRelease(&m->widget, (void *) release, m);

    pnWidget_addAction(&m->widget, PN_MENUITEM_CB_CLICK,
            (void *) clickAction, 0/*add()*/, 0/*actionData*/,
            0/*callbackSize*/);

    pnWidget_setBackgroundColor(&m->widget, 0xFFCD55CD);

    return &m->widget;
}
