#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"
#include "rand.h"


static void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static struct PnWidget *win = 0;


static void MakeWidget(void) {

    struct PnWidget *w = pnWidget_create(win/*parent*/,
            100/*width*/, 400/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(2);

    win = pnWindow_create(0, 0, 0,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    MakeWidget();
    pnSplitter_create(win, 15/*width*/, 0/*height*/, 0/*size*/);
    MakeWidget();
    MakeWidget();

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
