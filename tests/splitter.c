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

    win = pnWindow_create(0, 20/*width*/, 20/*height*/,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    pnWidget_setBackgroundColor(win, Color());
    ASSERT(win);

    struct PnWidget *w1 = MakeWidget();
    struct PnWidget *w2 = MakeWidget();
WARN("w1=%p  w2=%p", w1, w2);

    bool isHorizontal = true;
#ifdef VERTICAL
    isHorizontal = false;
#endif

    pnSplitter_create(win/*parent*/, w1, w2,
        isHorizontal/*isHorizontal*/, 0/*size*/);

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
