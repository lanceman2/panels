// references:
//
// https://specifications.freedesktop.org/icon-theme-spec/latest/#directory_layout
//
// https://www.cairographics.org/samples/clip_image/
//
// In libpanels the image widget is intended to be used for widget icons,
// but the image widget has no utility to find icon image files.  We thought
// of calling it the icon widget, but functionally it's more of just a
// tool to open and read an image file and put a representation of that
// file in the widget's pixels, it creates an image in a widget.
//
// The image widget does not do any theme icon file lookup stuff.  The
// icon widget is an image widget with the theme icon file lookup stuff
// built into it.
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
#include "cairoWidget.h"


static
void destroy(struct PnWidget *w, struct PnImage *image) {

    DASSERT(image);
    DASSERT(image = (void *) w);
    DASSERT(w->type == PnSurfaceType_image);
    DASSERT(image->surface);

    cairo_surface_destroy(image->surface);
    // The widget will be destroyed after this returns.
}

static int cairoDraw(struct PnWidget *widget,
            cairo_t *cr, struct PnImage *image) {
    DASSERT(image);
    DASSERT(widget->type == PnSurfaceType_image);
    DASSERT(image == (void *) widget);
    DASSERT(image->surface);
    DASSERT(cr);

    cairo_surface_t *surface = cairo_get_target(cr);
    DASSERT(surface);
    double w = cairo_image_surface_get_width(surface);
    DASSERT(w);
    double h = cairo_image_surface_get_height(surface);
    DASSERT(h);

    const uint32_t color = widget->backgroundColor;

    cairo_set_source_rgba(cr,
            PN_R_DOUBLE(color), PN_G_DOUBLE(color),
            PN_B_DOUBLE(color), PN_A_DOUBLE(color));
    cairo_paint(cr);

    // Align the image using the widget align attribute. 
    double x = 0, y = 0;
    //
    switch(widget->align & PN_ALIGN_X) {

        case PN_ALIGN_X_LEFT:
            x = 0;
            break;
        case PN_ALIGN_X_RIGHT:
            x = w - (double) image->width;
            break;
        default:
        //case PN_ALIGN_X_CENTER:
        //case PN_ALIGN_X_JUSTIFIED;
            x = (w - (double) image->width)/2.0;
            break;
    }
    //
    switch(widget->align & PN_ALIGN_Y) {

        case PN_ALIGN_Y_TOP:
            y = 0;
            break;
        case PN_ALIGN_Y_BOTTOM:
            y = h - (double) image->height;
            break;
        default:
        //case PN_ALIGN_Y_CENTER:
        //case PN_ALIGN_Y_JUSTIFIED;
            y = (h - (double) image->height)/2.0;
            break;
    }

    cairo_set_source_surface(cr, image->surface, x, y);
    cairo_rectangle(cr, x, y, image->width, image->height);
    cairo_fill(cr);

    return 0;
}


// TODO: option to scale or crop?
//
struct PnWidget *pnImage_create(struct PnWidget *parent,
        const char *filename,
        // We'll use this width and height to scale the image.  If the
        // user wants padding then they can add that by putting this in a
        // container widget.
        //
        // TODO: do we want the image in the widget to expand?
        // That may require reloading the image file or keeping more than
        // one Cairo surface, because cairo_scale() has to be lossy when
        // it shrinks images.
        uint32_t width, uint32_t height,
        enum PnAlign align,
        enum PnExpand expand, size_t size) {

    struct PnImage *image;

    if(size < sizeof(*image))
        size = sizeof(*image);

    cairo_surface_t *surface =
        cairo_image_surface_create_from_png(filename);
    if(!surface) {
        ERROR("cairo_image_surface_create_from_png(\"%s\") failed",
                filename);
        return 0;
    }
    int w = cairo_image_surface_get_width(surface);
    int h = cairo_image_surface_get_height(surface);
    if(w <= 0 || h <= 0) {
        ERROR("cairo_image_surface_create_from_png(\"%s\") "
                "kind of failed", filename);
        cairo_surface_destroy(surface);
        return 0;
    }
    if(width == 0)
        width = w;
    if(height == 0)
        height = h;

#if 0
    cairo_t *cr = cairo_create(surface);
    // Use width and height to scale the image.
    cairo_scale(cr, w/((double)width), h/((double)height));

    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    // We need this surface to be a happy surface.
    DASSERT(CAIRO_STATUS_SUCCESS ==
            cairo_surface_status(surface));
#endif

    image = (void *) pnWidget_create(parent,
        width, height,
        PnLayout_None/*will have NO children*/,
        align, expand, size);
    ASSERT(image);
    DASSERT(image->widget.type & WIDGET);
    image->widget.type |= W_IMAGE;
    image->surface = surface;
    image->width = w;
    image->height = h;

    pnWidget_setCairoDraw(&image->widget, (void *) cairoDraw, image);
    pnWidget_addDestroy(&image->widget, (void *) destroy, image);

    return &image->widget;
}
