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
    struct PnWidget *button;
    int buttonNum;
};

static bool click(struct PnWidget *button, struct button *b) {

    ASSERT(b);
    ASSERT(button);
    ASSERT(button == b->button);

    fprintf(stderr, "Button %d is %s toggled\n",
            b->buttonNum,
            pnToggleButton_getToggled(button)?"":" not");

    return false;
}


static bool press(struct PnWidget *button, struct button *b) {

    ASSERT(b);
    ASSERT(button);
    ASSERT(button == b->button);

    fprintf(stderr, "PRESS button %d", b->buttonNum);
    fprintf(stderr, " is%s toggled",
            pnToggleButton_getToggled(button)?"":" not");
    putc('\n', stderr);

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

    static bool initToggle = true;
    b->button =
        pnToggleButton_create(
            win/*parent*/,
            30/*width*/, 20/*height*/,
            PnLayout_LR/*layout*/, 0/*align*/,
            PnExpand_HV/*expand*/,
            0/*label*/,
            // Alternate toggle values.
            (initToggle = !initToggle),
            0);

    ASSERT(b->button);

#define LEN  (122)

    char labelText[LEN];
    snprintf(labelText, LEN, "Button %d", buttonCount);

    struct PnWidget *w = pnCheck_create(b->button/*parent*/,
        30/*width*/, 30/*height*/,
        PnAlign_LC, 0/*expand*/);
    ASSERT(w);
    pnToggleButton_addCheck(b->button, w);

#if 0 // To test with more than one check:
w = pnCheck_create(b->button/*parent*/,
        30/*width*/, 30/*height*/,
        PnAlign_LC, 0/*expand*/);
    ASSERT(w);
    pnToggleButton_addCheck(b->button, w);

w = pnCheck_create(b->button/*parent*/,
        30/*width*/, 30/*height*/,
        PnAlign_LC, 0/*expand*/);
    ASSERT(w);
    pnToggleButton_addCheck(b->button, w);
#endif

    w =  pnLabel_create(
            b->button/*parent*/,
            0/*width*/, 30/*height*/,
            4/*xPadding*/, 4/*yPadding*/,
            0/*align*/, PnExpand_HV/*expand*/, labelText);
    ASSERT(w);


    b->buttonNum = buttonCount++;

    pnWidget_addCallback(b->button,
            PN_BUTTON_CB_CLICK, click, b);
    pnWidget_addCallback(b->button,
            PN_BUTTON_CB_PRESS, press, b);
    pnWidget_addDestroy(b->button, destroy, b);
    pnWidget_setBackgroundColor(b->button, 0xFF707070, 0);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    win = pnWindow_create(0, 0, 0,
            0/*x*/, 0/*y*/, PnLayout_LR, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xAA010101, 0);

    for(int i=0; i<8; ++i)
        Button();

    pnWindow_show(win);

    Run(win);

    return 0;
}
