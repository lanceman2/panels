enum PnPlotType {

    PnPlotType_static,
    PnPlotType_dynamic
};


struct PnPlot {

    struct PnCallback callback; // inherit first.

    struct PnGraph *graph;
    // For a static or scope plot these are connected to the appropriate
    // object at the start of a plot draw.
    // This is just so we do not deference one more pointer, like
    // p->pointCr and not p->g->pointCr, in a tight loop.
    cairo_t *lineCr, *pointCr;

    struct PnZoom *zoom;

    enum PnPlotType type;

    // Just for scope plots.  Is zero for static plots.
    uint32_t shiftX, shiftY;

    uint32_t lineColor, pointColor;

    double lineWidth, pointSize;
    // The last point drawn is needed to draw lines when
    // there is a line being drawn in the future.
    double x, y; // last point
};


