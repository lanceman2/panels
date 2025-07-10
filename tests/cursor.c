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

    //DSPEW("cursorName=\"%s\"", cursorName);

    ASSERT(pnWindow_pushCursor(w, cursorName));
    return true; // true => take focus
}


static void leave(struct PnWidget *w, void *userData) {

    //INFO("cursorName=\"%s\"", (const char *) userData);

    pnWindow_popCursor(w);
}


static void MakeWidget(const char *cursorName) {

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


static void Do(void) {

    win = pnWindow_create(0, 20, 20,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);

    const char *cursorNames[] = {
        // TODO: Where can we get these strings?
        "n-resize", "nw-resize", "w-resize", "ne-resize",
        "e-resize", "sw-resize", "se-resize", "s-resize",
        0 };
    const char **curserName = cursorNames;

    while(*curserName)
        MakeWidget(*curserName++);

    pnWindow_show(win);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(2);

    // Testing that it worked with more than one window.  There is a thing
    // in Wayland that sets the cursor that is related to particular
    // windows for wl_pointer_set_cursor() (It seems wonky to me), so we
    // need this to have more than one window for testing.
    Do();
    Do();
    Do();

    Run(win);

    return 0;
}
