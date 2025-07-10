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

    struct PnWidget *grid = pnWidget_createAsGrid(
            win/*parent*/, 5/*width*/, 5/*height*/,
            0/*align*/, PnExpand_HV/*expand*/, 
            0/*numColumns*/, 0/*numRows*/,
            0/*size*/);
    ASSERT(grid);
    pnWidget_setBackgroundColor(grid, 0xFF000000);

    pnWindow_show(win);

    Run(win);

    return 0;
}
