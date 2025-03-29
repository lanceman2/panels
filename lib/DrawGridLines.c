
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>

#include <cairo/cairo.h>

#include "gridLines.h"

#include "../include/panels.h"
#include "debug.h"


// TODO: Should this be in a header file?
// It's set in constructor() in constructor.c.
extern char decimal_point;


// Return true if there are still zooms to pop.
//
// We do not pop (free) the last one; unless we are destroying the graph.
// We need to keep the last one so we can draw something.
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

    return (g->zoom == g->top)?false:true;
}

// Returns true if there are zooms left to pop after this call.
//
bool pnGraph_popZoom(struct PnGraph *g) {

    if(g->zoom == g->top)
        // No need to redraw.
        return false;

    return _PopZoom(g);
}

static inline void _PushZoom(struct PnGraph *g,
        double xMin, double xMax, double yMin, double yMax) {

    DASSERT(g);
    DASSERT(g->width);
    DASSERT(g->height);

    struct PnZoom *z = malloc(sizeof(*z));
    ASSERT(z, "malloc(%zu) failed", sizeof(*z));

    z->xSlope = (xMax - xMin)/g->width;
    z->ySlope = (yMin - yMax)/g->height;
    z->xShift = xMin - g->padX * z->xSlope;
    z->yShift = yMax - g->padY * z->ySlope;

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
    // The current zoom (g->zoom) is always the last one, is this one.
    g->zoom = z;
}


void pnGraph_pushZoom(struct PnGraph *g,
        double xMin, double xMax, double yMin, double yMax) {

    DASSERT(xMin < xMax);
    DASSERT(yMin < yMax);

    // Zooming can fail if the numbers get small relative difference
    // values:
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
        return;
    }

    _PushZoom(g, xMin, xMax, yMin, yMax);
}


static inline double RoundUp(double x, uint32_t *subDivider,
                    int32_t *p_out) {

    // TODO: This could be rewritten by using an understanding of floating
    // point representation.  In this writing we did not even try to
    // understand how floating point numbers are stored, we just used math
    // functions that do the right thing.

    ASSERT(x > 0.0);
    ASSERT(isnormal(x));
    ASSERT(x < DBL_MAX/8.0); // max is about 1.0e308

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

    // Now trim digits off of mant.  Like for example:
    //
    //    1.1234 => 2.0   or  4.6234 => 5.0

    uint32_t ix = (uint32_t) mant;

    // We make it larger, not smaller.  Just certain values look good in
    // plot grid lines.
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
        cairo_line_to(cr, pix , height);
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
// Returns the distance between lines for a sub grid in plot
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
        cairo_line_to(cr, pix , g->height + 2*g->padY);
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


// TODO: There are a lot of user configurable parameters in this
// function.
void pnGraph_drawGrids(const struct PnGraph *g, cairo_t *cr,
        bool show_subGrid) {

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


    if(!show_subGrid)
      goto drawGrid;

    cairo_set_source_rgb(cr,
            g->subGridColor.r, g->subGridColor.g, g->subGridColor.b);

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
        default:
            ASSERT(0, "Bad Code");
            break;
    }

drawGrid:

    cairo_set_source_rgb(cr,
            g->gridColor.r, g->gridColor.g, g->gridColor.b);
    DrawVGrid(cr, lineWidth, g->zoom, g,
            fontSize, startX, deltaX);
    DrawHGrid(cr, lineWidth, g->zoom, g,
            fontSize, startY, deltaY);

    cairo_set_source_rgb(cr,
            g->axesLabelColor.r, g->axesLabelColor.g,
            g->axesLabelColor.b);
    DrawVGridLabels(cr, lineWidth, g->zoom, g,
            fontSize, startX, deltaX, powX);
    DrawHGridLabels(cr, lineWidth, g->zoom, g,
            fontSize, startY, deltaY, powY);
}
