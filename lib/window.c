#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include  "display.h"


// Add a window, win, to the window list in d.windows.
//
// d.windows points to the last window made, win.
static inline void AddWindow(struct PnWindow *win) {

    DASSERT(d.wl_display);
    DASSERT(!win->next);
    DASSERT(!win->prev);

    if(d.windows) {
        DASSERT(!d.windows->next);
        d.windows->next = win;
        win->prev = d.windows;
    }

    d.windows = win;
}

// Remove a window, win, from the window list in d.windows.
//
static inline void RemoveWindow(struct PnWindow *win) {

    DASSERT(d.wl_display);
    DASSERT(d.windows);

    if(win->next) {
        DASSERT(d.windows != win);
        win->next->prev = win->prev;
    } else {
        DASSERT(d.windows == win);
        d.windows = win->prev;
    }
    if(win->prev)
        win->prev->next = win->next;
}


struct PnWindow *pnWindow_create(uint32_t w, uint32_t h) {

    if(CheckDisplay()) return 0;

    struct PnWindow *win = calloc(1, sizeof(*win));
    ASSERT(win, "calloc(1,%zu) failed", sizeof(*win));

    AddWindow(win);
    return win;
}


void pnWindow_destroy(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(d.wl_display);

    RemoveWindow(win);

    DZMEM(win, sizeof(*win));
    free(win);
}

