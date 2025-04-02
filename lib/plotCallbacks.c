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
static int32_t x_0, y_0;


// widget callback functions specific to the plot:
//
bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnPlot *p) {
    DASSERT(!state);
    state = ENTERED;

    return true; // take focus
}

bool leave(struct PnWidget *w, struct PnPlot *p) {

    DASSERT(state & ENTERED);
    state = 0;

    return true; // take focus
}

bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnPlot *p) {

    DASSERT(state & ENTERED);
    if(which != 0) return false;
    state |= PRESSED;

    x_0 = x;
    y_0 = y;

    return true;
}

bool motion(struct PnWidget *w, int32_t x, int32_t y,
            struct PnPlot *p) {
    DASSERT(state & ENTERED);
    if(!(state & PRESSED)) return false;

//fprintf(stderr, "  %" PRIi32 ",%" PRIi32, x - x_0, y - y_0);

    p->slideX = x - x_0;
    p->slideY = y - y_0;
    int32_t padX = p->padX;
    int32_t padY = p->padY;
    DASSERT(padX >= 0);
    DASSERT(padY >= 0);

    if(p->slideX > padX)
        p->slideX = padX;
    else if(p->slideX < - padX)
        p->slideX = - padX;
    if(p->slideY > padY)
        p->slideY = padY;
    else if(p->slideY < - padY)
        p->slideY = - padY;

//fprintf(stderr, "  %" PRIi32 ",%" PRIi32, p->slideX, p->slideY);

    pnWidget_queueDraw(&p->widget);

    return true;
}

bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnPlot *p) {
    DASSERT(state & ENTERED);
    if(which != 0) return false;
    DASSERT(state & PRESSED);
    state &= ~PRESSED;

    double dx, dy;
    dx = x_0 - x;
    dy = y_0 - y;

    double padX = p->padX;
    double padY = p->padY;

    if(dx > padX)
        dx = padX;
    else if(dx < - padX)
        dx = - padX;
    if(dy > padY)
        dy = padY;
    else if(dy < - padY)
        dy = - padY;

    x_0 = y_0 = p->slideX = p->slideY = 0;

    _pnPlot_pushZoom(p, // make a new zoom in the zoom stack
            pixToX(padX + dx, p->zoom),
            pixToX(p->width + padX + dx, p->zoom),
            pixToY(p->height + padY + dy, p->zoom),
            pixToY(padY + dy, p->zoom));
    cairo_t *cr = cairo_create(p->bgSurface);
    _pnPlot_drawGrids(p, cr);
    cairo_destroy(cr);
    pnWidget_queueDraw(&p->widget);


WARN();
    return true;
}
