#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

#define EXPAND  (PnExpand_HV)


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWindow *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/, 0,
            PnExpand_HV);
    ASSERT(win);

    struct PnWidget *w = pnWidget_create(
            (struct PnSurface *) win/*parent*/,
            100/*width*/, 200/*height*/,
            0/*direction*/, 0/*align*/,
            EXPAND/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCCCF0000);

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
