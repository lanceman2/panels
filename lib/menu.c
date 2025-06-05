#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <linux/input-event-codes.h>
#include <cairo/cairo.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"
#include "SetColor.h"


struct PnMenu {

    struct PnWidget widget; // inherit first
};


#define MIN_WIDTH   (9)
#define MIN_HEIGHT  (9)
#define LW          (2) // Line Width
#define R           (6) // Radius
#define PAD         (2)

static inline void DrawBorder(cairo_t *cr) {

    cairo_surface_t *crs = cairo_get_target(cr);
    int w = cairo_image_surface_get_width(crs);
    int h = cairo_image_surface_get_height(crs);
    if(w < MIN_WIDTH)
        w = MIN_WIDTH;
    if(h < MIN_HEIGHT)
        h = MIN_HEIGHT;
    // If w or h are small this may try to draw outside of the Cairo
    // drawing area, but Cairo can handle culling out extra pixels that
    // are outside the Cairo drawing area.

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
}


static int cairoDraw(struct PnWidget *w,
            cairo_t *cr, struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m == (void *) w);
    DASSERT(GET_WIDGET_TYPE(w->type) == W_MENU);
DASSERT(cr);

    uint32_t color = pnWidget_getBackgroundColor(w);

    SetColor(cr, color);
    cairo_paint(cr);

    return 0;
}


static bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m == (void *) w);
    DASSERT(w->type == PnSurfaceType_menu);
    DASSERT(GET_WIDGET_TYPE(w->type) == W_MENU);

    fprintf(stderr, "\n    press(%p)[%" PRIi32 ",%" PRIi32 "]\n",
            w, x, y);

    return true; // true to grab
}

static bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m == (void *) w);
    DASSERT(w->type == PnSurfaceType_menu);
    DASSERT(GET_WIDGET_TYPE(w->type) == W_MENU);

    fprintf(stderr, "\n  release(%p)[%" PRIi32 ",%" PRIi32 "]\n",
            w, x, y);

    return true;
}

static bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m == (void *) w);
    DASSERT(w->type == PnSurfaceType_menu);
    DASSERT(GET_WIDGET_TYPE(w->type) == W_MENU);

    fprintf(stderr, "\n    enter(%p)[%" PRIi32 ",%" PRIi32 "]\n",
            w, x, y);

    return true; // take focus
}

static void leave(struct PnWidget *w, struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m == (void *) w);
    DASSERT(w->type == PnSurfaceType_menu);
    DASSERT(GET_WIDGET_TYPE(w->type) == W_MENU);

    fprintf(stderr, "\n    leave(%p)[]\n", w);
}


struct PnWidget *pnMenu_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand,
        size_t size) {

    if(size < sizeof(struct PnMenu))
        size = sizeof(struct PnMenu);
    //
    struct PnMenu *m = (void *) pnWidget_create(parent,
            width, height,
            layout, align, expand, size);
    if(!m)
        return 0; // Failure.

    DASSERT(m->widget.type == PnSurfaceType_widget);
    m->widget.type = PnSurfaceType_menu;
    DASSERT(m->widget.type & WIDGET);

    pnWidget_setEnter(&m->widget, (void *) enter, m);
    pnWidget_setLeave(&m->widget, (void *) leave, m);
    pnWidget_setPress(&m->widget, (void *) press, m);
    pnWidget_setRelease(&m->widget, (void *) release, m);
    pnWidget_setCairoDraw(&m->widget, (void *) cairoDraw, m);

    pnWidget_setBackgroundColor(&m->widget, 0xFFCDCDCD);

    return &m->widget;
}
