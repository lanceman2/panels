#include <stdlib.h>
#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"


static
uint32_t GetColor(void) {

    return 0xF2000000 | (rand() & 0x00FFFFFF);
}


static
struct PnWidget *W(void *container, enum PnLayout layout) {

    struct PnWidget *w = pnWidget_create((void *) container,
            14/*width*/, 14/*height*/,
            layout, 0/*align*/,
            PnExpand_H | PnExpand_V/*expand*/, 0);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, GetColor());
    return w;
}


static void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}



int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    srand(89);

    struct PnWindow *win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    //pnWindow_setBackgroundColor(win, GetColor());
    struct PnWidget *w[30];

    w[0] = W(win, PnLayout_LR);
    w[29] = W(win, PnLayout_TB);
    w[1] = W(win, PnLayout_LR);
    w[2] = W(win, PnLayout_BT);
    w[3] = W(w[1], PnLayout_LR);
    w[4] = W(w[1], PnLayout_BT);
    w[5] = W(w[4], PnLayout_BT);
    w[6] = W(w[4], PnLayout_TB);
    w[6] = W(w[6], PnLayout_TB);
    w[6] = W(w[4], PnLayout_TB);
    w[6] = W(w[1], PnLayout_TB);
    w[7] = W(w[2], PnLayout_LR);
    w[8] = W(w[7], PnLayout_LR);
    w[9] = W(w[7], PnLayout_LR);
    w[10] = W(w[9], PnLayout_LR);
    w[5] = W(w[4], PnLayout_BT);
    w[6] = W(w[4], PnLayout_TB);
    w[7] = W(w[2], PnLayout_LR);
    w[8] = W(w[7], PnLayout_BT);
    w[9] = W(w[7], PnLayout_LR);
    w[10] = W(w[9], PnLayout_LR);
    w[10] = W(w[9], PnLayout_LR);
    w[5] = W(w[4], PnLayout_BT);
    w[6] = W(w[4], PnLayout_TB);
    w[7] = W(w[2], PnLayout_LR);
    w[8] = W(w[7], PnLayout_BT);
    w[9] = W(w[7], PnLayout_LR);
    w[10] = W(w[9], PnLayout_LR);
#if 1
    w[0] = W(win, PnLayout_LR);
    w[1] = W(win, PnLayout_LR);
    w[2] = W(win, PnLayout_BT);
    w[3] = W(w[1], PnLayout_LR);
    w[4] = W(w[1], PnLayout_BT);
    w[5] = W(w[4], PnLayout_BT);
    w[6] = W(w[4], PnLayout_LR);
    w[7] = W(w[2], PnLayout_LR);
    w[8] = W(w[7], PnLayout_TB);
    w[9] = W(w[7], PnLayout_LR);
    w[10] = W(w[9], PnLayout_LR);
    w[5] = W(w[4], PnLayout_BT);
    w[6] = W(w[4], PnLayout_TB);
    w[7] = W(w[2], PnLayout_LR);
    w[8] = W(w[7], PnLayout_LR);
    w[9] = W(w[7], PnLayout_LR);
    w[10] = W(w[9], PnLayout_LR);

    w[5] = W(w[4], PnLayout_BT);
    w[6] = W(w[4], PnLayout_TB);
    w[2] = W(w[10], PnLayout_LR);
    w[8] = W(w[7], PnLayout_LR);
    w[9] = W(w[7], PnLayout_LR);
    w[10] = W(w[9], PnLayout_LR);
    w[5] = W(w[4], PnLayout_BT);
    w[5] = W(w[4], PnLayout_BT);
    w[5] = W(w[4], PnLayout_BT);
    w[9] = W(w[4], PnLayout_BT);
    w[6] = W(w[4], PnLayout_TB);
    w[7] = W(w[2], PnLayout_LR);
    w[8] = W(w[7], PnLayout_LR);
    w[9] = W(w[7], PnLayout_LR);
    w[10] = W(w[9], PnLayout_LR);
#endif

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
