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


// Popup windows (surfaces) get/have a pointer focus grab as part of the
// wayland protocol.  Popups are special/temporary.

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

    DSPEW();
}

static void popup_done(void *data, struct xdg_popup *xdg_popup) {

    struct PnWindow *win = data;

    DSPEW();
    pnWindow_destroy(win);
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


bool InitPopup(struct PnWindow *win,
        int32_t w, int32_t h,
        int32_t x, int32_t y) {

    DASSERT(win);
    DASSERT(win->surface.type == PnSurfaceType_popup);
    struct PnWindow *parent = win->popup.parent;
    DASSERT(parent);

    // A user could do this accidentally, I did and I'm a developer.
    ASSERT(parent->surface.type == PnSurfaceType_toplevel);

    DASSERT(!win->popup.xdg_positioner);
    DASSERT(!win->popup.xdg_popup);
    DASSERT(w > 0);
    DASSERT(h > 0);

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
