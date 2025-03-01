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

static struct PnWindow *win;

struct label {

    struct PnLabel *label;
    int labelNum;
};


static void destroy(struct PnLabel *label, struct label *l) {
    DASSERT(label);
    DASSERT(l);
    DASSERT(label == l->label);
    DZMEM(l, sizeof(*l));
    free(l);
}

static void Label(void) {

    struct label *l = calloc(1, sizeof(*l));
    ASSERT(l, "calloc(1,%zu) failed", sizeof(*l));
    const char *format = "L %" PRIu32
        " Padding=(%" PRIu32 ", %" PRIu32 ")";
    uint32_t xPadding = 10*Rand(0,1);
    uint32_t yPadding = 10*Rand(0,1);

    const size_t Len = strlen(format) + 2;
    char text[Len];
    snprintf(text, Len, format, labelCount++, xPadding, yPadding);

    l->label = (void *) pnLabel_create(
            (struct PnSurface *) win/*parent*/,
            0/*width*/, 30/*height*/,
            xPadding, yPadding,
            Rand(0,15)/*align*/,
            PnExpand_HV/*expand*/,
            text, 0/*size*/);
    ASSERT(l->label);
    pnLabel_setFontColor(l->label, 0xF0000000);
    pnWidget_setBackgroundColor((void *)l->label, Color());
    l->labelNum = labelCount;
    pnWidget_addDestroy((void *) l->label, (void *) destroy, l);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(3);

    win = pnWindow_create(0, 3, 3,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setBackgroundColor(win, 0xA0010101);

    for(int i=0; i<5; ++i)
        Label();

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
