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


static inline void Splipper_preExpandH(const struct PnWidget *w,
        const struct PnAllocation *a,
        struct PnSplitter *s) {

    if(a->width <= s->slider->reqWidth) {
        w->l.firstChild->culled = true;
        w->l.lastChild->culled = true;
        s->slider->culled = false;
        s->slider->allocation.x = a->x;
        s->slider->allocation.width = a->width;
        return;
    }

    DASSERT(a->width > s->slider->reqWidth);

    if(w->l.firstChild->reqWidth >= w->l.lastChild->reqWidth) {

        if(w->l.firstChild->reqWidth + s->slider->reqWidth >= a->width) {
            w->l.lastChild->culled = true;
            w->l.lastChild->reqWidth = 1;
            w->l.firstChild->reqWidth = a->width - s->slider->reqWidth;
        } else {
            w->l.lastChild->culled = false;
            w->l.lastChild->reqWidth = a->width
                - w->l.firstChild->reqWidth - s->slider->reqWidth;
        }

        w->l.firstChild->culled = false;
        s->slider->culled = false;
        w->l.firstChild->allocation.x = a->x;
        w->l.firstChild->allocation.width = w->l.firstChild->reqWidth;
        s->slider->allocation.x = a->x + w->l.firstChild->reqWidth;
        s->slider->allocation.width = s->slider->reqWidth;
        w->l.lastChild->allocation.x = s->slider->allocation.x +
            s->slider->allocation.width;
        w->l.lastChild->allocation.width = w->l.lastChild->reqWidth;
        return;
    }
    {
        if(w->l.lastChild->reqWidth + s->slider->reqWidth >= a->width) {
            w->l.firstChild->culled = true;
            w->l.firstChild->reqWidth = 1;
            w->l.lastChild->reqWidth = a->width - s->slider->reqWidth;
            w->l.firstChild->allocation.x = a->x;
            w->l.firstChild->allocation.width = 0;
        } else {
            w->l.firstChild->culled = false;
            w->l.firstChild->reqWidth = a->width
                - w->l.lastChild->reqWidth - s->slider->reqWidth;
            w->l.firstChild->allocation.x = a->x;
            w->l.firstChild->allocation.width = w->l.firstChild->reqWidth;
        }

        w->l.lastChild->culled = false;
        s->slider->culled = false;
        s->slider->allocation.x = w->l.firstChild->allocation.x +
            w->l.firstChild->allocation.width;
        s->slider->allocation.width = s->slider->reqWidth;
        w->l.lastChild->allocation.x = s->slider->allocation.x +
            s->slider->reqWidth;
        w->l.lastChild->allocation.width = w->l.lastChild->reqWidth;
    }
}

static inline void Splipper_preExpandV(const struct PnWidget *w,
        const struct PnAllocation *a,
        const struct PnSplitter *s) {

    if(a->height <= s->slider->reqHeight) {
        w->l.firstChild->culled = true;
        w->l.lastChild->culled = true;
        s->slider->culled = false;
        s->slider->allocation.y = a->y;
        s->slider->allocation.height = a->height;
        return;
    }

    DASSERT(a->height > s->slider->reqHeight);

    if(w->l.firstChild->reqHeight >= w->l.lastChild->reqHeight) {

        if(w->l.firstChild->reqHeight + s->slider->reqHeight >= a->height) {
            w->l.lastChild->culled = true;
            w->l.lastChild->reqHeight = 1;
            w->l.firstChild->reqHeight = a->height - s->slider->reqHeight;
        } else {
            w->l.lastChild->culled = false;
            w->l.lastChild->reqHeight = a->height
                - w->l.firstChild->reqHeight - s->slider->reqHeight;
        }

        w->l.firstChild->culled = false;
        s->slider->culled = false;
        w->l.firstChild->allocation.y = a->y;
        w->l.firstChild->allocation.height = w->l.firstChild->reqHeight;
        s->slider->allocation.y = a->y + w->l.firstChild->reqHeight;
        s->slider->allocation.height = s->slider->reqHeight;
        w->l.lastChild->allocation.y = s->slider->allocation.y +
            s->slider->allocation.height;
        w->l.lastChild->allocation.height = w->l.lastChild->reqHeight;
        return;
    }
    {
        if(w->l.lastChild->reqHeight + s->slider->reqHeight >= a->height) {
            w->l.firstChild->culled = true;
            w->l.firstChild->reqHeight = 1;
            w->l.lastChild->reqHeight = a->height - s->slider->reqHeight;
            w->l.firstChild->allocation.y = a->y;
            w->l.firstChild->allocation.height = 0;
        } else {
            w->l.firstChild->culled = false;
            w->l.firstChild->reqHeight = a->height
                - w->l.lastChild->reqHeight - s->slider->reqHeight;
            w->l.firstChild->allocation.y = a->y;
            w->l.firstChild->allocation.height = w->l.firstChild->reqHeight;
        }

        w->l.lastChild->culled = false;
        s->slider->culled = false;
        s->slider->allocation.y = w->l.firstChild->allocation.y +
            w->l.firstChild->allocation.height;
        s->slider->allocation.height = s->slider->reqHeight;
        w->l.lastChild->allocation.y = s->slider->allocation.y +
            s->slider->reqHeight;
        w->l.lastChild->allocation.height = w->l.lastChild->reqHeight;
    }

}

void Splipper_preExpand(const struct PnWidget *w,
        const struct PnAllocation *a) {

    DASSERT(GET_WIDGET_TYPE(w->type) == W_SPLITTER);
    DASSERT(w->layout == PnLayout_LR ||
            w->layout == PnLayout_TB);
    struct PnSplitter *s = (void *) w;

    if(w->layout == PnLayout_LR)
        Splipper_preExpandH(w, a, s);
    else
        Splipper_preExpandV(w, a, s);
}


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

static inline bool MoveSlider(struct PnSplitter *s,
        struct PnWidget *slider) {
    DASSERT(slider);
    DASSERT(s);
    DASSERT(s->slider == slider);


    return true; // need allocation and redraw
}

static bool motion(struct PnWidget *slider,
            int32_t x, int32_t y, struct PnSplitter *s) {

    DASSERT(slider);
    DASSERT(s);
    DASSERT(slider == s->slider);

    if(x_0 == INT32_MAX) return false;

    WARN("x,y=%" PRIi32 ",%" PRIi32, x, y);

    if(MoveSlider(s, slider)) {
        pnWidget_getAllocations(&s->widget);
        pnWidget_queueDraw(&s->widget);
    }

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
