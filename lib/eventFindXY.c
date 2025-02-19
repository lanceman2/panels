#define _GNU_SOURCE
#include <link.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>


#include "../include/panels.h"
#include "debug.h"
#include  "display.h"


// Find the most childish surface (widget) that has the mouse pointer
// position, d.x and d.y, in it.
//
// TODO: Do we need a faster surface search data structure?
//
// TODO: Maybe the widget (surface) order in the searching could be more
// optimal.
//
static struct PnSurface *FindSurface(const struct PnWindow *win,
        struct PnSurface *s, uint32_t x, uint32_t y) {

    DASSERT(win);
    DASSERT(s->firstChild);
    DASSERT(!s->culled);

    // Let's see if the x,y positions always make sense.
    DASSERT(s->allocation.x <= x);
    DASSERT(s->allocation.y <= y);
    if(x >= s->allocation.x + s->allocation.width)
        --x;
    if(y >= s->allocation.y + s->allocation.height) {
        WARN("y=%" PRIu32 "s->allocation.y + s->allocation.height=%" PRIu32,
                y, s->allocation.y + s->allocation.height);
    }

    DASSERT(x < s->allocation.x + s->allocation.width);
    DASSERT(y < s->allocation.y + s->allocation.height);

    // At this point in the code "s" contains the pointer position, x,y.
    // Now search the children to see if x,y is in one of them.

    struct PnSurface *c;

    switch(s->direction) {

        case PnDirection_One:
        case PnDirection_LR:
            for(c = s->firstChild; c; c = c->nextSibling) {
                if(c->culled) continue;
                if(x < c->allocation.x)
                    // x is to the left of surface "c"
                    break; // done in x
                if(x >= c->allocation.x + c->allocation.width)
                    // x is to the right of surface "c" so
                    // look at siblings to the right.
                    continue;
                // The x is in surface "c"
                if(y < c->allocation.y)
                    // y is above the surface "c"
                    break;
                if(y >= c->allocation.y + c->allocation.height)
                    // y is below the surface "c"
                    break;
                // this surface "c" contains x,y.
                if(c->firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        case PnDirection_RL:
            for(c = s->lastChild; c; c = c->prevSibling) {
                if(c->culled) continue;
                if(x < c->allocation.x)
                    // x is to the left of surface "c"
                    break; // done in x
                if(x >= c->allocation.x + c->allocation.width)
                    // x is to the right of surface "c" so
                    // look at siblings to the right.
                    continue;
                // The x is in surface "c"
                if(y < c->allocation.y)
                    // y is above the surface "c"
                    break;
                if(y >= c->allocation.y + c->allocation.height)
                    // y is below the surface "c"
                    break;
                // this surface "c" contains x,y.
                if(c->firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        case PnDirection_TB:
            for(c = s->firstChild; c; c = c->nextSibling) {
                if(c->culled) continue;
                if(y < c->allocation.y)
                    // y is to the above of surface "c"
                    break; // done in y
                if(y >= c->allocation.y + c->allocation.height)
                    // y is below surface "c" so
                    // look at siblings below.
                    continue;
                // The y is in surface "c"
                if(x < c->allocation.x)
                    // x is to the left the surface "c"
                    break;
                if(x >= c->allocation.x + c->allocation.width)
                    // x is to the right the surface "c"
                    break;
                // this surface "c" contains x,y.
                if(c->firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        case PnDirection_BT:
            for(c = s->lastChild; c; c = c->prevSibling) {
                if(c->culled) continue;
                if(y < c->allocation.y)
                    // y is to the above of surface "c"
                    break; // done in y
                if(y >= c->allocation.y + c->allocation.height)
                    // y is below surface "c" so
                    // look at siblings below.
                    continue;
                // The y is in surface "c"
                if(x < c->allocation.x)
                    // x is to the left the surface "c"
                    break;
                if(x >= c->allocation.x + c->allocation.width)
                    // x is to the right the surface "c"
                    break;
                // this surface "c" contains x,y.
                if(c->firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        default:
            ASSERT(0, "Write more code");
            return 0;
    }


    return s;
}


void GetPointerSurface(const struct PnWindow *win) {

    // Lets guess that the mouse pointer is at the same widget.
    // If not then:
    if(!(d.pointerSurface &&
                d.pointerSurface->allocation.x <= d.x && 
                d.x < d.pointerSurface->allocation.x + 
                        d.pointerSurface->allocation.width &&
                d.pointerSurface->allocation.y <= d.y && 
                d.y < d.pointerSurface->allocation.y + 
                        d.pointerSurface->allocation.height))
        // Start at the window surface.
        d.pointerSurface = (void *) d.pointerWindow;

    // The d.x, d.y position can move out from the pointerSurface
    // without changing the pointerSurface because of a pointer grab.

    if(d.pointerSurface->firstChild &&
            d.x >= d.pointerSurface->allocation.x &&
            d.y >= d.pointerSurface->allocation.y &&
            d.x < d.pointerSurface->allocation.x +
                d.pointerSurface->allocation.width &&
            d.y < d.pointerSurface->allocation.y +
                d.pointerSurface->allocation.height)
        // See if it's in a deeper child surface.
        d.pointerSurface = FindSurface(d.pointerWindow, d.pointerSurface,
                d.x, d.y);
}


// Find d.x, d.y, and d.pointerSurface which is the x,y position and
// surface (widget) that contains that position.
//
// The mouse pointer window, d.pointerWindow, must be known.
//
void GetSurfaceWithXY(const struct PnWindow *win,
        wl_fixed_t x,  wl_fixed_t y, bool isEnter) {

    DASSERT(d.pointerWindow);
    DASSERT(win == d.pointerWindow);

    // Maybe we round up wrong...??  We fix it below.

    if(wl_fixed_to_double(x) > 0.0)
        d.x = (wl_fixed_to_double(x) + 0.5);
    else
        d.x = (wl_fixed_to_double(x) - 0.5);
    if(wl_fixed_to_double(x) > 0.0)
        d.y = (wl_fixed_to_double(y) + 0.5);
    else
        d.y = (wl_fixed_to_double(y) - 0.5);


//WARN("            %d,%d", d.x, d.y);

    if(isEnter) {
        // I do not like this.  With the compositor that I'm using I can
        // get x,y values not in the window with window enter events.
        // That makes no fucking sense, the mouse pointer is not in the
        // window, but the enter event says it is.  It could be round off
        // error.
#define ERR_LEN  (1)
        DASSERT(d.x >= - ERR_LEN);
        DASSERT(d.y >= - ERR_LEN);
        DASSERT(d.x < win->surface.allocation.width + ERR_LEN);
        DASSERT(d.y < win->surface.allocation.height + ERR_LEN);

        if(d.x < 0)
            d.x = 0;
        else if(d.x >= win->surface.allocation.width)
            d.x = win->surface.allocation.width - 1;
        if(d.y < 0)
            d.y = 0;
        else if(d.y >= win->surface.allocation.height)
            d.y = win->surface.allocation.height - 1;
    } else
        // The mouse pointer can send events if there is a pointer grab and in
        // that case the position made be to the left and/or above the window,
        // making d.x and/or d.y negative or larger than the width and height
        // of the window.
        if(d.x < 0 || d.y < 0 ||
                d.x >= win->surface.allocation.width ||
                d.y >= win->surface.allocation.height)
            return;

    GetPointerSurface(win);
}

// Find the next focused widget (focusSurface), switch between the last
// focused widget and the next, calling next focused widget enter callback
// and then the previous focused widget leave callback.
//
void DoEnterAndLeave(void) {

    DASSERT(d.pointerSurface);
    // d.pointerSurface is the surface with the mouse pointer in it.

    struct PnSurface *oldFocus = d.focusSurface;
    d.focusSurface = 0;   

    for(struct PnSurface *s = d.pointerSurface; s; s = s->parent) {
        DASSERT(!s->culled);
        if(oldFocus == s) {
            // This surface "s" already has focus.
            d.focusSurface = s;
            break;
        }

        if(s->enter) {
            if(oldFocus) {
                DASSERT(oldFocus->leave);
                oldFocus->leave(oldFocus, oldFocus->leaveData);
                oldFocus = 0;
            }
            if(s->enter(s, d.x, d.y, s->enterData) && s->leave)
                // We have a new focused surface.
                d.focusSurface = s;
            else
                // "s" did not take focus.
                continue;
            break;
        }
    }

    if(oldFocus && d.focusSurface != oldFocus) {
        DASSERT(oldFocus->leave);
        oldFocus->leave(oldFocus, oldFocus->leaveData);
    }
}

void DoMotion(struct PnSurface *s) {

    DASSERT(s);
    for(; s; s = s->parent) {
        DASSERT(!s->culled);
        if(s->motion && s->motion(s, d.x, d.y, s->motionData))
            break;
    }
}
