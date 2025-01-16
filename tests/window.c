
#include "../include/panels.h"

#include "../lib/debug.h"


// This callback just lets us know that the window was destroy without
// this code destroying it; the desktop user did it.
static void destroy(struct PnWindow *window, void *userData) {
    struct PnWindow **win = userData;
    ASSERT(window == *win);
    *win = 0;
}


int main(void) {

    struct PnWindow *win = pnWindow_create(0, 700, 350, 0, 0, 0);
    ASSERT(win);

    pnWindow_setCBDestroy(win, destroy, &win);

    pnWindow_show(win, true);

    //pnWindow_show(pnWindow_create(400, 350), true);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif



    fprintf(stderr, "   win=%p\n", win);
    // This passes Valgrind with or without destroying the window
    // given the libpanels.so destructor cleans up the windows and
    // display if the user does not.

    if(win)
        // Yes.  The win pointer is still valid.
        // Just testing that we can call this:
        pnWindow_destroy(win);

    return 0;
}
