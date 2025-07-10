#include <signal.h>
#include <unistd.h>

#include "../include/panels.h"

#include "../lib/debug.h"

static void caughter(int sig) {
    ASSERT(0, "caught signal %d", sig);
}

struct PnWidget *win = 0;

static void DestroyWindow(struct PnWidget *w, void *userData) {

    win = 0;
}

// TODO:  This is totally broken.
// Maybe do it like: ref: https://github.com/emersion/wleird.git
// unmap.c


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, caughter));

    win = pnWindow_create(0, 700, 350, 0, 0, 0, 0, 0);
    ASSERT(win);
    pnWidget_addDestroy(win, DestroyWindow, 0);
    pnWindow_show(win);

    // Popup window because of win parent.
    struct PnWidget *pop;

#if 0
    pop = pnWindow_create(win/*parent*/,
            100, 380, 0/*x*/, 0/*y*/, 0, 0, 0);
    ASSERT(pop);
    pnWidget_setBackgroundColor(pop, 0xFAA99999);
    pnWindow_show(pop);

    pop = pnWindow_create(win/*parent*/, 100, 300,
            200/*x*/, 0/*y*/, 0, 0, 0);
    ASSERT(pop);
    pnWidget_setBackgroundColor(pop, 0xFA299981);
#endif

    bool run = false;
#ifdef RUN
    run = true;
#endif

    pop = win;

    do {

        pnWindow_show(pop);

        // Run the main loop until it appears to have drawn a window.
        pnWindow_isDrawnReset(pop);
        while(win && !pnWindow_isDrawn(pop))
            if(!pnDisplay_dispatch()) {
                run = false;
                break;
            }
        if(!win) break;


    } while(run && win);

    return 0;
}
