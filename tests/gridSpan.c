#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"
#include "rand.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static struct PnWidget *grid;
static const uint32_t textHeight = 25;
static const uint32_t width = 35, height = 25;


static inline void AddWidget(const char *text,
        uint32_t column, uint32_t row,
        uint32_t cSpan, uint32_t rSpan) {
    struct PnWidget *w;

    if(text)
        w = pnLabel_create(
                0, 0/*width*/, textHeight,
                10/*xPadding*/, 5/*yPadding*/,
                PnAlign_RC/*align*/, PnExpand_HV/*expand*/,
                text, 0/*size*/);
    else
        w = pnWidget_createInGrid(
                grid, width, height,
                PnLayout_None,
                0/*align*/, PnExpand_HV/*expand*/,
                column, row,
                cSpan, rSpan,
                0/*size*/);

    if(text)
        pnWidget_addChildToGrid(grid, w,
            column, row, cSpan, rSpan);

    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    srand(1);

    struct PnWidget *win = pnWindow_create(0/*parent*/,
            0/*width*/, 0/*height*/, 0/*x*/, 0/*y*/,
            PnLayout_One,
            0/*align*/, PnExpand_HV/*expand*/);
    ASSERT(win);

    grid = pnWidget_createAsGrid(
            win/*parent*/, 5/*width*/, 5/*height*/,
            0/*align*/, PnExpand_HV/*expand*/,
            0/*numColumns*/, 0/*numRows*/,
            0/*size*/);
    ASSERT(grid);
    pnWidget_setBackgroundColor(grid, 0xFF000000);

    AddWidget("text", 0, 0, 1, 1);
    AddWidget(0, 0, 1, 1, 1);
    AddWidget(0, 1, 0, 1, 1);
    AddWidget(0, 1, 1, 1, 1);
    AddWidget(0, 1, 0, 1, 1);
    AddWidget(0, 1, 1, 1, 1);

    AddWidget(0, 2, 1, 1, 1);



    AddWidget(0, 0, 2, 2, 1);


    pnWindow_show(win, true);

    Run(win);

    return 0;
}
