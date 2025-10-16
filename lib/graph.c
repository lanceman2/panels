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



// TODO: Should this be in a header file?
// It's set in constructor() in constructor.c.
extern char decimal_point;


// Return true if there are still zooms to pop.
//
// We do not pop (free) the last one; unless we are destroying the
// graph. We need to keep the last one so we can draw something.
//
static inline bool _PopZoom(struct PnGraph *g) {

    DASSERT(g->zoom);
    DASSERT(g->top);

    if(!g->zoom) {
        DASSERT(!g->top);
        return false;
    }

    // g->zoom is always the last zoom.
    DASSERT(!g->zoom->next);

    if(g->zoom == g->top)
        return false;

    DASSERT(g->zoom->prev);

    struct PnZoom *z = g->zoom;
    g->zoom = z->prev;
    g->zoom->next = 0;
    --g->zoomCount;
 
    DZMEM(z, sizeof(*z));
    free(z);

    DSPEW("Zoom count %" PRIu32, g->zoomCount);

    return (g->zoom == g->top)?false:true;
}

static inline void DestroyBGSurface(struct PnGraph *g) {
    if(g->bgSurface) {
        DASSERT(g->width);
        DASSERT(g->height);
        DASSERT(g->bgMemory);
        DASSERT(g->cr);
        DASSERT(g->pointCr);
        DASSERT(g->lineCr);
        cairo_destroy(g->cr);
        cairo_destroy(g->pointCr);
        cairo_destroy(g->lineCr);
        DZMEM(g->bgMemory, sizeof(*g->bgMemory)*g->width*g->height);
        free(g->bgMemory);
        cairo_surface_destroy(g->bgSurface);
        g->bgSurface = 0;
        g->bgMemory = 0;
        g->cr = 0;
    } else {
        DASSERT(!g->width);
        DASSERT(!g->height);
        DASSERT(!g->bgMemory);
        DASSERT(!g->cr);
    }
}

static inline void CreateBGSurface(struct PnGraph *g,
        uint32_t w, uint32_t h) {

    DASSERT(g);
    DASSERT(w);
    DASSERT(h);
    DASSERT(g->width == w);
    DASSERT(g->height == h);

    // Add the view box wiggle room, so that the user could pan the view
    // plus and minus the pad values (padX, panY), with the mouse pointer
    // or something.
    //
    // TODO: We could make the padX, and padY, a function of w, and h.
    //
    w += 2 * g->padX;
    h += 2 * g->padY;

    // Note: the memory and Cairo surface may be larger than g->width
    // times g->height (with the padding).
    g->bgMemory = calloc(sizeof(*g->bgMemory), w * h);
    ASSERT(g->bgMemory, "calloc(%zu, %" PRIu32 "*%" PRIu32 ") failed",
            sizeof(*g->bgMemory), w, h);

    g->bgSurface = cairo_image_surface_create_for_data(
            (void *) g->bgMemory,
            CAIRO_FORMAT_ARGB32,
            w, h,
            w * 4/*stride in bytes*/);
    DASSERT(g->bgSurface);
    g->cr = cairo_create(g->bgSurface);
    DASSERT(g->cr);
    g->lineCr = cairo_create(g->bgSurface);
    DASSERT(g->lineCr);
    cairo_set_operator(g->lineCr, CAIRO_OPERATOR_OVER);
    g->pointCr = cairo_create(g->bgSurface);
    DASSERT(g->pointCr);
    cairo_set_operator(g->pointCr, CAIRO_OPERATOR_OVER);
}


// Returns true if there are zooms left to pop after this call.
//
bool _pnGraph_popZoom(struct PnGraph *g) {

    if(g->zoom == g->top)
        // No need to redraw.
        return false;

    return _PopZoom(g);
}

static inline void FreeZooms(struct PnGraph *g) {

    if(!g->zoom) return;

    DASSERT(g->top);

    // Free the zooms.
    while(_pnGraph_popZoom(g));

    // Free the last zoom, that will not pop.  If it did pop, that would
    // suck for user's zooming ability.
    DZMEM(g->zoom, sizeof(*g->zoom));
    free(g->zoom);
    g->zoom = 0;
    g->top = 0;
}

// Add a zoom to the zoom list.
//
void AddZoom(struct PnGraph *g, struct PnZoom *z) {

    DASSERT(g);
    DASSERT(z);

    // Add to the end of the list of zooms
    if(g->zoom) {
        DASSERT(g->top);
        // The current zoom (g->zoom) is always the last one.
        DASSERT(!g->zoom->next);
        g->zoom->next = z;
        z->prev = g->zoom;
        ++g->zoomCount;
    } else {
        DASSERT(!g->top);
        z->prev = 0;
        g->top = z;
    }
    z->next = 0;
    // The current zoom (g->zoom) is always the last one, it's this one.
    g->zoom = z;
}

void PushCopyZoom(struct PnGraph *p) {

    DASSERT(p);
    DASSERT(p->width);
    DASSERT(p->height);

    struct PnZoom *z = malloc(sizeof(*z));
    ASSERT(z, "malloc(%zu) failed", sizeof(*z));
    const struct PnZoom *old = p->zoom;
    DASSERT(old);

    z->xSlope = old->xSlope;
    z->ySlope = old->ySlope;
    z->xShift = old->xShift;
    z->yShift = old->yShift;

    AddZoom(p, z);
}

#define ZOOM_MAX  (100)

static inline void _PushZoom(struct PnGraph *g,
        double xMin, double xMax, double yMin, double yMax) {

    DASSERT(g);
    DASSERT(g->width);
    DASSERT(g->height);

    struct PnZoom *z;

    if(g->zoomCount < ZOOM_MAX) {
        z = malloc(sizeof(*z));
        ASSERT(z, "malloc(%zu) failed", sizeof(*z));
        if(g->zoomCount == ZOOM_MAX - 1)
            INFO("At the zoom limit of %" PRIu32,
                    g->zoomCount + 1);
    } else {
        DASSERT(g->zoom);
        DASSERT(g->top);
        // In this case we reuse the last zoom.
        z = g->zoom;
    }

    z->xSlope = (xMax - xMin)/g->width;
    z->ySlope = (yMin - yMax)/g->height;
    z->xShift = xMin - g->padX * z->xSlope;
    z->yShift = yMax - g->padY * z->ySlope;

    if(z != g->zoom)
        AddZoom(g, z);
}

bool CheckZoom(double xMin, double xMax, double yMin, double yMax) {

    DASSERT(xMin < xMax);
    DASSERT(yMin < yMax);

    // Zooming can fail if the numbers get small relative difference
    // values (this is from trial and error, fuck with it at your
    // peril):
    double rdx = fabs(2.0*(xMax - xMin)/(xMax + xMin));
    double rdy = fabs(2.0*(yMax - yMin)/(yMax + yMin));
    if(rdx < 1.0e-8 || rdy < 1.0e-8) {
        // The double has about 15 decimal digits precision, and we need
        // some precision (bits) to use for converting numbers so we can
        // plot them.
        NOTICE("Failed to make a new zoom due to "
                "lack of relative number differences.\n"
                "The relative difference in x and y about: "
                "%.3g and %.3g", rdx, rdy);
        return true;
    }
    return false; // success.
}

bool _pnGraph_pushZoom(struct PnGraph *g,
        double xMin, double xMax, double yMin, double yMax) {

    DASSERT(xMin < xMax);
    DASSERT(yMin < yMax);

    if(CheckZoom(xMin, xMax, yMin, yMax))
        return true;

    _PushZoom(g, xMin, xMax, yMin, yMax);
    return false; // success.
}

static inline double RoundUp(double x, uint32_t *subDivider,
                    int32_t *p_out) {

    // TODO: This could be rewritten by using an understanding of floating
    // point representation.  In this writing we did not even try to
    // understand how floating point numbers are stored, we just used math
    // functions that do the right thing.

    ASSERT(x > 0.0);
    ASSERT(isnormal(x));
    ASSERT(x < DBL_MAX/8.0); // max is about 1.0e+308

    double pow = log10(x);
    // x = 10^pow
    int32_t p;
    if(pow > 0.0)
        p = (pow + 0.5);
    else
        p = (pow - 0.5);
    // x ~= 10^p  example if pow ==  1.3  p = 1
    //                    if pow == -1.3  p = -1
    double tenPow = exp10((double) p);
    double mant = x/tenPow;

    while(mant < 1.0) {
        mant *= 10.0;
        --p;
    }

    // I'm not sure how the mantissa is defined, but I don't care.  I just
    // have that:  
    //
    //       x = mant * 10 ^ p  [very close]
    //
    // Ya, it better be that this is so:
    DASSERT(x < 1.000001 * mant * exp10(p));
    DASSERT(x > 0.999999 * mant * exp10(p));

    //DSPEW("x = %lg = %lf * 10 ^(%d)", x, mant, p);

    // Now trim digits off of mant. Rounding up.  Like for example:
    //
    //    1.1234 => 2.0   or  4.6234 => 5.0

    uint32_t ix = (uint32_t) mant;

    // We make it larger, not smaller.  Just certain values look good in
    // plot (graph) grid lines.
    //
    switch(ix) {
        case 9:
        case 8:
        case 7:
        case 6:
        case 5:
            ix = 10;
            break;
        case 4:
        case 3:
        case 2:
            ix = 5;
            break;
        case 1:
            ix = 2;
            break;
        case 0:
            ASSERT(0, "Bad Code");
            break;
    }

    x = ix * exp10(p);

    //DSPEW("x = %lg <= %" PRIu32" * 10 ^(%d)", x, ix, p);

    *subDivider = ix;
    *p_out = p;

    return x;
}

static inline void DrawVSubGrid(cairo_t *cr, const struct PnZoom *z,
        double start, double delta, double end, double height) {

    for(double x = start; x <= end; x += delta) {
        double pix = xToPix(x, z);
        cairo_move_to(cr, pix, 0);
        cairo_line_to(cr, pix, height);
        cairo_stroke(cr);
    }
}

static inline void DrawHSubGrid(cairo_t *cr, const struct PnZoom *z,
        double start, double delta, double end, double width) {

    for(double y = start; y <= end; y += delta) {
        double pix = yToPix(y, z);
        cairo_move_to(cr, 0, pix);
        cairo_line_to(cr, width, pix);
        cairo_stroke(cr);
    }
}


// This will find a rounded up distance between lines that looks nice.
// For example a data scaled space between lines of 2.0000e-3 and not
// 1.8263e-3.
//
// Returns the distance between lines for a sub grid in graph
// coordinates.
//
static inline double GetVGrid(cairo_t *cr,
        double lineWidth/*vertical line width in pixels*/, 
        double pixelSpace/*minimum pixels between lines*/,
        const struct PnZoom *z,
        double fontSize, double *start_out,
        uint32_t *subDivider, int32_t *pow) {

    DASSERT(lineWidth <= pixelSpace);
    // delta is in user values not pixels.
    double delta = z->xSlope * pixelSpace;
    delta = RoundUp(delta, subDivider, pow);

    // start a little behind pixel 0.
    double start = pixToX(0, z) - delta;
    // Make the start a integer number of deltX

    int32_t n;
    if(start < 0.0)
        n = (start/delta - 0.5);
    else
        n = (start/delta + 0.5);
    // Make start a multiple of delta
    start = n * delta;

    //DSPEW("V delta = %lg  start= %lg", delta, start);

    *start_out = start;
    return delta;
}

static inline double GetHGrid(cairo_t *cr,
        double lineWidth/*vertical line width in pixels*/, 
        double pixelSpace/*minimum pixels between lines*/,
        const struct PnZoom *z, double width, double height,
        double fontSize, double *start_out,
        uint32_t *subDivider, int32_t *pow) {

    DASSERT(lineWidth <= pixelSpace);

    double delta = - z->ySlope * pixelSpace;
    delta = RoundUp(delta, subDivider, pow);

    // start a little behind pixel 0.
    double start = pixToY(height-1, z) - delta;
    // Make the start a integer number of delta

    int32_t n;
    if(start < 0.0)
        n = (start/delta - 0.5);
    else
        n = (start/delta + 0.5);
    // Make start a multiple of delta
    start = n * delta;

    //DSPEW("H delta = %lg  start= %lg", delta, start);

    *start_out = start;
    return delta;
}

static inline char *StripZeros(char *label) {

    char *s = label + strlen(label) - 1;
    while(*s == '0' && *(s-1) != decimal_point) {
        *s = '\0';
        --s;
    }

    return s;
}

// Count the non-zero digits to the right of decimal_point
static inline size_t CountToPoint(char *s) {

    s = StripZeros(s);
    size_t l = 0;
    while(*s && *s != decimal_point) {
        --s;
        ++l;
    }
    return l;
}

// Count the non-zero digits to the right of decimal_point
static inline size_t CountLabel(char *label, size_t LEN,
        double x, double exp10pow, int32_t pow) {

    if(pow >= -2 && pow <= 3) {
        snprintf(label, LEN, "%3.4f", x);
        return CountToPoint(label);
    }

    snprintf(label, LEN, "%4.4f", x/exp10pow);
    return CountToPoint(label);
}

static inline void GetLabel(char *label, size_t LEN,
        double x, double exp10pow, int32_t pow, size_t lpdp) {

    //snprintf(label, LEN, "%1.1e", x);

    if(pow >= -2 && pow <= 3) {
        snprintf(label, LEN, "%3.4f", x);
        // Remove extra digits
        char *s = label;
        while(*s && *s != decimal_point)
            s++;
        *(s + lpdp + 1) = '\0';
        return;
    }

    snprintf(label, LEN, "%4.4f", x/exp10pow);
    size_t l = strlen(label);

    char *s = label;
    while(*s && *s != decimal_point)
        s++;
    *(s + lpdp + 1) = '\0';
    l = strlen(label);
    // Zero is special.  We leave the exponent off of zero.
    //
    // Note, the zero value was rounded to be exactly zero; otherwise it
    // shows just the round-off cruft, if there is any.
    //
    if(x != 0)
        snprintf(label+l, LEN-l, "e%+d", pow);
}

static inline void DrawVGrid(cairo_t *cr,
        double lineWidth/*vertical line width in pixels*/, 
        const struct PnZoom *z, const struct PnGraph *g,
        double fontSize, double start,
        double delta) {

    cairo_set_line_width(cr, lineWidth);

    double end = pixToX(g->width + 2*g->padX, z) + delta;

    // TODO: Optimize performance in this loop.
    //
    for(double x = start; x <= end; x += delta) {

        double pix = xToPix(x, z);
        cairo_move_to(cr, pix, 0);
        cairo_line_to(cr, pix, g->height + 2*g->padY);
        cairo_stroke(cr);
    }
}

// This is the ugliest code ever written.
//
static inline void DrawVGridLabels(cairo_t *cr,
        double lineWidth/*vertical line width in pixels*/, 
        const struct PnZoom *z, const struct PnGraph *g,
        double fontSize, double start,
        double delta, int32_t pow) {

    double end = pixToX(g->width + 2*g->padX, z) + delta;

    const size_t T_SIZE = 32;
    char label[T_SIZE];

    lineWidth *= 0.8;

    ++pow;
    double exp10pow = exp10(pow);

    size_t mantissaLen = 0;
    for(double x = start; x <= end; x += delta) {

        if(fabs(x) < delta/100)
            // With infinite precision this would not happen, but we do
            // not have infinite precision so we'll just fix the value so
            // that we print 0 and not something like 1.3e-32.
            x = 0.0;
        size_t len = CountLabel(label, T_SIZE, x, exp10pow, pow);
        if(len > mantissaLen)
            mantissaLen = len;
    }

    for(double x = start; x <= end; x += delta) {

        if(fabs(x) < delta/100)
            // With infinite precision this would not happen, but we do
            // not have infinite precision so we'll just fix the value so
            // that we print 0 and not something like 1.3e-32.
            x = 0.0;

        double pix = xToPix(x, z);
        GetLabel(label, T_SIZE, x, exp10pow, pow, mantissaLen);
        cairo_move_to(cr, pix + lineWidth, 
            g->height + g->padY - lineWidth);
        cairo_show_text(cr, label);

        if(g->height < 2*fontSize) continue;

        cairo_move_to(cr, pix + lineWidth, g->padY - lineWidth);
        cairo_show_text(cr, label);

        cairo_move_to(cr, pix + lineWidth,
            g->height + 2 * g->padY - lineWidth);
        cairo_show_text(cr, label);
    }
}

static inline void DrawHGrid(cairo_t *cr,
        double lineWidth/*vertical line width in pixels*/, 
        const struct PnZoom *z, const struct PnGraph *g,
        double fontSize, double start,
        double delta) {

    cairo_set_line_width(cr, lineWidth);

    double end = pixToY(0, z) + delta;

    // TODO: Optimize performance in this loop.
    //
    for(double y = start; y <= end; y += delta) {

        double pix = yToPix(y, z);
        cairo_move_to(cr, 0, pix);
        cairo_line_to(cr, g->width + 2*g->padX, pix);
        cairo_stroke(cr);
    }
}

static inline void DrawHGridLabels(cairo_t *cr,
        double lineWidth/*line width in pixels*/, 
        const struct PnZoom *z, const struct PnGraph *g,
        double fontSize, double start,
        double delta, int32_t pow) {

    double end = pixToY(0, z) + delta;

    const size_t T_SIZE = 32;
    char label[T_SIZE];

    double textX = fontSize;

    lineWidth *= 0.8;

    ++pow;
    double exp10pow = exp10(pow);

    size_t mantissaLen = 0;
    for(double x = start; x <= end; x += delta) {

        if(fabs(x) < delta/100)
            // With infinite precision this would not happen, but we do
            // not have infinite precision so we'll just fix the value so
            // that we print 0 and not something like 1.3e-32.
            x = 0.0;
        size_t len = CountLabel(label, T_SIZE, x, exp10pow, pow);
        if(len > mantissaLen)
            mantissaLen = len;
        else
            break;
    }

    for(double y = start; y <= end; y += delta) {

        if(fabs(y) < delta/100)
            // With infinite precision this would not happen, but we do
            // not have infinite precision so we'll just fix the value so
            // that we print 0 and not something like 1.3e-32.
            y = 0.0;

        double pix = yToPix(y, z);
        GetLabel(label, T_SIZE, y, exp10pow, pow, mantissaLen);
        cairo_move_to(cr, textX, pix - lineWidth - 1);
        cairo_show_text(cr, label);

        cairo_move_to(cr, textX + g->padX, pix - lineWidth - 1);
        cairo_show_text(cr, label);

        cairo_move_to(cr, textX + g->padX + g->width, pix - lineWidth - 1);
        cairo_show_text(cr, label);
     }
}

static inline void DrawBackgroundColor(const struct PnGraph *g,
        cairo_t *cr) {

    // We want the background to be what it is and not a combo.
    uint32_t bgColor = g->widget.backgroundColor;
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    SetColor(cr, bgColor);
    cairo_paint(cr);
}


// TODO: There are a lot of user configurable parameters in this
// function.  Lots of different line widths.
//
void _pnGraph_drawGrids(struct PnGraph *g, cairo_t *cr) {

    bool show_subGrid = g->show_subGrid;

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
            CAIRO_FONT_WEIGHT_NORMAL);
    const double fontSize = 20;
    cairo_set_font_size(cr, fontSize);

    double startX, deltaX, startY, deltaY;
    uint32_t subDividerX, subDividerY;
    double lineWidth = 5.7;
    int32_t powX, powY;

    deltaX = GetVGrid(cr, lineWidth, PIXELS_PER_MAJOR_GRID_LINE,
            g->zoom, fontSize, &startX, &subDividerX, &powX);

    deltaY = GetHGrid(cr, lineWidth, PIXELS_PER_MAJOR_GRID_LINE,
            g->zoom, g->width + 2*g->padX, g->height + 2*g->padY,
            fontSize, &startY, &subDividerY, &powY);

    DrawBackgroundColor(g, cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    if(!show_subGrid)
      goto drawGrid;

    if(!g->subGridColor) {
        subDividerX = 0;
        subDividerY = 0;
    } else {
        SetColor(cr, g->subGridColor);
    }

    switch(subDividerX) {
        case 10:
            cairo_set_line_width(cr, 1.5);
            DrawVSubGrid(cr, g->zoom, startX, deltaX/10.0,
                    pixToX(g->width + 2*g->padX, g->zoom) + deltaX,
                    g->height + 2*g->padY);
            cairo_set_line_width(cr, 4.0);
            DrawVSubGrid(cr, g->zoom, startX + deltaX/2, deltaX,
                    pixToX(g->width + 2*g->padX, g->zoom) + deltaX,
                    g->height + 2*g->padY);
            break;
        case 5:
            cairo_set_line_width(cr, 4.0);
            DrawVSubGrid(cr, g->zoom, startX, deltaX/5.0,
                    pixToX(g->width + 2*g->padX, g->zoom) + deltaX,
                    g->height + 2*g->padY);
            cairo_set_line_width(cr, 0.7);
            DrawVSubGrid(cr, g->zoom, startX, deltaX/10.0,
                    pixToX(g->width + 2*g->padX, g->zoom) + deltaX,
                    g->height + 2*g->padY);
            break;
        case 2:
            cairo_set_line_width(cr, 4.2);
            DrawVSubGrid(cr, g->zoom, startX + deltaX/2, deltaX,
                    pixToX(g->width + 2*g->padX, g->zoom) + deltaX,
                    g->height + 2*g->padY);
            cairo_set_line_width(cr, 1.5);
            DrawVSubGrid(cr, g->zoom, startX, deltaX/10.0,
                    pixToX(g->width+ 2*g->padX, g->zoom) + deltaX,
                    g->height + 2*g->padY);
            break;
        case 0:
            break;
        default:
            ASSERT(0, "Bad Code");
            break;
    }

    switch(subDividerY) {
        case 10:
            cairo_set_line_width(cr, 1.5);
            DrawHSubGrid(cr, g->zoom, startY, deltaY/10.0,
                    pixToY(0, g->zoom) + deltaY,
                    g->width + 2*g->padX);
            cairo_set_line_width(cr, 4.0);
            DrawHSubGrid(cr, g->zoom, startY + deltaY/2, deltaY,
                    pixToY(0, g->zoom) + deltaY,
                    g->width + 2*g->padX);
            break;
        case 5:
            cairo_set_line_width(cr, 4.0);
            DrawHSubGrid(cr, g->zoom, startY, deltaY/5.0,
                    pixToY(0, g->zoom) + deltaY,
                    g->width + 2*g->padX);
            cairo_set_line_width(cr, 0.7);
            DrawHSubGrid(cr, g->zoom, startY, deltaY/10.0,
                    pixToY(0, g->zoom) + deltaY,
                    g->width + 2*g->padX);
            break;
        case 2:
            cairo_set_line_width(cr, 4.2);
            DrawHSubGrid(cr, g->zoom, startY + deltaY/2, deltaY,
                    pixToY(0, g->zoom) + deltaY,
                    g->width + 2*g->padX);
            cairo_set_line_width(cr, 1.5);
            DrawHSubGrid(cr, g->zoom, startY, deltaY/10.0,
                    pixToY(0, g->zoom) + deltaY,
                    g->width + 2*g->padX);
            break;
        case 0:
            break;
        default:
            ASSERT(0, "Bad Code");
            break;
    }

drawGrid:

    if(g->gridColor) {
        SetColor(cr, g->gridColor);
        DrawVGrid(cr, lineWidth, g->zoom, g,
                fontSize, startX, deltaX);
        DrawHGrid(cr, lineWidth, g->zoom, g,
                fontSize, startY, deltaY);
    }

    bool haveXZero = !((xToPix(g->xMin, g->zoom) >
                g->width + lineWidth*1.9 + g->padX) ||
            (xToPix(g->xMax, g->zoom) <= - lineWidth*1.9 - g->padX));
    bool haveYZero = !((yToPix(g->yMax, g->zoom) >
                g->height + lineWidth*1.9 + g->padY) ||
            (yToPix(g->yMin, g->zoom) <= - lineWidth*1.9 - g->padY));
//WARN("haveXZero=%d  haveYZero=%d", haveXZero, haveYZero);

    // The zero axis grid lines are special.  If they are showing we make
    // them standout.
    double pix;

    if(haveXZero && g->gridColor) {
        cairo_set_line_width(cr, lineWidth * 1.9);
        SetColor(cr, g->gridColor);
        pix = xToPix(0, g->zoom);
        cairo_move_to(cr, pix, 0);
        cairo_line_to(cr, pix, g->height + 2*g->padY);
        cairo_stroke(cr);
    }
    if(haveYZero && g->gridColor) {
        cairo_set_line_width(cr, lineWidth * 1.9);
        SetColor(cr, g->gridColor);
        pix = yToPix(0, g->zoom);
        cairo_move_to(cr, 0, pix);
        cairo_line_to(cr, g->width + 2*g->padX, pix);
        cairo_stroke(cr);
    }

    if(haveXZero && g->zeroLineColor) {
        cairo_set_line_width(cr, 1.2);
        SetColor(cr,g->zeroLineColor);
        //const double dashes[2] = { 14.0, 9.0 };
        //cairo_set_dash(cr, dashes, 2/*numDashes*/, 0.0/*startOffset*/);
        pix = xToPix(0, g->zoom);
        cairo_move_to(cr, pix, 0);
        cairo_line_to(cr, pix, g->height + 2*g->padY);
        cairo_stroke(cr);
    }
    if(haveYZero && g->zeroLineColor) {
        cairo_set_line_width(cr, 1.2);
        SetColor(cr,g->zeroLineColor);
        //const double dashes[2] = { 14.0, 9.0 };
        //cairo_set_dash(cr, dashes, 2/*numDashes*/, 0.0/*startOffset*/);
        pix = yToPix(0, g->zoom);
        cairo_move_to(cr, 0, pix);
        cairo_line_to(cr, g->width + 2*g->padX, pix);
        cairo_stroke(cr);
    }

    if(g->labelsColor) {
        SetColor(cr, g->labelsColor);
        DrawVGridLabels(cr, lineWidth, g->zoom, g,
                fontSize, startX, deltaX, powX);
        DrawHGridLabels(cr, lineWidth, g->zoom, g,
                fontSize, startY, deltaY, powY);
    }

    pnWidget_callAction(&g->widget, PN_GRAPH_CB_STATIC_DRAW);
}

static
void destroy(struct PnWidget *w, struct PnGraph *p) {

    DASSERT(p);
    DASSERT(p = (void *) w);

    DestroyBGSurface(p);
    FreeZooms(p);
}

static inline
void GetPadding(uint32_t width, uint32_t height,
        uint32_t *padX, uint32_t *padY) {

    // TODO: Figure out the correct monitor display we are using.
    // Starting with d.outputs[0]

    DASSERT(d.numOutputs);
    DASSERT(d.outputs);
    DASSERT(d.outputs[0]);
    const uint32_t screen_width = d.outputs[0]->screen_width;
    const uint32_t screen_height = d.outputs[0]->screen_height;
    DASSERT(screen_width);
    DASSERT(screen_height);

    if(width < screen_width/3)
        *padX = width;
    else
        *padX = screen_width/3 + (width - screen_width/3)/4;

    if(height < screen_height/3)
        *padY = height;
    else
        *padY = screen_height/3 + (height - screen_height/3)/4;

    if(*padX < MINPAD)
        *padX = MINPAD;
    if(*padY < MINPAD)
        *padY = MINPAD;

    //DSPEW("                padX=%d  padY=%d", *padX, *padY);
}

// The width and/or height of the drawing Area changed and so must all the
// zoom scaling.
//
// This assumes that the displayed user values did not change, just the
// drawing area changed size; i.e. there is a bigger or smaller user view
// but the plots did not shift in x or y direction.
//
static inline void FixZoomsScale(struct PnGraph *g,
        uint32_t width, uint32_t height) {

    DASSERT(g);
    DASSERT(g->top);
    DASSERT(g->zoom);
    DASSERT(g->xMin < g->xMax);
    DASSERT(g->yMin < g->yMax);
    DASSERT(g->width);
    DASSERT(g->height);
    // The width and height should have changed.
    DASSERT(g->width != width || g->height != height);

    // The padX and padY may or may not be changing.
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


static void Config(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, uint32_t stride/*4 bytes*/,
            struct PnGraph *g) {

    ASSERT(w);
    ASSERT(h);
    DASSERT(widget == (void *) g);

    if(g->bgSurface && g->width == w && g->height == h) {
        DASSERT(g->bgMemory);
        return;
    }

    DestroyBGSurface(g);

    if(!g->zoom) {
        DASSERT(!g->top);
        DASSERT(!g->width);
        DASSERT(!g->height);
        DASSERT(!g->padX);
        DASSERT(!g->padY);
        // For starting with the first rendering of g->bgSurface.
        g->width = w;
        g->height = h;
        GetPadding(h, h, &g->padX, &g->padY);
        _pnGraph_pushZoom(g, g->xMin, g->xMax, g->yMin, g->yMax);
    } else {
        DASSERT(g->top);
        DASSERT(g->width);
        DASSERT(g->height);
        // This sets g->width, g->height, g->padX, and g->padY too.
        // This fixes all the zooms in the list.
        FixZoomsScale(g, w, h);
    }

    CreateBGSurface(g, w, h);
    _pnGraph_drawGrids(g, g->cr);
}

static inline void OverlayGridSurface(struct PnGraph *g,
        cairo_t *cr) {

    // Transfer the bgSurface to this cr (and its surface).
    //
    // Tests show this uses 1/2 the CPU usage as redrawing the grid lines
    // every time.
    //
    // We let the background pixels that we just painted to be seen, that
    // is if the pixels are transparent.  Alpha is fun.
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, g->bgSurface,
            - g->padX + g->slideX, - g->padY + g->slideY);
    cairo_paint(cr);
}

static int cairoDraw(struct PnWidget *w, cairo_t *cr,
            struct PnGraph *g) {

    OverlayGridSurface(g, cr);

    if(g->boxX != INT32_MAX) {
        // Draw a zoom box.
        //cairo_set_operator(cr, CAIRO_OPERATOR_XOR);
        cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);
        //cairo_set_operator(cr, CAIRO_OPERATOR_EXCLUSION);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_rectangle(cr,
                    g->boxX, g->boxY,
                    g->boxWidth, g->boxHeight);
        cairo_fill(cr);
    }
    // return 0 -> done; return 1 -> to call again next frame like in (1
    // second)/60.  Your frame period depends on many things.  This is using
    // a wayland compositor server which determines the frame rate.
    //
    // TODO: We need to look into user egl with wayland.
    return 0;
}

void pnGraph_setView(struct PnWidget *w,
        double xMin, double xMax, double yMin, double yMax) {

    DASSERT(w);
    ASSERT(IS_TYPE1(w->type, PnWidgetType_graph));
    DASSERT(xMin < xMax);
    DASSERT(yMin < yMax);

    struct PnGraph *p = (void *) w;

    bool haveZooms = (p->zoom)?true:false;

    FreeZooms(p);

    p->xMin = xMin;
    p->xMax = xMax;
    p->yMin = yMin;
    p->yMax = yMax;

    // We don't make a zoom if we didn't have one yet.
    if(haveZooms)
        _pnGraph_pushZoom(p, xMin, xMax, yMin, yMax);
}

struct PnWidget *pnGraph_create(struct PnWidget *parent,
        uint32_t width, uint32_t height, enum PnAlign align,
        enum PnExpand expand) {

    struct PnGraph *g;

    g = (void *) pnWidget_create(
            parent/*parent*/,
            80/*width*/, 60/*height*/,
            PnLayout_Cover/* => We can have a child that is drawn on top of
                           this grid lines "graph" widget.  That one way to
                           make a 2D plotter.  The transparent part of the
                           child widget will show the under laying grid
                           lines.*/,
            align, expand, sizeof(*g));
    ASSERT(g);

    // It starts out life as a widget:
    DASSERT(g->widget.type == PnWidgetType_widget);
    // And now it becomes a graph:
    g->widget.type = PnWidgetType_graph;

    // Add widget callback functions:
    pnWidget_setCairoDraw(&g->widget, (void *) cairoDraw, g);
    pnWidget_setConfig(&g->widget, (void *) Config, g);
    pnWidget_addDestroy(&g->widget, (void *) destroy, g);
    //
    pnWidget_setEnter(&g->widget,   (void *) enter, g);
    pnWidget_setLeave(&g->widget,   (void *) leave, g);
    pnWidget_setPress(&g->widget,   (void *) press, g);
    pnWidget_setRelease(&g->widget, (void *) release, g);
    pnWidget_setMotion(&g->widget,  (void *) motion, g);
    pnWidget_setAxis(&g->widget,  (void *) axis, g);

    pnWidget_addAction(&g->widget, PN_GRAPH_CB_STATIC_DRAW,
            (void *) StaticDrawAction, AddStaticPlot, 0/*actionData*/,
            sizeof(struct PnStaticPlot));

    // floating point scaled size exposed pixels without the padX and
    // padY added (not in number of pixels):
    g->xMin=0.0;
    g->xMax=1.0;
    g->yMin=0.0;
    g->yMax=1.0;

    // Reset zoom box draw:
    g->boxX = INT32_MAX;

    // Defaults:
    //
    // Colors bytes in order: A,R,G,B
    g->subGridColor   = 0xFF70B070;
    g->gridColor      = 0xFFE0E0E0;
    g->labelsColor = 0xFFFFFFFF;
    g->zeroLineColor  = 0xFFFF0000;

    g->show_subGrid = true;

    // The rest of PnGraph is zero from pnWidget_create() -> calloc()
    // above.

    return &g->widget;
}
