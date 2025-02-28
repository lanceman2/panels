#include <string.h>
#include <stdio.h>
#include <cairo.h>

#include "../lib/debug.h"


int main(void) {

    const int w = 1000, h = 500;
    cairo_surface_t *surface;
    cairo_t *cr;
    char text[]="Go3423iiiiiiiiiiiiiiiii4324234y";
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cr = cairo_create(surface);

    const double height = h;
    const double width  = w;

    //https://www.cairographics.org/samples/text_extents/
    cairo_text_extents_t extents;

    double size = h;
    cairo_set_font_size(cr, size);
    cairo_text_extents(cr, text, &extents);

    if(extents.width > width) {
        size *= width/extents.width;
        cairo_set_font_size(cr, size);
        cairo_text_extents(cr, text, &extents);
    }
    while(extents.width > width) {
        // The font sizes seem to not be scalable continuously,
        // so we keep shrinking the font until it fits.
        // We do not want to know what the algorithm is so
        // long as this does not take too long.
        size *= 0.998;
        fprintf(stderr, "#");
        cairo_set_font_size(cr, size);
        cairo_text_extents(cr, text, &extents);
    }
 
    if(extents.height > height) {
        size *= height/extents.height;
        cairo_set_font_size(cr, size);
        cairo_text_extents(cr, text, &extents);
    }
    while(extents.height > height) {
        // The font sizes seem to not be scalable continuously,
        // so we keep shrinking the font until it fits.
        // We do not want to know what the algorithm is so
        // long as this does not take too long.
        size *= 0.997;
        fprintf(stderr, "*");
        cairo_set_font_size(cr, size);
        cairo_text_extents(cr, text, &extents);
    }
    fprintf(stderr, "\n");
 
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_paint(cr);

    cairo_set_source_rgba(cr, 0, 0, 0, 1);
        cairo_move_to(cr, 0.5 *(width - extents.width) - extents.x_bearing,
                - extents.y_bearing + 0.5 * (height - extents.height));

    cairo_show_text (cr, text);

    /* Write output and clean up */
    cairo_surface_write_to_png(surface, "tmp_CairoTextSize.png");
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    cairo_debug_reset_static_data();

    return 0;
}
