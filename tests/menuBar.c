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


static void AddMenu(struct PnWidget *parent) {

    ASSERT(parent);

    struct PnWidget *menu = pnMenu_create(
            parent,
            30 + 24 * Rand(1,4)/*width*/, 50/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_None/*expand*/,
            0/*text label*/);
    ASSERT(menu);

    pnWidget_setBackgroundColor(menu, Color());
}


static void MenuBar(struct PnWidget *parent) {

    ASSERT(parent);

    struct PnWidget *w = pnWidget_create(
            parent,
            0/*width*/, 0/*height*/,
            0/*layout*/, PnAlign_LT/*align*/,
            PnExpand_None/*expand*/, 0/*size*/);
    ASSERT(w);

    for(uint32_t i=Rand(3,9); i != -1; --i)
        AddMenu(w);

    pnWidget_setBackgroundColor(w, Color());
}

static void SubWindow(struct PnWidget *parent) {

    ASSERT(parent);

    struct PnWidget *w = pnWidget_create(
            parent,
            0/*width*/, 0/*height*/,
            PnLayout_TB/*layout*/, PnAlign_LT/*align*/,
            PnExpand_HV/*expand*/, 0/*size*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());

    MenuBar(w);

    w = pnWidget_create(
            w/*parent*/, 100/*width*/, 80/*height*/,
            0/*layout*/, PnAlign_LT/*align*/,
            PnExpand_HV/*expand*/, 0/*size*/);
    ASSERT(w);

    pnWidget_setBackgroundColor(w, Color());
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    srand(13);

    struct PnWidget *win = pnWindow_create(0,
            10/*width*/, 10/*height*/,
            0/*x*/, 0/*y*/, PnLayout_TB, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xAA010101);

    SubWindow(win);
    SubWindow(win);
    SubWindow(win);

    pnWindow_show(win);

    Run(win);

    return 0;
}
