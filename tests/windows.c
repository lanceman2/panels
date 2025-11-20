
#include "../include/panels.h"

#include "../lib/debug.h"
#include "run.h"


static struct PnWidget *win;

static void CreateWindow(uint32_t w, uint32_t h) {

    win = pnWindow_create(0, w, h, 0, 0, 0, 0, PnExpand_HV);
    ASSERT(win);
    pnWindow_show(win);
}

#ifdef RUN
static int DummyReader(int fd, void *userData) {
    return 0;
}
#endif


int main(void) {

    CreateWindow(400, 600);
    CreateWindow(700, 300);
    CreateWindow(200, 500);

#ifdef RUN
    // Needed to test pnDisplay_run() with epoll code in it.
    //
    // We add a dummy reader that never gets its' callback called.
    // That will force it to use the code with epoll_wait(), thinking
    // that we have a file reader.
    ASSERT(pnDisplay_addReader(0, 0, DummyReader, 0) == false);
    pnDisplay_run();
#else
    Run(win);
#endif

    return 0;
}
