#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"
#include "rand.h"


static void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static struct PnWidget *win = 0;

static bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, void *userData) {

    const char *cursorName = (const char *) userData;

    DSPEW("cursorName=\"%s\"", cursorName);

    ASSERT(pnWindow_pushCursor(cursorName));
    return true; // true => take focus
}


void leave(struct PnWidget *widget, void *userData) {

    const char *cursorName = (const char *) userData;

    INFO("cursorName=\"%s\"", cursorName);
}


void MakeWidget(const char *cursorName) {

DSPEW("cursorName=\"%s\"", cursorName);

    struct PnWidget *w = pnWidget_create(win/*parent*/,
            100/*width*/, 400/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());
    pnWidget_setEnter(w, enter, (char *) cursorName);
    pnWidget_setLeave(w, leave, (char *) cursorName);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(2);

    win = pnWindow_create(0, 20, 20,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);

    const char *cursorNames[] = { "left_ptr",
        "n-resize", "nw-resize", "w-resize", "ne-resize",
        "e-resize", "sw-resize", "se-resize", "s-resize",
        0 };
    const char **curserName = cursorNames;

    while(*curserName)
        MakeWidget(*curserName++);

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
