
#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"


// This callback just lets us know that the window was destroy without
// this code destroying it; the desktop user did it.
static void destroy(struct PnWidget *window, void *userData) {
    struct PnWidget **win = userData;
    ASSERT(window == *win);
    *win = 0;
}


int main(void) {

    struct PnWidget *win = pnWindow_create(0, 700, 350,
            0, 0, 0, 0, PnExpand_HV);
    ASSERT(win);

    pnWindow_setDestroy(win, destroy, &win);

    pnWindow_show(win);


    Run(win);


    fprintf(stderr, "   win=%p\n", win);
    // This passes Valgrind with or without destroying the window
    // given the libpanels.so destructor cleans up the windows and
    // display if the user does not.

    if(win)
        // This gets around that problem of self managing objects, that
        // is, this code can manage this "win" object but so can an
        // unforeseen widget in the "window manager" stuff; so we have
        // this pnWindow_setDestroy() thing to let this code in this file
        // know if the window was destroyed.
        //
        // Yes.  The win pointer is still valid.
        // Just testing that we can call this:
        pnWidget_destroy(win);

    // We just wanted to exercise this function.  The libpanels.so
    // destructor calls it, but calling it here too should be fine.
    pnDisplay_destroy();

    return 0;
}
