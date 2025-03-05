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

    struct PnWindow *win = pnWindow_createAsGrid(0/*parent*/,
        2/*width*/, 2/*height*/, 0/*x*/, 0/*y*/,
        0/*align*/, 0/*expand*/,
        8/*numColumns*/, 5/*numRows*/);
    ASSERT(win);

    pnWindow_show(win, true);

    //Run(win);


    fprintf(stderr, "   win=%p\n", win);
    // This passes Valgrind with or without destroying the window
    // given the libpanels.so destructor cleans up the windows and
    // display if the user does not.

    if(win)
        // Yes.  The win pointer is still valid.
        // Just testing that we can call this:
        pnWindow_destroy(win);

    return 0;
}
