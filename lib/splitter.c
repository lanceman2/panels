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


#define H_CURSOR_NAME  "e-resize"
#define V_CURSOR_NAME  "n-resize"


struct PnSplitter {
    struct PnWidget widget; // inherit first

    char *cursorName;

    bool direction; // true ==> horizontal, not true ==> vertical

    bool cursorSet;
};


static bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnSplitter *s) {
    return false;
}

static bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnSplitter *s) {
    DASSERT(s == (void *) w);
    DASSERT(s);

    return false;
}

static bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnSplitter *s) {
    DASSERT(s);
    s->cursorSet = pnWindow_pushCursor(s->cursorName)?true:false;
    return true; // take focus
}

static void leave(struct PnWidget *w, struct PnSplitter *s) {
    DASSERT(s);
    if(s->cursorSet)
        pnWindow_popCursor();
}

static
void destroy(struct PnWidget *w, struct PnSplitter *s) {

    DASSERT(s);
    DASSERT(s == (void *) w);
}

struct PnWidget *pnSplitter_create(struct PnWidget *parent,
        uint32_t width, uint32_t height, size_t size) {

    ASSERT((width && !height) || (height && !width),
            "height=0 for a horizontal splitter "
            "and width=0 for a vertical splitter");
    if(width) {
        ASSERT(parent &&
            (parent->layout == PnLayout_LR ||
             parent->layout == PnLayout_RL),
            "the horizontal splitter must be "
            "a child of horizontal container");
    } else {
        ASSERT(parent &&
            (parent->layout == PnLayout_TB ||
             parent->layout == PnLayout_BT),
            "the vertical splitter must be "
            "a child of vertical container");
    }

    enum PnExpand expand = PnExpand_V;
    if(height) expand = PnExpand_H;

    if(!width) width = 1;
    if(!height) height = 1;

    if(size < sizeof(struct PnSplitter))
        size = sizeof(struct PnSplitter);
    //
    struct PnSplitter *s = (void *) pnWidget_create(parent,
            width, height,
            PnLayout_None, PnAlign_CC, expand, size);
    if(!s)
        // A common error mode is that the parent cannot have children.
        // pnWidget_create() should spew for us.
        return 0; // Failure.

    if(expand == PnExpand_V) {
        s->cursorName = H_CURSOR_NAME;
    } else {
        DASSERT(expand == PnExpand_H);
        s->cursorName = V_CURSOR_NAME;
    }

    // Setting the widget surface type.  We decrease the data, but
    // increase the complexity.  See enum PnSurfaceType in display.h.
    // It's so easy to forget about all these bits, but DASSERT() is my
    // hero.
    DASSERT(s->widget.type == PnSurfaceType_widget);
    s->widget.type = PnSurfaceType_splitter;
    DASSERT(s->widget.type & WIDGET);

    // Add widget callback functions:
    pnWidget_setEnter(&s->widget, (void *) enter, s);
    pnWidget_setLeave(&s->widget, (void *) leave, s);
    pnWidget_setPress(&s->widget, (void *) press, s);
    pnWidget_setRelease(&s->widget, (void *) release, s);
    pnWidget_addDestroy(&s->widget, (void *) destroy, s);

    pnWidget_setBackgroundColor(&s->widget, 0xFFFF0000);

    return &s->widget;
}
