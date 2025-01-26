#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include  "display.h"


// We use lots of function recursion to get widget positions, sizes, and
// culling.  This code may hurt your head.


// This function calls itself.
//
// This culling got with this PnSurface::culled flag is just effecting the
// showing (and not showing) of widgets: due to the window size not being
// large enough, or the panel's API (application programming interface)
// user hiding the widget.
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
// flags until after we compare widget sizes with the window size.
//
// Think of PnSurface::culled as a temporary flag that is only used when
// we are allocating widget sizes and positions.
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
// surface becomes a non-container widget (with no children, a leaf).
//
// "panels" does not so much distinguish between container widgets and
// non-container widgets.  Panels widgets can change between being a
// container and non-container widget just by removing or adding child
// widgets from them, without recreating the widget.
//
// Note: a container widget does not change to being a leaf widget by
// having all its children culled.  If a container widget has all
// its children culled, then the container will be culled too.
//
// Think of PnSurface::canExpand as a temporary flag that is only used
// when we are allocating widget sizes and positions.
//
// Note: enum PnExpand is a bits flag with two bits, it's not a bool.
// A widget can't expand if no bits are set in the "expand" flag.
//
// Returns the temporary correct canExapnd attribute of "s".
//
static uint32_t ResetCanExpand(struct PnSurface *s) {

    DASSERT(s);
    DASSERT(!s->culled);

    if(s->firstChild)
        // We may add changes to this later:
        s->canExpand = 0;
    else {
        // This is a leaf node.  We are at an end of the function call
        // stack.  s->expand is a user set attribute.
        s->canExpand = s->expand;
        return s->canExpand;
    }

    // Now "s" has children.

    // TODO: deal with containers that are not PnDirection_LR,
    // PnDirection_RL, PnDirection_TB, or PnDirection_BT.
    ASSERT(s->direction >= PnDirection_LR ||
            s->direction <= PnDirection_BT, "WRITE MORE CODE");

    for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling) {
        if(c->culled)
            // Skip culled widgets.
            continue;
        // If any of the children can expand than "s" can expand, so long
        // as there's no culling to get to them.
        s->canExpand |= ResetCanExpand(c);
    }

    // Return if we have any leaf children that can expand.
    return s->canExpand;
}


#if 0
void CheckCanExpand(const struct PnSurface *s) {
    ASSERT(s);
    ASSERT(!s->culled);
    ASSERT(s->canExpand == PnExpand_HV);

    for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling) {
        if(c->culled)
            // Skip culled widgets.
            continue;
        CheckCanExpand(c);
    }
}
#endif


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


// This function calls itself.  I don't think we'll blow the function call
// stack here.  How many layers of widgets do we have?
//
// A not showing widget has it's "culling" flag set to true (before this
// function call), if not it's false (before this function call).
//
static
void TallyRequestedSizes(const struct PnSurface *s,
        struct PnAllocation *a) {

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
static inline bool RecurseCullX(
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
static inline bool RecurseCullY(
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
uint32_t CullX(const struct PnAllocation *a, struct PnSurface *c) {

    DASSERT(c);
    DASSERT(c->parent);
    DASSERT(c->parent->direction == PnDirection_TB ||
            c->parent->direction == PnDirection_BT);

    bool haveChildShowing = false;
    uint32_t haveCullRet = 0;

    // Now cull in the x direction.
    for(; c; c = c->nextSibling) {
        if(c->culled) continue;
        if(RecurseCullX(a, c, &c->allocation))
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
uint32_t CullY(const struct PnAllocation *a, struct PnSurface *c) {

    DASSERT(c);
    DASSERT(c->parent);
    DASSERT(c->parent->direction == PnDirection_LR ||
            c->parent->direction == PnDirection_RL);

    bool haveChildShowing = false;
    uint32_t haveCullRet = 0;

    // Now cull in the y direction.
    for(; c; c = c->nextSibling) {
        if(c->culled) continue;
        if(RecurseCullY(a, c, &c->allocation))
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
            haveCullRet = CullX(a, s->firstChild);
            if(haveCullRet) return haveCullRet;
            c = s->lastChild;
            while(c && c->culled)
                c = c->prevSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = RecurseCullY(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->prevSibling) {
                c = c->prevSibling;
                if(!c->culled)
                    RecurseCullY(a, c, &c->allocation);
            }
            if(c->culled) haveCullRet = NO_SHOWING_CHILD;
            return haveCullRet;

        case PnDirection_BT:
            haveCullRet = CullX(a, s->firstChild);
            if(haveCullRet) return haveCullRet;
            c = s->firstChild;
            while(c && c->culled)
                c = c->nextSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = RecurseCullY(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->nextSibling) {
                c = c->nextSibling;
                if(!c->culled)
                    RecurseCullY(a, c, &c->allocation);
            }
            if(c->culled) haveCullRet = NO_SHOWING_CHILD;
            return haveCullRet;

        case PnDirection_LR:
            haveCullRet = CullY(a, s->firstChild);
            if(haveCullRet) return haveCullRet;
            c = s->lastChild;
            while(c && c->culled)
                c = c->prevSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = RecurseCullX(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->prevSibling) {
                c = c->prevSibling;
                if(!c->culled)
                    RecurseCullX(a, c, &c->allocation);
            }
            if(c->culled) haveCullRet = NO_SHOWING_CHILD;
            return haveCullRet;

        case PnDirection_RL:
            haveCullRet = CullY(a, s->firstChild);
            if(haveCullRet) return haveCullRet;
            c = s->firstChild;
            while(c && c->culled)
                c = c->nextSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = RecurseCullX(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->nextSibling) {
                c = c->nextSibling;
                if(!c->culled)
                    RecurseCullX(a, c, &c->allocation);
            }
            if(c->culled) haveCullRet = NO_SHOWING_CHILD;
            return haveCullRet;

        case PnDirection_None:
        default:
            ASSERT(0, "s->type=%d s->direction=%d", s->type, s->direction);
            break;
    }

    // We should not get here.
    ASSERT(0);
    return 0;
}

struct PnSurface *Next(struct PnSurface *s) {
    DASSERT(s);
    return s->nextSibling;
}

struct PnSurface *Prev(struct PnSurface *s) {
    DASSERT(s);
    return s->prevSibling;
}


// Expand children of "s" while taking space to the right.  It's to the
// right because that's the convention that window managers and widget
// APIs have chosen before me.  All operating systems do this.
//
// Shrink a window and if that program lets you (shrink it) you'll see it
// culls widgets to the right and bottom; on any common operating system.
// That's why (or consistent with) +x is to the right and +y is in the
// down direction.
//
// This will pad (border) the right edge by GetBWidth(s).  If it can't pad
// the right edge (put the container full right border in), it will not
// change anything, except maybe the x positions.
//
// For a horizontal container widget "trimming" is when the window is not
// large enough to show all the right border of the container widget.
// Leaf widgets do not get trimmed, if they do not have their minimum
// required size they get culled.  Culling happens before this function
// call.
//
// So now child positions are no longer valid and must be computed from
// the parent "s" positions in "a".  The child widths may increase here,
// but do not decrease.  Hence the term Expand.
//
// And, the surface "s" positions and sizes, "a", are correct and hence
// constant in this function prototype.  "Correct" in this case, is only a
// matter of how we define it here, correct is relative to context.  Yes,
// I'm insane.
//
static inline void ExpandHShared(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnSurface *first,
        struct PnSurface *(*next)(struct PnSurface *s)) {

    DASSERT(first);
    DASSERT(next);
    DASSERT(s);
    DASSERT(a == &s->allocation);

    uint32_t border = GetBWidth(s);
    uint32_t numExpand = 0;
    uint32_t neededWidth = 0;
    struct PnSurface *c;
    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        neededWidth += border + c->allocation.width;
        if(c->canExpand & PnExpand_H)
            ++numExpand;
    }

    // When we expand we do not fill the container right edge border.
    // But, it may be filled in a little already, via what we called
    // "trimming".

    // It may be that none of the widgets increase their width because
    // surface "s" is squeezing them to their limit; but we still need to
    // recalculate the children x positions.

    // We will change these children and then go on to expand these
    // children's children.

    uint32_t extra = 0;
    if(neededWidth + border < a->width)
        extra = a->width - (neededWidth + border);
    uint32_t padPer = 0;
    if(numExpand)
        padPer = extra/numExpand;
    // endPiece is the leftover pixels from extra not being a integer
    // multiple of padPer.  It's added to the last widget width.
    uint32_t endPiece = extra - numExpand * padPer;
    // The starting left side widget has the smallest x position.
    uint32_t x = a->x + border;

    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        struct PnAllocation *ca = &c->allocation;
        ca->x = x;
        // Add to the width if we can.
        if(c->canExpand & PnExpand_H) {
            ca->width += padPer;
            --numExpand;
            if(!numExpand)
                // endPiece can be zero.
                ca->width += endPiece;
        } // else ca->width does not change.
        x += ca->width + border;
    }
    DASSERT(x - border <= a->x + a->width);
}


// Child positions are no longer valid an must be computed from the parent
// "s" positions in "a" (which are correct).
//
// See comments in and above ExpandHShared().  There's symmetry here.
//
static inline void ExpandVShared(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnSurface *first,
        struct PnSurface *(*next)(struct PnSurface *s)) {

    DASSERT(first);
    DASSERT(next);
    DASSERT(s);
    DASSERT(a == &s->allocation);

    uint32_t border = GetBHeight(s);
    uint32_t numExpand = 0;
    uint32_t neededHeight = 0;
    struct PnSurface *c;
    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        neededHeight += border + c->allocation.height;
        if(c->canExpand & PnExpand_V)
            ++numExpand;
    }

    // When we expand we do not fill the container bottom edge border.
    // But, it may be filled in a little already, via what we called
    // "trimming".

    // It may be that none of the widgets increase their height because
    // surface "s" is squeezing them to their limit; but we still need to
    // recalculate the children y positions.

    // We will change these children and then go on to expand these
    // children's children.

    uint32_t extra = 0;
    if(neededHeight + border < a->height)
        extra = a->height - (neededHeight + border);
    uint32_t padPer = 0;
    if(numExpand)
        padPer = extra/numExpand;
    // endPiece is the leftover pixels from extra not being a integer
    // multiple of padPer.  It's added to the last widget height.
    uint32_t endPiece = extra - numExpand * padPer;
    // The starting top side widget has the smallest y position.
    uint32_t y = a->y + border;

    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        struct PnAllocation *ca = &c->allocation;
        ca->y = y;
        // Add to the height if we can.
        if(c->canExpand & PnExpand_V) {
            ca->height += padPer;
            --numExpand;
            if(!numExpand)
                // endPiece can be zero.
                ca->height += endPiece;
        } // else ca->height does not change.
        y += ca->height + border;
    }
    DASSERT(y - border <= a->y + a->height);
}

// allocation a is the parent allocation and it is correct.
//
// Fix the x and width of ca.
//
static inline void ExpandH(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnSurface *c, struct PnAllocation *ca) {
    DASSERT(a);
    DASSERT(c);
    DASSERT(ca == &c->allocation);
    DASSERT(s);
    DASSERT(s->firstChild);
    DASSERT(s->lastChild);
    DASSERT(&s->allocation == a);
    DASSERT(!c->culled);

    uint32_t border = GetBWidth(s);
    ca->x = a->x + border;

    if(!(c->canExpand & PnExpand_H)) return;

    // Now see how to set the width, or not.

    DASSERT(ca->width);
    DASSERT(ca->width <= a->width + border);
    DASSERT(a->width > border);
    // The width of "ca" could be correct already, and may have
    // this "correct" size by "trimming", so we do not change it
    // if it already has some width based on this logic.

    if(a->width < 2 * border) return;
        // "s" was trimmed.  The end container border (at the right edge
        // of the window) was cut; so there is not a possibly of expanding
        // "c".  Only containers get trimmed their right and bottom
        // borders trimmed, leaf widgets get culled (not trimmed).
    if(ca->width < a->width - 2 * border)
        ca->width = a->width - 2 * border;
}

// "s" is the parent of "c".  "a" is the allocation of "s".
// "a" is correct.
//
// Fix the y and height of "ca" which is the allocation of "c".
// "c" fills all the space of "s" in this y direction.
//
static inline void ExpandV(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnSurface *c, struct PnAllocation *ca) {
    DASSERT(a);
    DASSERT(c);
    DASSERT(ca == &c->allocation);
    DASSERT(s);
    DASSERT(s->firstChild);
    DASSERT(s->lastChild);
    DASSERT(&s->allocation == a);
    DASSERT(!c->culled);
    DASSERT(s->canExpand & PnExpand_V);

    uint32_t border = GetBHeight(s);
    ca->y = a->y + border;

    if(!(c->canExpand & PnExpand_V)) return;

    // Now see how to set the height, or not.

    DASSERT(ca->height);
    DASSERT(ca->height <= a->height + border);
    DASSERT(a->height > border);
    // The height of "ca" could be correct already, and may have
    // this "correct" size by "trimming", so we do not change it
    // if it already has some height based on this logic.

    if(a->height < 2 * border) return;
        // "s" was trimmed.  The end border (at the bottom edge of the
        // window) was cut; so there is not a possibly of expanding "c".
        // Only containers get trimmed their right and bottom borders
        // trimmed, leaf widgets get culled (not trimmed).
    if(ca->height < a->height - 2 * border)
        ca->height = a->height - 2 * border;
}


// This function calls itself.
//
// At this call "s" has it's position and size set "correctly" and so do
// all the parent levels above "s" (grand parents, great grand parents,
// and so on).
//
// We are changing the position and size of the children of "s", and
// the children's children, the children's children's children, and so
// on.
//
static void ExpandChildren(const struct PnSurface *s,
        const struct PnAllocation *a) {

    // "s" has a correct allocation, "a".

    DASSERT(s);
    DASSERT(s->firstChild);
    DASSERT(s->lastChild);
    DASSERT(!s->culled);
    DASSERT(s->canExpand);

    struct PnSurface *c;

    switch(s->direction) {

        case PnDirection_One:
            DASSERT(s->firstChild == s->lastChild);
        case PnDirection_LR:
            ExpandHShared(s, a, s->firstChild, Next);
            for(c = s->firstChild; c; c = c->nextSibling)
                if(!c->culled)
                    ExpandV(s, a, c, &c->allocation);
            break;

        case PnDirection_TB:
            ExpandVShared(s, a, s->firstChild, Next);
            for(c = s->firstChild; c; c = c->nextSibling)
                if(!c->culled)
                    ExpandH(s, a, c, &c->allocation);
            break;

        case PnDirection_BT:
            ExpandVShared(s, a, s->lastChild, Prev);
            for(c = s->firstChild; c; c = c->nextSibling)
                if(!c->culled)
                    ExpandH(s, a, c, &c->allocation);
            break;

        case PnDirection_RL:
            ExpandHShared(s, a, s->lastChild, Prev);
            for(c = s->firstChild; c; c = c->nextSibling)
                if(!c->culled)
                    ExpandV(s, a, c, &c->allocation);
            break;

        case PnDirection_None:
        default:
            ASSERT(0);
            break;
    }

    // Now that all the children of "s" have correct allocations we can
    // expand (fix) the "s" children's children allocations.

    for(c = s->firstChild; c; c = c->nextSibling)
        if(!c->culled && c->canExpand && c->firstChild)
            ExpandChildren(c, &c->allocation);
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
// A better function name may be GetWdigetPositionAndSize().
// And the struct PnAllocation should be struct PositionAndSize.
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
        // tests/random_widgets.c is a test that should work this loop
        // through its paces.

        ++loopCount;

        if(loopCount >= 4) {
            // TODO: change this to DSPEW() or remove loopCount.
            INFO("calling ClipOrCullChildren() %" PRIu32 " times",
                    loopCount);
        }

        haveChildShowing = false;
        // It can happen that the window is so small that no widget can
        // fix in it: all the widgets get culled.
        for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling)
            if(!c->culled) {
                haveChildShowing = true;
                break;
            }

    } while(haveChildShowing && ClipOrCullChildren(s, a));


    ResetCanExpand(s);

    //CheckCanExpand(s);


    // Expand widgets that can be expanded.  Note this only fills in
    // otherwise blank container spaces.
    if(s->canExpand)
        ExpandChildren(s, a);

    // Without extra space in the containers these do nothing.  These may
    // change the widgets x and y positions, but will not change widget
    // widths and heights.
    AlignChildrenX(s, a);
    AlignChildrenY(s, a);

//INFO("w,h=%" PRIi32",%" PRIi32, a->width, a->height);
}
