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

// Find d.x, d.y, and d.pointerSurface which is the x,y position and
// surface (widget) that contains that position.
//
// The mouse pointer window, d.pointerWindow, must be known.
//
void GetSurfaceWithXY(const struct PnWindow *win,
        wl_fixed_t x,  wl_fixed_t y) {

    DASSERT(d.pointerWindow);

    // Maybe we round up wrong...??  We fix it below.
    d.x = (wl_fixed_to_double(x) + 0.5);
    d.y = (wl_fixed_to_double(y) + 0.5);

    // Sometimes the values are just shit.

    // Hit these, WTF.
    //DASSERT(d.x < (uint32_t) -1);
    //DASSERT(d.y < (uint32_t) -1);

    if(d.x >= win->surface.allocation.width) {
        // I don't trust the wayland compositor to give me good numbers.
        // I've seen values of x larger than the width of the window.
        // Maybe we round up wrong...
        // I've seen this assertion activated, so what the hell:
        // why is the wayland compositor sending us position values that
        // are not in the window.
        //DASSERT(d.x - win->surface.allocation.width < 3);
        // x values go from 0 to width -1.
        d.x = win->surface.allocation.width - 1;
    }
    if(d.y >= d.pointerWindow->surface.allocation.height) {
        // I don't trust the wayland compositor to give me good numbers.
        // I've seen values of y larger than the height of the window.
        //DASSERT(d.y - d.pointerWindow->surface.allocation.height < 3);
        // y values go from 0 to height -1.
        d.y = d.pointerWindow->surface.allocation.height - 1;
    }

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

    if(d.pointerSurface->firstChild)
        // See if it's in a deeper child surface.
        d.pointerSurface = FindSurface(d.pointerWindow, d.pointerSurface,
                d.x, d.y);

//WARN("Got widget=%p at %" PRIu32 ",%" PRIu32, d.pointerSurface, d.x, d.y);
}

// Find the next focused widget (focusSurface), switch between the last
// focused widget and the next, calling next focused widget enter callback
// and then the lasted focused widget leave callback.
//
void DoEnterAndLeave(void) {

    DASSERT(d.pointerSurface);

    for(struct PnSurface *s = d.pointerSurface; s; s = s->parent) {
        DASSERT(!s->culled);
        if(d.focusSurface == s)
            // This surface "s" already has focus.
            break;
        if(s->enter && s->enter(s, d.x, d.y, s->enterData)) {
            if(d.focusSurface && d.focusSurface->leave)
                d.focusSurface->leave(d.focusSurface,
                        d.focusSurface->leaveData);
            d.focusSurface = s;
            break;
        }
    }
}
