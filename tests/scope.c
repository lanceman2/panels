#include <signal.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <math.h>

#include <wayland-client.h>

#include "../include/panels.h"
#include "../lib/debug.h"


static inline bool preDispatch(struct wl_display *d, int wl_fd) {

    if(wl_display_dispatch_pending(d) < 0)
        // TODO: Handle failure modes.
        return true; // fail

        // TODO: Add code (with poll(2)) to handle the errno == EAGAIN
        // case.  I think the wl_fd is a bi-directional file descriptor.

    // See
    // https://www.systutorials.com/docs/linux/man/3-wl_display_flush/
    //
    // apt install libwayland-doc  gives a large page 'man wl_display'
    // but not 'man wl_display_dispatch'
    //
    // wl_display_flush(d) Flushes write commands to compositor.
    errno = 0;
    // wl_display_flush(d) Flushes write commands to compositor.

    int ret = wl_display_flush(d);

    while(ret == -1 && errno == EAGAIN) {
    
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(wl_fd, &wfds);
        // This may never happen, so lets see it.
        DSPEW("waiting to flush wayland display");
        ret = select(wl_fd+1, 0, &wfds, 0, 0);
        switch(ret) {
            case -1:
                ERROR("select failed");
                exit(1);
            case 1:
                ASSERT(FD_ISSET(wl_fd, &wfds));
                break;
            default:
                ASSERT(0, "select() returned %d", ret);
        }

        errno = 0;
        ret = wl_display_flush(d);
    }
    if(ret == -1) {
        ERROR("wl_display_flush() failed");
        return true;
    }
    return false; // success.
}


static inline void Run(struct PnWidget *win) {
    
    struct wl_display *d = pnDisplay_getWaylandDisplay();
    ASSERT(d);
    int wl_fd = wl_display_get_fd(d);
    ASSERT(wl_fd == 3);

    fd_set rfds;
    FD_ZERO(&rfds);

    // Run the main loop until the GUI user causes it to stop.
    while(true) {

        if(preDispatch(d, wl_fd)) return;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(wl_fd, &rfds);
        int ret = select(wl_fd+1, &rfds, 0, 0, 0);

        switch(ret) {
            case -1:
                ERROR("select failed");
                exit(1);
            case 1:
                ASSERT(FD_ISSET(wl_fd, &rfds));
                if(wl_display_dispatch(d) == -1 || !pnDisplay_haveWindow())
                    // Got running window based GUI.
                    return;
                break;
            default:
                ASSERT(0, "select() returned %d", ret);
        }
    }
}

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

bool Plot(struct PnWidget *g, struct PnPlot *p, void *userData,
        double xMin, double xMax, double yMin, double yMax) {

    const double tMax = 20 * M_PI;

    for(double t = 0.0; t <= 2*tMax + 10; t += 0.1) {
        double a = 1.0 - t/tMax;
        pnGraph_drawPoint(p, a * cos(t), a * sin(t));
    }
    return false;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWidget *win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1100, 900);

    // The auto 2D plotter grid (graph)
    struct PnWidget *w = pnGraph_create(
            win/*parent*/,
            90/*width*/, 70/*height*/, 0/*align*/,
            PnExpand_HV/*expand*/);
    ASSERT(w);
    //                  Color Bytes:  A R G B
    pnWidget_setBackgroundColor(w, 0xA0101010, 0);

    struct PnPlot *p = pnScopePlot_create(w, Plot, catcher);
    ASSERT(p);
    // This plot, p, is owned by the graph, w.
    pnPlot_setLineColor(p, 0xFFFF0000);
    pnPlot_setPointColor(p, 0xFF00FFFF);
    pnPlot_setLineWidth(p, 3.2);
    pnPlot_setPointSize(p, 4.5);

    pnGraph_setView(w, -1.05, 1.05, -1.05, 1.05);

    pnWindow_show(win);

    Run(win);
    return 0;
}
