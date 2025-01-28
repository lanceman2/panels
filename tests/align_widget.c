#include <signal.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "rand.h"

#define SEED (1)

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}


static struct PnWidget *Widget(void *parent) {

    DASSERT(parent);

    struct PnWidget *w = pnWidget_create(
            (struct PnSurface *) parent,
            50/*width*/,
            50/*height*/,
            0/*direction*/,
            PnAlign_RT/*align*/,
            PnExpand_V * Rand(0, 1));
            //PnExpand_HV/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());

    return w;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(SEED);

    struct PnWindow *win = pnWindow_create(0,\
            20/*width*/, 20/*height*/,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/,
            0/*align*/, PnExpand_HV);
    ASSERT(win);
    pnWindow_setBackgroundColor(win, 0xFF000000);

    struct PnWidget *w = pnWidget_create(
            (struct PnSurface *) win,
            40/*width*/,
            40/*height*/,
            PnDirection_LR/*direction*/,
            0/*align*/, PnExpand_H/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());


    for(uint32_t i=0; i<5; ++i)
        Widget(w);

    w = pnWidget_create(
            (struct PnSurface *) win,
            40/*width*/,
            40/*height*/,
            PnDirection_LR/*direction*/,
            0/*align*/, PnExpand_H & 0/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());


    for(uint32_t i=0; i<5; ++i)
        Widget(w);


    pnWindow_show(win, true);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    return 0;
}
