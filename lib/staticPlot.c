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
#include "graph.h"


void AddStaticPlot(struct PnWidget *w, struct PnCallback *callback,
        uint32_t actionIndex, void *actionData) {

    DASSERT(actionIndex == PN_GRAPH_CB_STATIC_DRAW);
    DASSERT(w);
    ASSERT(GET_WIDGET_TYPE(w->type) == W_GRAPH);

    // Set the plot default settings:
    struct PnPlot *p = (void *) callback;
    //                 A R G B
    p->lineColor =  0xFFF0F030;
    p->pointColor = 0xFFFF0000;
    p->lineWidth = 6.0;
    p->pointSize = 6.1;
    p->graph = (void *) w;

    WARN();
}


static inline void SetColor(cairo_t *cr, uint32_t color) {
    DASSERT(cr);
    cairo_set_source_rgba(cr, PN_R_DOUBLE(color),
            PN_G_DOUBLE(color), PN_B_DOUBLE(color), PN_A_DOUBLE(color));
}


bool StaticDrawAction(struct PnGraph *g, struct PnCallback *callback,
        bool (*userCallback)(struct PnWidget *g, struct PnPlot *p,
                void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {
    DASSERT(g);
    DASSERT(actionData == 0);
    DASSERT(actionIndex == PN_GRAPH_CB_STATIC_DRAW);
    DASSERT(g->cr);
    DASSERT(g->lineCr);
    DASSERT(g->pointCr);
    DASSERT(g->bgSurface);
    ASSERT(GET_WIDGET_TYPE(g->widget.type) == W_GRAPH);
    DASSERT(userCallback);

    struct PnPlot *p = (void *) callback;

    // Initialize the last plotted x value.
    g->x = DBL_MAX;

    // userCallback() is the libpanels API user set callback.
    //
    // We let the user return the value.  true will eat the event and stop
    // this function from going through (calling) all connected
    // callbacks.

    cairo_t *pcr = g->pointCr;
    cairo_t *lcr = g->lineCr;

    SetColor(pcr, p->pointColor);
    SetColor(lcr, p->lineColor);
    cairo_set_line_width(lcr, p->lineWidth);

    bool ret = userCallback(&g->widget, p, userData);

    const double hw = p->pointSize;
    const double w = 2.0*hw;

    if(g->x != DBL_MAX && p->pointSize > 0) {
        // Draw the last x, y point.
        cairo_rectangle(pcr, g->x - hw, g->y - hw, w, w);
        cairo_fill(pcr);
    }

    return ret;
}

// Return true for cull.
static inline bool CullPoint(struct PnGraph *g, double x, double y) {

    return false;
}

// Return true for cull.
static inline bool CullLine(struct PnGraph *g,
        double x0, double y0, double x1, double y1) {
    return false;
}



void pnGraph_drawPoint(struct PnPlot *p,
        double x, double y) {

    DASSERT(p);
    struct PnGraph *g = p->graph;
    DASSERT(g);
    ASSERT(GET_WIDGET_TYPE(g->widget.type) == W_GRAPH);

    const double hw = p->pointSize;
    const double w = 2.0*hw;

    struct PnZoom *z = g->zoom;
    cairo_t *cr = g->lineCr;

    x = xToPix(x, z);
    y = yToPix(y, z);

    if(g->x != DBL_MAX) {

        // TODO: maybe remove these ifs by using different functions.
        if(p->lineWidth > 0) {
            cairo_move_to(cr, g->x, g->y);
            cairo_line_to(cr, x, y);
            // Calling this often must save having to store all
            // those points in the cairo_t object.
            cairo_stroke(cr);
        }

        if(p->pointSize) {
            // Draw the last x, y point.
            cairo_rectangle(g->pointCr, g->x - hw, g->y - hw, w, w);
            cairo_fill(g->pointCr);
        }
    }

    g->x = x;
    g->y = y;
}

void pnGraph_drawPoints(struct PnPlot *p,
        const double *ax, const double *ay, uint32_t numPoints) {

    DASSERT(p);
    struct PnGraph *g = p->graph;
    DASSERT(g);
    ASSERT(GET_WIDGET_TYPE(g->widget.type) == W_GRAPH);

    if(!numPoints) return;

    DASSERT(ax);
    DASSERT(ay);

    const double hw = p->pointSize;
    const double w = 2.0*hw;

    struct PnZoom *z = g->zoom;
    cairo_t *cr = g->lineCr;
    uint32_t i = 0;

    // TODO: This is not very optimal.
 
    if(g->x == DBL_MAX) {
        g->x = xToPix(*ax++, z);
        g->y = yToPix(*ay++, z);
        ++i;
    }

    for(; i<numPoints; ++i) {

        double x = xToPix(*ax++, z);
        double y = yToPix(*ay++, z);

        // TODO: maybe remove these ifs by using different functions.
        if(p->lineWidth > 0) {
            cairo_move_to(cr, g->x, g->y);
            cairo_line_to(cr, x, y);
            // Calling this often must save having to store all
            // those points in the cairo_t object.
            cairo_stroke(cr);
        }

       if(p->pointSize > 0) {
            // Draw the last x, y point.
            cairo_rectangle(g->pointCr, g->x - hw, g->y - hw, w, w);
            cairo_fill(g->pointCr);
       }

        g->x = x;
        g->y = y;
    }
}
