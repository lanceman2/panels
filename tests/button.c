#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static int buttonCount = 0;

static struct PnWindow *win;

struct button {
    // Fuck, fuck, shit, we can't inherit PnButton because it's opaque
    // at this level of coding.  I guess C++ classes have that same
    // problem too; C++ big ass header files from hell are needed to
    // inherit classes.
    //
    // We just have the PnButton pointer and not the data here:
    struct PnButton *button;
    int buttonNum;
};

static bool click(struct PnButton *button, struct button *b) {

    ASSERT(b);
    ASSERT(button);
    ASSERT(button == b->button);

    fprintf(stderr, "CLICK button %d\n", b->buttonNum);
    return false;
}

static void destroy(struct PnButton *button, struct button *b) {

    DZMEM(b, sizeof(*b));
    free(b);
}

static void Button(void) {

    struct button *b = calloc(1, sizeof(*b));
    ASSERT(b, "calloc(1,%zu) failed", sizeof(*b));

    b->button = (void *) pnButton_create(
            (struct PnSurface *) win/*parent*/,
            0/*width*/, 0/*height*/,
            0/*direction*/, 0/*align*/,
            PnExpand_HV/*expand*/,
            "Quit", false/*toggle*/, 0/*size*/);
    ASSERT(b->button);

    b->buttonNum = buttonCount++;

    pnWidget_addCallback((void *) b->button,
            PN_BUTTON_CB_CLICK, click, b);
    pnWidget_addDestroy((void *) b->button, (void *) destroy, b);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    win = pnWindow_create(0, 3, 3,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setBackgroundColor(win, 0xA0010101);

    for(int i=0; i<8; ++i)
        Button();

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
