#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"
#include "rand.h"


static void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static struct PnWidget *win = 0;
static uint32_t count = 0;


static void Check(void) {

    ASSERT(win);

    struct PnWidget *w = pnCheck_create(win,
        31/*width*/, 31/*height*/,
        0/*align*/, 0/*expand*/);

    ASSERT(w);

    if((count++) % 2)
        pnCheck_set(w, true);
    // else it's false.

    pnWidget_setBackgroundColor(w, Color(), 0);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    srand(6);

    win = pnWindow_create(0, 0, 0,
            0/*x*/, 0/*y*/, PnLayout_LR, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xAA010101, 0);

    for(int i=0; i<6; ++i)
        Check();

    pnWindow_show(win);

    Run(win);

    return 0;
}
