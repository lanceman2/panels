#include <cairo/cairo.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"
#include "SetColor.h"
#include "check.h"


#define LW    (2.8)
#define PAD   (3.2)

static int cairoDraw(struct PnWidget *w,
            cairo_t *cr, struct PnCheck *c) {
    DASSERT(w);
    DASSERT(w == (void *) c);
    DASSERT(IS_TYPE1(w->type, PnWidgetType_check));
    DASSERT(cr);

    struct PnAllocation a;
    pnWidget_getAllocation(w, &a);

    DASSERT(a.width);
    DASSERT(a.height);

    double x, y;

    SetColor(cr, w->backgroundColor);
    cairo_paint(cr);

    // Draw a box.
    SetColor(cr, 0xFF000000);
    cairo_set_line_width(cr, LW);
    cairo_move_to(cr, PAD, y = PAD);
    cairo_line_to(cr, x = a.width - PAD, y);
    cairo_line_to(cr, x, y += a.height - 2*PAD);
    cairo_line_to(cr, x = PAD, y);
    cairo_line_to(cr, x, PAD);
    cairo_close_path(cr);
    cairo_stroke(cr);

    if(c->checked) {
        // Draw a check.
        SetColor(cr, 0xFF000000);
        cairo_set_line_width(cr, LW);
        cairo_move_to(cr, PAD + 1.2*LW, a.height/2.0);
        cairo_line_to(cr, a.width/2.0, a.height - 1.2*LW - PAD);
        cairo_line_to(cr, a.width - PAD - 1.2*LW, PAD + 1.2*LW);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_stroke(cr);
    }

    return 0;
}


static void destroy(struct PnWidget *w, struct PnCheck *c) {
    DASSERT(c);
    DASSERT(c == (void *) w);
    DASSERT(IS_TYPE1(w->type, PnWidgetType_check));

    RemoveToggleCheck(c);
}


struct PnWidget *pnCheck_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnAlign align,
        enum PnExpand expand) {

    ASSERT(width > 2*PAD);
    ASSERT(height > 2*PAD);

    struct PnCheck *c = (void *) pnWidget_create(parent,
            width, height,
            0/*layout*/, align, expand, sizeof(*c));
    if(!c)
        // A common error mode is that the parent cannot have children.
        // pnWidget_create() should spew for us.
        return 0; // Failure.

    DASSERT(c->widget.type == PnWidgetType_widget);
    c->widget.type = PnWidgetType_check;
    DASSERT(IS_TYPE1(c->widget.type, PnWidgetType_check));

    pnWidget_setCairoDraw(&c->widget, (void *) cairoDraw, c);
    pnWidget_addDestroy(&c->widget, (void *) destroy, c);


    pnWidget_setBackgroundColor(&c->widget, 0xFFCDCDCD, 0);

    return &c->widget;
}


void pnCheck_set(struct PnWidget *w, bool on) {

    DASSERT(w);
    DASSERT(IS_TYPE1(w->type, PnWidgetType_check));

    struct PnCheck *c = (void *) w;

    // Will this "on" work if "on" is set to any values that are non
    // zero?  Like:
    //if(on == c->checked) return;
    // Or is this needed?  And there's the question of portability
    // which we can't test on one OS (computer).
    if((on?true:false) == c->checked) return;

    c->checked = on?true:false;
    pnWidget_queueDraw(w, 0);
}
