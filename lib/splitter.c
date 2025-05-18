// This file is linked into libpanels.so if WITH_CAIRO is defined
// in ../config.make, otherwise it's not.
//
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cairo/cairo.h>
#include <linux/input-event-codes.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"
#include "allocation.h"
#include "splitter.h"



static int32_t x_0 = INT32_MAX, y_0;


static bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnSplitter *s) {
    DASSERT(s);
    DASSERT(w == s->slider);
    if(which != BTN_LEFT) return false;

    DASSERT(x_0 == INT32_MAX);
    DASSERT(x != INT32_MAX);

    x_0 = x;
    y_0 = y;

    return true;
}

static bool motion(struct PnWidget *w,
            int32_t x, int32_t y, struct PnSplitter *s) {

    DASSERT(s);
    DASSERT(w == s->slider);

    if(x_0 == INT32_MAX) return false;

    WARN("x,y=%" PRIu32 ",%" PRIu32, x, y);

    return true;
}

static bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnSplitter *s) {
    DASSERT(s);
    DASSERT(w == s->slider);
    if(which != BTN_LEFT) return false;

    DASSERT(x_0 != INT32_MAX);

    x_0 = INT32_MAX;

    return true;
}

static bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnSplitter *s) {
    DASSERT(s);
    DASSERT(w == s->slider);
    s->cursorSet = pnWindow_pushCursor(s->cursorName)?true:false;
    return true; // take focus
}

static void leave(struct PnWidget *w, struct PnSplitter *s) {
    DASSERT(s);
    DASSERT(w == s->slider);
    if(s->cursorSet)
        pnWindow_popCursor();
}

static
void destroy(struct PnWidget *w, struct PnSplitter *s) {

    DASSERT(s);
    DASSERT(s == (void *) w);
    DASSERT(s->slider);

    pnWidget_destroy(s->slider);
    s->slider = 0;
}

#define DEFAULT_LEN  (17)

struct PnWidget *pnSplitter_create(struct PnWidget *parent,
        struct PnWidget *first, struct PnWidget *second,
        bool isHorizontal /*or it's vertical*/,
        size_t size) {

    // For now, let these be in existence.
    // TODO: re-parenting.
    DASSERT(parent);
    DASSERT(first);
    DASSERT(second);

    // Disable for now.
    if(true) {
        ERROR("REMOVE THIS BLOCK OF CODE");
        return 0;
    }

    if(size < sizeof(struct PnSplitter))
        size = sizeof(struct PnSplitter);

    enum PnExpand expand;
    enum PnLayout layout;

    if(isHorizontal) {
        layout = PnLayout_HSplitter;
        expand = PnExpand_V;
    } else {
        layout = PnLayout_VSplitter;
        expand = PnExpand_H;
    }

    struct PnWidget *slider = pnWidget_create(0/*parent*/,
            DEFAULT_LEN/*width*/, DEFAULT_LEN/*height*/,
            PnLayout_None, PnAlign_CC, expand, 0/*size*/);
    if(!slider)
        // pnWidget_create() will have spewed.
        return 0; // Failure.

    struct PnSplitter *s = (void *) pnWidget_create(parent,
            0/*width*/, 0/*height*/,
            layout, PnAlign_CC, 0/*expand*/, size);
    if(!s) {
        // pnWidget_create() will have spewed.
        pnWidget_destroy(slider);
        return 0; // Failure.
    }

    DASSERT(GET_WIDGET_TYPE(s->widget.type) == W_SPLITTER);

    s->widget.l.firstChild = slider;
    s->widget.l.lastChild = slider;
    s->slider = slider;
    s->firstSize = 0.5;

    DASSERT(!slider->pl.nextSibling);
    DASSERT(!slider->pl.prevSibling);

    // Child widgets are always added in this order:
    //
    //   slider   first   second
    //

    if(first) {
        // TODO: re-parenting.
        DASSERT(!first->parent);
        DASSERT(!first->pl.prevSibling);
        DASSERT(!first->pl.nextSibling);

        first->parent = &s->widget;
        slider->pl.nextSibling = first;
        first->pl.prevSibling = slider;
        s->widget.l.lastChild = first;
    }
    if(second) {
        // TODO: re-parenting.
        DASSERT(!second->parent);
        DASSERT(!second->pl.nextSibling);
        DASSERT(!second->pl.prevSibling);

        second->parent = &s->widget;
        s->widget.l.lastChild = second;
        if(first)
            first->pl.nextSibling = second;
        else
            slider->pl.nextSibling = second;
    }

    if(isHorizontal)
        s->cursorName = "e-resize";
    else
        s->cursorName = "n-resize";

    // Add widget callback functions:
    pnWidget_setEnter(slider, (void *) enter, s);
    pnWidget_setLeave(slider, (void *) leave, s);
    pnWidget_setPress(slider, (void *) press, s);
    pnWidget_setRelease(slider, (void *) release, s);
    pnWidget_setMotion(slider, (void *) motion, s);

    pnWidget_addDestroy(&s->widget, (void *) destroy, s);

    pnWidget_setBackgroundColor(&s->widget, 0xFF999999);
    pnWidget_setBackgroundColor(slider,     0xFFFF0000);

    return &s->widget;
}
