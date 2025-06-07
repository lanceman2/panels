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

// CAUTION: we use GoTo in this function, but just to break from if/else
// blocks and go down this function.
//
static inline void Splipper_preExpandH(const struct PnWidget *w,
        const struct PnAllocation *a,
        struct PnSplitter *s) {

    DASSERT(a->width);
    DASSERT(a->height);
 
    struct PnWidget *slider = s->slider;
    struct PnWidget *first = w->l.firstChild;
    struct PnWidget *last = w->l.lastChild;

    if(a->width <= s->slider->reqWidth) {
        slider->culled = false;
        // We use the two flags s->firstHidden and s->lastHidden to mark
        // which one is hidden, so that it can stay hidden until the user
        // slides the slider to show it.
        if(s->firstHidden)
            goto firstHidden;
        if(s->lastHidden)
            goto lastHidden;
        // Else, we arbitrarily pick the "last" widget to be hidden.
        s->lastHidden = true;
        goto lastHidden;
    }

    // The slider will be shown.
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
    } else if(first->reqWidth + slider->reqWidth +
            last->reqWidth <= a->width) {

        // first->reqWidth + slider->reqWidth + last->reqWidth <= a->width
        uint32_t extra = a->width - first->reqWidth -
            slider->reqWidth - last->reqWidth;
        first->reqWidth += extra/2;
        last->reqWidth = a->width -
            first->reqWidth - slider->reqWidth;
        goto finish;
    } else {
        // first->reqWidth + slider->reqWidth + last->reqWidth == a->width
        goto finish;
    }

// We mark only one of the two widgets (first and last) as hidden; the
// other non-hidden one may get culled too, but will be shown on a window
// resize event that makes the splitter widget container larger.
//
// The flags: s->firstHidden and s->lastHidden, give the splitter widget
// dynamical hysteresis.  It sucks without them.

firstHidden:
        DASSERT(s->firstHidden);
        DASSERT(!s->lastHidden);
        first->reqWidth = 0;
        first->culled = true;
        if(a->width > slider->reqWidth) {
            last->reqWidth = a->width - slider->reqWidth;
            last->culled = false;
        } else {
            last->reqWidth = 0;
            last->culled = true;
        }
        goto finish;

lastHidden:
        DASSERT(s->lastHidden);
        DASSERT(!s->firstHidden);
        last->reqWidth = 0;
        last->culled = true;
        if(a->width > slider->reqWidth) {
            first->reqWidth = a->width - slider->reqWidth;
            first->culled = false;
        } else {
            first->reqWidth = 0;
            first->culled = true;
        }

finish:
    //DASSERT(first->reqWidth != 0);
    //DASSERT(last->reqWidth != 0);

    // Now compute position and sizes in the widget allocation.
    uint32_t x = a->x;
    if(!first->culled) {
        first->allocation.x = x;
        first->allocation.width = first->reqWidth;
        x += first->allocation.width;
    }
    if(!slider->culled) {
        slider->allocation.x = x;
        if(first->culled && last->culled)
            slider->allocation.width = a->width;
        else
            slider->allocation.width = slider->reqWidth;
        x += slider->allocation.width;
    }
    if(!last->culled) {
        last->allocation.x = x;
        last->allocation.width = last->reqWidth;
#ifdef DEBUG
        x += last->allocation.width;
#endif
    }

    DASSERT(x == a->x + a->width);
}

// This is, line for line, the same as Splipper_preExpandV() but with some
// variable name substitutions.
//
static inline void Splipper_preExpandV(const struct PnWidget *w,
        const struct PnAllocation *a,
        struct PnSplitter *s) {

    DASSERT(a->width);
    DASSERT(a->height);
 
    struct PnWidget *slider = s->slider;
    struct PnWidget *first = w->l.firstChild;
    struct PnWidget *last = w->l.lastChild;

    if(a->height <= s->slider->reqHeight) {
        slider->culled = false;
        // We use the two flags s->firstHidden and s->lastHidden to mark
        // which one is hidden, so that it stay hidden until the user
        // slides the slider to show it.
        if(s->firstHidden)
            goto firstHidden;
        if(s->lastHidden)
            goto lastHidden;
        s->lastHidden = true;
        goto lastHidden;
    }

    // The slider will be shown.
    slider->culled = false;

    DASSERT(a->height > slider->reqHeight);

    if(s->firstHidden)
        goto firstHidden;

    if(s->lastHidden)
        goto lastHidden;

    DASSERT(first->reqHeight > 0);
    DASSERT(last->reqHeight > 0);
    DASSERT(first->reqHeight < -50);
    DASSERT(last->reqHeight < -50);

    if(first->reqHeight + slider->reqHeight +
            last->reqHeight > a->height) {
        // The 3 widgets are too large.
        uint32_t extra = first->reqHeight + slider->reqHeight +
            last->reqHeight - a->height;

        if(first->reqHeight >= last->reqHeight) {
            first->culled = false;
            if(first->reqHeight > extra) {
                last->culled = false;
                first->reqHeight -= extra;
                last->reqHeight = a->height -
                    first->reqHeight - slider->reqHeight;
                goto finish;
            } else {
                s->lastHidden = true;
                goto lastHidden;
            }
        } else {
            // last->reqHeight > first->reqHeight
            last->culled = false;
            if(last->reqHeight > extra) {
                first->culled = false;
                last->reqHeight -= extra;
                first->reqHeight = a->height -
                    last->reqHeight - slider->reqHeight;
                goto finish;
            } else {
                s->firstHidden = true;
                goto firstHidden;
            }
        }
    } else if(first->reqHeight + slider->reqHeight +
            last->reqHeight < a->height) {

        uint32_t extra = a->height - first->reqHeight -
            slider->reqHeight - last->reqHeight;
        first->reqHeight += extra/2;
        last->reqHeight = a->height -
            first->reqHeight - slider->reqHeight;
        goto finish;
    } else {
        // first->reqHeight + slider->reqHeight + last->reqHeight == a->height
        goto finish;
    }


firstHidden:
    DASSERT(s->firstHidden);
    DASSERT(!s->lastHidden);
    first->reqHeight = 0;
    first->culled = true;
    if(a->height > slider->reqHeight) {
        last->reqHeight = a->height - slider->reqHeight;
        last->culled = false;
    } else {
        last->reqHeight = 0;
        last->culled = true;
    }
    goto finish;


lastHidden:
    DASSERT(s->lastHidden);
    DASSERT(!s->firstHidden);
    last->reqHeight = 0;
    last->culled = true;
    if(a->height > slider->reqHeight) {
        first->reqHeight = a->height - slider->reqHeight;
        first->culled = false;
    } else {
        first->reqHeight = 0;
        first->culled = true;
    }

finish:
    //DASSERT(first->reqHeight != 0);
    //DASSERT(last->reqHeight != 0);

    // Now compute position and sizes in the widget allocation.
    uint32_t y = a->y;
    if(!first->culled) {
        first->allocation.y = y;
        first->allocation.height = first->reqHeight;
        y += first->allocation.height;
    }
    if(!slider->culled) {
        slider->allocation.y = y;
        if(first->culled && last->culled)
            slider->allocation.height = a->height;
        else
            slider->allocation.height = slider->reqHeight;
        y += slider->allocation.height;
    }
    if(!last->culled) {
        last->allocation.y = y;
        last->allocation.height = last->reqHeight;
#ifdef DEBUG
        y += last->allocation.height;
#endif
    }

    DASSERT(y == a->y + a->height);
}

void Splipper_preExpand(const struct PnWidget *w,
        const struct PnAllocation *a) {

    DASSERT(w);
    DASSERT(IS_TYPE(w->type, W_SPLITTER));
    DASSERT(w->layout == PnLayout_LR ||
            w->layout == PnLayout_TB);
    struct PnSplitter *s = (void *) w;

    if(w->layout == PnLayout_LR)
        Splipper_preExpandH(w, a, s);
    else
        Splipper_preExpandV(w, a, s);
}

#define ACTION_BUTTON  (BTN_LEFT)

static bool press(struct PnWidget *slider,
            uint32_t which, int32_t x, int32_t y,
            struct PnSplitter *s) {
    DASSERT(s);
    DASSERT(slider);
    DASSERT(slider == s->slider);
    DASSERT(slider->parent);
    DASSERT(!slider->culled);
    DASSERT(slider->parent->layout == PnLayout_LR ||
            slider->parent->layout == PnLayout_TB);

    if(which != ACTION_BUTTON) return false;

    DASSERT(x_0 == INT32_MAX);
    DASSERT(x < INT32_MAX - 50);
    DASSERT(y < INT32_MAX - 50);

    struct PnAllocation sa; // slider allocation
    pnWidget_getAllocation(s->slider, &sa);
    DASSERT(sa.x >= 0);
    DASSERT(sa.y >= 0);

    struct PnAllocation pa; // parent allocation
    pnWidget_getAllocation(slider->parent, &pa);
    DASSERT(pa.x >= 0);
    DASSERT(pa.y >= 0);

    if(slider->parent->layout == PnLayout_LR) {
        if(x < pa.x || x >= pa.x + pa.width ||
                pa.width == sa.width /*parent too small*/)
            return false;
        DASSERT(pa.width > sa.width);
    } else { // slider->parent->layout == PnLayout_TB
        if(y < pa.y || y >= pa.y + pa.height ||
                pa.height == sa.height /*parent too small*/)
            return false;
        DASSERT(pa.height > sa.height);
    }

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

    // We have a press for this slider.
    x_0 = x - sa.x;
    y_0 = y - sa.y;

    // We got a pointer press; it must be in the slider:
    DASSERT(x_0 >= 0 && x_0 < sa.width);
    DASSERT(y_0 >= 0 && y_0 < sa.height);

    return true;
}

// For horizontal splitter.
//
// Returns false to change the positions.
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

    DASSERT(pa.width >= sa.width);
    DASSERT(pa.x <= sa.x && pa.x + pa.width >= sa.x + sa.width);

    if(sa.x == pa.x && slider->reqWidth >= pa.width) {
        // There is not enough room to move the slider.
        x_0 = INT32_MAX;
        return true;
    }

    // Constrain the motion.
    if(x_to < pa.x)
        x_to = pa.x;
    else if(x_to + sa.width > pa.x + pa.width)
        x_to = pa.x + pa.width - sa.width;

    if(x_to == sa.x)
        // We are already there.
        return true;

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
        s->widget.l.firstChild->reqWidth = 0;
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
        s->widget.l.lastChild->reqWidth = 0;
        s->lastHidden = true;
        // Moving the slider can bring it back.
    }

    // We cannot hide both firstChild and lastChild.
    if(s->firstHidden && s->lastHidden)
        s->firstHidden = false;

    return false;
}

// For vertical splitter.
//
// Returns false to change the positions.
//
static inline bool MoveSliderV(struct PnSplitter *s,
        struct PnWidget *slider, int32_t y) {

    DASSERT(slider->parent);
    DASSERT(s->widget.l.firstChild);
    DASSERT(s->widget.l.lastChild);

    int32_t y_to = y - y_0;

    if(y_to < 0)
        y_to = 0;

    struct PnAllocation sa;
    pnWidget_getAllocation(slider, &sa);

    struct PnAllocation pa;
    pnWidget_getAllocation(slider->parent, &pa);

    DASSERT(pa.height >= sa.height);
    DASSERT(pa.y <= sa.y && pa.y + pa.height >= sa.y + sa.height);

    if(sa.y == pa.y && slider->reqHeight >= pa.height) {
        // There is not enough room to move the slider.
        x_0 = INT32_MAX;
        return true;
    }

    // Constrain the motion.
    if(y_to < pa.y)
        y_to = pa.y;
    else if(y_to + sa.height > pa.y + pa.height)
        y_to = pa.y + pa.height - sa.height;

    if(y_to == sa.y)
        // We are already there.
        return true;

    //WARN("moving slider to y=%" PRIi32 " from y=%" PRIu32, y_to, sa.y);

    // Note: the parent allocation height is larger than the slider
    // allocation (by at least 1).
    //
    // Set y_to to the position of the slider.  The reqHeight of the slider
    // does not change.

    if(y_to > pa.y) {
        // We have room for the firstChild.
        s->widget.l.firstChild->reqHeight = y_to - pa.y;
        s->firstHidden = false;
    } else {
        // No room for firstChild.
        s->widget.l.firstChild->reqHeight = 0;
        s->firstHidden = true;
        // Moving the slider can bring it back.
    }

    if(y_to + slider->reqHeight < pa.y + pa.height) {
        // We have room for the lastChild.
        s->widget.l.lastChild->reqHeight =
            pa.y + pa.height - y_to - slider->reqHeight;
        s->lastHidden = false;
    } else {
        // No room for lastChild.
        s->widget.l.lastChild->reqHeight = 0;
        s->lastHidden = true;
        // Moving the slider can bring it back.
    }

    // We cannot hide both firstChild and lastChild.
    if(s->firstHidden && s->lastHidden)
        s->firstHidden = false;

    return false;
}

static bool motion(struct PnWidget *slider,
            int32_t x, int32_t y, struct PnSplitter *s) {

    DASSERT(slider);
    DASSERT(s);
    DASSERT(slider == s->slider);
    DASSERT(s->widget.layout == PnLayout_LR ||
            s->widget.layout == PnLayout_TB);

    if(x_0 == INT32_MAX)
        // The correct mouse button was not pressed now or
        // there is not enough room to move the mouse.  Either
        // way we can't change anything.
        return false;

    // We have the press:

    //WARN("x,y=%" PRIi32 ",%" PRIi32, x, y);

    if(s->widget.layout == PnLayout_LR) {
        if(MoveSliderH(s, slider, x))
            return true; // no change
    } else if(MoveSliderV(s, slider, y))
        return true; // no change

    // We have slider motion, so re-allocate and redraw the widgets
    // involved.
    pnWidget_queueDraw(&s->widget, true/*allocate*/);

    return true;
}

static bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnSplitter *s) {
    DASSERT(s);
    DASSERT(w == s->slider);

    if(x_0 == INT32_MAX || which != ACTION_BUTTON)
        return false;

    // This is the flag state for being not pressed or the parent widget
    // is too small for the slider to move.
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
    DASSERT(IS_TYPE(s->widget.type, W_SPLITTER));
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
