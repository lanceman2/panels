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



void DrawAll(struct PnWindow *win, struct PnBuffer *buffer) {

    DASSERT(win);
    DASSERT(win->wl_surface);
    DASSERT(win->needDraw || buffer);
    DASSERT(win->dqRead);
    DASSERT(win->dqWrite);
    DASSERT(win->dqRead != win->dqWrite);
    // The read queue should be empty
    DASSERT(!win->dqRead->first);
    DASSERT(!win->dqRead->last);

    if(win->needAllocate)
        GetWidgetAllocations(win);


    // GetNextBuffer() can reallocate the buffer if the width or height
    // passed here is different from the width and height of the buffer it
    // is getting.
    if(!buffer)
        buffer = GetNextBuffer(win,
                win->surface.allocation.width,
                win->surface.allocation.height);

    if(!buffer) {
        // I think this is okay.  The wayland compositor is just a little
        // busy now.  I think we will get this done later from a wl_buffer
        // release event callback; see buffer.c.
        if(win->needAllocate)
            win->needAllocate = false;
        return;
    }

    win->needDraw = false;

    if(win->dqWrite->first) {
        // We no longer need to save the past queued draw requests given
        // we will now draw all widgets in this window; at least the ones
        // that are showing.  There's no reason to switch the read and
        // write draw queues, given we are not dequeuing as we draw.
        DASSERT(win->dqWrite->last);
        FlushDrawQueue(win);
        // New draw queue requests will be added to this now empty write
        // queue.
    } else {
        DASSERT(!win->dqWrite->last);
    }

    DASSERT(win->surface.allocation.width == buffer->width);
    DASSERT(win->surface.allocation.height == buffer->height);

    pnSurface_draw(&win->surface, buffer);

    if(win->needAllocate)
        win->needAllocate = false;

    d.surface_damage_func(win->wl_surface, 0, 0,
            buffer->width, buffer->height);

    wl_surface_attach(win->wl_surface, buffer->wl_buffer, 0, 0);

    // I think this, wl_surface_commit(), needs to be last.  The order of
    // the other functions may not matter much.
    wl_surface_commit(win->wl_surface);
}


static void configure(struct PnWindow *win,
	    struct xdg_surface *xdg_surface, uint32_t serial) {

    DASSERT(win);
    DASSERT(win->xdg_surface);
    DASSERT(win->xdg_surface == xdg_surface);

    //DSPEW("Got xdg_surface_configure events serial=%"
    //            PRIu32 " for window=%p", serial, win);

    xdg_surface_ack_configure(win->xdg_surface, serial);

    if(!win->surface.allocation.width || !win->surface.allocation.height) {
        // This is the first call to draw real pixels.
        win->needAllocate = true;
        DrawAll(win, 0);
    }

    if(win->needDraw)
        // We need to wait for the wayland compositor to tell us we can
        // draw.
        _pnWindow_addCallback(win);
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


static void frame_new(struct PnWindow *win,
        struct wl_callback* cb, uint32_t a) {

    DASSERT(win);
    // So the question I have is: Can we get a callback event (this
    // called) after we destroy the wl_callback (before this is called)?
    // The wayland client API could do whatever it likes, it knows when we
    // destroy the wl_callback, but the server being in an asynchronous
    // process could send this event after we destroy it (but the wayland
    // client API in this process could just ignore it knowing that we
    // destroyed it).  Oh boy.
    //
    // I wonder, will we hit any of these assertions.
    DASSERT(cb);
    DASSERT(win->wl_callback == cb);
    // There should be draw requests in the write draw queue.

    wl_callback_destroy(cb);
    win->wl_callback = 0;

    if(win->needDraw)
        DrawAll(win, 0);
    else if(win->dqWrite->first)
        DrawFromQueue(win);

    if(win->haveDrawn && win->haveDrawn < 2)
        ++win->haveDrawn;
}


static struct wl_callback_listener callback_listener = {
    .done = (void (*)(void* data, struct wl_callback* cb, uint32_t a))
        frame_new
};


static inline
struct PnWindow *_pnWindow_createFull(struct PnWindow *parent,
        uint32_t w, uint32_t h, int32_t x, int32_t y,
        enum PnLayout layout, enum PnAlign align,
        enum PnExpand expand,
        uint32_t numColumns, uint32_t numRows) {
    DASSERT(layout != PnLayout_None || (w && h));

    if(layout == PnLayout_None && !(w && h)) {
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

    win->buffer.pixels = MAP_FAILED;
    win->buffer.fd = -1;

    win->dqWrite = win->drawQueues;
    win->dqRead = win->drawQueues + 1;

    * (enum PnExpand *) &win->surface.expand = expand;
    win->surface.layout = layout;
    win->surface.align = align;
    win->surface.backgroundColor = PN_WINDOW_BGCOLOR;
    win->needDraw = true;
    win->surface.window = win;
    // Default for windows so that user build the window before showing
    // it.
    win->surface.hidden = true;

    InitSurface(&win->surface, numColumns, numRows, 0, 0);

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

    return win;

fail:

    pnWindow_destroy(win);

    return 0;
}

struct PnWindow *pnWindow_createAsGrid(struct PnWindow *parent,
        uint32_t w, uint32_t h, int32_t x, int32_t y,
        enum PnAlign align, enum PnExpand expand,
        uint32_t numColumns, uint32_t numRows) {
    return _pnWindow_createFull(parent, w, h, x, y,
            PnLayout_Grid, align, expand, numColumns, numRows);
}

struct PnWindow *pnWindow_create(struct PnWindow *parent,
        uint32_t w, uint32_t h, int32_t x, int32_t y,
        enum PnLayout layout, enum PnAlign align,
        enum PnExpand expand) {
    ASSERT(layout != PnLayout_Grid);
    return _pnWindow_createFull(parent, w, h, x, y, layout, align,
            expand, -1/*numColumns*/, -1/*numRows*/);
}


void pnWindow_show(struct PnWindow *win, bool show) {

    DASSERT(win);
    DASSERT(win->wl_surface);
    ASSERT(show, "WRITE MORE CODE FOR THIS CASE");

    // During and after pnWindow_create() non of the wl_suface (and other
    // wayland client window objects) callbacks are called yet.

    // Make it be one of two values.
    show = show ? true: false;

    if(win->surface.hidden == !show)
        // No change.
        //
        // TODO: Nothing to do unless we can pop up the window that is
        // hidden by the desktop window manager.
        return;

    win->surface.hidden = !show;

    // The win->surface.culled variable is not used in windows.

    if(show && !win->buffer.wl_buffer)
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
        win->destroy(win, win->destroyData);

    // If there is state in the display that refers to this surface
    // (window) take care to not refer to it.  Like if this window surface
    // had focus for example.
    RemoveSurfaceFromDisplay((void *) win);

    // Destroy all child widgets in the list from win->surface.l.firstChild
    // or win->surface.g.child[][].
    DestroySurfaceChildren(&win->surface);

    DestroySurface(&win->surface);

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

    // Make sure buffer is freed up.
    FreeBuffer(&win->buffer);


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

void pnWindow_setDestroy(struct PnWindow *win,
        void (*destroy)(struct PnWindow *window, void *userData),
        void *userData) {

    DASSERT(win);

    win->destroy = destroy;
    win->destroyData = userData;
}


bool _pnWindow_addCallback(struct PnWindow *win) {

    DASSERT(win);

    if(win->wl_callback) return false;

    win->wl_callback = wl_surface_frame(win->wl_surface);
    if(!win->wl_callback) {
        ERROR("wl_surface_frame() failed");
        return true; // failure
    }
    if(wl_callback_add_listener(win->wl_callback,
                    &callback_listener, win)) {
        ERROR("wl_callback_add_listener() failed");
        wl_callback_destroy(win->wl_callback);
        win->wl_callback = 0;
        return true; // failure
    }

    wl_surface_commit(win->wl_surface);

    return false;
}


// This is not that show/hide stuff; though it's related.
//
bool pnWindow_isDrawn(struct PnWindow *win) {
    DASSERT(win);

    if(win->haveDrawn >= 2)
        return true;

    // We request a wl_callback.  We are just assuming that if the
    // wayland compositor releases a frame via wl_callback that the
    // window is showing.  That may not be true, but it's all I do.
    // This may be the best we can do to answer the question: "Is
    // the window being shown to the user now?"
    _pnWindow_addCallback(win);
    win->haveDrawn = 1;
    return false;
}

void pnWindow_isDrawnReset(struct PnWindow *win) {
    DASSERT(win);
    _pnWindow_addCallback(win);
    win->haveDrawn = 1;
}
