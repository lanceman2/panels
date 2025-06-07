#include <signal.h>
#include "../include/panels.h"

#include "../lib/debug.h"

#include "rand.h"
#include "run.h" // define Run().


#ifdef WITH_CAIRO
#  define W (20)
#  define H (20)
#else
#  define W (700)
#  define H (400)
#endif



void run(void) {

    struct PnWidget *win = pnWindow_create(0, W, H,
            0, 0, 0, 0, PnExpand_HV);
    ASSERT(win);

    pnWidget_setBackgroundColor(win, Color()/*random color*/);

#ifdef WITH_CAIRO
    struct PnWidget *w = pnLabel_create(win/*parent*/,
            0/*width*/, 200/*height*/,
            20, 20,
            0/*align*/,
            PnExpand_H/*expand*/,
            "Hello World!");
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color()/*random color*/);
#endif

    pnWindow_show(win, true);

    Run(win);

    // We are testing the reset ability of the "panels" singleton object.
    // This destroys everything created by the libpanels library except
    // some of the libcairo and libfontconfig stuff (this stuff we refer
    // to is objects which are internal to libcairo and libfontconfig);
    // but that seems to be okay, the libcairo and libfontconfig seem to
    // be self consistent, that is not letting an "static global" data
    // fuck up non-libpanels code.  Most libraries are not this robust.
    // All the GTK and Qt libraries can't do this without leaking system
    // resources when they are unloaded (by design, not enough detail
    // here, see comments elsewhere).
    pnDisplay_destroy();
}

static void catcher(int sig) {
    ASSERT(0, "caught signal number %d", sig);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(101);

    run();
    run();
    run();
    run();

    return 0;
}
