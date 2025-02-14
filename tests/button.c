#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWindow *win = pnWindow_create(0, 30, 30,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/, 0,
            PnExpand_HV);
    ASSERT(win);

    struct PnWidget *b = pnButton_create(
            (struct PnSurface *) win/*parent*/,
            "Quit");
    ASSERT(b);
    uint32_t color = pnWidget_getBackgroundColor(b);
    color &= 0x00FFFFFF; // change the Alpha
    //color |= 0x88000000;
    pnWidget_setBackgroundColor(b, color);

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
