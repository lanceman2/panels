
#include "../include/panels.h"

#include "../lib/debug.h"


int main(void) {

    struct PnWindow *win = pnWindow_create(400, 350);
    ASSERT(win);
    pnWindow_destroy(win);
    pnWindow_create(400, 350);
    win = pnWindow_create(400, 350);
    ASSERT(win);
    pnWindow_destroy(win);
    pnWindow_create(400, 350);
    win = pnWindow_create(400, 350);
    ASSERT(win);
    pnWindow_create(400, 350);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    fprintf(stderr, "win=%p\n", win);
    // This passes Valgrind with or without destroying all the windows
    // given the libpanels.so destructor cleans up the windows and display
    // if the user does not.

#ifndef RUN
    // If this ran the desktop user could have already destroyed the
    // window, so we don't want to try to destroy it again.
    pnWindow_destroy(win);
#endif

    return 0;
}
