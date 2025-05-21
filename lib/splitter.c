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

    //WARN("x,y=%" PRIu32 ",%" PRIu32, x, y);

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
INFO();
    return true;
}

static bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnSplitter *s) {
    DASSERT(s);
    DASSERT(w == s->slider);

    s->cursorSet = pnWindow_pushCursor(w, s->cursorName)?true:false;
    return true; // take focus
}

static void leave(struct PnWidget *w, struct PnSplitter *s) {
    DASSERT(s);
    DASSERT(w == s->slider);

    if(s->cursorSet)
        pnWindow_popCursor(w);
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

    if(size < sizeof(struct PnSplitter))
        size = sizeof(struct PnSplitter);

    enum PnExpand expand;
    enum PnLayout layout;

    if(isHorizontal) {
        layout = PnLayout_LR;
        expand = PnExpand_V;
    } else {
        layout = PnLayout_TB;
        expand = PnExpand_H;
    }

    struct PnSplitter *s = (void *) pnWidget_create(parent,
            0/*width*/, 0/*height*/,
            layout, PnAlign_CC, 0/*expand*/, size);
    if(!s)
        // pnWidget_create() will have spewed.
        return 0; // Failure.

    struct PnWidget *slider = pnWidget_create(0/*parent*/,
            DEFAULT_LEN/*width*/, DEFAULT_LEN/*height*/,
            PnLayout_None, PnAlign_CC, expand, 0/*size*/);
    if(!slider) {
        // pnWidget_create() will have spewed.
        pnWidget_destroy(&s->widget);
        return 0; // Failure.
    }

    // Child widgets (when added) are always added in this order:
    //
    //   first  slider  second
    //

    if(first) {
        // TODO: re-parenting.
        DASSERT(first->parent == &d.widget);
        if(isHorizontal)
            first->expand |= PnExpand_H;
        else
            first->expand |= PnExpand_V;
        pnWidget_addChild(&s->widget, first);
    }

    pnWidget_addChild(&s->widget, slider);

    if(second) {
        // TODO: re-parenting.
        DASSERT(second->parent == &d.widget);
        if(isHorizontal)
            second->expand |= PnExpand_H;
        else
            second->expand |= PnExpand_V;
        pnWidget_addChild(&s->widget, second);
    }

    // Setting the widget surface type.
    DASSERT(s->widget.type == PnSurfaceType_widget);
    s->widget.type = PnSurfaceType_splitter;
    DASSERT(s->widget.type & WIDGET);
    DASSERT(GET_WIDGET_TYPE(s->widget.type) == W_SPLITTER);
    // Note: the widget type is "splitter" and the widget layout
    // type is PnLayout_LR or PnLayout_TB.

    s->slider = slider;
    s->firstSize = 0.5;

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

    pnWidget_setBackgroundColor(&s->widget, 0xFF999999);
    pnWidget_setBackgroundColor(slider,     0xFFFF0000);

    return &s->widget;
}
