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


void AddStaticPlot(struct PnWidget *w, struct PnCallback *callback,
        uint32_t actionIndex, void *actionData) {

    DASSERT(actionIndex == PN_GRAPH_CB_STATIC_DRAW);
    DASSERT(w);
    ASSERT(IS_TYPE(w->type, W_GRAPH));

    // Set the plot default settings:
    struct PnPlot *p = (void *) callback;
    //                 A R G B
    p->lineColor =  0xFFF0F030;
    p->pointColor = 0xFFFF0000;
    p->lineWidth = 6.0;
    p->pointSize = 6.1;
    p->graph = (void *) w;
}


bool StaticDrawAction(struct PnGraph *g, struct PnCallback *callback,
        bool (*userCallback)(struct PnWidget *g, struct PnPlot *p,
                void *userData),
        void *userData, uint32_t actionIndex, void *actionData) {
    DASSERT(g);
    DASSERT(actionData == 0);
    DASSERT(actionIndex == PN_GRAPH_CB_STATIC_DRAW);
    DASSERT(g->zoom);
    DASSERT(g->cr);
    DASSERT(g->lineCr);
    DASSERT(g->pointCr);
    DASSERT(g->bgSurface);
    ASSERT(IS_TYPE(g->widget.type, W_GRAPH));
    DASSERT(userCallback);

    struct PnPlot *p = (void *) callback;

    // Initialize the last plotted x value.
    p->x = DBL_MAX;

    // userCallback() is the libpanels API user set callback.
    //
    // We let the user return the value.  true will eat the event and stop
    // this function from going through (calling) all connected
    // callbacks.

    cairo_t *pcr = g->pointCr;
    cairo_t *lcr = g->lineCr;

    // TODO: This is a little redundant, but we need these pointers in "p"
    // (too) so we can inline the pnGraph_drawPoint() function.
    //
    p->lineCr = g->lineCr;
    p->pointCr = g->pointCr;
    p->zoom = g->zoom;

    SetColor(pcr, p->pointColor);
    SetColor(lcr, p->lineColor);
    cairo_set_line_width(lcr, p->lineWidth);

    bool ret = userCallback(&g->widget, p, userData);

    const double hw = p->pointSize;
    const double w = 2.0*hw;

    if(p->x != DBL_MAX && p->pointSize > 0) {
        // Draw the last x, y point.
        cairo_rectangle(pcr, p->x - hw, p->y - hw, w, w);
        cairo_fill(pcr);
    }

    return ret;
}
