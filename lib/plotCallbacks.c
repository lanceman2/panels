#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>

#include <cairo/cairo.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"
#include "plot.h"


// We have just one mouse pointer object (and one enter/leave event cycle
// at a time).  We can assume that using just one instance of mouse
// pointer state variables is all that is needed; hence we can have these
// as simple static variables (and not allocate something like them for
// each plot).

#define ENTERED   01
#define PRESSED   02


static uint32_t state;


// widget callback functions specific to the plot:
//
bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnPlot *p) {
    DASSERT(!state);
    state = ENTERED;

WARN();
    return true; // take focus
}

bool leave(struct PnWidget *w, struct PnPlot *p) {

    DASSERT(state & ENTERED);
    state = 0;
WARN();
    return true; // take focus
}

bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnPlot *p) {
    return true;
}

bool motion(struct PnWidget *w, int32_t x, int32_t y,
            struct PnPlot *p) {
    return true;
}

bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnPlot *p) {
    return true;
}
