// The use of pnDisplay_run() is not required to use libpanels.so.
// Requiring the use of pnDisplay_run() would be considered bad form.  See
// ../README.md.  Users of libpanels.so can make their own main loop code
// and the use of pnDisplay_run() is not required.
//
// Note: It appears that Wayland does not allow more than one client
// display object per process.  So, libpanels.so has just one possible
// display object in "d.wl_display".  We don't need to pass a pointer
// to it as an argument, given there can only be one of them.

#define _GNU_SOURCE
#include <link.h>
#include <sys/epoll.h>
#include <unistd.h>
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
#include "mainLoop.h"


struct wl_display *pnDisplay_getWaylandDisplay(void) {

    if(!d.wl_display) {
        if(_pnDisplay_create()) {
            DASSERT(0);
            return 0;
        }
    }

    return d.wl_display;
}

static inline bool InitDisplay(void) {

    if(!d.wl_display) {
        if(_pnDisplay_create()) {
            DASSERT(0);
            return true;
        }
    }
    return false;
}

// Because the wayland-client only lets us have one wayland display
// we can have this, "wl_fd", as a global static variable.
static int wl_fd = -1;

// This function may need to have some ASSERT() calls replaced with
// an error check and a return.
//
// We do this before we call a blocking call like select(2) or
// epoll_wait(2); with the Wayland client fd.
//
// Returns 0 on success and continue calling
// Returns 1 on success and finish calling
// Returns -1 on error and finish calling
//
static int PreDispatch(void *userData) {

    DASSERT(d.wl_display);
    DASSERT(wl_fd >= 0);
    DASSERT(d.mainLoop);

    if(wl_display_dispatch_pending(d.wl_display) < 0) {
        // TODO: Handle failure modes.
        ERROR("wl_display_dispatch_pending() failed");
        return -1; // fail
    }

    // See
    // https://www.systutorials.com/docs/linux/man/3-wl_display_flush/
    //
    // apt install libwayland-doc  gives a large page 'man wl_display'
    // but not 'man wl_display_dispatch'
    //
    // wl_display_flush(d) Flushes write commands to compositor.
    errno = 0;

    int ret = wl_display_flush(d.wl_display);

    while(ret == -1 && errno == EAGAIN) {
        // Keep trying.
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(wl_fd, &wfds);
        // This may never happen, so lets see it if it does.
        NOTICE("waiting to flush wayland display");
        ret = select(wl_fd+1, 0, &wfds, 0, 0);
        switch(ret) {
            case -1:
                ERROR("select failed");
                return -1;
            case 1:
                DASSERT(FD_ISSET(wl_fd, &wfds));
                if(!FD_ISSET(wl_fd, &wfds)) {
                    ERROR("select failed to set fd=%d", wl_fd);
                    return -1;
                }
                break;
            default:
                DASSERT(0, "select() returned %d", ret);
                ERROR("select() returned %d", ret);
                return -1;
        }

        errno = 0;
        ret = wl_display_flush(d.wl_display);
    }
    if(ret == -1) {
        ERROR("wl_display_flush() failed");
        return -1;
    }
    return 0; // success.
}


static int Wayland_Read(int fd, void *userData) {

    if(!d.wl_display || !pnDisplay_haveWindow()) {
        // Got no running window based GUI before
        // calling wl_display_dispatch().
        // Return true to remove the wayland fd.
        DSPEW("Cleaning up wl_display d.wl_display=%p "
                "pnDisplay_haveWindow()=%d",
                d.wl_display, pnDisplay_haveWindow());
        return 2; // 2 -> stop all callbacks
    }

    if(wl_display_dispatch(d.wl_display) == -1 ||
            !pnDisplay_haveWindow()) {
        DSPEW("Cleaning up wl_display d.wl_display=%p "
                "pnDisplay_haveWindow()=%d",
                d.wl_display, pnDisplay_haveWindow());
        // Got no running window based GUI.
        // Return true to remove the wayland fd from the epoll_wait() file
        // descriptor group.
        return 2; // 2 -> stop all callbacks
    }

    return 0;
}


static inline bool Init(void) {

    if(InitDisplay()) return true; // error

    if(!d.mainLoop && !(d.mainLoop = pnMainLoop_create())) {
        DASSERT(0);
        return true;
    }

    wl_fd = wl_display_get_fd(d.wl_display);
    ASSERT(wl_fd >= 0);

    if(pnMainLoop_addReader(d.mainLoop, wl_fd, false/*edge_trigger*/,
            Wayland_Read, 0/*userData*/))
        return true; // error and fail

    return false; // success.
}


bool pnDisplay_haveWindow(void) {
    return (d.wl_display && d.windows)?true:false;
}


// This can block.
//
// Return true if we can keep running.
//
bool pnDisplay_dispatch(void) {

    if(InitDisplay()) return false;

    if(wl_display_dispatch(d.wl_display) != -1 &&
            d.windows/*we have at least one window*/)
        // We can keep going.
        return true;

    return false;
}

static int PostDispatch(void *userData) {

        // If this Wayland client is still running there must
        // be a Wayland reader file descriptor in use.
        //
        if(!d.wl_display || !d.mainLoop ||
                // the Wayland_Read() function can make the following so:
                !d.mainLoop->readers || wl_fd != d.mainLoop->readers->fd)
            return 1; // The Wayland display stopped.

    return 0; // keep running.
}


static inline bool EpollRun(void) {

    DASSERT(d.wl_display);
    DASSERT(d.mainLoop);
    DASSERT(d.mainLoop->epollFd >= 0);

    wl_fd = wl_display_get_fd(d.wl_display);
    ASSERT(wl_fd >= 0);
    DASSERT(wl_fd == d.mainLoop->readers->fd);

#if 1
    return pnMainLoop_run(d.mainLoop, PreDispatch, 0, PostDispatch, 0);
#else
    // This is the same as calling pnMainLoop_run():
    //
    // Run the main loop until the GUI user causes it to stop.
    while(true) {

        int ret;

        if((ret = PreDispatch(0))) {
            if(ret > 0)
                return false; // no error
            else
                return true; // error
        }

        if(pnMainLoop_wait(d.mainLoop)) return true;

        if((ret = PostDispatch(0))) {
            if(ret > 0)
                return false; // no error
            else
                return true; // error
        }
    }
#endif

    return false; // success
}

// Return true on failure.
//
// This is a little like gtk_main() except that: we can call it more than
// once, and we can cleanup, the under laying objects and system
// resources.
//
bool pnDisplay_run() {

    if(InitDisplay()) return true; // error case.

    if(!d.mainLoop) {
        while(wl_display_dispatch(d.wl_display) != -1) {
            if(!d.windows/*we do not have at least one window*/)
                // We can't keep going.
                return false;
        }
        return false;
    }

    return EpollRun();
}


// Return true on failure.
//
bool pnDisplay_addReader(int fd, bool edge_trigger,
        int (*Read)(int fd, void *userData), void *userData) {

    if(Init()) return true;

    if(pnMainLoop_addReader(d.mainLoop, fd, edge_trigger,
                Read, userData)) return true;

    return false; // success
}

// Return true on failure.
//
bool pnDisplay_removeReader(int fd) {

    DASSERT(fd >= 0);

    if(Init()) return true;

    if(!d.mainLoop) {
        ERROR("No mainloop present");
        return true;
    }

    return
        pnMainLoop_removeReader(d.mainLoop, fd);
}

