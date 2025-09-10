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



static
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

    struct PnWidget *w = &win->widget;

    if(w->needAllocate) {
        struct PnAllocation *a = &w->allocation;
        if(win->preferredWidth && win->preferredHeight &&
                !w->allocation.width) {
            DASSERT(!a->height);
            a->width = win->preferredWidth;
            a->height = win->preferredHeight;
         }
        _pnWidget_getAllocations(w);
    }

    // GetNextBuffer() can reallocate the buffer if the width or height
    // passed here is different from the width and height of the buffer it
    // is getting.
    if(!buffer)
        buffer = GetNextBuffer(win,
                w->allocation.width,
                w->allocation.height);
    if(!buffer) {
        // I think this is okay.  The wayland compositor is just a little
        // busy now.  I think we will get this done later from a wl_buffer
        // release event callback; see buffer.c.
        if(w->needAllocate)
            w->needAllocate = false;
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

    DASSERT(w->allocation.width == buffer->width);
    DASSERT(w->allocation.height == buffer->height);

    pnSurface_draw(w, buffer, w->needAllocate);

    if(w->needAllocate)
        w->needAllocate = false;

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

    if(!win->widget.allocation.width ||
            !win->widget.allocation.height) {
        // This is the first call to draw real pixels.
        win->widget.needAllocate = true;
        DrawAll(win, 0);
        return;
    }

    if((win->widget.type & TOPLEVEL) && d.topMenu && !d.pointerWindow)
        // We had active pop-up menus, so now hide them.
        HidePopupMenus();

    if(win->widget.type & POPUP)
        // TODO: We're not sure that this works if we show and then hide
        // and then show again.
        DrawAll(win, 0);
    else if(win->needDraw)
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

// Returns true on failure.
//
// Sets up the wl_surface and xdg_surface; which are the Wayland objects
// that are common to the toplevel window and the popup window.
//
bool InitWaylandWindow(struct PnWindow *win) {
    DASSERT(win);
    DASSERT(!win->wl_surface);
    DASSERT(!win->xdg_surface);

    win->needDraw = true;

    win->wl_surface = wl_compositor_create_surface(d.wl_compositor);
    if(!win->wl_surface) {
        ERROR("wl_compositor_create_surface() failed");
        return true;
    }

    wl_surface_set_user_data(win->wl_surface, win);

    GetSurfaceDamageFunction(win);

    win->xdg_surface = xdg_wm_base_get_xdg_surface(
            d.xdg_wm_base, win->wl_surface);

    if(!win->xdg_surface) {
        ERROR("xdg_wm_base_get_xdg_surface() failed");
        return true;
    }
    //
    if(xdg_surface_add_listener(win->xdg_surface,
                &xdg_surface_listener, win)) {
        ERROR("xdg_surface_add_listener(,,) failed");
        return true;
    }
    return false; // false => success.
}


static inline
struct PnWindow *_pnWindow_createFull(struct PnWidget *pWidget,
        uint32_t w, uint32_t h, int32_t x, int32_t y,
        enum PnLayout layout, enum PnAlign align,
        enum PnExpand expand,
        uint32_t numColumns, uint32_t numRows) {

    if(layout == PnLayout_None && !(w && h)) {
        ERROR("A window with no child widgets must have non-zero "
                "width and height, w,h=%" PRIu32 ",%" PRIu32,
                w, h);
        return 0;
    }

    if(CheckDisplay()) return 0;

    struct PnWindow *win = calloc(1, sizeof(*win));
    ASSERT(win, "calloc(1,%zu) failed", sizeof(*win));

    win->widget.reqWidth = w;
    win->widget.reqHeight = h;

    if(!pWidget) {
        // We are making a toplevel window.
        win->widget.type = TOPLEVEL;
        AddWindow(win, d.windows, &d.windows);
    } else {
        // We are making a popup window.
        win->widget.type = POPUP;

        // Let pwin be the parent toplevel window we seek.
        struct PnWindow *pwin = (void *) pWidget;

        if(pWidget->type & POPUP) {
            // We'll let the parent start as another popup, but find the
            // toplevel window from that.
            pwin = pwin->popup.parent;
            DASSERT(pwin);
            DASSERT(pwin->widget.type & TOPLEVEL);
        } else if(!(pWidget->type & TOPLEVEL)) {
            // We'll find the toplevel from a non-window widgets window.
            pwin = pWidget->window;
            DASSERT(pwin);
            if(pwin->widget.type & POPUP) {
                pwin = pwin->popup.parent;
                DASSERT(pwin);
            }
            DASSERT(pwin->widget.type & TOPLEVEL);
        }

        win->popup.parent = pwin;
        AddWindow(win, pwin->toplevel.popups, &pwin->toplevel.popups);
    }

    win->buffer.pixels = MAP_FAILED;
    win->buffer.fd = -1;

    win->dqWrite = win->drawQueues;
    win->dqRead = win->drawQueues + 1;

    * (enum PnExpand *) &win->widget.expand = expand;
    win->widget.layout = layout;
    win->widget.align = align;
    win->widget.backgroundColor = PN_WINDOW_BGCOLOR;
    win->widget.window = win;

    InitSurface(&win->widget, numColumns, numRows, 0, 0);

    if(InitWaylandWindow(win))
        goto fail;

    switch(win->widget.type & (TOPLEVEL | POPUP)) {
        case TOPLEVEL:
            if(InitToplevel(win))
                goto fail;
            break;
        case POPUP:
            // We'll make xdg_positioner and xdg_popup later, at
            // pnWindow_show().  The wayland xdg_positioner and xdg_popup
            // can't be made until we know the position and size of the
            // popup, like from the popup's children widgets that are
            // added.
            //InitPopup(win, w, h, x, y);
            win->popup.x = x;
            win->popup.y = y;
            //win->widget.allocation.width = w;
            //win->widget.allocation.height = h;
    }

    return win;

fail:

    pnWidget_destroy(&win->widget);

    return 0;
}

struct PnWidget *pnWindow_createAsGrid(struct PnWidget *parent,
        uint32_t w, uint32_t h, int32_t x, int32_t y,
        enum PnAlign align, enum PnExpand expand,
        uint32_t numColumns, uint32_t numRows) {
    //DASSERT(numColumns);
    //DASSERT(numRows);
    return (void *) _pnWindow_createFull(parent, w, h, x, y,
            PnLayout_Grid, align, expand, numColumns, numRows);
}

struct PnWidget *pnWindow_create(struct PnWidget *parent,
        uint32_t w, uint32_t h, int32_t x, int32_t y,
        enum PnLayout layout, enum PnAlign align,
        enum PnExpand expand) {
    ASSERT(layout != PnLayout_Grid);
    return (void *) _pnWindow_createFull(parent, w, h, x, y,
            layout, align, expand, -1/*numColumns*/, -1/*numRows*/);
}

// Return true on failure.
//
bool pnWindow_show(struct PnWidget *w) {

    DASSERT(w);
    ASSERT(w->type & (TOPLEVEL | POPUP));
    struct PnWindow *win = (void *) w;

    if(w->type & POPUP)
        return pnPopup_show(w, win->popup.x, win->popup.y);

    DASSERT(win->wl_surface);

    pnWidget_queueDraw(w, false);
    return false; // success
}


void _pnWindow_destroy(struct PnWidget *w) {

    DASSERT(w);
    DASSERT(d.wl_display);
    ASSERT(w->type & (TOPLEVEL | POPUP));

    struct PnWindow *win = (void *) w;

    if(win->destroy)
        win->destroy(w, win->destroyData);

#if 0
    // If there is state in the display that refers to this surface
    // (window) take care to not refer to it.  Like if this window surface
    // had focus for example.
    RemoveSurfaceFromDisplay((void *) win);

    // Destroy all child widgets in the list from win->widget.l.firstChild
    // or win->widget.g.child[][].
    DestroySurfaceChildren(&win->widget);

    DestroySurface(&win->widget);

#endif


    if(win->widget.type & POPUP) {
        // A popup
        DASSERT(win->popup.parent);
        DASSERT(win->popup.parent->widget.type & TOPLEVEL);
        // Remove this popup from the parents popup window list
        RemoveWindow(win, win->popup.parent->toplevel.popups,
                &win->popup.parent->toplevel.popups);
    } else {
        // A toplevel
        DASSERT(win->widget.type & TOPLEVEL);
        // Remove this toplevel from the displays toplevel window list
        RemoveWindow(win, d.windows, &d.windows);
        // Destroy any child popup windows that this toplevel owns.
        while(win->toplevel.popups)
            pnWidget_destroy(&win->toplevel.popups->widget);
    }


    // Clean up stuff in reverse order that stuff was created.

    if(win->wl_callback)
        wl_callback_destroy(win->wl_callback);

    // Make sure buffer is freed up.
    FreeBuffer(&win->buffer);


    switch(win->widget.type & (TOPLEVEL | POPUP)) {
        case TOPLEVEL:
            if(win->toplevel.decoration)
                zxdg_toplevel_decoration_v1_destroy(
                        win->toplevel.decoration);
            if(win->toplevel.xdg_toplevel)
                xdg_toplevel_destroy(win->toplevel.xdg_toplevel);
            break;
        case POPUP:
            // Destroy the popup.xdg_popup and popup.xdg_positioner
            // if they exist.
            DestroyPopup(win);
    }

    if(win->xdg_surface)
        xdg_surface_destroy(win->xdg_surface);

    if(win->wl_surface)
        wl_surface_destroy(win->wl_surface);


    memset(win, 0, sizeof(*win));
    free(win);
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
bool pnWindow_isDrawn(struct PnWidget *w) {

    DASSERT(w);
    ASSERT((w->type & TOPLEVEL) || (w->type & POPUP));
    struct PnWindow *win = (void *) w;

    if(win->haveDrawn >= 2)
        return true;

    // We request a wl_callback.  We are just assuming that if the Wayland
    // compositor releases a frame via wl_callback that the window is
    // showing.  That may not be true, but it's all I can do.  This may be
    // the best we can do to answer the question: "Is the window being
    // shown to the user now?"
    _pnWindow_addCallback(win);
    win->haveDrawn = 1;
    return false;
}

void pnWindow_isDrawnReset(struct PnWidget *w) {

    DASSERT(w);
    ASSERT((w->type & TOPLEVEL) || (w->type & POPUP));
    struct PnWindow *win = (void *) w;
    _pnWindow_addCallback(win);
    win->haveDrawn = 1;
}

void pnWindow_setPreferredSize(struct PnWidget *w,
        uint32_t width, uint32_t height) {
    DASSERT(w);
    ASSERT(w->type & TOPLEVEL);

    struct PnWindow *win = (void *) w;
    // Note: the value(s) zero works okay, and means that these
    // are unset.
    win->preferredWidth = width;
    win->preferredHeight = height;
}
