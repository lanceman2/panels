#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}


static struct PnWindow *win;

struct button {

    struct PnButton *button;

};


static void Button(void) {

    struct button *b = (void *) pnButton_create(
            (struct PnSurface *) win/*parent*/,
            "Quit", false/*toggle*/, sizeof(*b));
    ASSERT(b);


}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/, 0,
            PnExpand_HV);
    ASSERT(win);

    for(int i=0; i<10; ++i)
        Button();

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
