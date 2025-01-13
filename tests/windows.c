
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

    // This passes Valgrind with or without destroying all the windows
    // given the libpanels.so destructor cleans up the windows and display
    // if the user does not.
    pnWindow_destroy(win);
    fprintf(stderr, "win=%p", win);

    return 0;
}
