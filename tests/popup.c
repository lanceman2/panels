
#include "../include/panels.h"

#include "../lib/debug.h"


int main(void) {

    struct PnWindow *win = pnWindow_create(0, 700, 350, 0);
    ASSERT(win);
    pnWindow_show(win, true);

    // Popup window because of win parent.
    win = pnWindow_create(win/*parent*/, 700, 350, 0);
    ASSERT(win);
    pnWindow_show(win, true);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif


    return 0;
}
