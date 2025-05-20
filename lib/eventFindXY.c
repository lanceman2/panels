#define _GNU_SOURCE
#include <link.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>


#include "../include/panels.h"
#include "debug.h"
#include "display.h"


// Find the most childish surface (widget) that has the mouse pointer
// position, x and y, in it.
//
// TODO: Do we need a faster surface search data structure?  Yes.  We
// should make the searching of lists converge with O(logN) (base 2),
// using an interaction that is bisecting the iterator.  Halving the
// search pool at each iteration step.  PnLayout_Grid does this, but other
// layouts do not.  Also, I think there are even faster way than that...
//
// TODO: Maybe the widget (surface) order in the searching could be more
// optimal.
//
static
struct PnWidget *FindSurface(const struct PnWindow *win,
        struct PnWidget *s, uint32_t x, uint32_t y) {

    DASSERT(win);
    DASSERT(!s->culled);

    // Let's see if the x,y positions always make sense.
    DASSERT(x >= s->allocation.x);
    DASSERT(y >= s->allocation.y);
    DASSERT(x < s->allocation.x + s->allocation.width);
    DASSERT(y < s->allocation.y + s->allocation.height);

    // At this point in the code "s" contains the pointer position, x,y.
    // Now search the children to see if x,y is in one of them.

    struct PnWidget *c;

    switch(s->layout) {

        case PnLayout_One:
        case PnLayout_Cover:
        case PnLayout_LR:
            for(c = s->l.firstChild; c; c = c->pl.nextSibling) {
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
                if(c->l.firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        case PnLayout_RL:
            for(c = s->l.lastChild; c; c = c->pl.prevSibling) {
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
                if(c->l.firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        case PnLayout_TB:
            for(c = s->l.firstChild; c; c = c->pl.nextSibling) {
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
                if(c->l.firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        case PnLayout_BT:
            for(c = s->l.lastChild; c; c = c->pl.prevSibling) {
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
                if(c->l.firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        case PnLayout_Grid: {
            // Do a bisection search for O(2*log2N).
            //
            // TODO: Looks like a lot of overhead for small grids.
            //
            DASSERT(s->g.grid);
            struct PnWidget ***child = s->g.grid->child;
            DASSERT(child);
            DASSERT(s->g.numColumns);
            DASSERT(s->g.numRows);
            uint32_t *X = s->g.grid->x;
            DASSERT(X);
            if(X[0] > x)
                // Special case: it's in the left border.
                break;
            if(x >= X[s->g.numColumns])
                break;
            uint32_t i = s->g.numColumns/2;
            uint32_t *Y = s->g.grid->y;
            DASSERT(Y);
            if(Y[0] > y)
                // Special case: it's in the top border.
                break;
            if(y >= Y[s->g.numRows])
                break;
            uint32_t j = s->g.numRows/2;
            // Find i and j such that:
            // X[i] <= x < X[i+1] and Y[j] <= y < Y[j+1].
            if(i) {
                uint32_t step = i;
                if(step == 0) step = 1;
                while(X[i] > x || x >= X[i+1]) {
                    if(step >= 2)
                        step /= 2;
                    if(X[i] > x)
                        i -= step;
                    else
                        i += step;
                }
            }
            if(j) {
                uint32_t step = j;
                if(step == 0) step = 1;
                while(Y[j] > y || y >= Y[j+1]) {
                    if(step >= 2)
                        step /= 2;
                    if(Y[j] > y)
                        j -= step;
                    else
                        j += step;
                }

            }
            DASSERT(X[i] <= x);
            DASSERT(x < X[i+1]);
            DASSERT(Y[j] <= y);
            DASSERT(y < Y[j+1]);

            c = child[j][i];
            if(!c || c->culled) break;
            // See if child "c" has the pointer in it.

            if(c->allocation.x <= x &&
                    x < c->allocation.x + c->allocation.width &&
                    c->allocation.y <= y &&
                    y < c->allocation.y + c->allocation.height) {
                if(HaveChildren(c))
                    return FindSurface(win, c, x, y);
                return c;
            }
            break;
        }

        default:
            ASSERT(0, "Write more code");
            return 0;
    }

    return s;
}


void GetPointerSurface(void) {

    DASSERT(d.pointerWindow);
    DASSERT(d.pointerWindow->widget.allocation.x == 0);
    DASSERT(d.pointerWindow->widget.allocation.y == 0);

    bool inPointerWidget = false;

    if(d.pointerWidget)
        inPointerWidget =
            d.pointerWidget->allocation.x >= d.x && 
            d.x < d.pointerWidget->allocation.x + 
                    d.pointerWidget->allocation.width &&
            d.pointerWidget->allocation.y >= d.y && 
            d.y < d.pointerWidget->allocation.y + 
                    d.pointerWidget->allocation.height;

    if(inPointerWidget) {
        // Lets start with the mouse pointer is at the same widget.
        if(HaveChildren(d.pointerWidget))
            d.pointerWidget = FindSurface(d.pointerWindow, d.pointerWidget,
                    d.x, d.y);
        // else it's already at the most childish widget under the
        // pointer.
        return;
    }

    if(d.x < 0 || d.y < 0 ||
            d.x >= d.pointerWindow->widget.allocation.width ||
            d.y >= d.pointerWindow->widget.allocation.height) {
        // The pointer is outside the window.
        //
        // The focus widget and the pointer widget are not necessarily the
        // same, because of "pointer button grab".
        //
        // We likely have a wayland window button grab and the pointer
        // moved outside the window so we can't pick a new pointerWidget
        // since we are not pointing to anyplace in the window.
        d.pointerWindow = 0;
        return;
    }

    // It moved out from where it was. Start at the window surface.
    d.pointerWidget = FindSurface(d.pointerWindow,
            (void *) d.pointerWindow, d.x, d.y);
}


// Find d.x, d.y, and d.pointerWidget which is the x,y position and
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
#if 0
#define ERR_LEN  (5) // Have seen assertion with ERR_LEN (2)
        DASSERT(d.x >= - ERR_LEN);
        DASSERT(d.y >= - ERR_LEN);
        DASSERT(d.x < win->widget.allocation.width + ERR_LEN);
        DASSERT(d.y < win->widget.allocation.height + ERR_LEN);
#endif

        if(d.x < 0)
            d.x = 0;
        else if(d.x >= win->widget.allocation.width)
            d.x = win->widget.allocation.width - 1;
        if(d.y < 0)
            d.y = 0;
        else if(d.y >= win->widget.allocation.height)
            d.y = win->widget.allocation.height - 1;
    } else
        // The mouse pointer can send events if there is a pointer grab and in
        // that case the position made be to the left and/or above the window,
        // making d.x and/or d.y negative or larger than the width and height
        // of the window.
        if(d.x < 0 || d.y < 0 ||
                d.x >= win->widget.allocation.width ||
                d.y >= win->widget.allocation.height)
            return;

    GetPointerSurface();
}

// Find the next focused widget (focusWidget), switch between the last
// focused widget and the next, calling next focused widget enter callback
// and then the previous focused widget leave callback.
//
void DoEnterAndLeave(void) {

    DASSERT(d.pointerWidget);
    // d.pointerWidget is the surface with the mouse pointer in it.

    struct PnWidget *oldFocus = d.focusWidget;
    d.focusWidget = 0;   

    for(struct PnWidget *s = d.pointerWidget; s; s = s->parent) {
        DASSERT(!s->culled);
        if(oldFocus == s) {
            // This widget surface "s" already has focus.
            d.focusWidget = s;
            break;
        }

        if(s->enter) {
            if(oldFocus) {
                DASSERT(oldFocus->leave);
                oldFocus->leave((void *) oldFocus, oldFocus->leaveData);
                oldFocus = 0;
            }
            if(s->enter((void *) s, d.x, d.y, s->enterData) && s->leave)
                // We have a new focused widget surface.
                d.focusWidget = s;
            else
                // "s" did not take focus.
                continue;
            break;
        }
    }

    if(oldFocus && d.focusWidget != oldFocus) {
        DASSERT(oldFocus->leave);
        oldFocus->leave((void *) oldFocus, oldFocus->leaveData);
    }
}

void DoMotion(struct PnWidget *s) {

    DASSERT(s);
    for(; s; s = s->parent) {
        DASSERT(!s->culled);
        if(s->motion && s->motion((void *) s, d.x, d.y, s->motionData))
            break;
    }
}
