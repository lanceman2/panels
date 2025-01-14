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


// We need to make it clear that this is a draw (render) caused by a
// wayland compositor configure event.  So, it was not caused by
// the API users code telling us to draw.
//
static void ConfigureRender(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->wl_surface);

    // GetNextBuffer() can reallocate the buffer
    // if the width or height passed here is different than the
    // width and height of the buffer it is getting.
    struct PnBuffer *buffer = GetNextBuffer(win,
            win->surface.allocation.width,
            win->surface.allocation.height);
    if(!buffer)
        // I think this is okay.  The wayland compositor is just a little
        // busy now.  I think will get this done later.
        return;

    pn_drawFilledRectangle(buffer->pixels/*surface starting pixel*/,
        0/*x*/, 0/*y*/, buffer->width, buffer->height,
        buffer->width * PN_PIXEL_SIZE/*stride*/,
        0x0AAFAA00 /*color in ARGB*/);

    wl_surface_attach(win->wl_surface, buffer->wl_buffer, 0, 0);

    d.surface_damage_func(win->wl_surface, 0, 0,
            buffer->width, buffer->height);

    wl_surface_commit(win->wl_surface);
    buffer->busy = true;
}


static void xdg_surface_handle_configure(struct PnWindow *win,
	    struct xdg_surface *xdg_surface, uint32_t serial) {

    DASSERT(win);
    DASSERT(win->xdg_surface);
    DASSERT(win->xdg_surface == xdg_surface);

    //DSPEW("Got xdg_surface_configure events serial=%"
    //            PRIu32 " for window=%p", serial, win);

    xdg_surface_ack_configure(win->xdg_surface, serial);

    ConfigureRender(win);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = (void *) xdg_surface_handle_configure,
};

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


// Add a window, win, to the window list at the last.
//
static inline void AddWindow(struct PnWindow *win,
        struct PnWindow *last, struct PnWindow **lastPtr) {

    DASSERT(d.wl_display);
    DASSERT(!win->next);
    DASSERT(!win->prev);

    if(last) {
        DASSERT(!last->next);
        last->next = win;
        win->prev = last;
    }

    *lastPtr = win;
}

// Remove a window, win, from the window list.
//
static inline void RemoveWindow(struct PnWindow *win,
        struct PnWindow *last, struct PnWindow **lastPtr) {

    DASSERT(d.wl_display);
    DASSERT(last);

    if(win->next) {
        DASSERT(last != win);
        win->next->prev = win->prev;
    } else {
        DASSERT(last == win);
        *lastPtr = win->prev;
    }
    if(win->prev)
        win->prev->next = win->next;
}


struct PnWindow *pnWindow_create(uint32_t w, uint32_t h,
        enum PnGravity gravity) {

    if(CheckDisplay()) return 0;

    struct PnWindow *win = calloc(1, sizeof(*win));
    ASSERT(win, "calloc(1,%zu) failed", sizeof(*win));

    win->surface.type = PnSurfaceType_toplevel;
    DASSERT(win->surface.type < PnSurfaceType_widget);
    win->buffer[0].pixels = MAP_FAILED;
    win->buffer[1].pixels = MAP_FAILED;
    win->buffer[0].fd = -1;
    win->buffer[1].fd = -1;
    win->surface.gravity = gravity;

    AddWindow(win, d.windows, &d.windows);

    win->wl_surface = wl_compositor_create_surface(d.wl_compositor);
    if(!win->wl_surface) {
        ERROR("wl_compositor_create_surface() failed");
        goto fail;
    }

    wl_surface_set_user_data(win->wl_surface, win);

    GetSurfaceDamageFunction(win);

    win->xdg_surface = xdg_wm_base_get_xdg_surface(
            d.xdg_wm_base, win->wl_surface);

    if(!win->xdg_surface) {
        ERROR("xdg_wm_base_get_xdg_surface() failed");
        goto fail;
    }
    //
    if(xdg_surface_add_listener(win->xdg_surface,
                &xdg_surface_listener, win)) {
        ERROR("xdg_surface_add_listener(,,) failed");
        goto fail;
    }

    // Now create wayland toplevel specific stuff.
    //
    win->toplevel.xdg_toplevel = xdg_surface_get_toplevel(win->xdg_surface);
    if(!win->toplevel.xdg_toplevel) {
        ERROR("xdg_surface_get_toplevel() failed");
        goto fail;
    }
    //
    if(xdg_toplevel_add_listener(win->toplevel.xdg_toplevel,
                &xdg_toplevel_listener, win)) {
        ERROR("xdg_toplevel_add_listener(,,) failed");
        goto fail;
    }


    if(d.zxdg_decoration_manager) {
        // Let the compositor do window decoration management
	win->decoration =
	        zxdg_decoration_manager_v1_get_toplevel_decoration(
		        d.zxdg_decoration_manager, win->toplevel.xdg_toplevel);
        if(!win->decoration) {
            ERROR("zxdg_decoration_manager_v1_get_toplevel_decoration() failed");
            goto fail;
        }
	zxdg_toplevel_decoration_v1_set_mode(win->decoration,
		ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }

    // TODO: Remove win->width and win->height??
    //
    // This is how we C casting to change const variables:
    //*((uint32_t *) &win->width) = w;
    //*((uint32_t *) &win->height) = h;

    win->surface.allocation.width = w;
    win->surface.allocation.height = h;

    return win;

fail:

    pnWindow_destroy(win);

    return 0;
}


void pnWindow_show(struct PnWindow *win, bool show) {

    DASSERT(win);
    DASSERT(win->wl_surface);
    ASSERT(show, "WRITE MORE CODE FOR THIS CASE");

    // During and after pnWindow_create() non of the wl_suface (and other
    // wayland client window objects) callbacks are called yet.

    if(win->showing) return;

    // void wl_surface_commit().
    wl_surface_commit(win->wl_surface);

    win->showing = true;

    // this will eventually cause wl_display_dispatch() to call
    // toplevel_configure() for this window, win.
    // and then xdg_surface_handle_configure()
}



void pnWindow_destroy(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(d.wl_display);
    DASSERT(win->surface.type < PnSurfaceType_widget);

    if(win->destroy)
        win->destroy(win, win->destroyUserData);

    RemoveWindow(win, d.windows, &d.windows);

    // Clean up stuff in reverse order that stuff was created.

    if(win->wl_callback)
        wl_callback_destroy(win->wl_callback);

    // Make sure both buffers are freed up.
    FreeBuffer(win->buffer);
    FreeBuffer(win->buffer + 1);


    if(win->decoration)
        zxdg_toplevel_decoration_v1_destroy(win->decoration);

    switch(win->surface.type) {
        case PnSurfaceType_toplevel:
            if(win->toplevel.xdg_toplevel)
                xdg_toplevel_destroy(win->toplevel.xdg_toplevel);
            break;
        case PnSurfaceType_popup:
            break;
        default:
    }

    if(win->xdg_surface)
        xdg_surface_destroy(win->xdg_surface);

    if(win->wl_surface)
        wl_surface_destroy(win->wl_surface);



    DZMEM(win, sizeof(*win));
    free(win);
}


void pnWindow_setCBDestroy(struct PnWindow *win,
        void (*destroy)(struct PnWindow *window, void *userData),
        void *userData) {

    DASSERT(win);

    win->destroy = destroy;
    win->destroyUserData = userData;
}
