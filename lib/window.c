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


// We need to make it clear that this is a draw (render) caused by a
// wayland compositor configure event.  So, it was not caused by the API
// users code telling us to draw (queue a draw on a surface).
//
static inline void DrawAll(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->wl_surface);

    if(win->needAllocate)
        GetWidgetAllocations(win);

    // GetNextBuffer() can reallocate the buffer if the width or height
    // passed here is different from the width and height of the buffer it
    // is getting.
    struct PnBuffer *buffer = GetNextBuffer(win,
            win->surface.allocation.width,
            win->surface.allocation.height);
    if(!buffer)
        // I think this is okay.  The wayland compositor is just a little
        // busy now.  I think we will get this done later.
        return;

    DASSERT(win->surface.allocation.width == buffer->width);
    DASSERT(win->surface.allocation.height == buffer->height);

    pnSurface_draw(&win->surface, buffer);

    wl_surface_attach(win->wl_surface, buffer->wl_buffer, 0, 0);

    d.surface_damage_func(win->wl_surface, 0, 0,
            buffer->width, buffer->height);

    wl_surface_commit(win->wl_surface);
    buffer->busy = true;
}


static void configure(struct PnWindow *win,
	    struct xdg_surface *xdg_surface, uint32_t serial) {

    DASSERT(win);
    DASSERT(win->xdg_surface);
    DASSERT(win->xdg_surface == xdg_surface);

    //DSPEW("Got xdg_surface_configure events serial=%"
    //            PRIu32 " for window=%p", serial, win);

    xdg_surface_ack_configure(win->xdg_surface, serial);

    DrawAll(win);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = (void *) configure,
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


struct PnWindow *pnWindow_create(struct PnWindow *parent,
        uint32_t w, uint32_t h, int32_t x, int32_t y,
        enum PnGravity gravity) {

    DASSERT(gravity != PnGravity_None || (w && h));

    if(gravity == PnGravity_None && !(w && h)) {
        ERROR("A window with no child widgets must have non-zero "
                "width and height, w,h=%" PRIu32 ",%" PRIu32,
                w, h);
        return 0;
    }


    if(CheckDisplay()) return 0;

    struct PnWindow *win = calloc(1, sizeof(*win));
    ASSERT(win, "calloc(1,%zu) failed", sizeof(*win));

    // This is how we C casting to change const variables:
    *((uint32_t *) &win->surface.width) = w;
    *((uint32_t *) &win->surface.height) = h;

    if(parent) {
        // A popup
        win->popup.parent = parent;
        win->surface.type = PnSurfaceType_popup;
        AddWindow(win, parent->toplevel.popups,
                &parent->toplevel.popups);
    } else {
        // A toplevel
        win->surface.type = PnSurfaceType_toplevel;
        AddWindow(win, d.windows, &d.windows);
    }

    DASSERT(win->surface.type < PnSurfaceType_widget);

    win->buffer[0].pixels = MAP_FAILED;
    win->buffer[1].pixels = MAP_FAILED;
    win->buffer[0].fd = -1;
    win->buffer[1].fd = -1;
    win->needAllocate = true;

    win->surface.gravity = gravity;
    win->surface.borderWidth = PN_BORDER_WIDTH;
    win->surface.backgroundColor = PN_WINDOW_BGCOLOR;
    InitSurface(&win->surface);

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

    switch(win->surface.type) {
        case PnSurfaceType_toplevel:
            if(InitToplevel(win))
                goto fail;
            break;
        case PnSurfaceType_popup:
            DASSERT(parent);
            if(InitPopup(win, w, h, x, y))
                goto fail;
            break;
        default:
            ASSERT(0, "Write more code here case=%d", win->surface.type);
    }

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

    // Make it be one of two values.
    show = show ? true: false;

    if(win->surface.showing == show)
        // No change.
        //
        // TODO: Nothing to do unless we can pop up the window that is
        // hidden by the desktop window manager.
        return;

    win->surface.showing = show;

    if(win->needAllocate)
        GetWidgetAllocations(win);

    if(show && !win->buffer[0].wl_buffer)
        // This is the first surface commit.
        //
        // This has no error return.
        wl_surface_commit(win->wl_surface);
    else
        pnWindow_queueDraw(win);
}

void pnWindow_destroy(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(d.wl_display);
    DASSERT(win->surface.type < PnSurfaceType_widget);

    if(win->destroy)
        // This is called before we start destroying stuff.
        win->destroy(win, win->destroyUserData);

    // Remove all child widgets in the list from win->surface.firstChild
    while(win->surface.firstChild)
        pnWidget_destroy((void *) win->surface.firstChild);

    if(win->surface.type == PnSurfaceType_popup) {
        // A popup
        DASSERT(win->popup.parent);
        DASSERT(win->popup.parent->surface.type ==
                PnSurfaceType_toplevel);
        // Remove this popup from the parents popup window list
        RemoveWindow(win, win->popup.parent->toplevel.popups,
                &win->popup.parent->toplevel.popups);
    } else {
        // A toplevel
        DASSERT(win->surface.type == PnSurfaceType_toplevel);
        // Remove this toplevel from the displays toplevel window list
        RemoveWindow(win, d.windows, &d.windows);
        // Destroy any child popup windows that this toplevel owns.
        while(win->toplevel.popups)
            pnWindow_destroy(win->toplevel.popups);
    }

    // Clean up stuff in reverse order that stuff was created.

    if(win->wl_callback)
        wl_callback_destroy(win->wl_callback);

    // Make sure both buffers are freed up.
    FreeBuffer(win->buffer);
    FreeBuffer(win->buffer + 1);


    switch(win->surface.type) {
        case PnSurfaceType_toplevel:
            if(win->decoration)
                zxdg_toplevel_decoration_v1_destroy(
                        win->decoration);
            if(win->toplevel.xdg_toplevel)
                xdg_toplevel_destroy(win->toplevel.xdg_toplevel);
            break;
        case PnSurfaceType_popup:
            if(win->popup.xdg_popup) {
                xdg_popup_destroy(win->popup.xdg_popup);
                win->popup.xdg_popup = 0;
            }
            if(win->popup.xdg_positioner) {
                xdg_positioner_destroy(win->popup.xdg_positioner);
                win->popup.xdg_positioner = 0;
            }
            break;
        default:
    }

    if(win->xdg_surface)
        xdg_surface_destroy(win->xdg_surface);

    if(win->wl_surface)
        wl_surface_destroy(win->wl_surface);


    memset(win, 0, sizeof(*win));
    free(win);
}

void pnWindow_setCBDestroy(struct PnWindow *win,
        void (*destroy)(struct PnWindow *window, void *userData),
        void *userData) {

    DASSERT(win);

    win->destroy = destroy;
    win->destroyUserData = userData;
}

void pnWindow_queueDraw(struct PnWindow *win) {

    if(!win->surface.showing) return;

    ASSERT("WRITE MORE CODE HERE");
}
