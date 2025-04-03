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

// We have: drag zooming, box zooming

// Which mouse button does what:
#define BUTTON_DRAG     0 // left mouse button = 0
#define BUTTON_BOX      2 // right mouse button = 2


// State in state flag:
//
#define ENTERED   01 // TODO: this may not be needed.
// Did the mouse pointer move with a action button pressed?
#define MOVED     02

// Actions in state flag:
#define ACTION_DRAG       04
#define ACTION_BOX       010
#define ACTIONS          (ACTION_DRAG | ACTION_BOX)


static uint32_t state = 0;
static int32_t x_0, y_0;




// widget callback functions specific to the plot widget:
//
bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnPlot *p) {
    DASSERT(!state);
    DASSERT(p->boxX == INT32_MAX);
    state = ENTERED;

    return true; // take focus
}

bool leave(struct PnWidget *w, struct PnPlot *p) {

    DASSERT(p->boxX == INT32_MAX);
    DASSERT(state & ENTERED);
    DASSERT(!(state & MOVED));
    DASSERT(!(state & ACTIONS));
    state = 0;

    return true; // leave focus
}

static inline void FinishBoxZoom(struct PnPlot *p,
        int32_t x, int32_t y) {

    p->boxX = INT32_MAX;

    if(x_0 == x || y_0 == y) {
        _pnPlot_popZoom(p);
        goto finish;
    }

    double xMin = pixToX(p->padX + x_0, p->zoom);
    double xMax = pixToX(p->padX + x, p->zoom);
    double yMin = pixToY(p->padY + y, p->zoom);
    double yMax = pixToY(p->padY + y_0, p->zoom);

    DASSERT(xMin != xMax);
    DASSERT(yMin != yMax);

    if(xMin > xMax) {
        double min = xMin;
        xMin = xMax;
        xMax = min;
    }
    if(yMin > yMax) {
        double min = yMin;
        yMin = yMax;
        yMax = min;
    }

    _pnPlot_pushZoom(p, // make a new zoom in the zoom stack
              xMin, xMax, yMin, yMax);

finish:

    cairo_t *cr = cairo_create(p->bgSurface);
    _pnPlot_drawGrids(p, cr);
    cairo_destroy(cr);
    pnWidget_queueDraw(&p->widget);
}

static inline void FinishDragZoom(
        struct PnPlot *p, int32_t x, int32_t y) {

    double dx, dy;
    dx = x_0 - x;
    dy = y_0 - y;

    if(dx == 0 && dy == 0 && !(state & MOVED))
        // Nothing to do.
        return;

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
}

bool motion(struct PnWidget *w, int32_t x, int32_t y,
            struct PnPlot *p) {
    DASSERT(p);
    DASSERT(state & ENTERED);
    uint32_t action = state & ACTIONS;
    if(!action) return false;
    // We can only have one action at a time.
    DASSERT(action == ACTION_DRAG || action == ACTION_BOX);

    if(!(state & MOVED))
        // We have motion.  This lets mouse button release know that we
        // may need to make a zoom action.  We can have a mouse button
        // press event without having motion.
        state |= MOVED;

    switch(action) {
        case ACTION_DRAG: {
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
            pnWidget_queueDraw(&p->widget);
            return true;
        }
        case ACTION_BOX: {
            DASSERT(!p->slideX);
            DASSERT(!p->slideY);
            p->boxX = x_0;
            p->boxY = y_0;
            p->boxWidth = x - x_0;
            p->boxHeight = y - y_0;
            pnWidget_queueDraw(&p->widget);
            return true;
        }
    }
    // We should not get here.
    DASSERT(0);
    return false;
}

bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnPlot *p) {
    DASSERT(p);
    DASSERT(state & ENTERED);
    uint32_t action = state & ACTIONS;
    if(!action) return false;
    // We can only have one action at a time.
    DASSERT(action == ACTION_DRAG || action == ACTION_BOX);
    // Reset the action state.
    state &= ~ACTIONS;
    if(state & MOVED)
        // Reset MOVED state.
        state &= ~MOVED;

    switch(action) {
        case ACTION_DRAG:
            FinishDragZoom(p, x, y);
            return true;
        case ACTION_BOX:
            // Reset zoom box draw:
            FinishBoxZoom(p, x, y);
            return true;
    }
    // We should not get here.
    DASSERT(0);
    return false;
}

bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnPlot *p) {
    DASSERT(p);
    DASSERT(state & ENTERED);

    uint32_t action = state & ACTIONS;
    // We can only have one action at a time or none.
    DASSERT(action == ACTION_DRAG || action == ACTION_BOX ||
            action == 0);
    // Reset the action state.
    state &= ~ACTIONS;

    // Finish an old action, if there is one.
    switch(action) {
        case ACTION_DRAG:
            FinishDragZoom(p, x, y);
            break;
        case ACTION_BOX:
            break;
    }

    // Reset zoom box draw:
    p->boxX = INT32_MAX;

    // Start a new action.
    x_0 = x;
    y_0 = y;
    // Reset MOVED state.
    state &= ~MOVED;

    DASSERT(state & ENTERED);

    switch(which) {
        case BUTTON_DRAG:
            state |= ACTION_DRAG;
            return true;
        case BUTTON_BOX:
            state |= ACTION_BOX;
            return true;
    }
    return false;
}
