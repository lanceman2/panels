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

// We have: drag zooming, box zooming, and axis zooming.

// Which mouse button does what:
#define BUTTON_DRAG     0 // left mouse button = 0
#define BUTTON_BOX      2 // right mouse button = 2


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
static int32_t x_0, y_0; // press initial pointer position
static int32_t x_l = INT32_MAX, y_l; // last pointer position


// widget callback functions specific to the plot widget:
//
bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnPlot *p) {
    DASSERT(!state);
    DASSERT(p->boxX == INT32_MAX);
    state = ENTERED;

    x_l = x;
    y_l = y;

    return true; // take focus
}

bool leave(struct PnWidget *w, struct PnPlot *p) {

    DASSERT(p->boxX == INT32_MAX);
    DASSERT(state & ENTERED);
    DASSERT(!(state & MOVED));
    DASSERT(!(state & ACTIONS));
    state = 0;
    x_l = INT32_MAX;

    return true; // leave focus
}

static inline void FinishBoxZoom(struct PnPlot *p,
        int32_t x, int32_t y) {

    p->boxX = INT32_MAX;

    // We pop a zoom if the zoom box is very small in x or y.
    if(abs(x - x_0) < 5 || abs(y - y_0) < 5) {
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

// This is magnitude of the value I keep seeing from axis().  
#define CLICK_VAL  (15.0)

bool axis(struct PnWidget *w,
            uint32_t time, uint32_t which, double value,
            struct PnPlot *p) {

    DASSERT(p);
    DASSERT(&p->widget == w);
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

    struct PnZoom *z = p->zoom;

    // We had these values before but we re-parameterized the
    // transformation in terms of shifts and slopes.
    // This may not be optimal (for compute speed), but this
    // is very easy to follow, and is plenty fast.
    double xMin = pixToX(p->padX, z);
    double xMax = pixToX(p->width + p->padX, z);
    double yMax = pixToY(p->padY, z);
    double yMin = pixToY(p->height + p->padY, z);


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

    if(p->zoomCount < 2) {
        // Save a copy of the current zoom so that the user can "zoom-out"
        // back to it.  It's nice to be able to go back to the plot view
        // that the plot started at.
        PushCopyZoom(p);
        // Now we'll change the current newer zoom.
        z = p->zoom;
    }

    // We don't create a new zoom, except for when there a too few zooms
    // (like the above if block).  We just recalculate the current zoom.
    // But, first we need these mapped shifted values:
    double X_l = pixToX(x_l + p->padX, z);
    double Y_l = pixToY(y_l + p->padY, z);
    // Now we don't need the old scales (slopes), so we get the new ones:
    z->xSlope = (xMax - xMin)/p->width;
    z->ySlope = (yMin - yMax)/p->height;
    //z->xShift = xMin - p->padX * z->xSlope;
    //z->yShift = yMax - p->padY * z->ySlope;
    // Now the relative scale is good.  But, we wish to have the current
    // mouse position, x_l, y_l, map to the same position as it did before
    // this scaling.  Now we recalculate the shifts:
    //
    // X_1 = (x_1 + padX) * xSlope + Xshift
    // ==> Xshift = X_1 - (x_1 + padX) * xSlope
    z->xShift = X_l - (x_l + p->padX) * z->xSlope;
    z->yShift = Y_l - (y_l + p->padY) * z->ySlope;

    //DSPEW(" %d,%d   %lg,%lg == %lg,%lg", x_l, y_l, X_l, Y_l,
    //        pixToX(x_l + p->padX, z), pixToY(y_l + p->padY, z));

    // Draw the p->bgSurface which will be the new plot "background"
    // surface with this zoom.

    cairo_t *cr = cairo_create(p->bgSurface);
    _pnPlot_drawGrids(p, cr);
    cairo_destroy(cr);

    pnWidget_queueDraw(&p->widget);
    return true;
}
