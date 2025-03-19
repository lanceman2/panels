
#include "../include/panels.h"

#include "../lib/debug.h"
#include "run.h"

int main(void) {

    struct PnWidget *win = pnWindow_create(0, 700, 350, 0, 0, 0, 0, 0);
    ASSERT(win);
    pnWindow_show(win, true);

    // Popup window because of win parent.
    struct PnWidget *pop = pnWindow_create(win/*parent*/,
            100, 380, 0/*x*/, 0/*y*/, 0, 0, 0);
    ASSERT(pop);
    pnWidget_setBackgroundColor(pop, 0xFAA99999);
    pnWindow_show(pop, true);

    pop = pnWindow_create(win/*parent*/, 100, 300,
            200/*x*/, 0/*y*/, 0, 0, 0);
    ASSERT(pop);
    pnWidget_setBackgroundColor(pop, 0xFA299981);

    pnWindow_show(pop, true);

    Run(win);

    return 0;
}
