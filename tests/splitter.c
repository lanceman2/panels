#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"
#include "rand.h"


static void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static struct PnWidget *win;


static struct PnWidget *MakeWidget(void) {

    struct PnWidget *w = pnWidget_create(0/*parent*/,
            300/*width*/, 300/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());
    return w;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(3);

    win = pnWindow_create(0, 110/*width*/, 110/*height*/,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    pnWidget_setBackgroundColor(win, Color());
    ASSERT(win);
    pnWindow_setPreferredSize(win, 900, 640);

    struct PnWidget *w1 = MakeWidget();
    struct PnWidget *w2 = MakeWidget();

    bool isHorizontal = true;
#ifdef VERTICAL
    isHorizontal = false;
#endif

    pnSplitter_create(win/*parent*/, w1, w2,
        isHorizontal/*isHorizontal*/);

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
