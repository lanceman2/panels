
#include "../include/panels.h"

#include "../lib/debug.h"


int main(void) {

    struct PnWindow *win = pnWindow_create(400, 350);
    ASSERT(win);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    fprintf(stderr, "win=%p\n", win);
    // This passes Valgrind with or without destroying the window
    // given the libpanels.so destructor cleans up the windows and
    // display if the user does not.
    pnWindow_destroy(win);

    return 0;
}
