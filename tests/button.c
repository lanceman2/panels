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

static struct PnWidget *win;

struct button {
    // Fuck, fuck, shit, we can't inherit PnButton because it's opaque
    // at this level of coding.  I guess C++ classes have that same
    // problem too; C++ big ass header files from hell are needed to
    // inherit classes.
    //
    // We just have the PnButton pointer and not the data here:
    struct PnWidget *button;
    int buttonNum;
};

static bool click(struct PnWidget *button, struct button *b) {

    ASSERT(b);
    ASSERT(button);
    ASSERT(button == b->button);

    fprintf(stderr, "CLICK button %d\n", b->buttonNum);
    return false;
}

static bool press(struct PnWidget *button, struct button *b) {

    ASSERT(b);
    ASSERT(button);
    ASSERT(button == b->button);

    fprintf(stderr, "PRESS button %d\n", b->buttonNum);
    return false;
}


static void destroy(struct PnWidget *button, void *b) {

    DZMEM(b, sizeof(struct button));
    free(b);
}

static void Button(void) {

    ASSERT(win);
    struct button *b = calloc(1, sizeof(*b));
    ASSERT(b, "calloc(1,%zu) failed", sizeof(*b));

    b->button = pnButton_create(
            win/*parent*/,
            30/*width*/, 20/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_HV/*expand*/,
            "Quit", false/*toggle*/);
    ASSERT(b->button);

    b->buttonNum = buttonCount++;

    pnWidget_addCallback(b->button,
            PN_BUTTON_CB_CLICK, click, b);
    pnWidget_addCallback(b->button,
            PN_BUTTON_CB_PRESS, press, b);
    pnWidget_addDestroy(b->button, destroy, b);
    pnWidget_setBackgroundColor(b->button, 0xFF707070);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    win = pnWindow_create(0, 0, 0,
            0/*x*/, 0/*y*/, PnLayout_LR, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xAA010101);

    for(int i=0; i<8; ++i)
        Button();

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
