#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "rand.h"
#define RUN
#include "run.h"


static void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static struct PnWidget *grid;

static bool click(struct PnWidget *b, struct PnWidget *button) {

    ASSERT(b);
    ASSERT(b == button);

    fprintf(stderr, "CLICKED button %p\n", button);
    return false;
}

static void Button(uint32_t i, uint32_t j) {


    const char *format = "Button %" PRIu32 ",%" PRIu32;
    const size_t Len = strlen(format) + 12;
    char text[Len];
    snprintf(text, Len, format, i, j);

    struct PnWidget *button = pnButton_create(0/*parent*/,
            0/*width*/, 0/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_VH, 0/*label*/, false/*toggle*/);
    ASSERT(button);
    pnWidget_addChildToGrid(grid, button, i, j, 1, 1);

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

    grid = pnWindow_createAsGrid(
        0 /*parent*/,
        5/*width*/, 5/*height*/, 0/*x*/, 0/*y*/,
        0/*align*/, PnExpand_HV/*expand*/,
        0/*numColumns*/, 0/*numRows*/);
    ASSERT(grid);
    pnWidget_setBackgroundColor(grid, 0xFF000000);

    for(uint32_t i=0; i<5; ++i)
        for(uint32_t j=0; j<5; ++j)
            Button(i, j);

    pnWindow_show(grid);

    Run(grid);

    return 0;
}
