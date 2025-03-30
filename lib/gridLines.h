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
  |<- padX ->|      |   VIEW BOX (vb)  |<- padX ->|
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
  area relative to the VIEW BOX with:

        cairo_set_source_surface(cr, grid_surface,
                - padX + slideX, - padY + slideY);

  where slideX and slideY is the distance in pixels to move the grid line
  surface, grid_surface relative to the current cr surface.

*/


struct PnZoom {

    struct PnZoom *prev, *next; // To keep a list of zooms

    double xSlope/* = (xMax - xMin)/pixWidth */;
    double ySlope/* = (yMin - yMax)/pixHeight */;
    double xShift/* = xMin - padX * xSlope */;
    double yShift/* = yMax - padY * ySlope */;
};


struct PnColor {

    // For Cairo drawing we need floating point numbers.
    double a, r, g, b;
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
// vertical line drawing which is easy to calculate the anti-aliasing then
// Cairo's general anti-aliased line (lines at all angles).  But, the
// grid background drawing is not a performance bottleneck, so fuck it.
//
// The foreground points plotted may just directly set the pixel colors as
// a uint32_t (32 bit int), or a slower Cairo based foreground drawing.
// Cairo drawing can be about 10 to 1000 times slower than just changing a
// few individual pixels numbers in the mapped memory.  It's just that we
// are comparing the speed of doing something (memory copying all the
// fucking widget pixels for any one frame, after floating point fucking
// all of them) to the speed of doing next to nothing (like 1 out of a
// 1000 pixels being changed).  It depends where the bottleneck is.  If
// you do not need fast pixel changes, Cairo drawing could be fine.
// Cairo drawing will likely look better than just simple integer pixel
// bit diddling.  It's hard to do anti-aliasing without floating point
// calculations.
//
// Purpose slowly draw background once (one frame), then quickly draw on
// top of it, 20 million points over many many frames.
//
// We already have a layout widget called PnGrid, so:
//
// The auto 2D plotter grid (graph)
//
struct PnGraph {

    // This bgSurface uses a alloc(3) allocated memory buffer; and is not
    // the mmap(2) pixel buffer that is created for us by libpanels that
    // the Cairo object is tied to in our draw function (Draw() or
    // carioDraw()).  We use it just to store a background image of grid
    // lines that we "paste" to the Cairo object that is tied to in our
    // draw function.  In this sense we are double buffering.  We are
    // assuming there will be many 2D plot points plotted over time as
    // this "background grid" changes more slowly then we plot points.
    //
    cairo_surface_t *bgSurface;
    uint32_t *bgMemory; // This is the memory for the Cairo surface.

    // Pointers to a doubly listed list of Zooms.
    //
    // top is the first zoom level we start with.
    struct PnZoom *top; // first zoom level
    // zoom is the current zoom we are using.
    struct PnZoom *zoom; // current zoom level

    // User values on the edges of the drawing area.
    double xMin, xMax, yMin, yMax;

    // padX, padY is padding size added to cairo drawing surface.  The
    // padding is added to each of the 4 sides of the Cairo surface.
    // See the ASCII art above.
    uint32_t padX, padY;

    struct PnColor subGridColor, gridColor, axesLabelColor;

    // width and height of the view box; that is the width and height
    // without the padX and padY added to all sides.
    //
    //     surface_width  = width  + 2 * padX
    //     surface_height = height + 2 * padY
    //
    //     padX and/or padY can be 0.
    //
    uint32_t width, height;

    uint32_t zoomCount;
};




// The major grid lines get labeled with numbers.
//
// The closer together minor (sub) grid lines do not not get labeled with
// numbers.  There's an unusual case where grid line number labels can get
// large like for example 6.75684e-05 and the next grid line number is
// like 6.75685e-05; note the relative difference is 1.0e-10. Math round
// off errors start to kill the "plotting" at that point.
//
#define PIXELS_PER_MAJOR_GRID_LINE   (160)


// These are mappings to (and from) pixels on a cairo surface we are
// plotting points and/or lines on.
//
// Note: In Cairo pixel distances tend to be doubles hence
// all these functions return double.
//
static inline double xToPix(double x, const struct PnZoom *z) {
    return (x - z->xShift)/z->xSlope;
}
//
static inline double pixToX(double p, const struct PnZoom *z) {
    return p * z->xSlope + z->xShift;
}
//
static inline double yToPix(double y, const struct PnZoom *z) {
    return (y - z->yShift)/z->ySlope;
}
//
static inline double pixToY(double p, const struct PnZoom *z) {
    return p * z->ySlope + z->yShift;
}


static inline void DestroyBGSurface(struct PnGraph *g) {
    if(g->bgSurface) {
        DASSERT(g->width);
        DASSERT(g->height);
        DASSERT(g->bgMemory);
        DZMEM(g->bgMemory, sizeof(*g->bgMemory)*g->width*g->height);
        free(g->bgMemory);
        cairo_surface_destroy(g->bgSurface);
        g->bgSurface = 0;
        g->bgMemory = 0;
        g->width = 0;
        g->height = 0;
    } else {
        DASSERT(!g->width);
        DASSERT(!g->height);
        DASSERT(!g->bgMemory);
    }
}

static inline void CreateBGSurface(struct PnGraph *g,
        uint32_t w, uint32_t h) {

    g->width = w;
    g->height = h;

    // Add the view box wiggle room, so that the user could pan the view
    // plus and minus the pad values (padX, panY), with the mouse pointer
    // or something.
    //
    // TODO: We could make the padX, and padY, a function of w, and h.
    //
    w += 2 * g->padX;
    h += 2 * g->padY;

    g->bgMemory = calloc(sizeof(*g->bgMemory), w * h);
    ASSERT(g->bgMemory, "calloc(%zu, %" PRIu32 "*%" PRIu32 ") failed",
            sizeof(*g->bgMemory), w, h);

    g->bgSurface = cairo_image_surface_create_for_data(
            (void *) g->bgMemory,
            CAIRO_FORMAT_ARGB32,
            w, h,
            w * 4/*stride in bytes*/);
    DASSERT(g->bgSurface);
}

// TODO: Add a zoom rescaler.
//
static inline void FreeZooms(struct PnGraph *g) {

    if(!g->zoom) return;

    DASSERT(g->top);

    // Free the zooms.
    while(pnGraph_popZoom(g));

    // Free the last zoom, that will not pop.  If it did pop, that would
    // suck for user's zooming ability.
    DZMEM(g->zoom, sizeof(*g->zoom));
    free(g->zoom);
    g->zoom = 0;
    g->top = 0;
}
