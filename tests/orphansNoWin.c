#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWidget *w = pnWidget_create(
            0/*0=no parent*/,
            100/*width*/, 200/*height*/,
            0/*layout*/, 0/*align*/,
            0/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, 0xCCCF0000);

    w = pnWidget_create(w/*parent*/,
            100/*width*/, 300/*height*/,
            0/*layout*/, 0/*align*/,
            0/*expand*/, 0);
    ASSERT(w);

    struct PnWidget *cw = pnWidget_create(
            w/*parent*/,
            100/*width*/, 100/*height*/,
            PnLayout_LR/*layout*/, 0/*align*/,
            0/*expand*/, 0);
    ASSERT(cw);


    return 0;
}
