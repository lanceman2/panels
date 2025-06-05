// This file is linked into libpanels.so if WITH_CAIRO is defined
// in ../config.make, otherwise it's not.
//
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cairo/cairo.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"


#define MIN_WIDTH   (10)
#define MIN_HEIGHT  (10)
#define PAD         (2)


struct PnLabel {

    struct PnWidget widget; // inherit first
    char *text;
    double fontSize;
    uint32_t fontColor; // text color
    uint32_t xPadding, yPadding;
};


static inline void Draw(struct PnLabel *l, cairo_t *cr) {

    DASSERT(cr);
    DASSERT(l);
    char *text = l->text;
    DASSERT(text);
    DASSERT(strlen(text));

    const uint32_t color = l->widget.backgroundColor;

    cairo_set_source_rgba(cr,
            PN_R_DOUBLE(color), PN_G_DOUBLE(color),
            PN_B_DOUBLE(color), PN_A_DOUBLE(color));
    cairo_paint(cr);

    cairo_set_source_rgba(cr,
            PN_R_DOUBLE(l->fontColor),
            PN_G_DOUBLE(l->fontColor),
            PN_B_DOUBLE(l->fontColor),
            PN_A_DOUBLE(l->fontColor));

    struct PnAllocation a;
    pnWidget_getAllocation(&l->widget, &a);
    double w = a.width;
    double h = a.height;
    cairo_text_extents_t extents;

    cairo_set_font_size(cr, l->fontSize);
    cairo_text_extents(cr, text, &extents);

    double x, y;

    switch(l->widget.align & PN_ALIGN_X) {
        case PN_ALIGN_X_LEFT:
            x = - extents.x_bearing + l->xPadding;
            break;
        case PN_ALIGN_X_RIGHT:
            x = - extents.x_bearing + w - extents.width - l->xPadding;
            break;
        default:
        //case PN_ALIGN_X_CENTER:
        //case PN_ALIGN_X_JUSTIFIED:
            x = 0.5 *(w - extents.width) - extents.x_bearing;
            break;
    }

    switch(l->widget.align & PN_ALIGN_Y) {
        case PN_ALIGN_Y_TOP:
            y = - extents.y_bearing + l->yPadding;
            break;
        case PN_ALIGN_Y_BOTTOM:
            y = - extents.y_bearing + h - extents.height - l->yPadding;
            break;
        default:
        //case PN_ALIGN_Y_CENTER:
        //case PN_ALIGN_Y_JUSTIFIED:
            y = - extents.y_bearing + 0.5 * (h - extents.height);
            break;
    }

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);
}

static void config(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h,
            uint32_t stride/*4 byte chunks*/,
            struct PnLabel *l) {
    DASSERT(l);
    DASSERT(l == (void *) widget);
    DASSERT(w);
    DASSERT(h);
    DASSERT(widget->type == PnSurfaceType_label);
    //l->width = w;
    //l->height = h;
}

static int cairoDraw(struct PnWidget *w,
            cairo_t *cr, struct PnLabel *l) {
    DASSERT(l);
    DASSERT(w->type == PnSurfaceType_label);
    DASSERT(l == (void *) w);
    DASSERT(cr);

    Draw(l, cr);

    return 0;
}


static
void destroy(struct PnWidget *w, struct PnLabel *l) {

    DASSERT(l);
    DASSERT(l = (void *) w);
    DASSERT(l->text);
    DZMEM(l->text, strlen(l->text));
    free(l->text);
}

static inline
uint32_t GetWidthAndFontSize(const char *text, uint32_t h,
        double *sizeOut) {

    DASSERT(strlen(text));
    DASSERT(h);
    DASSERT(sizeOut);
    cairo_surface_t *surface;
    cairo_t *cr;
    // We never draw to this tiny surface.  I wonder if we can use a null
    // surface.
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, h);
    cr = cairo_create(surface);

    const double height = h;

    //https://www.cairographics.org/samples/text_extents/
    cairo_text_extents_t extents;

    double size = h;
    cairo_set_font_size(cr, size);
    cairo_text_extents(cr, text, &extents);
 
    if(extents.height > height) {
        size *= height/extents.height;
        cairo_set_font_size(cr, size);
        cairo_text_extents(cr, text, &extents);
    }
    while(extents.height > height) {
        // The font sizes seem to not be scalable continuously, so we keep
        // shrinking the font until it fits.  We do not want to know what
        // the algorithm is so long as this does not take too long.
        size *= 0.997;
        //fprintf(stderr, "*");
        cairo_set_font_size(cr, size);
        cairo_text_extents(cr, text, &extents);
    }
    //fprintf(stderr, "width=%lg\n", extents.width);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    *sizeOut = size;
    return extents.width + 1;
}


struct PnWidget *pnLabel_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        uint32_t xPadding, uint32_t yPadding,
        enum PnAlign align, // for text alignment
        enum PnExpand expand,
        const char *text, size_t size) {

    DASSERT(text);
    DASSERT(strlen(text));
    ASSERT(yPadding < height);

    if(height < MIN_HEIGHT)
        height = MIN_HEIGHT;

    double fontSize = height;

    if(width == 0)
        width = GetWidthAndFontSize(text, height - yPadding,
                &fontSize) + 2*xPadding;

    if(width < MIN_WIDTH)
        width = MIN_WIDTH;
    if(width < xPadding)
        width = xPadding;

    if(size < sizeof(struct PnLabel))
        size = sizeof(struct PnLabel);
    //
    struct PnLabel *l = (void *) pnWidget_create(parent,
            width, height,
            PnLayout_None, align/*align*/,
            expand, size);
    if(!l)
        // TODO: Not sure that this can fail, other than memory allocation
        // failure.  For now, propagate the failure from
        // pnWidget_create() (which will spew for us).
        return 0; // Failure.

    // It starts out life as a widget:
    DASSERT(l->widget.type == PnSurfaceType_widget);
    // And now it becomes a label:
    l->widget.type = PnSurfaceType_label;
    // which is also a widget too (and more bits):
    DASSERT(l->widget.type & WIDGET);

    l->fontSize = fontSize;
    l->fontColor = 0xFF000000;
    l->text = strdup(text);
    ASSERT(l->text, "strdup() failed");
    l->xPadding = xPadding;
    l->yPadding = yPadding;

    //ASSERT(l->widget.window);

    pnWidget_setConfig(&l->widget, (void *) config, l);
    pnWidget_setCairoDraw(&l->widget, (void *) cairoDraw, l);
    pnWidget_addDestroy(&l->widget, (void *) destroy, l);

    return &l->widget; // We inherited PnWidget.
}

void pnLabel_setFontColor(struct PnWidget *w, uint32_t color) {

    DASSERT(w);
    ASSERT(w->type == PnSurfaceType_label);
    struct PnLabel *l = (void *) w;
    l->fontColor = color;
}
