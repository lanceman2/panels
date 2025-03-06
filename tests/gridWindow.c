#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"
#include "rand.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

const uint32_t numColumns = 5, numRows = 6;

struct PnWindow *win;

void Widget(uint32_t x, uint32_t y) {

    struct PnWidget *w = pnWidget_createInGrid((void *) win,
            20/*width*/, 20/*height*/,
        PnLayout_One,
        0/*align*/, PnExpand_HV, 
        x/*columnNum*/, y/*rowNum*/,
        1/*columnSpan*/, 1/*rowSpan*/,
        0/*size*/);
    pnWidget_setBackgroundColor(w, Color());
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    srand(2);

    win = pnWindow_createAsGrid(0/*parent*/,
            5/*width*/, 5/*height*/, 0/*x*/, 0/*y*/,
            0/*align*/, PnExpand_HV/*expand*/,
            numColumns, numRows);
    ASSERT(win);

    for(uint32_t y=0; y<numRows; ++y)
        for(uint32_t x=0; x<numColumns; ++x)
            Widget(x, y);

    pnWindow_show(win, true);

    //Run(win);

    return 0;
}
