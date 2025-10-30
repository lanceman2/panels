/*
  The padX and padY provide extra grid line drawing area so you can move
  it relative to another cairo surface that you write it to.

  ------------------------------------------------
  |                       ^                       |
  |   Cairo Surface       |                       |
  |                     padY                      |
  |                       |                       |
  |                       V                       |
  |          ---------------------------          |
  |          |      ^                  |          |
  |          |      |                  |          |
  |          |  vbHeight               |          |
  |          |      |                  |          |
  |<- padX ->|      |    VIEW BOX      |<- padX ->|
  |          |      |                  |          |
  |          |<-----|--- vbWidth ----->|          |
  |          |      |                  |          |
  |          |      V                  |          |
  |          ---------------------------          |
  |                       ^                       |
  |                       |                       |
  |                     padY                      |
  |                       |                       |
  |                       V                       |
  -------------------------------------------------

  The user of this Cairo Surface can slide this
  surface relative to the VIEW BOX with:
        cairo_set_source_surface(cr, grid_surface, padX, padY);
        cairo_set_source_surface(cr, grid_surface,
                - padX + slideX, - padY + slideY);
        cairo_fill(cr);

  where slideX and slideY is the distance in pixels to move the grid line
  surface, grid_surface relative to the current cr surface.

*/




// For oscilloscope like 2D plots
//
// For a scope there are two "signal" like things causing the points
// and/or lines to be drawn:
//   1. the I/O event from the operating system data input device or
// something like a virtual input device from the panels API user's code
// which may have "throttling" code in it; or a
//   2. signal received from the wayland compositor (like in the static
// case below).
//
// When does the panels API user code plot points and/or lines?
//
// We need to keep the fast data input from being drawn when it never
// shows to the user because it is drawn faster than the wayland
// compositor refresh rate.
//
// Overrun we define as when the fast data input is drawn and it never
// shows to the user because it is being drawn faster than the wayland
// compositor refresh rate.  If the user just misses seeing 2 frames out
// of 10 drawn, that may be okay; but missing 999 frames out of 1000
// frames may be wasting a lot of operating system resources for
// nothing.
//
// For device input under-run (device input is relatively slow), input
// needs to queue a draw when the input comes.  Both the device input and
// the wayland compositor (configure, mouse pointer motions events, and
// keyboard events) must drive the plot drawing.
//
struct PnScopePlot {

    struct PnPlot plot;

};

// For captured/stored 2D plots
//
// The plotted points and/or lines come from a user callback function that
// is called from a "signal" (fd pop or something) received from the
// wayland compositor.  The panels API user's code only indirectly causes
// this callback to be called through events like a after a window resize
// or mouse pointer motion zooming event.
//
struct PnStaticPlot {

    struct PnPlot plot;

};


struct PnGraphSurface {

    // Just a stupid-ass compo structure to keep things together in
    // struct PnGraph below.

    // Allocated memory image surface
    cairo_surface_t *surface;
    // drawing contexts for the surface
    cairo_t *lineCr, *pointCr;
};


// A 2D plotter has a lot of parameters.  This "graph" thingy is just the
// parameters that we choose to draw the background line grid of a 2D
// graph (plotter).  It has a "zoom" object in it that is parametrization
// of the 2D (linear) transformation for the window (widget/pixels) view.
// This is the built-in stuff.  Of course a user could draw arbitrarily
// pixels on top of this.  And so, yes, this is not natively 2D vector
// graphics, but one could over-lay a reduction of 2D vector graphics on
// top of this.  I don't think that computers are fast enough to make
// real-time (operator-in-the-loop) 2D vector graphics (at 60 Hz or
// faster), we must use pixels as the basis, though the grid uses Cairo, a
// floating point calculation of pixels values for the very static (not
// changing fast) background plotter grid lines.
//
// Note: we could speed up the background grid drawing by not using Cairo,
// because the background grid drawing requires just horizontal and
// vertical line drawing with which it is easier to calculate the
// anti-aliasing then Cairo's general anti-aliased line (lines at general
// angles).  But, the grid background drawing is not a performance
// bottleneck, so fuck it.
//
// The foreground points plotted may just directly set the pixel colors as
// a uint32_t (32 bit int), or a slower Cairo based foreground drawing.
// Cairo drawing can be about 10 to 1000 times slower than just changing a
// few individual pixels numbers in the mapped memory.  It's just that we
// are comparing the speed of doing something (memory copying all the
// widget pixels for any one frame, after floating point fucking all of
// them) to the speed of doing next to nothing (like 1 out of a 1000
// pixels being changed).  It depends where the bottleneck is.  If you do
// not need fast pixel changes, Cairo drawing could be fine.  Cairo
// drawing will likely look better than just simple integer pixel bit
// diddling.  It's hard to do anti-aliasing without floating point
// calculations.
//
// We can slowly draw background once (one frame), then quickly draw on
// top of it, 20 million points over many many frames.
//
// We already have a layout widget called PnGrid, we don't call this grid
// too, though it draws a grid of horizontal and vertical lines.
//
// The graph contains a automatic 2D plotter grid (graph), "like" part of
// quickplot (apt install quickplot [is X11/GTK3 based]).  The old program
// called "quickplot" is not compatible with the Wayland desktop.
//
struct PnGraph {

    struct PnWidget widget; // inherit first.

    struct PnPlot *plots; // A list of 2D plots: static or scope type.

    // This bgSurface uses a alloc(3) allocated memory buffer; and is not
    // the mmap(2) pixel buffer that is created for us by libpanels that
    // the Cairo object is tied to in our draw function (Draw() or
    // carioDraw()).  We use it just to store a background image of grid
    // lines that we "paste" to the Cairo object that is tied to in our
    // draw function.  In this sense we are double buffering.  We are
    // assuming there will be many 2D plot points plotted over time as
    // this "background grid" changes more slowly then we plot points on
    // top of it.
    //
    // For drawing the grid and static plots (points and/or lines).
    struct PnGraphSurface bgSurface;

    // For drawing scope plots (points and/or lines).
    struct PnGraphSurface scopeSurface;

    // This, cr,  is used when the plot grid is drawn to bgSurface.
    cairo_t *cr;

    // Pointers to a doubly listed list of Zooms.
    //
    // top is the first zoom level we start with.
    struct PnZoom *top; // first zoom level
    // zoom is the current zoom we are using.
    struct PnZoom *zoom; // current zoom level

    // Plotted X and Y values on the edges of the drawing area.
    //
    // Used for the first zoom.  After the first zoom these are not kept;
    // we keep shift and slope instead.
    double xMin, xMax, yMin, yMax;

    uint32_t subGridColor, gridColor, labelsColor, zeroLineColor;

    // padX, padY is padding size added to cairo drawing surface.  The
    // padding is added to each of the 4 sides of the Cairo surface.
    // See the ASCII art above.
    uint32_t padX, padY;

    // width and height of the view box; that is the width and height
    // without the padX and padY added to all sides.
    //
    //     cairo_surface_width  = width  + 2 * padX
    //     cairo_surface_height = height + 2 * padY
    //
    //     padX and/or padY can be 0.
    //
    uint32_t width, height;

    // in pixels.
    int32_t slideX, slideY; // mouse pointer or "other" plot sliding.

    int32_t boxX, boxY, boxWidth, boxHeight; // To draw a zoom box.

    uint32_t zoomCount;

    bool show_subGrid;
    bool have_scopes;
};


// The major grid lines get labeled with numbers.
//
// The closer together minor (sub) grid lines do not get labeled with
// numbers.  There's an unusual case where grid line number labels can get
// large like for example 6.75684e-05 and the next grid line number is
// like 6.75685e-05; note the relative difference is 1.0e-10. Math round
// off errors start to kill the plot number labeling at that point.
//
#define PIXELS_PER_MAJOR_GRID_LINE   (160)




extern bool _pnGraph_pushZoom(struct PnGraph *g,
        double xMin, double xMax, double yMin, double yMax);
extern bool _pnGraph_popZoom(struct PnGraph *g);

extern void _pnGraph_drawGrids(struct PnGraph *g, cairo_t *cr);

extern bool CheckZoom(double xMin, double xMax,
        double yMin, double yMax);

void PushCopyZoom(struct PnGraph *g);

#define MINPAD   (100)


// widget callback functions specific to the graph widget:
extern bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnGraph *p);
extern bool leave(struct PnWidget *w, struct PnGraph *p);
extern bool motion(struct PnWidget *w, int32_t x, int32_t y,
            struct PnGraph *p);
extern bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnGraph *p);
extern bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnGraph *p);
extern bool axis(struct PnWidget *w,
            uint32_t time, uint32_t which, double value,
            struct PnGraph *p);

extern void AddStaticPlot(struct PnWidget *w,
        struct PnCallback *callback, uint32_t actionIndex,
        void *actionData);

extern bool StaticDrawAction(struct PnGraph *p,
        struct PnCallback *callback,
        bool (*userCallback)(struct PnWidget *graph,
                struct PnPlot *plot, void *userData,
                double xMin, double xMax, double yMin, double yMax),
        void *userData, uint32_t actionIndex, void *actionData);

extern void AddScopePlot(struct PnWidget *w,
        struct PnCallback *callback, uint32_t actionIndex,
        void *actionData);

extern bool ScopeDrawAction(struct PnGraph *p,
        struct PnCallback *callback,
        bool (*userCallback)(struct PnWidget *graph,
                struct PnPlot *plot, void *userData,
                double xMin, double xMax, double yMin, double yMax),
        void *userData, uint32_t actionIndex, void *actionData);



