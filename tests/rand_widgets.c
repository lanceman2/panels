#include <signal.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../include/panels.h"

#include "../lib/debug.h"

// For a given seed we get different repeatable results.
#define SEED            (8)
#define MAX_CONTAINERS  (4)
#define TOTAL_WIDGETS   (100)

// Make rectangles that are a multiple of this size in pixels:
// Unit must be greater than 0.
static const uint32_t Unit = 4;



static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}


// TODO: We could use all the random bits that are generated and not just
// throw away unused random bits.  Note we get at most 31 random bits,
// with last printing of RAND_MAX.  Also we don't need thread safety or a
// great amount of entropy (true randomness) here.
static int32_t Rand(int32_t min, int32_t max) {

    DASSERT(max >= min);
    DASSERT(max - min >= 0);
    // My brain cannot think through integer arithmetic today so
    // I'll just use floating point and convert back.
    double x = max - min;
    x /= RAND_MAX;
    x *= rand();
    x += min;
    if(x > 0)
        x += 0.5;
    else if(x < 0)
        x -= 0.5;
    // That's like:
    //
    //   x = min + (max - min) * rand()/RAND_MAX + 0.5;
    //
    // we added 0.5 to fix the round off shift.  I know it's not quite
    // right at the edges but it's close enough.  The distribution of
    // values will be just a little off near the min and max values,
    // unless max - min is a power of 2 (so I think).
    int32_t ret = x;
    if(ret > max)
        return max;
    else if(ret < min)
        return min;
    return ret;
}

// Random color
static uint32_t Color(void) {

    uint32_t c = rand() & 0xFFFFFF;
    return c | 0xFF000000;
}


// Return a random number from min to max with more min values.  We need
// this so we can have more of the common edge case.  Usually min is 1.
//
static
int32_t SkewMinRand(int32_t min, int32_t max, int32_t n) {

    DASSERT(max + n >= min, "Not: %" PRIi32 " > %" PRIi32, max+n, min);
    DASSERT(n >= 0);

    int32_t r = Rand(min, max + n); // r = [min, max + n]

    if(r > max)
        return min; // more min values
    return r;
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
            Unit * Rand(1,2)/*width*/,
            Unit * Rand(1,2)/*height*/,
            Rand(0,1) + SkewMinRand(0, 2, 2) /*direction*/,
            0/*align*/, PnExpand_HV/*expand*/);
    ASSERT(w);
    pnWidget_setBackgroundColor(w, Color());

    CheckAddContainer(w);

    return w;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    ASSERT(Unit > 0);
    srand(SEED);

#if 0 // Test Rand() or SkewMinRand()
    WARN("Unit=%" PRIu32 " RAND_MAX=%X", Unit, RAND_MAX);
    for(uint32_t i=0; i<2000; ++i)
        printf("%" PRIi32 " ", SkewMinRand(0, 9, 0));
        //printf("%" PRIi32 " ", Rand(1, 9));
    printf("\n");
    return 0;
#endif

    win = pnWindow_create(0, 10/*width*/, 10/*height*/,
            0/*x*/, 0/*y*/, PnDirection_LR/*direction*/,
            0/*align*/, PnExpand_HV);
    ASSERT(win);
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
