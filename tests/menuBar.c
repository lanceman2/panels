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

static int itemCount = 0;

static inline
void AddItem(struct PnWidget *menu) {

    const char *format = "item label %d";
    const size_t LEN = strlen(format) + 3;
    char label[LEN];
    snprintf(label, LEN, format, itemCount++);

    pnMenu_addItem(menu, label);
}

static int menuCount = 0;

static void AddMenu(struct PnWidget *parent) {

    ASSERT(parent);

    const char *format = "menu %d";
    const size_t LEN = strlen(format) + 3;
    char label[LEN];
    snprintf(label, LEN, format, menuCount++);

    struct PnWidget *menu = pnMenu_create(
            parent,
            3/*width*/, 3/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_None/*expand*/,
            label/*text label*/);
    ASSERT(menu);
    pnWidget_setBackgroundColor(menu, Color(), 0);

    AddItem(menu);
    AddItem(menu);
    AddItem(menu);
    AddItem(menu);
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

    pnWidget_setBackgroundColor(w, Color(), 0);
}

static void SubWindow(struct PnWidget *parent) {

    ASSERT(parent);

    struct PnWidget *w = pnWidget_create(
            parent,
            0/*width*/, 0/*height*/,
            PnLayout_TB/*layout*/, PnAlign_LT/*align*/,
            PnExpand_HV/*expand*/, 0/*size*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color(), 0);

    MenuBar(w);

    w = pnWidget_create(
            w/*parent*/, 100/*width*/, 80/*height*/,
            0/*layout*/, PnAlign_LT/*align*/,
            PnExpand_HV/*expand*/, 0/*size*/);
    ASSERT(w);

    pnWidget_setBackgroundColor(w, Color(), 0);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    srand(13);

    struct PnWidget *win = pnWindow_create(0,
            0/*width*/, 0/*height*/,
            0/*x*/, 0/*y*/, PnLayout_TB, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xAA010101, 0);

    SubWindow(win);
    SubWindow(win);
    SubWindow(win);

    pnWindow_show(win);

    Run(win);

    return 0;
}
