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

static int labelCount = 0;

static struct PnWidget *win;


static void Label(void) {

    const char *format = "L %" PRIu32
        " Padding=(%" PRIu32 ", %" PRIu32 ")";
    uint32_t xPadding = 10*Rand(0,1);
    uint32_t yPadding = 10*Rand(0,1);

    const size_t Len = strlen(format) + 2;
    char text[Len];
    snprintf(text, Len, format, labelCount++, xPadding, yPadding);

    struct PnWidget *label = (void *) pnLabel_create(
            win/*parent*/,
            0/*width*/, 30/*height*/,
            xPadding, yPadding,
            Rand(0,15)/*align*/,
            PnExpand_HV/*expand*/, text);
    ASSERT(label);
    pnLabel_setFontColor(label, 0xF0000000);
    pnWidget_setBackgroundColor(label, Color(), 0);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(3);

    win = pnWindow_create(0, 3, 3,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xA0010101, 0);

    for(int i=0; i<5; ++i)
        Label();

    pnWindow_show(win);

    Run(win);

    return 0;
}
