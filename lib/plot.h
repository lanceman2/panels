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
        cairo_set_source_surface(cr, grid_surface, padX, padY);
        cairo_set_source_surface(cr, grid_surface,
                - padX + slideX, - padY + slideY);
        cairo_fill(cr);

  where slideX and slideY is the distance in pixels to move the grid line
  surface, grid_surface relative to the current cr surface.

*/


struct PnZoom {

    struct PnZoom *prev, *next; // To keep a list of zooms in PnPlot

    double xSlope/* = (xMax - xMin)/pixWidth */;
    double ySlope/* = (yMin - yMax)/pixHeight */;
    double xShift/* = xMin - padX * xSlope */;
    double yShift/* = yMax - padY * ySlope */;
};


// A 2D plotter has a lot of parameters.  This "plot" thingy is just the
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
// The auto 2D plotter grid (graph) like part of quickplot.
//
struct PnPlot {

    struct PnWidget widget; // inherit first.

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

    uint32_t subGridColor, gridColor, axesLabelColor;

    // width and height of the view box; that is the width and height
    // without the padX and padY added to all sides.
    //
    //     cairo_surface_width  = width  + 2 * padX
    //     cairo_surface_height = height + 2 * padY
    //
    //     padX and/or padY can be 0.
    //
    // Is may be redundant, and the same as the widget size allocation
    // width and height.  But, we kind of need it when we free the
    // bgMemory, at which time the widget size allocation may not exist;
    // at least with DEBUG builds.
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


static inline
void GetPadding(uint32_t width, uint32_t height,
        uint32_t *padX, uint32_t *padY) {

    if(width < d.screen_width/3)
        *padX = width;
    else
        *padX = d.screen_width/3 + (width - d.screen_width/3)/4;

    if(height < d.screen_height/3)
        *padY = height;
    else
        *padY = d.screen_height/3 + (height - d.screen_height/3)/4;

    //DSPEW("                padX=%d  padY=%d", *padX, *padY);
}


// The width and/or height of the drawing Area changed and so must all the
// zoom scaling.  Create a zoom if there are none.
//
// This assumes that the displayed user values did not change, just the
// drawing area changed size; i.e. there is a bigger or smaller user view
// but the plots did not shift in x or y direction.
//
static inline void FixZoomsScale(struct PnPlot *g,
        uint32_t width, uint32_t height) {

    DASSERT(g);
    DASSERT(g->top);
    DASSERT(g->zoom);
    DASSERT(g->xMin < g->xMax);
    DASSERT(g->yMin < g->yMax);

    if(g->width == width && g->height == height)
        return;

    uint32_t padX, padY;
    // Get the new padding for the new width and height.
    GetPadding(width, height, &padX, &padY);

    struct PnZoom *z = g->top;
    z->xSlope = (g->xMax - g->xMin)/width;
    z->ySlope = (g->yMin - g->yMax)/height;
    z->xShift = g->xMin - padX * z->xSlope;
    z->yShift = g->yMax - padY * z->ySlope;

    for(z = z->next; z; z = z->next) {
        z->xShift += g->padX * z->xSlope;
        z->xSlope *= ((double) g->width)/((double) width);
        z->xShift -= padX * z->xSlope;

        z->yShift += g->padY * z->ySlope;
        z->ySlope *= ((double) g->height)/((double) height);
        z->yShift -= padY * z->ySlope;
    }

    g->width = width;
    g->height = height;
    g->padX = padX;
    g->padY = padY;
}
