#include <signal.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "rand.h"

// For a given seed we get different repeatable results.
#define SEED            (3)
#define MAX_CONTAINERS  (15)
// TOTAL_WIDGETS 1000 => program was a little sluggish but worked.
#define TOTAL_WIDGETS   (80)

// Make rectangles that are a multiple of this size in pixels:
//static const uint32_t Unit = 0;
static const uint32_t Unit = 15;



static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}


static struct PnSurface *containers[MAX_CONTAINERS] = {0};
static struct PnWindow *win = 0;
static uint32_t num_containers = 0;


static
void *GetContainer() {

    uint32_t i = SkewMinRand(0,num_containers-1, 0);

    return containers[i];
}

static
void CheckAddContainer(struct PnWidget *w) {

    if(SkewMinRand(0, 1, 0) == 0) return;

    if(num_containers < MAX_CONTAINERS)
        // Add to the end of the array:
        containers[num_containers++] = (void *) w;
    else
        // Replace a random array element:
        containers[Rand(0, num_containers-1)] = (void *) w;
}


static struct PnWidget *Widget() {

    DASSERT(win);

    // Get a parent at random:
    void *parent = GetContainer();

    DASSERT(parent);

    struct PnWidget *w = pnWidget_create(
            (struct PnSurface *) parent,
            Unit * Rand(0,2)/*width*/,
            Unit * Rand(0,2)/*height*/,
            Rand(0,1) + SkewMinRand(0, 2, 2) /*direction*/,
            // enum PnAlign goes from 0 to 15.
            SkewMinRand(0, 15, 2)/*align*/,
            Rand(0,3)/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());

    CheckAddContainer(w);

    return w;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(SEED);

    win = pnWindow_create(0, 3/*width*/, 3/*height*/,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/,
            0/*align*/, PnExpand_HV);
    ASSERT(win);
    pnWindow_setBackgroundColor(win, 0xFF000000);
    containers[0] = (void *) win;
    num_containers = 1;

    for(uint32_t i=0; i<TOTAL_WIDGETS; ++i)
        Widget();

    pnWindow_show(win, true);

#ifdef RUN
    while(pnDisplay_dispatch());
#endif

    return 0;
}
