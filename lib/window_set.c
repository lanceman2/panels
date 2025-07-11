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


void pnWindow_setShrinkWrapped(struct PnWidget *widget) {
    DASSERT(widget);
    ASSERT(widget->type & TOPLEVEL, "Not a toplevel window");
    struct PnWindow *win = (void *) widget;
    DASSERT(win->toplevel.xdg_toplevel);

    widget->allocation.width = 0;
    widget->allocation.height = 0;
    // Save the old preferredWidth and preferredHeight.
    uint32_t w = win->preferredWidth;
    uint32_t h = win->preferredHeight;
    win->preferredWidth = 0;
    win->preferredHeight = 0;
    _pnWidget_getAllocations(widget);
    // Restore the old preferredWidth and preferredHeight.
    win->preferredWidth = w;
    win->preferredHeight = h;

    pnWidget_queueDraw(widget, false);
}


void pnWindow_setDestroy(struct PnWidget *w,
        void (*destroy)(struct PnWidget *window, void *userData),
        void *userData) {

    DASSERT(w);
    ASSERT((w->type & TOPLEVEL) || (w->type & POPUP));
    struct PnWindow *win = (void *) w;
    DASSERT(win);

    win->destroy = destroy;
    win->destroyData = userData;
}


void pnWindow_setMinimized(struct PnWidget *win) {
    DASSERT(win);
    ASSERT(win->type & TOPLEVEL, "Not a toplevel window");
    DASSERT(((struct PnWindow *)win)->toplevel.xdg_toplevel);
    xdg_toplevel_set_minimized(((struct PnWindow *)win)
            ->toplevel.xdg_toplevel);
}

void pnWindow_setMaximized(struct PnWidget *win) {
    DASSERT(win);
    ASSERT(win->type & TOPLEVEL, "Not a toplevel window");
    DASSERT(((struct PnWindow *)win)->toplevel.xdg_toplevel);
    xdg_toplevel_set_maximized(((struct PnWindow *)win)
            ->toplevel.xdg_toplevel);
}

void pnWindow_unsetMaximized(struct PnWidget *win) {
    DASSERT(win);
    ASSERT(win->type & TOPLEVEL, "Not a toplevel window");
    DASSERT(((struct PnWindow *)win)->toplevel.xdg_toplevel);
    xdg_toplevel_unset_maximized(((struct PnWindow *)win)
            ->toplevel.xdg_toplevel);
}

void pnWindow_setFullscreen(struct PnWidget *win) {
    DASSERT(win);
    ASSERT(win->type & TOPLEVEL, "Not a toplevel window");
    DASSERT(((struct PnWindow *)win)->toplevel.xdg_toplevel);
    xdg_toplevel_set_fullscreen(((struct PnWindow *)win)
            ->toplevel.xdg_toplevel, 0);
}

void pnWindow_unsetFullscreen(struct PnWidget *win) {
    DASSERT(win);
    ASSERT(win->type & TOPLEVEL, "Not a toplevel window");
    DASSERT(((struct PnWindow *)win)->toplevel.xdg_toplevel);
    xdg_toplevel_unset_fullscreen(((struct PnWindow *)win)
            ->toplevel.xdg_toplevel);
}
