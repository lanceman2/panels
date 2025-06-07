#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "rand.h"
#include "run.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static int buttonCount = 0;

static struct PnWidget *hbox;
static struct PnWidget *win;

static bool click(struct PnWidget *b, struct PnWidget *button) {

    ASSERT(b);
    ASSERT(b == button);

    fprintf(stderr, "CLICKED button %p\n", button);
    return false;
}

static void Button(void) {

    if(!(buttonCount % 6))
        hbox = pnWidget_create(win,
                2/*width*/, 2/*height*/,
                PnLayout_LR, 0/*align*/,
                PnExpand_HV, 0/*size*/);
    ASSERT(hbox);

    const char *format = "Button %" PRIu32;
    const size_t Len = strlen(format) + 2;
    char text[Len];
    snprintf(text, Len, format, buttonCount++);

    struct PnWidget *button = pnButton_create(hbox,
            0/*width*/, 0/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_VH, 0/*label*/, false/*toggle*/);
    ASSERT(button);

    struct PnWidget *l = (void *) pnLabel_create(
            button/*parent*/,
            0/*width*/, 30/*height*/,
            4/*xPadding*/, 4/*yPadding*/,
            0/*align*/, PnExpand_HV/*expand*/, text);
    ASSERT(l);
    pnLabel_setFontColor(l, 0xF0000000);
    uint32_t color = 0xDA000000 | (0x00FFFFFF & Color());
    pnWidget_setBackgroundColor(l, color);
    pnWidget_setBackgroundColor(button, color);
    pnWidget_addCallback(button,
            PN_BUTTON_CB_CLICK, click, button);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(3);

    win = pnWindow_create(0, 3/*width*/, 3/*height*/,
            0/*x*/, 0/*y*/, PnLayout_TB/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xFF000000);

    for(int i=0; i<20; ++i)
        Button();

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
