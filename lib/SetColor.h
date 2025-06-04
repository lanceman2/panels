
static inline void SetColor(cairo_t *cr, uint32_t color) {
    DASSERT(cr);
    cairo_set_source_rgba(cr, PN_R_DOUBLE(color),
            PN_G_DOUBLE(color), PN_B_DOUBLE(color), PN_A_DOUBLE(color));
}
