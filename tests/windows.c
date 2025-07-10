
#include "../include/panels.h"

#include "../lib/debug.h"
#include "run.h"


static struct PnWidget *win;

static void CreateWindow(uint32_t w, uint32_t h) {

    win = pnWindow_create(0, w, h, 0, 0, 0, 0, PnExpand_HV);
    ASSERT(win);
    pnWindow_show(win);
}


int main(void) {

    CreateWindow(400, 600);
    CreateWindow(700, 300);
    CreateWindow(200, 500);

    Run(win);

    return 0;
}
