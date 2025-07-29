#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"
#include "rand.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

const uint32_t numColumns = 18, numRows = 10;

struct PnWidget *win;

uint32_t color;

bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, void *userData) {

    color = pnWidget_getBackgroundColor(w);
    pnWidget_setBackgroundColor(w, 0xAAFF0000, 0);
    pnWidget_queueDraw(w, 0);
    return true;
}

void leave(struct PnWidget *w, void *userData) {

    pnWidget_setBackgroundColor(w, color, 0);
    pnWidget_queueDraw(w, 0);
}

void Widget(uint32_t x, uint32_t y) {

    struct PnWidget *w = pnWidget_createInGrid(win,
            40/*width*/, 30/*height*/,
        PnLayout_One,
        Rand(0,16)/*align*/, Rand(0,0)/*Expand*/, 
        x/*columnNum*/, y/*rowNum*/,
        1/*columnSpan*/, 1/*rowSpan*/,
        0/*size*/);
    pnWidget_setBackgroundColor(w, Color(), 0);

    pnWidget_setEnter(w, enter, 0/*userData*/);
    pnWidget_setLeave(w, leave, 0/*userData*/);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    srand(2);

    win = pnWindow_createAsGrid(0/*parent*/,
            0/*width*/, 0/*height*/, 0/*x*/, 0/*y*/,
            0/*align*/, PnExpand_HV/*expand*/,
            numColumns, numRows);
    ASSERT(win);

    for(uint32_t y=0; y<numRows; ++y)
        for(uint32_t x=0; x<numColumns; ++x)
            Widget(x, y);

    pnWindow_show(win);

    Run(win);

    return 0;
}
