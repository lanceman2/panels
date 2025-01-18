#include <stdlib.h>

#include "../include/panels.h"

#include "../lib/debug.h"



uint32_t GetColor(void) {

    return 0xF2000000 | (rand() & 0x00FFFFFF);
}


static
struct PnWidget *W(void *container, enum PnGravity gravity) {

    struct PnWidget *w = pnWidget_create((void *) container,
            20/*width*/, 20/*height*/,
            gravity, 0/*greed*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, GetColor());
    return w;
}


int main(void) {

    srand(89);

    struct PnWindow *win = pnWindow_create(0, 20, 20,
            0/*x*/, 0/*y*/, PnGravity_LR/*gravity*/);
    ASSERT(win);
    //pnWindow_setBackgroundColor(win, GetColor());
    struct PnWidget *w[30];

    w[0] = W(win, PnGravity_LR);
    w[0] = W(win, PnGravity_TB);
    w[1] = W(win, PnGravity_LR);
    w[2] = W(win, PnGravity_BT);
    w[3] = W(w[1], PnGravity_LR);
    w[4] = W(w[1], PnGravity_BT);
    w[5] = W(w[4], PnGravity_BT);
    w[6] = W(w[4], PnGravity_TB);
    w[7] = W(w[2], PnGravity_LR);
    w[8] = W(w[7], PnGravity_LR);
    w[9] = W(w[7], PnGravity_LR);
    w[10] = W(w[9], PnGravity_LR);
    w[5] = W(w[4], PnGravity_BT);
    w[6] = W(w[4], PnGravity_TB);
    w[7] = W(w[2], PnGravity_LR);
    w[8] = W(w[7], PnGravity_BT);
    w[9] = W(w[7], PnGravity_LR);
    w[10] = W(w[9], PnGravity_LR);

    w[0] = W(win, PnGravity_LR);
    w[1] = W(win, PnGravity_LR);
    w[2] = W(win, PnGravity_BT);
    w[3] = W(w[1], PnGravity_LR);
    w[4] = W(w[1], PnGravity_BT);
    w[5] = W(w[4], PnGravity_BT);
    w[6] = W(w[4], PnGravity_TB);
    w[7] = W(w[2], PnGravity_LR);
    w[8] = W(w[7], PnGravity_LR);
    w[9] = W(w[7], PnGravity_LR);
    w[10] = W(w[9], PnGravity_LR);
    w[5] = W(w[4], PnGravity_BT);
    w[6] = W(w[4], PnGravity_TB);
    w[7] = W(w[2], PnGravity_LR);
    w[8] = W(w[7], PnGravity_LR);
    w[9] = W(w[7], PnGravity_LR);
    w[10] = W(w[9], PnGravity_LR);

    w[5] = W(w[4], PnGravity_BT);
    w[6] = W(w[4], PnGravity_TB);
    w[2] = W(w[10], PnGravity_LR);
    w[8] = W(w[7], PnGravity_LR);
    w[9] = W(w[7], PnGravity_LR);
    w[10] = W(w[9], PnGravity_LR);
    w[5] = W(w[4], PnGravity_BT);
    w[5] = W(w[4], PnGravity_BT);
    w[5] = W(w[4], PnGravity_BT);
    w[9] = W(w[4], PnGravity_BT);
    w[6] = W(w[4], PnGravity_TB);
    w[7] = W(w[2], PnGravity_LR);
    w[8] = W(w[7], PnGravity_LR);
    w[9] = W(w[7], PnGravity_LR);
    w[10] = W(w[9], PnGravity_LR);


    pnWindow_show(win, true);


#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    return 0;
}
