#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>
#include <wayland-client.h>

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "../include/panels.h"
#include "debug.h"
#include  "display.h"


// Popup windows (widget surfaces) get/have a pointer focus grab as part
// of the wayland protocol.  Popups are special/temporary.

// From:
// https://wayland-client-d.dpldocs.info/wayland.client.protocol.wl_shell_surface_set_popup.html
//
// DOC QUOTE: A popup surface is a transient surface with an added pointer
// grab.
//
// DOC QUOTE: The x and y arguments specify the location of the upper left
// corner of the surface relative to the upper left corner of the parent
// surface, in surface-local coordinates.


static void configure(void *data,
        struct xdg_popup *xdg_popup,
	int32_t x, int32_t y,
	int32_t width, int32_t height) {

    DASSERT(data);
    struct PnWindow *win = data;
    DASSERT(xdg_popup);
    DASSERT(xdg_popup == win->popup.xdg_popup);
    DASSERT(!win->widget.hidden);

/*
    struct PnAllocation a;
    pnWidget_getAllocation(&win->widget, &a);
    if(win->popup.x != x || win->popup.y != y
            || a.width != width || a.height != height) {
        // This is not a problem any more.
        WARN("tried for: x,y=%" PRIi32 ",%" PRIi32
                " and width,height=%" PRIi32 ",%" PRIi32
            " got: x,y=%" PRIi32 ",%" PRIi32
                " and width,height=%" PRIu32 ",%" PRIu32,
                win->popup.x, win->popup.y, a.width, a.height,
                x, y, width, height);
    }
*/
}


static void popup_done(void *data, struct xdg_popup *xdg_popup) {

    struct PnWindow *win = data;

    DSPEW();
    pnWidget_destroy(&win->widget);
    DSPEW();
}

static void repositioned(void *data,
        struct xdg_popup *xdg_popup,
	uint32_t token) {

    DSPEW();
}


static struct xdg_popup_listener xdg_popup_listener = {

    .configure = configure,
    .popup_done = popup_done,
    .repositioned = repositioned
};


// This just destroys the popup.xdg_positioner and the popup.xdg_popup
// part of the PnWindow.
//
void DestroyPopup(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->widget.type & POPUP);
 
    if(win->popup.xdg_popup) {
        xdg_popup_destroy(win->popup.xdg_popup);
        win->popup.xdg_popup = 0;
    }
    if(win->popup.xdg_positioner) {
        xdg_positioner_destroy(win->popup.xdg_positioner);
        win->popup.xdg_positioner = 0;
    }
}


bool InitPopup(struct PnWindow *win,
        int32_t w, int32_t h,
        int32_t x, int32_t y) {

    DASSERT(win);
    DASSERT(win->widget.type & POPUP);
    struct PnWindow *parent = win->popup.parent;
    DASSERT(parent);

    DASSERT(parent->widget.type & TOPLEVEL);
    DASSERT(!win->popup.xdg_positioner);
    DASSERT(!win->popup.xdg_popup);
    DASSERT(w > 0);
    DASSERT(h > 0);

    if(!win->wl_surface && InitWaylandWindow(win))
        return true; // true ==> fail

    win->popup.xdg_positioner =
        xdg_wm_base_create_positioner(d.xdg_wm_base);
    if(!win->popup.xdg_positioner) {
        ERROR("xdg_wm_base_create_positioner() failed");
        return true; // true ==> fail
    }

    xdg_positioner_set_size(win->popup.xdg_positioner, w, h);
    xdg_positioner_set_anchor_rect(win->popup.xdg_positioner, x, y, w, h);

    win->popup.xdg_popup = xdg_surface_get_popup(win->xdg_surface,
            parent->xdg_surface, win->popup.xdg_positioner);
    if(!win->popup.xdg_popup) {
        ERROR("xdg_surface_get_popup() failed");
        return true; // true ==> fail
    }

    if(xdg_popup_add_listener(win->popup.xdg_popup,
                &xdg_popup_listener, win)) {
        ERROR("xdg_positioner_add_listener(,,) failed");
        return true; // true ==> fail
    }

    return false; // return false for success
}


// TODO: It appears that once a Wayland popup window is shown
// the way to "hide it" is to destroy it.  I could be totally
// wrong about this, but this will at least give the appearance
// of hiding the popup, and not destroy all the non-wayland object
// stuff in this libpanels popup window thingy.
//
void pnPopup_hide(struct PnWidget *w) {
    DASSERT(w);
    ASSERT(w->type & POPUP);
    struct PnWindow *win = (void *) w;

    // Clean up stuff in reverse order that stuff was created.
    /////////////////////////////////////////////////////////////////

    if(win->wl_callback) {
        wl_callback_destroy(win->wl_callback);
        win->wl_callback = 0;
    }

    // Make sure buffer is freed up and reset.
    if(win->buffer.wl_buffer)
        FreeBuffer(&win->buffer);

    DestroyPopup(win);

    if(win->xdg_surface) {
        xdg_surface_destroy(win->xdg_surface);
        win->xdg_surface = 0;
    }

    if(win->wl_surface) {
        wl_surface_destroy(win->wl_surface);
        win->wl_surface = 0;
    }
}


// Return true on failure.
//
bool pnPopup_show(struct PnWidget *w, int32_t x, int32_t y) {

    DASSERT(w);
    ASSERT(w->type & POPUP);
    struct PnWindow *win = (void *) w;

    if(!win->popup.xdg_popup) {
        _pnWidget_getAllocations(w);
        DASSERT(w->allocation.width);
        DASSERT(w->allocation.height);
        // Now, we should have the window size information needed
        // for this:
        if(InitPopup(win,
                w->allocation.width, w->allocation.height, x, y))
            return true; // fail
    }

    DASSERT(win->wl_surface);

    pnWidget_queueDraw(w, false);
    return false; // success
}



