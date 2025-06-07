#include <signal.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "../include/panels.h"

#include "../lib/xdg-shell-protocol.h"
#include "../lib/xdg-decoration-protocol.h"

#include "../lib/debug.h"
#include "../lib/display.h"
#include "../lib/graph.h"

#include "run.h"
#include "rand.h"


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static int Draw(struct PnWidget *widget, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 byte chunks*/,
            void *userData) {

    static int count = 0;
    if(count % 60 == 0)
        fprintf(stderr, " %d ", count);
    uint32_t x = Rand(2, w-3);
    uint32_t y = Rand(2, h-3);
    uint32_t color = Color();
    pixels[x + y * stride] = color;
    pixels[x+1 + y * stride] = color;
    pixels[x-1 + y * stride] = color;
    pixels[x + (y+1) * stride] = color;
    pixels[x + (y-1) * stride] = color;
    pixels[x+2 + y * stride] = color;
    pixels[x-2 + y * stride] = color;
    pixels[x + (y+2) * stride] = color;
    pixels[x + (y-2) * stride] = color;
    ++count;
    return 1;
}

int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWidget *win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1100, 900);

    // The auto 2D plotter grid (graph)
    struct PnWidget *graph = pnGraph_create(
            win/*parent*/,
            90/*width*/, 70/*height*/, 0/*align*/,
            PnExpand_HV/*expand*/);
    ASSERT(graph);
    //                  Color Bytes:  A R G B
    pnWidget_setBackgroundColor(graph, 0xA0101010);
    pnGraph_setView(graph, -1.05, 1.05, -1.05, 1.05);

    struct PnWidget *over = pnWidget_create(graph/*parent*/,
        0/*width*/, 0/*height*/, PnLayout_Cover,
        0/*align*/, PnExpand_HV, 0/*size*/);
    ASSERT(over);
    pnWidget_setDraw(over, Draw, 0);
    pnGraph_setLabelsColor(graph, 0xFFFF0000);

    pnWindow_show(win, true);

    Run(win);
    return 0;
}
