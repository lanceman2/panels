#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>
#include <linux/input-event-codes.h>
#include <cairo/cairo.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"
#include "graph.h"


// We have just one mouse pointer object (and one enter/leave event cycle
// at a time).  We can assume that using just one instance of mouse
// pointer state variables is all that is needed; hence we can have these
// as simple static variables (and not allocate something like them for
// each graph 2D plotter).

// We have: drag zooming, box zooming, and axis zooming.

// Which mouse button does what:
#define BUTTON_DRAG     BTN_LEFT // left mouse button
#define BUTTON_BOX      BTN_RIGHT // right mouse button


// State in state flag:
//
#define ENTERED   01 // TODO: this may not be needed.
// Did the mouse pointer move with a action button pressed?
#define MOVED     02

// The Axis (middle mouse scroll) zoom does not need a flag in "state"
// because it just zooms without needing to know a previous position like
// in grab zooming and box zooming.  We don't let it "axis zoom" if we are
// in the middle of doing grab zooming or box zooming.

// Actions in state flag:
#define ACTION_DRAG       04
#define ACTION_BOX       010
#define ACTIONS          (ACTION_DRAG | ACTION_BOX)


static uint32_t state = 0;
static struct PnAllocation a;
static int32_t x_0, y_0; // press initial pointer position
static int32_t x_l = INT32_MAX, y_l; // last pointer position


// widget callback functions specific to the graph widget:
//
bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnGraph *g) {
    DASSERT(!state);
    DASSERT(g->boxX == INT32_MAX);
    state = ENTERED;

    // We need the position of the widget, "w".
    pnWidget_getAllocation(w, &a);

    // We get the positions relative to the widget, "w".
    x_l = x - a.x;
    y_l = y - a.y;

    return true; // take focus
}

bool leave(struct PnWidget *w, struct PnGraph *g) {

    DASSERT(g->boxX == INT32_MAX);
    DASSERT(state & ENTERED);
    DASSERT(!(state & MOVED));
    DASSERT(!(state & ACTIONS));
    state = 0;
    x_l = INT32_MAX;

    return true; // leave focus
}

static inline void FinishBoxZoom(struct PnGraph *g,
        int32_t x, int32_t y) {

    g->boxX = INT32_MAX;

    // We pop a zoom if the zoom box is very small in x or y.
    if(abs(x - x_0) < 5 || abs(y - y_0) < 5) {

        if(x == x_0 && y == y_0 && !g->zoomCount)
            // This nothing to change and no zoom box to erase.
            return;

        _pnGraph_popZoom(g);
        goto finish;
    }

    double xMin = pixToX(g->padX + x_0, g->zoom);
    double xMax = pixToX(g->padX + x, g->zoom);
    double yMin = pixToY(g->padY + y, g->zoom);
    double yMax = pixToY(g->padY + y_0, g->zoom);

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

    _pnGraph_pushZoom(g, // make a new zoom in the zoom stack
              xMin, xMax, yMin, yMax);

finish:

    cairo_t *cr = cairo_create(g->bgSurface.surface);
    _pnGraph_drawGrids(g, cr);
    cairo_destroy(cr);
    pnWidget_queueDraw(&g->widget, 0);
}

static inline void FinishDragZoom(
        struct PnGraph *g, int32_t x, int32_t y) {

    double dx, dy;
    dx = x_0 - x;
    dy = y_0 - y;

    if(dx == 0 && dy == 0 && !(state & MOVED))
        // Nothing to do.
        return;

    double padX = g->padX;
    double padY = g->padY;

    if(dx > padX)
        dx = padX;
    else if(dx < - padX)
        dx = - padX;
    if(dy > padY)
        dy = padY;
    else if(dy < - padY)
        dy = - padY;

    x_0 = y_0 = g->slideX = g->slideY = 0;

    _pnGraph_pushZoom(g, // make a new zoom in the zoom stack
            pixToX(padX + dx, g->zoom),
            pixToX(g->width + padX + dx, g->zoom),
            pixToY(g->height + padY + dy, g->zoom),
            pixToY(padY + dy, g->zoom));
    cairo_t *cr = cairo_create(g->bgSurface.surface);
    _pnGraph_drawGrids(g, cr);
    cairo_destroy(cr);
    pnWidget_queueDraw(&g->widget, 0);
}

bool motion(struct PnWidget *w, int32_t x, int32_t y,
            struct PnGraph *g) {
    DASSERT(g);
    DASSERT(state & ENTERED);
    uint32_t action = state & ACTIONS;

    // We get the positions relative to the widget, "w".
    x -= a.x;
    y -= a.y;

    // We need this position for axis zooming (if there is any).  It may
    // be that we could have queried the mouse pointer position at the
    // time of the axis event, but this is not much work and this can even
    // be more efficient if we axis zoom a lot.
    x_l = x;
    y_l = y;

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
            g->slideX = x - x_0;
            g->slideY = y - y_0;
            int32_t padX = g->padX;
            int32_t padY = g->padY;
            DASSERT(padX >= 0);
            DASSERT(padY >= 0);

            if(g->slideX > padX)
                g->slideX = padX;
            else if(g->slideX < - padX)
                g->slideX = - padX;
            if(g->slideY > padY)
                g->slideY = padY;
            else if(g->slideY < - padY)
                g->slideY = - padY;
            pnWidget_queueDraw(&g->widget, 0);
            return true;
        }
        case ACTION_BOX: {
            DASSERT(!g->slideX);
            DASSERT(!g->slideY);
            g->boxX = x_0;
            g->boxY = y_0;
            g->boxWidth = x - x_0;
            g->boxHeight = y - y_0;
            pnWidget_queueDraw(&g->widget, 0);
            return true;
        }
    }
    // We should not get here.
    DASSERT(0);
    return false;
}

bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnGraph *g) {
    DASSERT(g);
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

    // We get the positions relative to the widget, "w".
    x -= a.x;
    y -= a.y;

    switch(action) {
        case ACTION_DRAG:
            FinishDragZoom(g, x, y);
            return true;
        case ACTION_BOX:
            // Reset zoom box draw:
            FinishBoxZoom(g, x, y);
            return true;
    }
    // We should not get here.
    DASSERT(0);
    return false;
}

bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnGraph *g) {
    DASSERT(g);
    DASSERT(state & ENTERED);

    uint32_t action = state & ACTIONS;
    // We can only have one action at a time or none.
    DASSERT(action == ACTION_DRAG || action == ACTION_BOX ||
            action == 0);
    // Reset the action state.
    state &= ~ACTIONS;

    // We get the positions relative to the widget, "w".
    x -= a.x;
    y -= a.y;

    // Finish an old action, if there is one.
    switch(action) {
        case ACTION_DRAG:
            FinishDragZoom(g, x, y);
            break;
        case ACTION_BOX:
            break;
    }

    // Reset zoom box draw:
    g->boxX = INT32_MAX;

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

// This is magnitude of the value I keep seeing from axis().  
#define CLICK_VAL  (15.0)

bool axis(struct PnWidget *w,
            uint32_t time, uint32_t which, double value,
            struct PnGraph *g) {

    DASSERT(g);
    DASSERT(&g->widget == w);
    DASSERT(value);

    // I'm going to go ahead and say that google maps axis interface is
    // not a bad standard to follow. I observe on google maps:
    //
    //  [1]. + axis increases zoom, which is delta x (xMax - xMin) and
    //  delta y (yMax - yMin) decrease (with positive axis values), as we
    //  define it in this code. We can think of this as giving us a
    //  parameter related to the "speed" of the zooming relative to the
    //  rate of values coming from the axis input event (1 constraint
    //  equation).
    //  [2]. The mouse pointer position stays on the same scaled value
    //  before and after zooming (2 equations, x, y).
    //  [3]. Axis zooming keeps the aspect ratio constant (1 constraint
    //  equation).
    //
    //  That gives 4 equations for 4 unknowns (zoom parameters).
    //

    if(x_l == INT32_MAX // We have no mouse pointer position
            || value == 0.0 // We have no axis event value
            // We are in the middle of zooming using another method.
            || state & ACTIONS
        )
        return false;

    //INFO("value=%lf", value);

    value /= 15.0;
    value *= 0.01;

    struct PnZoom *z = g->zoom;

    // We had these values before but we re-parameterized the
    // transformation in terms of shifts and slopes.
    // This may not be optimal (for compute speed), but this
    // is very easy to follow, and is plenty fast.
    double xMin = pixToX(g->padX, z);
    double xMax = pixToX(g->width + g->padX, z);
    double yMax = pixToY(g->padY, z);
    double yMin = pixToY(g->height + g->padY, z);


    // Try to scale it by increasing or decreasing the difference between
    // plotted end values (max - min).
    double d = xMax - xMin;
    DASSERT(d > 0.0);
    d *= value;
    xMin -= d;
    xMax += d;

    d = yMax - yMin;
    DASSERT(d > 0.0);
    d *= value;
    yMin -= d;
    yMax += d;

    if(CheckZoom(xMin, xMax, yMin, yMax))
        // CheckZoom() will spew.
        //
        // This tried to zoom too much.  The floating point round-off is
        // too much; that is the relative differences are too small.
        return true;

    if(g->zoomCount < 2) {
        // Save a copy of the current zoom so that the user can "zoom-out"
        // back to it.  It's nice to be able to go back to the graph view
        // that the graph started at.
        PushCopyZoom(g);
        // Now we'll change the current newer zoom.
        z = g->zoom;
    }

    // We don't create a new zoom, except for when there a too few zooms
    // (like the above if block).  We just recalculate the current zoom.
    // But, first we need these mapped shifted values:
    double X_l = pixToX(x_l + g->padX, z);
    double Y_l = pixToY(y_l + g->padY, z);
    // Now we don't need the old scales (slopes), so we get the new ones:
    z->xSlope = (xMax - xMin)/g->width;
    z->ySlope = (yMin - yMax)/g->height;
    //z->xShift = xMin - g->padX * z->xSlope;
    //z->yShift = yMax - g->padY * z->ySlope;
    // Now the relative scale is good.  But, we wish to have the current
    // mouse position, x_l, y_l, map to the same position as it did before
    // this scaling.  Now we recalculate the shifts:
    //
    // X_1 = (x_1 + padX) * xSlope + Xshift
    // ==> Xshift = X_1 - (x_1 + padX) * xSlope
    z->xShift = X_l - (x_l + g->padX) * z->xSlope;
    z->yShift = Y_l - (y_l + g->padY) * z->ySlope;

    //DSPEW(" %d,%d   %lg,%lg == %lg,%lg", x_l, y_l, X_l, Y_l,
    //        pixToX(x_l + g->padX, z), pixToY(y_l + g->padY, z));

    // Draw the g->bgSurface.surface which will be the new graph
    // "background" surface with this zoom.

    cairo_t *cr = cairo_create(g->bgSurface.surface);
    _pnGraph_drawGrids(g, cr);
    cairo_destroy(cr);

    pnWidget_queueDraw(&g->widget, 0);
    return true;
}
