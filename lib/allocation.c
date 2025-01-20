#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include  "display.h"


// We use lots of recursion to get widget positions and sizes.


static inline uint32_t GetWidth(const struct PnSurface *s) {

    if(s->firstChild || s->width)
        return s->width;
    else
        return PN_DEFAULT_WIDGET_WIDTH;
}

static inline uint32_t GetHeight(const struct PnSurface *s) {

    if(s->firstChild || s->height)
        return s->height;
    else
        return PN_DEFAULT_WIDGET_HEIGHT;
}


// This function calls itself.  I don't think we'll blow the function call
// stack here.  How many layers of widgets do we have?
//
// The not showing widget has it's culling flag set true here, if not it's
// set to false.
//
static
void AddRequestedSizes(struct PnSurface *s) {

    DASSERT(!s->culled);

    // We start at 0 and sum from there.
    //
    s->allocation.width = 0;
    s->allocation.height = 0;
    s->allocation.x = 0;
    s->allocation.y = 0;

    bool gotOne = false;

    switch(s->direction) {
        case PnDirection_None:
            DASSERT(!s->firstChild);
            break;
        case PnDirection_One:
            DASSERT((!s->firstChild && !s->lastChild)
                    || s->firstChild == s->lastChild);
        case PnDirection_LR:
        case PnDirection_RL:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling) {
                c->culled = false;
                if(c->hiding) {
                    c->culled = true;
                    continue;
                }
                if(!gotOne) gotOne = true;
                AddRequestedSizes(c);
                // Now we have all "s" children and descendent sizes.
                s->allocation.width += c->allocation.width + GetWidth(s);
                if(s->allocation.height < c->allocation.height)
                    s->allocation.height = c->allocation.height;
            }
            if(gotOne)
                s->allocation.height += GetHeight(s);
            break;
        case PnDirection_BT:
        case PnDirection_TB:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling) {
                c->culled = false;
                if(c->hiding) {
                    c->culled = true;
                    continue;
                }
                if(!gotOne) gotOne = true;
                AddRequestedSizes(c);
                // Now we have all "s" children and descendent sizes.
                s->allocation.height += c->allocation.height +
                    GetHeight(s);
                if(s->allocation.width < c->allocation.width)
                    s->allocation.width = c->allocation.width;
            }
            if(gotOne)
                s->allocation.width += GetWidth(s);
            break;
        default:
    }

    // Add the last border or first size if no children.
    s->allocation.width += GetWidth(s);
    s->allocation.height += GetHeight(s);
}

struct PnSurface *Next(const struct PnSurface *s) {
    return s->nextSibling;
}

struct PnSurface *Prev(const struct PnSurface *s) {
    return s->prevSibling;
}


// This function needs to recompute all children widget X positions and
// widths.
//
void AllocateChildrenX(const struct PnSurface *s, const struct PnAllocation *a,
        struct PnSurface *firstChild, struct PnSurface *lastChild,
        struct PnSurface *(*next)(const struct PnSurface *c)) {


    // first get the total width needed, thn.
    uint32_t thn = GetWidth(s)/*border*/;
    uint32_t numExpand = 0;// to count the number of widgets that can
                           // expand.

    for(struct PnSurface *c = firstChild; c; c = next(c)) {
        thn += c->allocation.width + GetWidth(s)/*border*/;
        if(c->expand & PnExpand_H)
            ++numExpand;
    }

    uint32_t add = 0;
    uint32_t extra = 0;
    if(a->width > thn) {
        if(numExpand) {
            add = (a->width - thn)/numExpand;
            extra = a->width - thn - add * numExpand;
        }
    } else
        // There is no extra space to expand to.  There could be culling.
        numExpand = 0;


    uint32_t x = a->x + GetWidth(s)/*border*/;
    uint32_t xMax = a->x + a->width - GetWidth(s);

    for(struct PnSurface *c = firstChild; c; c = next(c)) {
        if(x > xMax) {
            // The rest of the children are culled.
            c->culled = true;
            continue;
        }
        c->allocation.x = x;
        if(numExpand && c->expand & PnExpand_H) {
            c->allocation.width += add;
            --numExpand;
            if(!numExpand && extra)
                // last one with expand set get the extra
                c->allocation.width += extra;
        }
        x += c->allocation.width + GetWidth(s);
    }

#ifdef DEBUG
    if(a->width >= thn)
        // We did not cull anything so:
        DASSERT(a->x + a->width == x,
                "a->x(%" PRIu32 ") + a->width(%" PRIu32
                ") != x(%" PRIu32 ") "
                "thn=%" PRIu32, a->x, a->width, x, thn);
#endif
}


// This function needs to recompute all children widget Y positions and
// heights.
//
void AllocateChildrenY(const struct PnSurface *s, const struct PnAllocation *a,
        struct PnSurface *firstChild, struct PnSurface *lastChild,
        struct PnSurface *(*next)(const struct PnSurface *c)) {

    // first get the total height needed, thn.
    uint32_t thn = GetHeight(s)/*border*/;
    uint32_t numExpand = 0;// to count the number of widgets that can
                           // expand.

    for(struct PnSurface *c = firstChild; c; c = next(c)) {
        thn += c->allocation.height + GetHeight(s)/*border*/;
        if(c->expand & PnExpand_V)
            ++numExpand;
    }

    uint32_t add = 0;
    uint32_t extra = 0;
    if(a->height > thn) {
        if(numExpand) {
            add = (a->height - thn)/numExpand;
            extra = a->height - thn - add * numExpand;
        }
    } else
        // There is no extra space to expand to.  There could be culling.
        numExpand = 0;

    uint32_t y = a->y + GetHeight(s)/*border*/;
    uint32_t yMax = a->y + a->height - GetHeight(s);

    for(struct PnSurface *c = firstChild; c; c = next(c)) {
        if(y + c->allocation.height > yMax) {
            // The rest of the children are culled.
            c->culled = true;
            continue;
        }
        c->allocation.y = y;
        if(numExpand && c->expand & PnExpand_V) {
            c->allocation.height += add;
            --numExpand;
            if(!numExpand && extra)
                // last one with expand set get the extra
                c->allocation.height += extra;
        }
        y += c->allocation.height + GetHeight(s);
    }

#ifdef DEBUG
    if(a->height >= thn)
        // We did not cull anything so:
        DASSERT(a->y + a->height == y,
                "a->y(%" PRIu32 ") + a->height(%" PRIu32
                ") != y(%" PRIu32 ") "
                "thn=%" PRIu32, a->y, a->height, y, thn);
#endif
}

// Get the x and width, or y and height of widgets.
//
static
void AllocateChildren(const struct PnSurface *s, const struct PnAllocation *a) {

    if(s->culled) return;

    uint32_t d;

    switch(s->direction) {

        case PnDirection_One:
        case PnDirection_TB:
            AllocateChildrenY(s, a, s->firstChild, s->lastChild, Next);
            break;

        case PnDirection_BT:
            // With children in reverse order.
            AllocateChildrenY(s, a, s->lastChild, s->firstChild, Prev);
            break;

        case PnDirection_LR:
        case PnDirection_RL:
            d = a->height - 2 * GetHeight(s);
            // The widgets are lad along the X direction at the top so we
            // can extend them down.
            for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling) {
                if(d < c->allocation.height) {
                    c->culled = true;
                    continue;
                }
                c->allocation.y = a->y + GetHeight(s);
                if((c->expand & PnExpand_V) && (c->allocation.height < d))
                    c->allocation.height = d;
            }
            break;

        case PnDirection_None:
        default:
            ASSERT(0);
            break;
    }

    switch(s->direction) {

        case PnDirection_One:
        case PnDirection_LR:
            AllocateChildrenX(s, a, s->firstChild, s->lastChild, Next);
            break;

        case PnDirection_RL:
            // With children in reverse order.
            AllocateChildrenX(s, a, s->lastChild, s->firstChild, Prev);
            break;

        case PnDirection_TB:
        case PnDirection_BT:
            d = a->width - 2 * GetWidth(s);
            // The widgets are lad along Y direction to the right so we
            // can extend them to the right.
            for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling) {
                if(d < c->allocation.width) {
                    c->culled = true;
                    continue;
                }
                c->allocation.x = a->x + GetWidth(s);
                if((c->expand & PnExpand_H) && (c->allocation.width < d))
                    c->allocation.width = d;
            }
            break;

        case PnDirection_None:
        default:
            ASSERT(0);
            break;
    }

    for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling)
        if(c->firstChild)
            AllocateChildren(c, &c->allocation);
}


// Move widget position without changing its size.
//
static
void AlignChildrenX(struct PnSurface *s, struct PnAllocation *a) {

    if(s->culled) return;

    switch(s->direction) {

        case PnDirection_One:
        case PnDirection_LR:
        case PnDirection_BT:
        case PnDirection_TB:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling);
            break;

        case PnDirection_RL:
            for(struct PnSurface *c = s->lastChild; c;
                    c = c->prevSibling);
            break;

        case PnDirection_None:
        default:
            ASSERT(0);
            break;
    }
}

static
void AlignChildrenY(struct PnSurface *s, struct PnAllocation *a) {

    if(s->culled) return;


}


// Get the positions and sizes of all the widgets in the window.
//
void GetWidgetAllocations(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->surface.type == PnSurfaceType_toplevel ||
            win->surface.type == PnSurfaceType_popup);
    DASSERT(!win->surface.allocation.x);
    DASSERT(!win->surface.allocation.y);
    DASSERT(!win->surface.hiding);

    if(!win->surface.firstChild) {
        // This is the case where an API user wants to draw on a simple
        // window, without dumb-ass widgets.  Fuck ya!  The main point of
        // this API.
        
        if(win->surface.allocation.width && win->surface.allocation.height)
            return;

        if(win->surface.width)
            win->surface.allocation.width = win->surface.width;
        else
            // This is the default size if the window has no size
            // or widgets in it.
            win->surface.allocation.width = PN_DEFAULT_WINDOW_WIDTH;

        if(win->surface.height)
            win->surface.allocation.height = win->surface.height;
        else
            // This is the default size if the window has no size
            // or widgets in it.
            win->surface.allocation.height = PN_DEFAULT_WINDOW_HEIGHT;

        return;
    }

    DASSERT(win->surface.firstChild->parent == &win->surface);
    DASSERT(win->surface.lastChild->parent == &win->surface);
    DASSERT(!win->surface.parent);
    DASSERT(!win->surface.allocation.x);
    DASSERT(!win->surface.allocation.y);



    // The size of the window is set in the users call to
    // PnWindow_create(), from a xdg_toplevel configure (resize) or
    // xdg_popup configure (resize) event.  So, we assume that
    // win->surface.allocation.width and win->surface.allocation.height
    // have been set at this time.

    // win->surface.allocation.width and win->surface.allocation.height
    // are the user requested sizes.  We save them for now.

    uint32_t width = win->surface.allocation.width;
    uint32_t height = win->surface.allocation.height;

    AddRequestedSizes(&win->surface);

    // Now we have the shrink wrapped window and widgets with no widgets
    // culled or expanded.  The window allocation is likely not the
    // size that was requested from the values before the call to
    // AddRequestedSizes().  This shrink wrapped size is what we can
    // start with and understand well.  We will start there and fix it.

    if(width && height) {
        win->surface.allocation.width = width;
        win->surface.allocation.height = height;
    }

    // We do not have widget x and y positions at this point.
    //
    // Get children x and y positions and recompute widths and heights all
    // while taking into account the widget's expand attribute (that is
    // whither or not the widget can expand to take up unused space in its
    // container.  And also cull out widgets that do not fit.
    AllocateChildren(&win->surface, &win->surface.allocation);

    // Without extra space in the containers these do nothing.  These may
    // change the widgets x and y positions, but will not change widget
    // widths and heights.
    AlignChildrenX(&win->surface, &win->surface.allocation);
    AlignChildrenY(&win->surface, &win->surface.allocation);

    win->needAllocate = false;
}
