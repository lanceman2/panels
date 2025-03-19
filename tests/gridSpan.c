#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"
#include "rand.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    srand(2);

    struct PnWidget *win = pnWindow_create(0/*parent*/,
            0/*width*/, 0/*height*/, 0/*x*/, 0/*y*/,
            PnLayout_One,
            0/*align*/, PnExpand_HV/*expand*/);
    ASSERT(win);

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
