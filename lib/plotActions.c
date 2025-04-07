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

bool StaticDrawAction(struct PnPlot *p,
        bool (*callback)(struct PnWidget *p, cairo_t *cr, void *userData),
        void *userData, void *actionData) {
    DASSERT(p);
    DASSERT(actionData == 0);
    DASSERT(p->cr);
    DASSERT(p->bgSurface);
    ASSERT(GET_WIDGET_TYPE(p->widget.type) == W_PLOT);
    DASSERT(callback);

    // Initialize the last plotted x value.
    p->x = DBL_MAX;

    // callback() is the libpanels API user set callback.
    //
    // We let the user return the value.  true will eat the event and stop
    // this function from going through (calling) all connected
    // callbacks.

    cairo_t *cr = p->cr;
    DASSERT(cr);

    p->crp = cairo_create(p->bgSurface);
    DASSERT(p->crp);
    cairo_set_source_rgba(p->crp, 1, 0.0, 0.0, 1.0);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 0.9, 0.9, 0.1, 1.0);

    cairo_set_line_width(cr, 6);

    bool ret = callback(&p->widget, p->cr, userData);

    const double hw = 6.1;
    const double w = 2.0*hw;

    if(p->x != DBL_MAX) {
       // Draw the last x, y point.
        cairo_rectangle(p->crp, p->x - hw, p->y - hw, w, w);
        cairo_fill(p->crp);
    }

    cairo_destroy(p->crp);

    return ret;
}

void pnPlot_drawPoint(struct PnWidget *plot,
        double x, double y) {

    struct PnPlot *p = (void *) plot;
    const double hw = 6.1;
    const double w = 2.0*hw;

    struct PnZoom *z = p->zoom;
    cairo_t *cr = p->cr;

    x = xToPix(x, z);
    y = yToPix(y, z);

    if(p->x != DBL_MAX) {
        cairo_move_to(cr, p->x, p->y);
        cairo_line_to(cr, x, y);
        // Calling this often must save having to store all
        // those points in the cairo_t object.
        cairo_stroke(cr);

        // Draw the last x, y point.
        cairo_rectangle(p->crp, p->x - hw, p->y - hw, w, w);
        cairo_fill(p->crp);
    }

    p->x = x;
    p->y = y;
}

void pnPlot_drawPoints(struct PnWidget *plot,
        const double *x, const double *y, uint32_t numPoints) {

}
