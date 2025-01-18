#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include  "display.h"



// This function calls itself.
static
void AddRequestedSizes(struct PnSurface *s) {

    // We start at 0 and sum from there.
    //
    s->allocation.width = 0;
    s->allocation.height = 0;

    bool gotOne = false;

    switch(s->direction) {
        case PnDirection_None:
            DASSERT(!s->firstChild);
            break;
        case PnDirection_One:
            DASSERT((!s->firstChild && !s->lastChild)
                    || s->firstChild == s->lastChild);
            if(!s->firstChild || !s->firstChild->showing)
                break;
            // Same as PnDirection_LR and PnDirection_RL but with one in child
            // list.
        case PnDirection_LR:
        case PnDirection_RL:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling) {
                if(!c->showing) continue;
                if(!gotOne) gotOne = true;
                AddRequestedSizes(c);
                s->allocation.width += c->allocation.width + s->width;
                if(s->allocation.height < c->allocation.height)
                    s->allocation.height = c->allocation.height;
            }
            if(gotOne)
                s->allocation.height += s->height;
            break;
        case PnDirection_BT:
        case PnDirection_TB:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling) {
                if(!c->showing) continue;
                if(!gotOne) gotOne = true;
                AddRequestedSizes(c);
                s->allocation.height += c->allocation.height +
                    s->height;
                if(s->allocation.width < c->allocation.width)
                    s->allocation.width = c->allocation.width;
            }
            if(gotOne)
                s->allocation.width += s->width;
            break;
        default:
    }

    // Add the last border or first size if no children.
    s->allocation.width += s->width;
    s->allocation.height += s->height;
//INFO("s->allocation.width=%" PRIu32, s->allocation.width);
}


static inline
void GrowWidth(struct PnSurface *s, uint32_t width) {

    //s->allocation.width = width;

WARN("MORE CODE HERE");
}

static inline
void ShrinkWidth(struct PnSurface *s, uint32_t width) {

    //s->allocation.width = width;

WARN("MORE CODE HERE");
}


#if 0
static inline
void GrowHeight(struct PnSurface *s, uint32_t height) {

    s->allocation.height = height;

WARN("MORE CODE HERE");
}

static inline
void ShrinkHeight(struct PnSurface *s, uint32_t height) {

    s->allocation.height = height;

WARN("MORE CODE HERE");
}
#endif


// Set the value of the surface, s, allocation, a.x, a.y.
//
// The parent, p, allocation, pa, is set up already, and so is the
// "adjacent sibling" allocation in a sibling direction given by the
// packing direction of the parent.  So it could be the nextSibling or the
// prevSibling is set up.  It depends on the packing direction of the
// parent.  We set the lesser x and y values first.
//
static inline void GetX(struct PnSurface *s, struct PnAllocation *a,
        /*p is parent*/ /*pa is parent allocation is set up*/
        struct PnSurface *p, struct PnAllocation *pa) {

    DASSERT(s);
    DASSERT(a);
    DASSERT(p);
    DASSERT(pa);
    DASSERT(pa->width);
    DASSERT(pa->height);
    DASSERT(s->parent == p);

    // widget allocation coordinates, x and y, are all relative to the
    // window.

    switch(p->direction) {

        case PnDirection_BT:
        case PnDirection_TB: 
        case PnDirection_One:
            a->x = pa->x + p->width;
            break;

        case PnDirection_LR:
            if(s->prevSibling)
                a->x = s->prevSibling->allocation.x +
                    s->prevSibling->allocation.width +
                    p->width;
            else
                a->x = pa->x + p->width;
            break;

        case PnDirection_RL:
            if(s->nextSibling)
                a->x = s->nextSibling->allocation.x +
                    s->nextSibling->allocation.width +
                    p->width;
            else
                a->x = pa->x + p->width;
            break;

        case PnDirection_Callback:
            ASSERT(0, "WRITE MORE CODE HERE");
            break;
        case PnDirection_None:
            ASSERT(0, "NOT GOOD CODE");
            break;
    }
}

static inline void GetY(struct PnSurface *s, struct PnAllocation *a,
        /*p is parent*/ /*pa is parent allocation is set up*/
        struct PnSurface *p, struct PnAllocation *pa) {

    DASSERT(s);
    DASSERT(a);
    DASSERT(p);
    DASSERT(pa);
    DASSERT(pa->width);
    DASSERT(pa->height);
    DASSERT(s->parent == p);

    // widget allocation coordinates are all relative to the window.

    switch(p->direction) {

        case PnDirection_LR:
        case PnDirection_RL:
        case PnDirection_One:
            a->y = pa->y + p->height;
            break;

        case PnDirection_TB:
            if(s->prevSibling)
                a->y = s->prevSibling->allocation.y +
                    s->prevSibling->allocation.height +
                    p->height;
            else
                a->y = pa->y + p->height;
            break;

        case PnDirection_BT:
            if(s->nextSibling)
                a->y = s->nextSibling->allocation.y +
                    s->nextSibling->allocation.height +
                    p->height;
            else
                a->y = pa->y + p->height;
            break;

        case PnDirection_Callback:
            ASSERT(0, "WRITE MORE CODE HERE");
            break;
        case PnDirection_None:
            ASSERT(0, "NOT GOOD CODE");
            break;
    }
}


static
void ExpandChildrenX(struct PnSurface *s, struct PnAllocation *a) {


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
void ExpandChildrenY(struct PnSurface *s, struct PnAllocation *a) {
}


// Move widget position without changing its size.
//
static
void AlignChildrenX(struct PnSurface *s, struct PnAllocation *a) {


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
}

static
void GetChildrenX(struct PnSurface *s, struct PnAllocation *a) {

    if(s->parent)
        GetX(s, a, s->parent, &s->parent->allocation);

    if(!s->firstChild) {
        DASSERT(!s->lastChild);
        return;
    }

    switch(s->direction) {

        case PnDirection_One:
        case PnDirection_LR:
        case PnDirection_BT:
        case PnDirection_TB:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling)
                GetChildrenX(c, &c->allocation);
            break;

        case PnDirection_RL:
            for(struct PnSurface *c = s->lastChild; c;
                    c = c->prevSibling)
                GetChildrenX(c, &c->allocation);
            break;

        case PnDirection_None:
        default:
            ASSERT(0);
            break;
    }
}

static
void GetChildrenY(struct PnSurface *s, struct PnAllocation *a) {

    if(s->parent)
        GetY(s, a, s->parent, &s->parent->allocation);

    switch(s->direction) {

        case PnDirection_One:
        case PnDirection_LR:
        case PnDirection_RL:
        case PnDirection_TB:
            for(struct PnSurface *c = s->firstChild; c;
                    c = c->nextSibling)
                GetChildrenY(c, &c->allocation);
            break;

        case PnDirection_BT:
            for(struct PnSurface *c = s->lastChild; c;
                    c = c->prevSibling)
                GetChildrenY(c, &c->allocation);
            break;

        case PnDirection_None:
        default:
            ASSERT(0);
            break;
    }
}

// Get the positions and sizes of all the widgets in the window.
//
void GetWidgetAllocations(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->surface.type == PnSurfaceType_toplevel ||
            win->surface.type == PnSurfaceType_popup);
    DASSERT(!win->surface.allocation.x);
    DASSERT(!win->surface.allocation.y);


    if(!win->surface.firstChild) return;

    // The size of the window is set in the users call to
    // PnWindow_create(), from a xdg_toplevel configure (resize) or
    // xdg_popup configure (resize) event.  So, we assume that
    // win->surface.allocation.width and win->surface.allocation.height
    // have been set at this time.

    uint32_t width = win->surface.allocation.width;
    //uint32_t height = win->surface.allocation.height;

    AddRequestedSizes(&win->surface);

    if(width > win->surface.allocation.width)
        GrowWidth(&win->surface, width);
    else if(width < win->surface.allocation.width)
        ShrinkWidth(&win->surface, width);

#if 0
    if(height > win->surface.allocation.height)
        GrowHeight(&win->surface, height);
    else if(height < win->surface.allocation.height)
        ShrinkHeight(&win->surface, height);
#endif

    DASSERT(!win->surface.allocation.x);
    DASSERT(!win->surface.allocation.y);

    // Get children x and y positions:
    if(win->surface.lastChild) {
        DASSERT(win->surface.firstChild->parent == &win->surface);
        DASSERT(win->surface.lastChild->parent == &win->surface);
        DASSERT(!win->surface.parent);

        GetChildrenX(&win->surface, &win->surface.allocation);
        GetChildrenY(&win->surface, &win->surface.allocation);

        ExpandChildrenX(&win->surface, &win->surface.allocation);
        ExpandChildrenY(&win->surface, &win->surface.allocation);

        AlignChildrenX(&win->surface, &win->surface.allocation);
        AlignChildrenY(&win->surface, &win->surface.allocation);
    }

    win->needAllocate = false;
}
