// The use of pnDisplay_run() is not required to use libpanels.so.
// Requiring the use of pnDisplay_run() would be considered bad form.  See
// ../README.md.  Users of libpanels.so can make their own main loop code
// and the use of pnDisplay_run() is not required.

#define _GNU_SOURCE
#include <link.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>
#include <linux/input-event-codes.h>
#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "../include/panels.h"
#include "debug.h"
#include "display.h"

static inline bool Init(void) {

    if(!d.wl_display) {
        if(_pnDisplay_create()) {
            DASSERT(0);
            return true;
        }
    }
    return false;
}


struct wl_display *pnDisplay_getWaylandDisplay(void) {

    if(Init()) return 0;

    return d.wl_display;
}


bool pnDisplay_haveWindow(void) {
    return (d.wl_display && d.windows)?true:false;
}


// This can block.
//
// Return true if we can keep running.
//
bool pnDisplay_dispatch(void) {

    if(Init()) return false;

    if(wl_display_dispatch(d.wl_display) != -1 &&
            d.windows/*we have at least one window*/)
        // We can keep going.
        return true;

    return false;
}

// Return true on failure.
//
// This is a little make gtk_main() except that: we can call it more than
// once, and we can cleanup the under laying objects.
//
bool pnDisplay_run() {

    if(Init()) return true; // error case.


    // Run the main loop until the GUI user causes it to stop.
    while(pnDisplay_dispatch());

    return false;
}

// Return true on failure.
//
bool pnDisplay_addReader(int fd,
        bool (*read)(int fd, void *userData), void *userData) {

    if(Init()) return true;

    return false;
}

// Return true on failure.
//
bool pnDisplay_removeReader(int fd) {

    if(Init()) return true;

    return false;
}

