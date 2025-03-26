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

    double a, r, g, b;
};

struct PnGraph {

    // Pointers to a doubly listed list of Zooms.
    //
    // top is the first zoom level we start with.
    struct PnZoom *top;  // first zoom level
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
