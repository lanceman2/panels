#include <signal.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../include/panels.h"
#include "../lib/debug.h"

#include "rand.h"
#include "run.h"

// For a given seed we get different repeatable results.
#define SEED            (3)
#define MAX_CONTAINERS  (15)
// TOTAL_WIDGETS 1000 => program was a little sluggish but worked.
#define TOTAL_WIDGETS   (80)

// Make rectangles that are a multiple of this size in pixels:
//static const uint32_t Unit = 0;
static const uint32_t Unit = 13;


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}


static struct PnSurface *containers[MAX_CONTAINERS] = {0};
static struct PnWidget *win = 0;
static uint32_t num_containers = 0;


static
void *GetContainer() {

    return containers[SkewMinRand(0,num_containers-1, 0)];
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

static bool EnterW(struct PnWidget *w,
            uint32_t x, uint32_t y, void *userData) {

    pnWidget_setBackgroundColor(w, 0xFFFF0000);
    pnWidget_queueDraw(w);

    return true; // take focus.
}

static void LeaveW(struct PnWidget *w, void *userData) {

    pnWidget_setBackgroundColor(w, *(uint32_t *) userData);
    pnWidget_queueDraw(w);
}

static bool Press(struct PnWidget *w, uint32_t which,
            int32_t x, int32_t y, void *userData) {

    ASSERT(which <= 2);

    uint32_t color = 0xFF000000 + (0xFF0000 >> (which * 8));

    pnWidget_setBackgroundColor(w, color);
    pnWidget_queueDraw(w);

    // taking focus lets us get the leave event handler called.
    return true; // true => eat it
}

static bool Release(struct PnWidget *w, uint32_t which,
            int32_t x, int32_t y, void *userData) {

    ASSERT(which <= 2);

    uint32_t color = pnWidget_getBackgroundColor(w);

    color &= 0xFFFFFFFF & (~(0xFF0000 >> (which * 8)));

    pnWidget_setBackgroundColor(w, color);
    pnWidget_queueDraw(w);
    return true;
}

static int count = 0;

// If the Enter() and Leave() calls are not paired up we assert.

static bool Enter(struct PnWidget *w,
            uint32_t x, uint32_t y, void *userData) {

    ++count;

    ASSERT(count == 1, "Number of Enter() and Leave() is wrong");
    pnWidget_setBackgroundColor(w, 0xFF642400);
    pnWidget_queueDraw(w);
    // taking focus lets us get the leave event handler called.
    return true; // true => take focus.
}

static void Leave(struct PnWidget *w, void *userData) {

    --count;

    ASSERT(count == 0, "Leave() missing Enter() before");
    ASSERT(userData);
    pnWidget_setBackgroundColor(w, *(uint32_t *) userData);
    pnWidget_queueDraw(w);
}

static void Destroy(struct PnWidget *w, void *data) {

    ASSERT(w);
    ASSERT(data);
    free(data);
}

static struct PnWidget *Widget() {

    DASSERT(win);

    // Get a parent at random:
    struct PnWidget *parent = GetContainer();

    DASSERT(parent);

    struct PnWidget *w = pnWidget_create(
            parent,
            Unit * Rand(0,2)/*width*/,
            Unit * Rand(0,2)/*height*/,
            Rand(0,1) + SkewMinRand(0, 2, 2) /*layout*/,
            // enum PnAlign goes from 0 to 15.
            SkewMinRand(0, 15, 2)/*align*/,
            Rand(0,3)/*expand*/, 0/*size*/);
    ASSERT(w);

    uint32_t *color = malloc(sizeof(*color));
    ASSERT(color, "malloc(%zu) failed", sizeof(*color));
    *color = Color();
    pnWidget_setBackgroundColor(w, *color);

    pnWidget_setEnter(w, Enter, color);
    pnWidget_setLeave(w, Leave, color);
    pnWidget_setPress(w, Press, color);
    pnWidget_setRelease(w, Release, color);
    pnWidget_addDestroy(w, Destroy, color);

    CheckAddContainer(w);

    return w;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(SEED);

    win = pnWindow_create(0, 16/*width*/, 16/*height*/,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/,
            0/*align*/, PnExpand_HV);
    ASSERT(win);
    uint32_t color = 0xFF000000;
    pnWidget_setBackgroundColor(win, color);
    pnWidget_setEnter(win, EnterW, &color);
    pnWidget_setLeave(win, LeaveW, &color);

    containers[0] = (void *) win;
    num_containers = 1;

    for(uint32_t i=0; i<TOTAL_WIDGETS; ++i)
        Widget();

    pnWindow_show(win, true);

    Run(win);

    return 0;
}
