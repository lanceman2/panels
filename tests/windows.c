
#include "../include/panels.h"

#include "../lib/debug.h"


static void CreateWindow(uint32_t w, uint32_t h) {

    struct PnWindow *win = pnWindow_create(0, w, h, 0, 0, 0, 0);
    ASSERT(win);
    pnWindow_show(win, true);
}

int main(void) {

    CreateWindow(400, 600);
    CreateWindow(700, 300);
    CreateWindow(200, 500);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    return 0;
}
