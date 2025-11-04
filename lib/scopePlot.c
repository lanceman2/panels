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
#include "SetColor.h"



void AddScopePlot(struct PnWidget *w, struct PnCallback *callback,
        uint32_t actionIndex, void *actionData) {

    DASSERT(actionIndex == PN_GRAPH_CB_SCOPE_DRAW);
    DASSERT(w);
    ASSERT(IS_TYPE1(w->type, PnWidgetType_graph));

    // Set the plot default settings:
    struct PnPlot *p = (void *) callback;
    struct PnGraph *g = (void *) w;
    //                 A R G B
    p->lineColor =  0xFFF0F030;
    p->pointColor = 0xFFFF0000;
    p->lineWidth = 6.0;
    p->pointSize = 6.1;
    p->graph = g;
    // TODO: removing scopes?
    if(!p->graph->have_scopes)
        p->graph->have_scopes = true;
}


bool ScopeDrawAction(struct PnGraph *g, struct PnCallback *callback,
        bool (*userCallback)(struct PnWidget *g, struct PnPlot *p,
                void *userData,
                double xMin, double xMax, double yMin, double yMax),
        void *userData, uint32_t actionIndex, void *actionData) {
    DASSERT(g);
    DASSERT(actionData == 0);
    DASSERT(actionIndex == PN_GRAPH_CB_SCOPE_DRAW);
    DASSERT(g->zoom);
    DASSERT(g->cr);
    DASSERT(g->bgSurface.lineCr);
    DASSERT(g->bgSurface.pointCr);
    DASSERT(g->bgSurface.surface);
    DASSERT(g->scopeSurface.lineCr);
    DASSERT(g->scopeSurface.pointCr);
    DASSERT(g->scopeSurface.surface);
    ASSERT(IS_TYPE1(g->widget.type, PnWidgetType_graph));
    DASSERT(userCallback);

    struct PnPlot *p = (void *) callback;

    // Initialize the last plotted x value.
    p->x = DBL_MAX;

    p->shiftX = g->padX - g->slideX;
    p->shiftY = g->padY - g->slideY;

    // userCallback() is the libpanels API user set callback.
    //
    // We let the user return the value.  true will eat the event and stop
    // this function from going through (calling) all connected
    // callbacks.

    cairo_t *pcr = g->scopeSurface.pointCr;
    cairo_t *lcr = g->scopeSurface.lineCr;

    // TODO: Put this in CreateBGSurface() in graph.c.
    cairo_set_operator(pcr, CAIRO_OPERATOR_SOURCE);
    cairo_set_operator(lcr, CAIRO_OPERATOR_SOURCE);

    // TODO: This is a little redundant, but we need these pointers in "p"
    // (too) so we can inline the pnGraph_drawPoint() function without
    // having to add many pointer dereferences, or having the user pass
    // a pointer to the graph (just the plot pointer is passed).
    //
    p->lineCr = g->scopeSurface.lineCr;
    p->pointCr = g->scopeSurface.pointCr;
    p->zoom = g->zoom;

    SetColor(pcr, p->pointColor);
    SetColor(lcr, p->lineColor);
    cairo_set_line_width(lcr, p->lineWidth);

    // userCallback() may call the pnGraph_drawPoint() function many
    // times.
    bool ret = userCallback(&g->widget, p, userData,
            g->xMin, g->xMax, g->yMin, g->yMax);

    const double hw = p->pointSize;
    const double w = 2.0*hw;

    if(p->x != DBL_MAX && p->pointSize > 0) {
        // Draw the last x, y point.
        cairo_rectangle(pcr, p->x - hw, p->y - hw, w, w);
        cairo_fill(pcr);
    }

    return ret;
}
