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



// widget callback functions specific to the plot:
bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnPlot *p) {
    return true;
}

bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnPlot *p) {
    return true;
}

bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnPlot *p) {
    return true; // take focus
}

bool leave(struct PnWidget *w, struct PnPlot *p) {
    return true; // take focus
}

bool motion(struct PnWidget *w, int32_t x, int32_t y,
            struct PnPlot *p) {
    return true;
}
