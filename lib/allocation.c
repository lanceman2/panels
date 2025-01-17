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

    switch(s->gravity) {
        case PnGravity_None:
            DASSERT(!s->firstChild);
            break;
        case PnGravity_One:
            DASSERT((!s->firstChild && !s->lastChild)
                    || s->firstChild == s->lastChild);
            if(!s->firstChild || !s->firstChild->showing)
                break;
            // Same as PnGravity_LR and PnGravity_RL but with one in child
            // list.
        case PnGravity_LR:
        case PnGravity_RL:
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
        case PnGravity_BT:
        case PnGravity_TB:
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
}

static inline
void GrowWidth(struct PnSurface *s, uint32_t width) {

    s->allocation.width = width;

WARN("MORE CODE HERE");
}

static inline
void ShrinkWidth(struct PnSurface *s, uint32_t width) {

    s->allocation.width = width;

WARN("MORE CODE HERE");
}

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


static inline
void GetXY(struct PnSurface *s) {
WARN("MORE CODE HERE");

}


// Get the positions and sizes of all the widgets in the window.
//
void GetWidgetAllocations(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->surface.type == PnSurfaceType_toplevel ||
            win->surface.type == PnSurfaceType_popup);
WARN();

    if(!win->surface.firstChild) return;

    // The size of the window is set in the users call to
    // PnWindow_create(), from a xdg_toplevel configure (resize) or
    // xdg_popup configure (resize) event.  So, we assume that
    // win->surface.allocation.width and win->surface.allocation.height
    // have been set at this time.

    uint32_t width = win->surface.allocation.width;
    uint32_t height = win->surface.allocation.height;

    AddRequestedSizes(&win->surface);

    if(width > win->surface.allocation.width)
        GrowWidth(&win->surface, width);
    else if(width < win->surface.allocation.width)
        ShrinkWidth(&win->surface, width);

    if(height > win->surface.allocation.height)
        GrowHeight(&win->surface, height);
    else if(height < win->surface.allocation.height)
        ShrinkHeight(&win->surface, height);

    // Get x and y positions:
    GetXY(&win->surface);

    win->needAllocate = false;
}
