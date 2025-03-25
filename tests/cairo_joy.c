#include <cairo.h>
#include <fontconfig/fontconfig.h>

int main(void) {

    const int w = 380, h = 200;
    cairo_surface_t *surface;
    cairo_t *cr;
    char text[] = "Memory Leak";

    /* Prepare drawing area */
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cr = cairo_create(surface);
    cairo_set_font_size(cr, 50.0);

    /* Drawing code goes here */
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_paint(cr);

    cairo_select_font_face(cr, "Sans",
            CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_move_to(cr, 20, 100);
    cairo_show_text(cr, text);

    /* Write output */
    cairo_surface_write_to_png(surface, "tmp_cairo_joy.png");

    // Clean up
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    cairo_debug_reset_static_data();
    FcFini();

    return 0;
}
