
#define LW          (2) // Line Width
#define R           (6) // Radius
#define PAD         (2)
#define MIN_WIDTH   (9)
#define MIN_HEIGHT  (9)


static inline void DrawHalo(cairo_t *cr,
        uint32_t minWidth, uint32_t minHeight,
        uint32_t color) {

    SetColor(cr, color);
    cairo_surface_t *crs = cairo_get_target(cr);
    int w = cairo_image_surface_get_width(crs);
    int h = cairo_image_surface_get_height(crs);
    if(w < minWidth)
        w = minWidth;
    if(h < minHeight)
        h = minHeight;
    // If w or h are small this may try to draw outside of the Cairo
    // drawing area, but Cairo can handle culling out extra pixels that
    // are outside the Cairo drawing area, if it's not very large numbers
    // and it is not.

    cairo_set_line_width(cr, LW);
    w -= 2*(R+PAD);
    h -= 2*(R+PAD);
    double x = PAD + R + w, y = PAD;

    cairo_move_to(cr, x, y);
    cairo_arc(cr, x, y += R, R, -0.5*M_PI, 0);
    cairo_line_to(cr, x += R, y += h);
    cairo_arc(cr, x -= R, y, R, 0, 0.5*M_PI);
    cairo_line_to(cr, x -= w, y += R);
    cairo_arc(cr, x, y -= R, R, 0.5*M_PI, M_PI);
    cairo_line_to(cr, x -= R, y -= h);
    cairo_arc(cr, x += R, y, R, M_PI, 1.5*M_PI);
    cairo_close_path(cr);
    cairo_stroke(cr);
}


static inline void DrawHalo2(cairo_t *cr,
        uint32_t minWidth, uint32_t minHeight,
        uint32_t color) {

    SetColor(cr, color);
    cairo_surface_t *crs = cairo_get_target(cr);
    int w = cairo_image_surface_get_width(crs);
    int h = cairo_image_surface_get_height(crs);
    if(w < minWidth)
        w = minWidth;
    if(h < minHeight)
        h = minHeight;
    // If w or h are small this may try to draw outside of the Cairo
    // drawing area, but Cairo can handle culling out extra pixels that
    // are outside the Cairo drawing area, if it's not very large numbers
    // and it is not.

    cairo_set_line_width(cr, 2*LW);
    w -= 2*(R+PAD) + LW;
    h -= 2*(R+PAD) + LW;
    double x = PAD + R + w, y = PAD;

    cairo_move_to(cr, x, y);
    cairo_arc(cr, x, y += R, R, -0.5*M_PI, 0);
    cairo_line_to(cr, x += R, y += h);
    cairo_arc(cr, x -= R, y, R, 0, 0.5*M_PI);
    cairo_line_to(cr, x -= w, y += R);
    cairo_arc(cr, x, y -= R, R, 0.5*M_PI, M_PI);
    cairo_line_to(cr, x -= R, y -= h);
    cairo_arc(cr, x += R, y, R, M_PI, 1.5*M_PI);
    cairo_close_path(cr);
    cairo_stroke(cr);
}

#undef LW
#undef R
#undef PAD
