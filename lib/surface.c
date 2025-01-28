#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include  "display.h"

#include "../include/panels_drawingUtils.h"


void GetSurfaceDamageFunction(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->wl_surface);

    if(!d.surface_damage_func) {
        // WTF (what the fuck): Why do they change the names of functions
        // in an API, and still keep both accessible even when one is
        // broken.
        //
        // This may not be setting the correct function needed to see what
        // surface_damage_func should be set to.  We just make it work on
        // the current system (the deprecated function
        // wl_surface_damage()).  When this fails you'll see (from the
        // stderr tty spew):
        // 
        //  wl_display@1: error 1: invalid method 9 (since 1 < 4), object wl_surface@3
        //
        // ...or something like that.
        //
        // Given it took me a month (at least) to find this (not so great
        // fix) we're putting lots of comment-age here.  I had a hard time
        // finding the point in the code where that broke it after it
        // keeps running with no error some time after.
        //
        // This one may be correct. We would have hoped that
        // wl_proxy_get_version() would have used argument prototype const
        // struct wl_proxy * so we do not change memory at/in
        // win->wl_surface, but alas they do not:
        uint32_t version = wl_proxy_get_version(
                (struct wl_proxy *) win->wl_surface);
        //
        // These two may be the wrong function to use (I leave here for
        // the record):
        //uint32_t version = xdg_toplevel_get_version(win->xdg_toplevel);
        //uint32_t version = xdg_surface_get_version(win->xdg_surface);

        switch(version) {
            case 1:
                // Older deprecated version (see:
                // https://wayland-book.com/surfaces-in-depth/damaging-surfaces.html)
                DSPEW("Using deprecated function wl_surface_damage()"
                        " version=%" PRIu32, version);
                d.surface_damage_func = wl_surface_damage;
                break;

            case 4: // We saw a version 4 in another compiled program that used
                    // wl_surface_damage_buffer()
            default:
                // newer version:
                DSPEW("Using newer function wl_surface_damage_buffer() "
                        "version=%" PRIu32, version);
                d.surface_damage_func = wl_surface_damage_buffer;
        }
    }
}


// Add the child, s, as the last child.
static inline
void AddChildSurface(struct PnSurface *parent, struct PnSurface *s) {

    DASSERT(parent);
    DASSERT(s);
    DASSERT(s->parent);
    DASSERT(s->parent == parent);
    DASSERT(!s->firstChild);
    DASSERT(!s->lastChild);
    DASSERT(!s->nextSibling);
    DASSERT(!s->prevSibling);

    if(parent->firstChild) {
        DASSERT(parent->lastChild);
        DASSERT(!parent->lastChild->nextSibling);
        DASSERT(!parent->firstChild->prevSibling);

        s->prevSibling = parent->lastChild;
        parent->lastChild->nextSibling = s;
    } else {
        DASSERT(!parent->lastChild);
        parent->firstChild = s;
    }
    parent->lastChild = s;

}

static inline
void RemoveChildSurface(struct PnSurface *parent, struct PnSurface *s) {

    DASSERT(s);
    DASSERT(parent);
    DASSERT(s->parent == parent);

    if(s->nextSibling) {
        DASSERT(parent->lastChild != s);
        s->nextSibling->prevSibling = s->prevSibling;
    } else {
        DASSERT(parent->lastChild == s);
        parent->lastChild = s->prevSibling;
    }

    if(s->prevSibling) {
        DASSERT(parent->firstChild != s);
        s->prevSibling->nextSibling = s->nextSibling;
        s->prevSibling = 0;
    } else {
        DASSERT(parent->firstChild == s);
        parent->firstChild = s->nextSibling;
    }

    s->nextSibling = 0;
    s->parent = 0;
}


// This function calls itself.
//
void pnSurface_draw(struct PnSurface *s, struct PnBuffer *buffer) {

    // All parent surfaces, including the window, could have nothing to draw
    // if their children widgets completely cover them.
    if(s->noDrawing) goto drawChildren;

    if(!s->draw)
        pn_drawFilledRectangle(buffer->pixels,
                s->allocation.x, s->allocation.y, 
                s->allocation.width, s->allocation.height,
                buffer->stride,
                s->backgroundColor /*color in ARGB*/);
    else
        s->draw(s,
                buffer->pixels + s->allocation.y * buffer->stride +
                s->allocation.x * PN_PIXEL_SIZE,
                s->allocation.width, s->allocation.height,
                buffer->stride, s->user_data);

drawChildren:
    // Now draw children (widgets).
    for(struct PnSurface *c = s->firstChild; c; c = c->nextSibling)
        if(!c->culled)
            pnSurface_draw(c, buffer);
}



// Return false on success.
//
bool InitSurface(struct PnSurface *s) {

    DASSERT(s);
    if(s->type != PnSurfaceType_widget) {
        DASSERT(!s->parent);
        return false;
    }
    DASSERT(s->parent);

    AddChildSurface(s->parent, s);

    return false;
}


void DestroySurface(struct PnSurface *s) {

    DASSERT(s);
    if(s->type != PnSurfaceType_widget) {
        DASSERT(!s->parent);
        return;
    }
    DASSERT(s->parent);

    RemoveChildSurface(s->parent, s);
}


void pnSurface_setBackgroundColor(
        struct PnSurface *s, uint32_t argbColor) {
    s->backgroundColor = argbColor;
}


