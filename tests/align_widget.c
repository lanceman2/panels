#include <signal.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "rand.h"
#include "run.h"

#define SEED (1)

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}


static struct PnWidget *Widget(struct PnWidget *parent) {

    DASSERT(parent);

    struct PnWidget *w = pnWidget_create(
            parent,
            50/*width*/,
            50/*height*/,
            0/*layout*/,
            0/*align*/,
            //PnExpand_H * Rand(0, 1));
            PnExpand_HV & 0/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());

    return w;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(SEED);

    struct PnWidget *win = pnWindow_create(0,\
            20/*width*/, 20/*height*/,
            0/*x*/, 0/*y*/, PnLayout_TB/*layout*/,
            0/*align*/, PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xFF000000);

    struct PnWidget *w = pnWidget_create(
            win,
            40/*width*/,
            40/*height*/,
            PnLayout_LR/*layout*/,
            PnAlign_RC/*align*/, PnExpand_HV/*expand*/,
            0/*size*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());


    for(uint32_t i=0; i<3; ++i)
        Widget(w);

    w = pnWidget_create(
            win,
            40/*width*/,
            40/*height*/,
            PnLayout_BT/*layout*/,
            PnAlign_JC/*align*/, PnExpand_H/*expand*/,
            0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());


    for(uint32_t i=0; i<2; ++i)
        Widget(w);


    pnWindow_show(win);

    Run(win);

    return 0;
}
