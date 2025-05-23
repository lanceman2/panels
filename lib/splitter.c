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


// Where on the slider (relative to the slider widget) did
// the button press happen.
static int32_t x_0 = INT32_MAX, y_0;


static inline void Splipper_preExpandH(const struct PnWidget *w,
        const struct PnAllocation *a,
        struct PnSplitter *s) {

    struct PnWidget *slider = s->slider;
    struct PnWidget *first = w->l.firstChild;
    struct PnWidget *last = w->l.lastChild;

    if(a->width <= s->slider->reqWidth) {
        first->culled = true;
        last->culled = true;
        slider->culled = false;
        slider->allocation.x = a->x;
        slider->allocation.width = a->width;
        return;
    }

    slider->culled = false;

    DASSERT(a->width > slider->reqWidth);

    if(s->firstHidden)
        goto firstHidden;

    if(s->lastHidden)
        goto lastHidden;

    DASSERT(first->reqWidth > 0);
    DASSERT(last->reqWidth > 0);
    DASSERT(first->reqWidth < -50);
    DASSERT(last->reqWidth < -50);


    if(first->reqWidth + slider->reqWidth +
            last->reqWidth > a->width) {
        // The 3 widgets are too large.
        uint32_t extra = first->reqWidth + slider->reqWidth +
            last->reqWidth - a->width;

        if(first->reqWidth >= last->reqWidth) {
            first->culled = false;
            if(first->reqWidth > extra) {
                last->culled = false;
                first->reqWidth -= extra;
                last->reqWidth = a->width -
                    first->reqWidth - slider->reqWidth;
                goto finish;
            } else {
                s->lastHidden = true;
                goto lastHidden;
            }
        } else {
            // last->reqWidth > first->reqWidth
            last->culled = false;
            if(last->reqWidth > extra) {
                first->culled = false;
                last->reqWidth -= extra;
                first->reqWidth = a->width -
                    last->reqWidth - slider->reqWidth;
                goto finish;
            } else {
                s->firstHidden = true;
                goto firstHidden;
            }
        }
    } else {
        // first->reqWidth + slider->reqWidth + last->reqWidth <= a->width
        uint32_t extra = a->width - first->reqWidth -
            slider->reqWidth - last->reqWidth;
        first->reqWidth += extra/2;
        last->reqWidth = a->width -
            first->reqWidth - slider->reqWidth;
        goto finish;
    }
 
firstHidden:
        DASSERT(s->firstHidden);
        DASSERT(!s->lastHidden);
        first->reqWidth = 1;
        first->culled = true;
        last->reqWidth = a->width - slider->reqWidth;
        goto finish;

lastHidden:
        DASSERT(s->lastHidden);
        DASSERT(!s->firstHidden);
        last->reqWidth = 1;
        last->culled = true;
        first->reqWidth = a->width - slider->reqWidth;

finish:
    DASSERT(first->reqWidth != 0);
    DASSERT(last->reqWidth != 0);

    // Now compute position and sizes in the widget allocation.
    uint32_t x = a->x;
    if(!first->culled) {
        first->allocation.x = x;
        first->allocation.width = first->reqWidth;
        x += first->allocation.width;
    }
    slider->allocation.x = x;
    slider->allocation.width = slider->reqWidth;
    x += slider->allocation.width;
    if(!last->culled) {
        last->allocation.x = x;
        last->allocation.width = last->reqWidth;
        x += last->allocation.width;
    }

    DASSERT(x == a->x + a->width,
            "x=%" PRIu32 " a->x=%" PRIu32 " a->width=%" PRIu32,
            x, a->x, a->width);
}

static inline void Splipper_preExpandV(const struct PnWidget *w,
        const struct PnAllocation *a,
        const struct PnSplitter *s) {


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


static bool press(struct PnWidget *slider,
            uint32_t which, int32_t x, int32_t y,
            struct PnSplitter *s) {
    DASSERT(s);
    DASSERT(slider);
    DASSERT(slider == s->slider);
    DASSERT(slider->parent);
    DASSERT(!slider->culled);

    if(which != BTN_LEFT) return false;

    DASSERT(x_0 == INT32_MAX);
    DASSERT(x != INT32_MAX);

    struct PnAllocation sa;
    pnWidget_getAllocation(s->slider, &sa);

    struct PnAllocation pa;
    pnWidget_getAllocation(slider->parent, &pa);


#ifdef DEBUG
    if(slider->parent->layout == PnLayout_LR) {
        DASSERT(pa.width > sa.width);
        DASSERT(pa.x <= sa.x && pa.x + pa.width >= sa.x + sa.width);
    } else {
        DASSERT(slider->parent->layout == PnLayout_TB);
        DASSERT(pa.height > sa.height);
        DASSERT(pa.y <= sa.y && pa.y + pa.height >= sa.y + sa.height);
    }
#endif

    // We cannot use the slider (and splitter) if the splitter widget
    // container surface is too small.  From above if only one of the
    // splitter child widgets is showing (because the splitter container
    // is small), that widget will be the slider.
    if(s->widget.layout == PnLayout_LR) {
        if(pa.width == sa.width)
            return false;
        DASSERT(pa.width > sa.width);
    } else {
        DASSERT(s->widget.layout == PnLayout_TB);
        if(pa.height == sa.height)
            return false;
        DASSERT(pa.height > sa.height);
    }

    x_0 = x - sa.x;
    y_0 = y - sa.y;

    // We got a pointer press; it must be in the slider:
    DASSERT(x_0 >= 0 && x_0 < sa.width);
    DASSERT(y_0 >= 0 && y_0 < sa.height);

    return true;
}

// For horizontal splitter.
//
// Returns true to change the positions.
//
static inline bool MoveSliderH(struct PnSplitter *s,
        struct PnWidget *slider, int32_t x) {

    DASSERT(slider->parent);
    DASSERT(s->widget.l.firstChild);
    DASSERT(s->widget.l.lastChild);

    int32_t x_to = x - x_0;

    if(x_to < 0)
        x_to = 0;

    struct PnAllocation sa;
    pnWidget_getAllocation(slider, &sa);

    struct PnAllocation pa;
    pnWidget_getAllocation(slider->parent, &pa);

    DASSERT(pa.width > sa.width);
    DASSERT(pa.x <= sa.x && pa.x + pa.width >= sa.x + sa.width);

    if(x_to < pa.x)
        x_to = pa.x;
    else if(x_to + sa.width > pa.x + pa.width)
        x_to = pa.x + pa.width - sa.width;

    if(x_to == sa.x)
        // We are already there.
        return false;

    //WARN("moving slider to x=%" PRIi32 " from x=%" PRIu32, x_to, sa.x);

    // Note: the parent allocation width is larger than the slider
    // allocation (by at least 1).
    //
    // Set x_to to the position of the slider.  The reqWidth of the slider
    // does not change.

    if(x_to > pa.x) {
        // We have room for the firstChild.
        s->widget.l.firstChild->reqWidth = x_to - pa.x;
        s->firstHidden = false;
    } else {
        // No room for firstChild.
        s->widget.l.firstChild->reqWidth = 1;
        s->firstHidden = true;
        // Moving the slider can bring it back.
    }

    if(x_to + slider->reqWidth < pa.x + pa.width) {
        // We have room for the lastChild.
        s->widget.l.lastChild->reqWidth =
            pa.x + pa.width - x_to - slider->reqWidth;
        s->lastHidden = false;
    } else {
        // No room for lastChild.
        s->widget.l.lastChild->reqWidth = 1;
        s->lastHidden = true;
        // Moving the slider can bring it back.
    }

    // We cannot hide both firstChild and lastChild.
    if(s->firstHidden && s->lastHidden)
        s->firstHidden = false;

    return true;
}

// For vertical splitter.
//
// Returns true to change the positions.
//
static inline bool MoveSliderV(struct PnSplitter *s,
        struct PnWidget *slider, int32_t y) {

    return true;
}


// This does the "action" of the splitter.
//
static inline bool MoveSlider(struct PnSplitter *s,
        struct PnWidget *slider, int32_t x, int32_t y) {
    DASSERT(slider);
    DASSERT(s);
    DASSERT(s->slider == slider);
    DASSERT(s->widget.layout == PnLayout_LR ||
            s->widget.layout == PnLayout_TB);

    if(s->widget.layout == PnLayout_LR)
        return MoveSliderH(s, slider, x);
    else
        return MoveSliderV(s, slider, y);
}

static bool motion(struct PnWidget *slider,
            int32_t x, int32_t y, struct PnSplitter *s) {

    DASSERT(slider);
    DASSERT(s);
    DASSERT(slider == s->slider);

    if(x_0 == INT32_MAX) return false;

    // We have the press too:

    //WARN("x,y=%" PRIi32 ",%" PRIi32, x, y);

    if(MoveSlider(s, slider, x, y))
        // TODO: Add config() callbacks some how.
        pnWidget_queueDraw(&s->widget, true/*allocate*/);

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
