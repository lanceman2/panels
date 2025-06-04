#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"
#include "rand.h"


static void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static struct PnWidget *win;

static void *myUserData = (void *) 0x00FF4FFF;


static
void pressAction(struct PnWidget *w, int32_t x, int32_t y,
        void *userData) {

    ASSERT(w);
    ASSERT(userData == myUserData);

    INFO("widget=%p x,y=%" PRIi32 ",%" PRIi32, w, x, y);

    pnWidget_setBackgroundColor(w, Color());
    pnWidget_queueDraw(w, false/*allocate*/);
}


static void MenuItem(void) {

    ASSERT(win);

    struct PnWidget *w = pnMenuItem_create(
            win/*parent*/,
            100/*width*/, 200/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_HV/*expand*/, 0/*size*/);
    ASSERT(w);

    pnWidget_setBackgroundColor(w, Color());
    pnWidget_addCallback(w, PN_GENERIC_CB_PRESS,
            pressAction, myUserData);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    win = pnWindow_create(0, 0, 0,
            0/*x*/, 0/*y*/, PnLayout_LR, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xAA010101);

    for(int i=0; i<8; ++i)
        MenuItem();

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
