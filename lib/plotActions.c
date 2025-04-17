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

    // Set the plot default settings:
    struct PnPlot *p = (void *) callback;
    //                 A R G B
    p->lineColor =  0xFFF0F030;
    p->pointColor = 0xFFFF0000;
    p->lineWidth = 6.0;
    p->pointSize = 6.1;

    WARN();
}


static inline void SetColor(cairo_t *cr, uint32_t color) {
    DASSERT(cr);
    cairo_set_source_rgba(cr, PN_R_DOUBLE(color),
            PN_G_DOUBLE(color), PN_B_DOUBLE(color), PN_A_DOUBLE(color));
}


bool StaticDrawAction(struct PnGraph *g, struct PnCallback *callback,
        bool (*userCallback)(struct PnWidget *g, struct PnPlot *p, void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {
    DASSERT(g);
    DASSERT(actionData == 0);
    DASSERT(actionIndex == PN_GRAPH_CB_STATIC_DRAW);
    DASSERT(g->cr);
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

    cairo_t *cr = g->cr;
    DASSERT(cr);

    g->crp = cairo_create(g->bgSurface);
    cairo_set_operator(g->crp, CAIRO_OPERATOR_OVER);
    DASSERT(g->crp);
    SetColor(g->crp, p->pointColor);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    SetColor(cr, p->lineColor);
    cairo_set_line_width(cr, p->lineWidth);

    bool ret = userCallback(&g->widget, p, userData);

    const double hw = 6.1;
    const double w = 2.0*hw;

    if(g->x != DBL_MAX) {
        // Draw the last x, y point.
        cairo_rectangle(g->crp, g->x - hw, g->y - hw, w, w);
        cairo_fill(g->crp);
    }

    cairo_destroy(g->crp);

    return ret;
}

void pnGraph_drawPoint(struct PnWidget *graph,
        double x, double y) {

    DASSERT(graph);
    ASSERT(GET_WIDGET_TYPE(graph->type) == W_GRAPH);

    struct PnGraph *g = (void *) graph;
    const double hw = 6.1;
    const double w = 2.0*hw;

    struct PnZoom *z = g->zoom;
    cairo_t *cr = g->cr;

    x = xToPix(x, z);
    y = yToPix(y, z);

    if(g->x != DBL_MAX) {
        cairo_move_to(cr, g->x, g->y);
        cairo_line_to(cr, x, y);
        // Calling this often must save having to store all
        // those points in the cairo_t object.
        cairo_stroke(cr);

        // Draw the last x, y point.
        cairo_rectangle(g->crp, g->x - hw, g->y - hw, w, w);
        cairo_fill(g->crp);
    }

    g->x = x;
    g->y = y;
}

void pnGraph_drawPoints(struct PnWidget *graph,
        const double *ax, const double *ay, uint32_t numPoints) {

    DASSERT(graph);
    ASSERT(GET_WIDGET_TYPE(graph->type) == W_GRAPH);

    if(!numPoints) return;

    DASSERT(ax);
    DASSERT(ay);

    struct PnGraph *g = (void *) graph;
    const double hw = 6.1;
    const double w = 2.0*hw;

    struct PnZoom *z = g->zoom;
    cairo_t *cr = g->cr;
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

        cairo_move_to(cr, g->x, g->y);
        cairo_line_to(cr, x, y);
        // Calling this often must save having to store all
        // those points in the cairo_t object.
        cairo_stroke(cr);

        // Draw the last x, y point.
        cairo_rectangle(g->crp, g->x - hw, g->y - hw, w, w);
        cairo_fill(g->crp);

        g->x = x;
        g->y = y;
    }
}
