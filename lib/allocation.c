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


// This function calls itself.
//
// If a parent is culled: then we do not need to think about culling
// the children, they are implied to be culled without setting the
// "culled" flag for the children.
//
static void ResetChildrenCull(struct PnSurface *s) {

    for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling) {
            c->culled = c->hidden;
        if(!c->culled)
            ResetChildrenCull(c);
    }
}

// This expandability attribute passes from child to parent, unlike
// culling that works from parent to child.  That is, if a child can
// expand than so can its parent (unrelated to the parent container
// surface user set expand flag).
//
// The parent container surface user set expand flag is ignored until the
// surface becomes a non-container widget (with no children).
//
// "panels" does not so much distinguish between container widgets and
// non-container widgets.  Panels widgets can change between being a
// container and non-container widget just by removing or adding child
// widgets from them, without recreating the widget.
//
static bool ResetCanExpand(struct PnSurface *s) {

    if(s->firstChild)
        // We may add changes to this later:
        s->canExpand = 0;
    else {
        // This is a leaf node.  We can at an end of the function call
        // stack.
        s->canExpand = s->expand;
        return s->canExpand;
    }

    for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling) {
        if(c->culled)
            // Skip culled widgets.
            continue;
        // If any of the children can expand than "s" can expand:
        s->canExpand |= ResetCanExpand(c);
    }

    // Return if we have any leaf children that can expand.
    return s->canExpand;
}


// This lets us have container widgets (and windows) with 
// zero width and/or height.  We only let it be zero if
// it's a container (has children).
//
static inline uint32_t GetWidth(const struct PnSurface *s) {

    if(s->firstChild || s->width)
        return s->width;

    if(s->type == PnSurfaceType_widget)
        return PN_DEFAULT_WIDGET_WIDTH;

    DASSERT(s->type == PnSurfaceType_popup ||
            s->type == PnSurfaceType_toplevel);
    return PN_DEFAULT_WINDOW_WIDTH;
}
//
static inline uint32_t GetHeight(const struct PnSurface *s) {

    if(s->firstChild || s->height)
        return s->height;

    if(s->type == PnSurfaceType_widget)
        return PN_DEFAULT_WIDGET_HEIGHT;

    DASSERT(s->type == PnSurfaceType_popup ||
            s->type == PnSurfaceType_toplevel);
    return PN_DEFAULT_WINDOW_HEIGHT;
}


struct PnSurface *Next(const struct PnSurface *s) {
    return s->nextSibling;
}
// The backwards next.
struct PnSurface *Prev(const struct PnSurface *s) {
    return s->prevSibling;
}


// This function calls itself.  I don't think we'll blow the function call
// stack here.  How many layers of widgets do we have?
//
// The not showing widget has it's culling flag set true here, if not it's
// set to false.
//
static
void AddRequestedSizes(struct PnSurface *s, struct PnAllocation *a) {

    DASSERT(!s->culled);

    // We start at 0 and sum from there.
    //
    a->width = 0;
    a->height = 0;
    a->x = 0;
    a->y = 0;

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
                if(c->culled) continue;
                if(!gotOne) gotOne = true;
                AddRequestedSizes(c, &c->allocation);
                // Now we have all "s" children and descendent sizes.
                a->width += c->allocation.width +
                        GetWidth(s);
                if(a->height < c->allocation.height)
                    a->height = c->allocation.height;
            }
            if(gotOne)
                a->height += GetHeight(s);
            break;
        case PnDirection_BT:
        case PnDirection_TB:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling) {
                if(c->culled) continue;
                if(!gotOne) gotOne = true;
                AddRequestedSizes(c, &c->allocation);
                // Now we have all "s" children and descendent sizes.
                a->height += c->allocation.height +
                        GetHeight(s);
                if(a->width < c->allocation.width)
                    a->width = c->allocation.width;
            }
            if(gotOne)
                a->width += GetWidth(s);
            break;
        default:
    }

    // Add the last border or first size if no children.
    a->width += GetWidth(s);
    a->height += GetHeight(s);
}


static void GetChildrenXY(const struct PnSurface *s,
        const struct PnAllocation *a) {

    DASSERT(!s->culled);
    DASSERT(s->firstChild);
    DASSERT(a->width);
    DASSERT(a->height);

    uint32_t borderX = GetWidth(s);
    uint32_t borderY = GetHeight(s);
    uint32_t x = a->x + borderX;
    uint32_t y = a->y + borderY;


    switch(s->direction) {

        case PnDirection_One:
        case PnDirection_TB:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling) {
                if(c->culled) continue;
                c->allocation.x = x;
                c->allocation.y = y;
                if(c->firstChild)
                    GetChildrenXY(c, &c->allocation);
                y += c->allocation.height + borderY;
            }
            break;

        case PnDirection_BT:
            for(struct PnSurface *c = s->lastChild; c;
                    c = c->prevSibling) {
                if(c->culled) continue;
                c->allocation.x = x;
                c->allocation.y = y;
                if(c->firstChild)
                    GetChildrenXY(c, &c->allocation);
                y += c->allocation.height + borderY;
            }
            break;

        case PnDirection_LR:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling) {
                if(c->culled) continue;
                c->allocation.x = x;
                c->allocation.y = y;
                if(c->firstChild)
                    GetChildrenXY(c, &c->allocation);
                x += c->allocation.width + borderX;
            }
            break;

        case PnDirection_RL:
            for(struct PnSurface *c = s->lastChild; c;
                    c = c->prevSibling) {
                if(c->culled) continue;
                c->allocation.x = x;
                c->allocation.y = y;
                if(c->firstChild)
                    GetChildrenXY(c, &c->allocation);
                x += c->allocation.width + borderX;
            }
            break;

        case PnDirection_None:
        default:
            ASSERT(0);
            break;
    }
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
// This does not allocate any memory.  The "Allocations" part of the name
// it borrowed from the GDK part of the GTK API, which has a rectangular
// space "allocation" class which gets setup when the widgets are
// "allocated" (find the position and size of widgets).
//
void GetWidgetAllocations(struct PnWindow *win) {

    DASSERT(win);
    struct PnSurface *s = &win->surface;
    struct PnAllocation *a = &s->allocation;
    DASSERT(s->type == PnSurfaceType_toplevel ||
            s->type == PnSurfaceType_popup);
    DASSERT(!a->x);
    DASSERT(!a->y);
    DASSERT(!s->hidden);
    DASSERT(!s->parent);
    DASSERT(!s->culled);

    win->needAllocate = false;

    if(!a->width && !s->firstChild) {
        DASSERT(!a->height);
        a->width = GetWidth(s);
        a->height = GetHeight(s);
    }

    // We already handled not letting the window resize if the user set
    // the expand flags so it cannot expand, in toplevel.c.
    //
    // TODO: This for popup windows?


    if(!s->firstChild) {
        DASSERT(!s->lastChild);
        // This is the case where an API user wants to draw on a simple
        // window, without dumb-ass widgets.  Fuck ya!  The main point of
        // this API is to do simple shit like draw pixels.
INFO("w,h=%" PRIi32",%" PRIi32, a->width, a->height);
        return;
    }

    DASSERT(s->firstChild->parent == s);
    DASSERT(s->lastChild->parent == s);

    ResetChildrenCull(s);
    ResetCanExpand(s);

    // Save the width and height.
    uint32_t width = a->width;
    uint32_t height = a->height;


    // This shrink wraps the widgets, only getting the widgets widths and
    // heights.  Without the culled widgets.
    AddRequestedSizes(s, a);

    // No x, y allocations yet.
    GetChildrenXY(s, a);

    // Still shrink wrapped.

    // These may have changed.
    if(width && height) {
        a->width = width;
        a->height = height;
    }

    // Now we need to expand or cull widget due to the width and height
    // not being the shrink wrapped width and height.



    // Without extra space in the containers these do nothing.  These may
    // change the widgets x and y positions, but will not change widget
    // widths and heights.
    AlignChildrenX(s, a);
    AlignChildrenY(s, a);

INFO("w,h=%" PRIi32",%" PRIi32, a->width, a->height);
}
