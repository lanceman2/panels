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
#include "../include/panels_drawingUtils.h"


static void toplevel_configure(struct PnWindow *win,
		struct xdg_toplevel *xdg_toplevel,
                int32_t w, int32_t h,
		struct wl_array *state) {
    DASSERT(win);
    DASSERT(xdg_toplevel);
    DASSERT(xdg_toplevel == win->toplevel.xdg_toplevel);

    //DSPEW("w,h=%" PRIi32",%" PRIi32, w, h);

    if(w <= 0 || h <= 0) return;

    win->surface.allocation.width = w;
    win->surface.allocation.height = h;
}

static void xdg_toplevel_handle_close(struct PnWindow *win,
		struct xdg_toplevel *xdg_toplevel) {

    DASSERT(win);
    DASSERT(xdg_toplevel);
    DASSERT(win->wl_surface);
    DASSERT(xdg_toplevel == win->toplevel.xdg_toplevel);

    pnWindow_destroy(win);
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = (void *) toplevel_configure,
	.close = (void *) xdg_toplevel_handle_close,
};


bool InitToplevel(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(!win->toplevel.xdg_toplevel);

    win->toplevel.xdg_toplevel = xdg_surface_get_toplevel(win->xdg_surface);
    if(!win->toplevel.xdg_toplevel) {
        ERROR("xdg_surface_get_toplevel() failed");
        return true;
    }
    //
    if(xdg_toplevel_add_listener(win->toplevel.xdg_toplevel,
                &xdg_toplevel_listener, win)) {
        ERROR("xdg_toplevel_add_listener(,,) failed");
        return true;
    }

    return false; // return false for success
}
