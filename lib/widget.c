#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "../include/panels.h"
#include "debug.h"
#include  "display.h"
#include "../include/panels_drawingUtils.h"


struct PnWidget *pnWidget_create(
        struct PnSurface *parent, uint32_t w, uint32_t h,
        enum PnGravity gravity, enum PnGreed greed) {

    ASSERT(parent);
    // If there will not be children in this new widget than
    // we need width, w, and height, h to be nonzero.
    ASSERT(gravity > PnGravity_None || (w && h),
            "Widgets with no children must have width and height");
    DASSERT(parent->gravity != PnGravity_None);
    DASSERT(parent->gravity != PnGravity_One || !parent->firstChild);

    if(parent->gravity == PnGravity_None) {
        ERROR("Parent surface cannot have children");
        return 0;
    }

    if(parent->gravity == PnGravity_One &&
            parent->firstChild) {
        ERROR("Parent surface cannot have more than one child");
        return 0;
    }

    struct PnWidget *widget = calloc(1, sizeof(*widget));
    ASSERT(widget, "calloc(1,%zu) failed", sizeof(*widget));

    widget->surface.parent = parent;
    widget->surface.type = PnSurfaceType_widget;
    // Not like GTK; we default to widgets showing; but the window
    // default to not showing after pnWindow_create().
    widget->surface.showing = true;

    if(InitSurface((void *) widget))
        goto fail;

    return widget;

fail:

    pnWidget_destroy(widget);
    return 0;
}


void pnWidget_destroy(struct PnWidget *widget) {

    DASSERT(widget);
    DASSERT(widget->surface.parent);
    ASSERT(widget->surface.type == PnSurfaceType_widget);

    DestroySurface((void *) widget);

    DZMEM(widget, sizeof(*widget));
    free(widget);
DSPEW();
}

void pnWidget_show(struct PnWidget *widget, bool show) {

    DASSERT(widget);
    DASSERT(widget->surface.type == PnSurfaceType_widget);

    // Make it be one of two values.  Because things like (3) == true too.
    show = show ? true : false;

    if(widget->surface.showing == show)
        // No change.
        return;

    widget->surface.showing = show;
    pnWindow_queueDraw(widget->window);
    // This change may change the size of the window and many of the
    // widgets in the window.
    widget->window->needAllocate = true;
}

void pnWidget_queueDraw(struct PnWidget *widget) {

    ASSERT("WRITE THIS CODE");
}
