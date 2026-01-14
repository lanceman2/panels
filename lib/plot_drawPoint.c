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
#include "graph.h"


// TODO: This is likely a (tight loop) bottle-neck in most user code, so
// we need to make it faster.  In-lining is step one.
//
// TODO: Add non-Cairo versions of point and line drawing; just directly
// writing to the mapped pixel memory.
//
void 
pnPlot_drawPoint(struct PnPlot *p, double x, double y) {

    const double hw = p->pointSize;
    const double w = 2.0*hw;

    struct PnZoom *z = p->zoom;
    cairo_t *cr = p->lineCr;


    // The p->shiftX and p->shiftY is only non-zero for the scope plot
    // because the scope plot is drawn on the widget surface which can be
    // smaller than the static plotting and grid surface.
    x = xToPix(x, z) - p->shiftX;
    y = yToPix(y, z) - p->shiftY;

    if(p->x != DBL_MAX) {

        // TODO: maybe remove these ifs by using different functions.
        if(p->lineWidth > 0) {
            cairo_move_to(cr, p->x, p->y);
            cairo_line_to(cr, x, y);
            // Calling this often must save having to store all
            // those points in the cairo_t object.
            cairo_stroke(cr);
        }

        if(p->pointSize) {
            // Draw the last x, y point.
            cairo_rectangle(p->pointCr, p->x - hw, p->y - hw, w, w);
            cairo_fill(p->pointCr);
        }
    }

    p->x = x;
    p->y = y;
}
