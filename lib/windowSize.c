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


void pnWindow_minimize(struct PnWidget *win) {
    DASSERT(win);
    ASSERT(win->type & TOPLEVEL);
    DASSERT(((struct PnWindow *)win)->toplevel.xdg_toplevel);
    xdg_toplevel_set_minimized(((struct PnWindow *)win)
            ->toplevel.xdg_toplevel);
}

void pnWindow_maximize(struct PnWidget *win) {
    DASSERT(win);
    ASSERT(win->type & TOPLEVEL);
    DASSERT(((struct PnWindow *)win)->toplevel.xdg_toplevel);
    xdg_toplevel_set_maximized(((struct PnWindow *)win)
            ->toplevel.xdg_toplevel);
}

void pnWindow_unsetMaximize(struct PnWidget *win) {
    DASSERT(win);
    ASSERT(win->type & TOPLEVEL);
    DASSERT(((struct PnWindow *)win)->toplevel.xdg_toplevel);
    xdg_toplevel_unset_maximized(((struct PnWindow *)win)
            ->toplevel.xdg_toplevel);
}

void pnWindow_fullscreen(struct PnWidget *win) {
    DASSERT(win);
    ASSERT(win->type & TOPLEVEL);
    DASSERT(((struct PnWindow *)win)->toplevel.xdg_toplevel);
    xdg_toplevel_set_fullscreen(((struct PnWindow *)win)
            ->toplevel.xdg_toplevel, 0);
}

void pnWindow_unsetFullscreen(struct PnWidget *win) {
    DASSERT(win);
    ASSERT(win->type & TOPLEVEL);
    DASSERT(((struct PnWindow *)win)->toplevel.xdg_toplevel);
    xdg_toplevel_unset_fullscreen(((struct PnWindow *)win)
            ->toplevel.xdg_toplevel);
}
