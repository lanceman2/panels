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
                0/*parent*/, 0/*width*/, textHeight,
                10/*xPadding*/, 0/*yPadding*/,
                PnAlign_RC/*align*/, PnExpand_HV/*expand*/,
                text);
    else
        w = pnWidget_createInGrid(
                grid, width, height,
                PnLayout_None,
                0/*align*/, PnExpand_HV/*expand*/,
                column, row,
                cSpan, rSpan,
                0/*size*/);
    ASSERT(w);

    if(text)
        pnWidget_addChildToGrid(grid, w,
            column, row, cSpan, rSpan);

#ifdef CULL
    pnWidget_setClipBeforeCull(w, false);
#endif

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
            win/*parent*/, 15/*width*/, 15/*height*/,
            0/*align*/, PnExpand_HV/*expand*/,
            // We'll let the grid grow to in size on its own.  Not as
            // efficient but needs to work, and be tested.
            0/*numColumns*/, 0/*numRows*/,
            0/*memory size*/);
    ASSERT(grid);
    pnWidget_setBackgroundColor(grid, 0xA0A08080); // muddy translucent.
    //pnWidget_setBackgroundColor(grid, 0xFF000000); // black

    // For testing the grid we intentionally do not fill all the cells
    // that are in the grid.  We also skip some cell indexes in whole rows
    // and columns, though not a good idea in user code, given it can
    // waste memory by having many empty array elements in the grid.

    AddWidget("I like eggs", 0, 0, 0, 0); // Note span=0 becomes 1

    // Call this once;
    AddWidget(0,  0, 1, 1, 1);
    // now twice, to test the cell clobber with Valgrind.  This should
    // free (destroy) the widget created above, and replace it.
    AddWidget(0,  0, 1, 1, 1);

    AddWidget(0,  3, 2, 1, 1);
    AddWidget(0,  4, 2, 1, 1);
    AddWidget(0,  5, 2, 1, 1);
    AddWidget(0,  6, 2, 1, 1);
    AddWidget(0,  4, 2, 1, 1);
    AddWidget(0,  0, 3, 1, 1);
    AddWidget(0,  0, 4, 1, 1);
    AddWidget(0,  1, 0, 1, 1);
    AddWidget(0,  1, 1, 1, 1);
    AddWidget(0,  1, 0, 1, 1);
    AddWidget(0,  1, 1, 1, 1);
    AddWidget(0,  2, 3, 2, 2);
    AddWidget(0,  4, 3, 1, 2);

    AddWidget(0,  3, 0, 4, 2);
    AddWidget(0,  2, 1, 1, 1);
    AddWidget(0,  0, 2, 2, 1);

    // Wasting space in the grid:
    //
    // Test having a cell gap at y=5 and 6
    AddWidget(0,  0, 7, 6, 1);
    // Test having a cell gap at x=7 and 8
    AddWidget(0,  9, 1, 2, 6);

    pnWindow_show(win);

    Run(win);

    return 0;
}
