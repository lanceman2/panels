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

    struct PnWidget *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);

    struct PnWidget *w = pnWidget_create(
            win/*parent*/,
            100/*width*/, 200/*height*/,
            0/*layout*/, 0/*align*/,
            EXPAND/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCCCF0000, 0);

    w = pnWidget_create(win/*parent*/,
            100/*width*/, 300/*height*/,
            0/*layout*/, 0/*align*/,
            EXPAND/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCC00CF00, 0);
    //pnWidget_show(w, false);

    struct PnWidget *cw = pnWidget_create(
            win/*parent*/,
            100/*width*/, 100/*height*/,
            PnLayout_LR/*layout*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0);
    ASSERT(cw);
    pnWidget_setBackgroundColor(cw, 0xFF0000CF, 0);

#if 1
    {
        w = pnWidget_create(cw/*parent*/,
            130/*width*/, 100/*height*/,
            0/*layout*/, 0/*align*/, PnExpand_V/*expand*/,
            0);
        ASSERT(w);
        pnWidget_setBackgroundColor(w, 0xCCCFFF00, 0);
        //pnWidget_show(w, false);

        w = pnWidget_create(cw/*parent*/,
            100/*width*/, 100/*height*/,
            0/*layout*/, PnAlign_CB/*align*/, 0/*expand*/,
            0);
        ASSERT(w);
        pnWidget_setBackgroundColor(w, 0xCC00CFFF, 0);
        pnWidget_show(w, true);
    }
#endif

    pnWindow_show(win);

    Run(win);

    return 0;
}
