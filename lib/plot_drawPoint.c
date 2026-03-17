// This out performs Cairo drawing by far.  It just draws directly to the
// mapped buffer uint32_t pixel values.  Cairo draws better (fancier), but
// is too slow to make a performant scope plot.
//
// We used the program "htop" as a integral part of our code development
// workflow.

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


static inline uint32_t
GetByte(uint32_t toColor, uint32_t fromColor, double fade, double org,
        uint32_t mask) {

    fromColor &= mask;
    toColor &= mask;

    double x = toColor * fade + fromColor * org;
    if(x >= (double) mask)
        return mask;
    else
        toColor = (uint32_t) x;

    return (toColor & mask);
}



// Returns the new faded color.
//
static inline
uint32_t Fade(uint32_t toColor, uint32_t fromColor, double fade) {

    double org = 1.0 - fade;

    // Relative amount of fade.
    // 0.0 <= fade < 1.0
    //
    // fade == 1.0  is toColor
    // fade == 0.0  is fromColor

    // Color is 32 bit in the order Alpha Red Green Blue one byte each.
    //
    // We just need to compute each color component separately

    return
        GetByte(toColor, fromColor, fade, org, 0xFF000000) | // Alpha
        GetByte(toColor, fromColor, fade, org, 0x00FF0000) | // Red
        GetByte(toColor, fromColor, fade, org, 0x0000FF00) | // Blue
        GetByte(toColor, fromColor, fade, org, 0x000000FF);  // Green
}


static void
Beam_drawPoint(struct PnPlot *p, double x, double y) {

    DASSERT(p);
    DASSERT(p->drawMethod == PnDrawMethod_beam);

    struct PnGraph *g = p->graph;
    DASSERT(g);
    DASSERT(g->beamPoints);
    DASSERT(g->widget.allocation.width == g->beamPoints_width);
    DASSERT(g->widget.allocation.height == g->beamPoints_height);

    struct PnBeam *beam = p->beam;
    DASSERT(beam);

    struct PnZoom *z = p->zoom;
    DASSERT(z);
    // We need the positions in the window widget space:
    uint32_t xi = xToPix(x, z) - p->shiftX + g->widget.allocation.x;
    uint32_t yi = yToPix(y, z) - p->shiftY + g->widget.allocation.y;

    // Cull if out of bounds.
    if(xi < g->widget.allocation.x ||
            xi >= g->widget.allocation.x + g->widget.allocation.width)
        return;
    if(yi < g->widget.allocation.y ||
            yi >= g->widget.allocation.y + g->widget.allocation.height)
        return;

    // Increment the current pointTime:
    ++beam->time;

    // Find this pixel in the window widget buffer memory:
    uint32_t *pixel = beam->pixels
        + xi
        + yi * beam->stride;

    // Find the x and y in the beamPoints array space.
    xi -= g->widget.allocation.x;
    yi -= g->widget.allocation.y;

    struct PnBeamPoint *bp = g->beamPoints +
        g->widget.allocation.width * yi + xi;
    DASSERT(bp);

    // Make it newer.
    bp->time = beam->time;

    if(bp->pixel) {
        if(beam->last == bp)
            // Same as last point.
            return;
        // We commandeer this bp entry:
        //
        // Remove this beam point from it's list.
        if(bp->next) {
            DASSERT(bp->beam->last != bp);
            bp->next->prev = bp->prev;
        } else {
            DASSERT(bp->beam->last == bp);
            bp->beam->last = bp->prev;
        }
        if(bp->prev) {
            DASSERT(bp->beam->first != bp);
            bp->prev->next = bp->next;
            bp->prev = 0;
        } else {
            DASSERT(bp->beam->first == bp);
            bp->beam->first = bp->next;
        }
        bp->next = 0;

        // bp->orgColor stays the same.
    } else {
        DASSERT(!bp->next);
        DASSERT(!bp->prev);
        bp->orgColor = *pixel;
        bp->pixel = pixel;
        bp->beam = beam;
    }

    // Add this beam point (bp) to the last in the beams point list.
    //
    bp->prev = beam->last;
    if(beam->last) {
        DASSERT(!beam->last->next);
        DASSERT(beam->first);
        DASSERT(!beam->first->prev);
        beam->last->next = bp;
    }
    beam->last = bp;
    if(bp->prev) {
        DASSERT(beam->first);
        DASSERT(beam->first != bp);
        bp->prev->next = bp;
    } else {
        DASSERT(!beam->first);
        beam->first = bp;
    }

    // Set the pixel color.
    *pixel = p->pointColor;

    if(beam->maxPoints == 0)
        // infinite number of points.
        return;

    ///////////////////////////////////////////////////
    // Now clip and fade the beam.
    ///////////////////////////////////////////////////

    for(bp = beam->first; bp;) {
        // We start with the oldest (largest age) one.
        int32_t age = beam->time - bp->time;
        // age gets smaller (younger) as we loop.

        if(age >= beam->maxPoints) {

            // This point is so old it will be changed to the original color,
            // and the point will be removed from the list.

            struct PnBeamPoint *next = bp->next;

            // Remove this beam point, bp.
            DASSERT(bp->pixel);
            *bp->pixel = bp->orgColor;
            memset(bp, 0, sizeof(*bp));
            bp = next;
            continue;
        }
        // else (age < beam->maxPoints)
        break;
    }

    // Now bp is the new oldest point that we'll keep.

    // This loop just does the fading of pixels.
    for(struct PnBeamPoint *pt = bp; pt; pt = pt->next) {
        int32_t age = beam->time - pt->time;
        // age gets smaller (younger) as we loop.
        int32_t fade_age = beam->maxPoints - beam->fadePoints;

        if(age < fade_age) break;

        // Fade this point pt.
        //
        *pt->pixel = Fade(pt->orgColor, p->pointColor,
                (age - fade_age)/((double) fade_age));
    }

    if(beam->first == bp)
        // No points removed.
        return;

    beam->first = bp;
    if(bp)
        bp->prev = 0;
    else
        beam->last = 0;
}

static void
Cairo_drawPoint(struct PnPlot *p, double x, double y) {
    
    const double hw = p->pointSize;
    const double w = 2.0*hw;

    struct PnZoom *z = p->zoom;
    cairo_t *cr = p->cairo.line;


    // The p->shiftX and p->shiftY is only non-zero for the scope plot
    // because the scope plot is drawn on the widget surface which can be
    // smaller than the static plotting and grid surface.
    x = xToPix(x, z) - p->shiftX;
    y = yToPix(y, z) - p->shiftY;

    if(p->x != DBL_MAX) {

        // TODO: maybe remove these ifs by using different functions.
        if(p->lineWidth > 0) {
            cairo_move_to(cr, p->x, p->y);
            cairo_line_to(cr, x, y);
            // Calling this often must save having to store all
            // those points in the cairo_t object.
            cairo_stroke(cr);
        }

        if(p->pointSize) {
            // Draw the last x, y point.
            cairo_rectangle(p->cairo.point, p->x - hw, p->y - hw, w, w);
            cairo_fill(p->cairo.point);
        }
    }

    p->x = x;
    p->y = y;
}


// TODO: This is likely called in a (tight loop) bottle-neck in most user
// code, so we need to make it faster.  In-lining is step one?
// Currently I'm not seeing it as a bottleneck.
//
void 
pnPlot_drawPoint(struct PnPlot *p, double x, double y) {

    switch(p->drawMethod) {

        case PnDrawMethod_cairo:
            Cairo_drawPoint(p, x, y);
            return;
        case PnDrawMethod_beam:
            Beam_drawPoint(p, x, y);
            return;
        default:
            ASSERT(0);
    }
}
