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


static void configure(struct PnWindow *win,
		struct xdg_toplevel *xdg_toplevel,
                int32_t w, int32_t h,
		struct wl_array *state) {
    DASSERT(win);
    DASSERT(xdg_toplevel);
    DASSERT(xdg_toplevel == win->toplevel.xdg_toplevel);

    if(w <= 0 || h <= 0) return;

    // Make it like it would be if we could not change width or height
    // based on the API user window widget surface PnExpand setting.
    if(!(win->widget.expand & PnExpand_H) &&
            win->widget.allocation.width)
        w = win->widget.allocation.width;
    //
    if(!(win->widget.expand & PnExpand_V) &&
            win->widget.allocation.height)
        h = win->widget.allocation.height;

    if(win->widget.allocation.width != w ||
            win->widget.allocation.height != h) {
        win->widget.allocation.width = w;
        win->widget.allocation.height = h;
        // We have a RESIZE.
        win->needDraw = true;

//WARN("w,h=%" PRIi32",%" PRIi32, w, h);
        if(!win->widget.needAllocate)
            win->widget.needAllocate = true;
    }
}

static void close(struct PnWindow *win,
		struct xdg_toplevel *xdg_toplevel) {

    DASSERT(win);
    DASSERT(xdg_toplevel);
    DASSERT(win->wl_surface);
    DASSERT(xdg_toplevel == win->toplevel.xdg_toplevel);

    pnWindow_destroy(&win->widget);
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = (void *) configure,
    .close = (void *) close,
};


bool InitToplevel(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->widget.type == PnSurfaceType_toplevel);
    DASSERT(!win->toplevel.xdg_toplevel);

    win->toplevel.xdg_toplevel =
        xdg_surface_get_toplevel(win->xdg_surface);
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

    if(d.zxdg_decoration_manager) {
        // Let the compositor do window decoration management
	win->decoration =
	        zxdg_decoration_manager_v1_get_toplevel_decoration(
		        d.zxdg_decoration_manager,
                        win->toplevel.xdg_toplevel);
        if(!win->decoration) {
            ERROR("zxdg_decoration_manager_v1_get_toplevel_decoration()"
                    " failed");
            return true;
        }
	zxdg_toplevel_decoration_v1_set_mode(win->decoration,
		ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }


    return false; // return false for success
}
