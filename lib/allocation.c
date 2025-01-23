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
// Returns true if s is culled.
//
// If a container is hidden than all its children are effectively culled
// and we do not bother marking the children as culled.
//
// Here we are resetting the culled flags so we just use the hidden
// flags.
//
static bool ResetChildrenCull(struct PnSurface *s) {

    bool culled = s->hidden;

    if(culled || !s->firstChild)
        return culled;

    // "s" is not culled yet, but it may get culled if no children
    // are not culled.

    culled = true;
    // It's culled unless a child is not culled.

    for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling) {
        c->culled = ResetChildrenCull(c);

        if(!c->culled && culled)
            // We have at least one child not culled and the container "s"
            // is not hidden.
            culled = false;
        // We will set all children's children.
    }

    return culled;
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

// This lets us have container widgets (and windows) with zero width
// and/or height.  We only let it be zero if it's a container (has
// children).  If it's a leaf widget we force it to have non-zero width.
//
// If it's a container widget (surface), get this left and right widget
// border width, else if it's a leaf widget (surface) return the minimum
// width of the widget (surface).
//
static inline uint32_t GetBWidth(const struct PnSurface *s) {

    if(s->firstChild || s->width)
        return s->width;

    if(s->type == PnSurfaceType_widget)
        return PN_DEFAULT_WIDGET_WIDTH;

    DASSERT(s->type == PnSurfaceType_popup ||
            s->type == PnSurfaceType_toplevel);
    return PN_DEFAULT_WINDOW_WIDTH;
}

// Get this upper (lower) widget border width, of a container.  If it's a
// leaf widget we force it to have non-zero height.
//
// If it's a container widget (surface), get this upper and lower widget
// border width, else if it's a leaf widget (surface) this returns the
// minimum height of the widget.

static inline uint32_t GetBHeight(const struct PnSurface *s) {

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
void TallyRequestedSizes(struct PnSurface *s, struct PnAllocation *a) {

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
                TallyRequestedSizes(c, &c->allocation);
                // Now we have all "s" children and descendent sizes.
                a->width += c->allocation.width +
                        GetBWidth(s);
                if(a->height < c->allocation.height)
                    a->height = c->allocation.height;
            }
            if(gotOne)
                a->height += GetBHeight(s);
            break;
        case PnDirection_BT:
        case PnDirection_TB:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling) {
                if(c->culled) continue;
                if(!gotOne) gotOne = true;
                TallyRequestedSizes(c, &c->allocation);
                // Now we have all "s" children and descendent sizes.
                a->height += c->allocation.height +
                        GetBHeight(s);
                if(a->width < c->allocation.width)
                    a->width = c->allocation.width;
            }
            if(gotOne)
                a->width += GetBWidth(s);
            break;
        default:
    }

    // Add the last border or first size if no children.
    a->width += GetBWidth(s);
    a->height += GetBHeight(s);
}


static void GetChildrenXY(const struct PnSurface *s,
        const struct PnAllocation *a) {

    DASSERT(!s->culled);
    DASSERT(s->firstChild);
    DASSERT(a->width);
    DASSERT(a->height);

    uint32_t borderX = GetBWidth(s);
    uint32_t borderY = GetBHeight(s);
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


static uint32_t ClipOrCullChildren(const struct PnSurface *s,
        const struct PnAllocation *a);

#define GOT_CULL            (01)
#define NO_SHOWING_CHILD    (GOT_CULL|02)


// Returns true or false  // had a cull or not
//
// true -> c or a descendent was culled
// false -> nothing was culled
//
static inline bool TryCullX(
        /*parent space allocation*/
        const struct PnAllocation *a,
        struct PnSurface *c, struct PnAllocation *ca) {

    DASSERT(!c->culled);

    if(ca->x > a->x + a->width) {
        // The inner edge does not even fit.
        // Cull this whole widget.
        c->culled = true;
        return true;
    }

    if(ca->x + ca->width > a->x + a->width) {
        // The outer edge does not fit.

        if(!c->firstChild) {
            // This is a leaf that does not fit so we just cull it.
            c->culled = true;
            return true;
        }

        // We may cull some of this widget.
        // First make c fit.
        ca->width = a->x + a->width - ca->x;

        uint32_t ret = ClipOrCullChildren(c, ca);

        // This child "c" may not be culled if it has children that fit.
        if(ret & GOT_CULL) {
            // We culled some children from "c".
            if(ret == NO_SHOWING_CHILD)
                // "c" has no children showing any more so cull it.
                c->culled = true;
            //else: "c" has children that still fit.
            //      We had a culling of some of the children of "c".

            return true;

        } //else {
            // "c" had no children culled, so we must have just trimmed
            // the end border of "c".  So this is not a culling, it's just
            // a container border trimming.  We'll come here again if cull
            // searching the other direction (Y) makes this loop again.
            // }
    }

    return false; // no culling.
}

// Returns true or false  // had a cull or not
//
// true -> c or a descendent was culled
// false -> nothing was culled
//
static inline bool TryCullY(
        /*parent space allocation*/
        const struct PnAllocation *a,
        struct PnSurface *c, struct PnAllocation *ca) {

    DASSERT(!c->culled);

    if(ca->y > a->y + a->height) {
        // The inner edge does not even fit.
        // Cull this whole widget.
        c->culled = true;
        return true;
    }

    if(ca->y + ca->height > a->y + a->height) {
        // The outer edge does not fit.

        if(!c->firstChild) {
            // This is a leaf that does not fit so we just cull it.
            c->culled = true;
            return true;
        }

        // We may cull some of this widget.
        // First make c fit.
        ca->height = a->y + a->height - ca->y;

        uint32_t ret = ClipOrCullChildren(c, ca);

        // This child "c" may not be culled if it has children that fit.
        if(ret & GOT_CULL) {
            // We culled some children from "c".
            if(ret == NO_SHOWING_CHILD)
                // "c" has no children showing any more so cull it.
                c->culled = true;
            //else: "c" has children that still fit.
            //      We had a culling of some of the children of "c".

            return true;

        } //else {
            // "c" had no children culled, so we must have just trimmed
            // the end border of "c".  So this is not a culling, it's just
            // a container border trimming.  We'll come here again if cull
            // searching the other direction (X) makes this loop again.
            // }
    }

    return false; // no culling.
}

// For when the direction of the packing for the container of "c"
// is vertical.
//
static inline
uint32_t CullX(const struct PnAllocation *a,
        struct PnSurface *c, uint32_t haveCullRet) {

    DASSERT(c);
    DASSERT(c->parent);
    DASSERT(c->parent->direction == PnDirection_TB ||
            c->parent->direction == PnDirection_BT);

    bool haveChildShowing = false;

    // Now cull in the x direction.
    for(; c; c = c->nextSibling) {
        if(c->culled) continue;
        if(TryCullX(a, c, &c->allocation))
            haveCullRet |= GOT_CULL;
        if(!c->culled)
            // At least one child still shows.
            haveChildShowing = true;
    }
    if(!haveChildShowing) return NO_SHOWING_CHILD;
    return haveCullRet;
}

// For when the direction of the packing for the container of "c"
// is horizontal.
//
static inline
uint32_t CullY(const struct PnAllocation *a,
        struct PnSurface *c, uint32_t haveCullRet) {

    DASSERT(c);
    DASSERT(c->parent);
    DASSERT(c->parent->direction == PnDirection_LR ||
            c->parent->direction == PnDirection_RL);

    bool haveChildShowing = false;

    // Now cull in the y direction.
    for(; c; c = c->nextSibling) {
        if(c->culled) continue;
        if(TryCullY(a, c, &c->allocation))
            haveCullRet |= GOT_CULL;
        if(!c->culled)
            // At least one child still shows.
            haveChildShowing = true;
    }
    if(!haveChildShowing) return NO_SHOWING_CHILD;
    return haveCullRet;
}


// Cull widgets that do not fit. This is the hard part. If we (newly) cull
// a widget we have to go back and reposition all widgets.  This window
// can have arbitrarily complex (can of worms) shapes.  Maybe after
// culling each widget we go up just one parent level and reshape the
// widgets; and iterate like that until there are no more new culls in the
// tree traversal.
//
// Returns 0 or GOT_CULL or NO_SHOWING_CHILD
//
//
// Returns true if there is a descendent of "s" is trimmed or culled.
//
// // Clip borders and cull until we can't cull.
//
// So, if there is a new cull in a parent node we return (pop the
// ClipOrCullChildren() call stack).  If we cull a leaf, we keeping
// clipping and culling that family of all leaf children until we can't
// cull and then pop the ClipOrCullChildren() call stack.
//
// Parent widgets without any showing children are culled as we pop the
// function call stack.
//
// The windows widgets are no longer shrink wrapped.
//
// This function indirectly calls itself.
//
static uint32_t ClipOrCullChildren(const struct PnSurface *s,
        const struct PnAllocation *a) {

    DASSERT(!s->culled);
    DASSERT(s->firstChild);

    //return 0;

    struct PnSurface *c; // child iterator.

    // Initialize return value.
    uint32_t haveCullRet = 0;


    switch(s->direction) {

        case PnDirection_One:
        case PnDirection_TB:
            c = s->lastChild;
            while(c && c->culled)
                c = c->prevSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = TryCullY(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->prevSibling) {
                c = c->prevSibling;
                if(!c->culled)
                    TryCullY(a, c, &c->allocation);
            }
            // Now cull in the x direction.
            return CullX(a, s->firstChild, haveCullRet);

        case PnDirection_BT:
            c = s->firstChild;
            while(c && c->culled)
                c = c->nextSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = TryCullY(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->nextSibling) {
                c = c->nextSibling;
                if(!c->culled)
                    TryCullY(a, c, &c->allocation);
            }
            // Now cull in the x direction.
            return CullX(a, s->firstChild, haveCullRet);

        case PnDirection_LR:
            c = s->lastChild;
            while(c && c->culled)
                c = c->prevSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = TryCullX(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->prevSibling) {
                c = c->prevSibling;
                if(!c->culled)
                    TryCullX(a, c, &c->allocation);
            }
            // Now cull in the y direction.
            return CullY(a, s->firstChild, haveCullRet);

        case PnDirection_RL:
            c = s->firstChild;
            while(c && c->culled)
                c = c->nextSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = TryCullX(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->nextSibling) {
                c = c->nextSibling;
                if(!c->culled)
                    TryCullX(a, c, &c->allocation);
            }
            // Now cull in the Y direction.
            return CullY(a, s->firstChild, haveCullRet);

        case PnDirection_None:
        default:
            ASSERT(0, "s->type=%d s->direction=%d", s->type, s->direction);
            break;
    }

    // We should not get here.
    ASSERT(0);
    return 0;
}

static
void Expand(struct PnSurface *s, struct PnAllocation *a) {

    if(s->culled) return;


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
        a->width = GetBWidth(s);
        a->height = GetBHeight(s);
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

    uint32_t loopCount = 0;

    // We set haveChildShowing after many calculations.
    bool haveChildShowing = false;

    do {
        // Save the width and height.
        uint32_t width = a->width;
        uint32_t height = a->height;

        // This shrink wraps the widgets, only getting the widgets widths
        // and heights.  Without the culled widgets.
        TallyRequestedSizes(s, a);
        // So now a->width and a->height may have changed.

        // No x and y allocations yet.
        GetChildrenXY(s, a);
        // Now we have  x and y allocations.

        // Still shrink wrapped and the top surface width and height are
        // set to the "shrink wrapped" width and height.
        //
        // If this is the first call to GetWidgetAllocations(), width and
        // height will be zero.  The shrink wrapped value of a->width and
        // a->height will always be greater than zero (that's just due to
        // how we define things).

        if(width >= a->width && height >= a->height) {
            a->width = width;
            a->height = height;
            // Done culling widgets, we have enough space in the window
            // for all showing widgets.  The window may be a little large
            // now, but after this loop we may expand some of the widgets
            // to fill the extra space in addition to "aligning" them.
            break;

        } else if(width) {
            // If one is set then both width and height are set.
            DASSERT(height);
            a->width = width;
            a->height = height;
        } else {
            // This is the first call to GetWidgetAllocations() for this
            // window and a->width and a->height where both zero at the
            // start of this "do" loop.
            //
            // If one is not set than both are not set.
            DASSERT(!width && !height);
        }

        DASSERT(a->width && a->height);

        // Clip and cull until we can't clip or cull.  After a clip or
        // cull we need to fix the widget packing; shrink wrapping and
        // re-positioning widgets.  Hence this loops until widgets do not
        // change how they are shown.
        //
        // The widgets can have a very complex structure. Basically the
        // window is a pile of worms (widgets).  If we resize or remove a
        // worm we need to shake the worms into a new positions refilling
        // new empty spaces as they come to be.  This looping will
        // converge (terminate) so long as there is a finite number of
        // widgets in the window.  The worst case would be that we cull or
        // resize one widget per loop; but it likely that many widgets
        // will cull or resize in each loop, if there is any culling.
        //
        // I wonder if GTK and/or Qt do optimal widget culling and
        // resizing like this.  I know that for most apps that we see the
        // widget layout is simple, and this would finish culling and
        // resizing in just one loop.  I'd guess they do it better then
        // this, but then again they leak memory and don't give a shit; so
        // I would not be surprised that if they fucked it up ... I don't
        // like fixing other peoples buggy code.  I'll fix my own buggy
        // code.
        //
        // tests/random_widgets.c is a test that should work this loop
        // through its paces.

        if(loopCount) {
            // TODO: change this to DSPEW() or remove loopCount.
            WARN("called ClipOrCullChildren() %" PRIu32 " times",
                    loopCount);
        }
        ++loopCount;

        haveChildShowing = false;
        for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling)
            if(!c->culled) {
                haveChildShowing = true;
                break;
            }

    } while(haveChildShowing && ClipOrCullChildren(s, a));

    // Expand widgets that can be expanded.  Note this only fills
    // otherwise blank spaces.
    Expand(s, a);

    // Without extra space in the containers these do nothing.  These may
    // change the widgets x and y positions, but will not change widget
    // widths and heights.
    AlignChildrenX(s, a);
    AlignChildrenY(s, a);

INFO("w,h=%" PRIi32",%" PRIi32, a->width, a->height);
}
