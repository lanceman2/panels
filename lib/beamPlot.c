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
#include "SetColor.h"


struct AddBeamParameters {

    // number of points that can be displayed at most.
    // 
    uint32_t maxPoints,
        // The number of points that are faded at the end of the stored
        // points that are displayed.
        fadePoints;
};


struct PnPlot *pnScopePlot_createWithBeam(struct PnWidget *graph,
        int32_t maxPoints, int32_t fadePoints,/*beam Life in number of frames*/
        bool (*plotter)(struct PnWidget *graph, struct PnPlot *plot,
            void *userData, double xMin, double xMax, double yMin, double yMax),
        void *userData) {

    // We must consolidate the arguments into one struct pointer to pass
    // to pnWidget_addCallback().  We call pnWidget_addCallback() in this
    // function so a stack variable is fine.
    //
    struct AddBeamParameters par = { 
        .maxPoints = maxPoints,
        .fadePoints = fadePoints
    };

    ASSERT(fadePoints > 0);
    ASSERT(fadePoints <= maxPoints || maxPoints == 0);
    ASSERT(maxPoints >= 0);

    // TODO: Add type check here for graph?  Like ASSERT().
    return pnWidget_addCallback(graph, PN_GRAPH_CB_SCOPE_BEAM,
            plotter, userData, &par);
}


// Free memory when the graph widget is destroyed.
//
// The beam plot need an extra memory allocation to store the beam point
// list in.  We clean it up with the graph.
//
// TODO: Do we want a way to destroy a beam scope plot thingy without
// destroying the graph?  Maybe not.  Looks like panel widget actions
// exist for the life of the panels widget, after they are created.
//
void destroy_beam(struct PnWidget *w, struct PnPlot *p) {

    DASSERT(p);
    DASSERT(w);
    ASSERT(IS_TYPE1(w->type, PnWidgetType_graph));

    struct PnGraph *g = (void *) w;
    struct PnBeam *b = p->beam;
    DASSERT(b);

    if(g->beamPoints) {
        // Cleanup at most one per graph.
        DASSERT(g->beamPoints_width);
        DASSERT(g->beamPoints_height);
        DZMEM(g->beamPoints,
                g->beamPoints_width * g->beamPoints_height *
                sizeof(*g->beamPoints));
        g->beamPoints = 0;
        g->beamPoints_width = 0;
        g->beamPoints_height = 0;
    }

    DZMEM(b, sizeof(*b));
    free(b);
    p->beam = 0;
}


// Add a scope plot with fading beam that does not use Cairo to draw.
// Cairo is still used to draw the grid background for the graph widget.
//
void AddScopeBeamPlot(struct PnWidget *w, struct PnCallback *callback,
        uint32_t actionIndex, void *actionData, void *addData) {

    DASSERT(actionIndex == PN_GRAPH_CB_SCOPE_BEAM);
    DASSERT(w);
    ASSERT(IS_TYPE1(w->type, PnWidgetType_graph));
    DASSERT(addData);

    // Set the plot default settings:
    struct PnPlot *p = (void *) callback;

    struct PnGraph *g = (void *) w;
    p->type = PnPlotType_dynamic;
    //                 A R G B
    p->lineColor =  0xFFF0F030;
    p->pointColor = 0xFFFF0000;
    p->lineWidth = 6.0;
    p->pointSize = 6.1;
    p->graph = g;

    p->drawMethod = PnDrawMethod_beam;

    struct AddBeamParameters *par = addData;

    struct PnBeam *beam;
    beam = p->beam = calloc(1, sizeof(*beam));
    ASSERT(beam, "calloc(1,%zu) failed", sizeof(*beam));
    pnWidget_addDestroy(w, (void *) destroy_beam, p);

    beam->maxPoints = par->maxPoints;
    beam->fadePoints = par->fadePoints;

    // TODO: removing scopes?
    if(!p->graph->have_scopes)
        p->graph->have_scopes = true;
}


// Call the users callback for the scope plot.
//
// This will collect points to the beam when the user calls 
//
bool ScopeBeamDrawAction(struct PnGraph *g, struct PnCallback *callback,
        bool (*userCallback)(struct PnWidget *g, struct PnPlot *p,
                void *userData,
                double xMin, double xMax, double yMin, double yMax),
        void *userData, uint32_t actionIndex, void *actionData) {
    DASSERT(g);
    DASSERT(actionData == 0);
    DASSERT(actionIndex == PN_GRAPH_CB_SCOPE_BEAM);
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
    DASSERT(g->have_scopes);
    DASSERT(g->widget.cairo_surface == g->scopeSurface.surface);

    struct PnPlot *p = (void *) callback;
    DASSERT(p->drawMethod == PnDrawMethod_beam);
    struct PnBeam *beam = p->beam;
    DASSERT(beam);

    beam->pixels = p->graph->widget.window->buffer.pixels;
    beam->stride = p->graph->widget.window->buffer.stride;
    uint32_t width = p->graph->widget.allocation.width;
    uint32_t height = p->graph->widget.allocation.height;

    DASSERT(beam->pixels);
    DASSERT(beam->stride);
    DASSERT(width);
    DASSERT(height);

    p->zoom = g->zoom;
    p->shiftX = g->padX - g->slideX;
    p->shiftY = g->padY - g->slideY;

    if(!g->beamPoints ||
            g->beamPoints_width != width ||
            g->beamPoints_height != height) {

        // All the fading beam plots share the same beamPoints array
        // that is for the whole graph.  One could have more than one
        // fading beam plot in a graph.
        //
        // TODO: Note this is quite a bit of memory being allocated.
        //
        size_t size = width * height * sizeof(*g->beamPoints);
        g->beamPoints = realloc(g->beamPoints, size);
        ASSERT(g->beamPoints, "realloc(,%zu) failed", size);
        g->beamPoints_width = width;
        g->beamPoints_height = height;
        memset(g->beamPoints, 0, size);
        beam->first = 0;
        beam->last = 0;
    } else if(g->beamReset) {
        size_t size = width * height * sizeof(*g->beamPoints);
        memset(g->beamPoints, 0, size);
        beam->first = 0;
        beam->last = 0;
    }

    // userCallback() may call the pnGraph_drawPoint() function many
    // times.
    bool ret = userCallback(&g->widget, p, userData,
            g->xMin, g->xMax, g->yMin, g->yMax);

    // g->pushBGSurface = true;

    return ret;
}
