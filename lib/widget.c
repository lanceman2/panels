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
        enum PnDirection direction, enum PnAlign align,
        enum PnExpand expand, size_t size) {

    ASSERT(parent);
    DASSERT(parent->direction != PnDirection_None);
    DASSERT(parent->direction != PnDirection_One || !parent->firstChild);

    if(parent->direction == PnDirection_None) {
        ERROR("Parent surface cannot have children");
        return 0;
    }

    if(parent->direction == PnDirection_One &&
            parent->firstChild) {
        ERROR("Parent surface cannot have more than one child");
        return 0;
    }

    // If there will not be children in this new widget than
    // we need width, w, and height, h to be nonzero.
    if(direction == PnDirection_None) {
        w = PN_DEFAULT_WIDGET_WIDTH;
        h = PN_DEFAULT_WIDGET_HEIGHT;
    }
    struct PnWidget *widget;

    if(size < sizeof(*widget))
        size = sizeof(*widget);

    widget = calloc(1, size);
    ASSERT(widget, "calloc(1,%zu) failed", size);
#ifdef DEBUG
    widget->size = size;
#endif
    widget->surface.parent = parent;
    widget->surface.direction = direction;
    widget->surface.align = align;
    * (enum PnExpand *) &widget->surface.expand = expand;
    widget->surface.type = PnSurfaceType_widget;

    if(parent->type & WIDGET)
        widget->surface.window = parent->window;
    else
        widget->surface.window = (void *) parent;

    // This is how we C casting to change const variables:
    *((uint32_t *) &widget->surface.width) = w;
    *((uint32_t *) &widget->surface.height) = h;

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
    ASSERT(widget->surface.type & WIDGET);

    // If there is state in the display that refers to this surface
    // (widget) take care to not refer to it.  Like if this widget
    // had focus for example.
    RemoveSurfaceFromDisplay((void *) widget);

    while(widget->destroys) {
        struct PnWidgetDestroy *destroy = widget->destroys;
        destroy->destroy(widget, destroy->destroyData);
        // pop one off the stack
        widget->destroys = destroy->next;
        // Free the struct PnWidgetDestroy element.
        DZMEM(destroy, sizeof(*destroy));
        free(destroy);
    }

    while(widget->surface.firstChild)
        pnWidget_destroy((void *) widget->surface.firstChild);

    DestroySurface((void *) widget);
    DZMEM(widget, widget->size);
    free(widget);
}

void pnWidget_show(struct PnWidget *widget, bool show) {

    DASSERT(widget);
    DASSERT(widget->surface.type & WIDGET);

    // Make it be one of two values.  Because we can have things like (3)
    // == true
    //
    // I just do not want to assume how many bits are set in the value of
    // true.  Maybe true is 0xFF of maybe it's 0x01.  Hell, I don't
    // necessarily know number of byte in the value of true and false.  I
    // don't even know if C casting fixes its value to just true and false
    // (I doubt it does).
    //
    show = show ? true : false;

    if(widget->surface.hidden == !show)
        // No change.
        return;

    widget->surface.hidden = !show;
    // This change may change the size of the window and many of the
    // widgets in the window.

    widget->surface.window->needAllocate = true; 

    //pnWindow_queueDraw(widget->window);
}

void pnWidget_addDestroy(struct PnWidget *w,
        void (*destroy)(struct PnWidget *widget, void *userData),
        void *userData) {

    DASSERT(w);

    if(!destroy) return;

    struct PnWidgetDestroy *dElement = calloc(1, sizeof(*dElement));
    ASSERT(dElement, "calloc(1,%zu) failed", sizeof(*dElement));
    dElement->destroy = destroy;
    dElement->destroyData = userData;

    // Add this new element to the widgets destroy list at the top:
    //
    // Save the last one, if any.
    dElement->next = w->destroys;
    // Make this element the top one in the stack.
    w->destroys = dElement;
}


void pnWidget_addAction(struct PnWidget *w,
        uint32_t actionIndex,
        bool *(*action)(struct PnWidget *widget,
            // The callback() can be any function prototype.  It's just a
            // pointer to any kind of function.  We'll pass this pointer
            // to the action function that knows what to do with it.
            void *callback, void *userData,
            void *actionData),
        void *actionData)
{
    DASSERT(w);
    DASSERT(action);

}

void pnWidget_callAction(struct PnWidget *w, uint32_t index) {
    DASSERT(w);
    ASSERT(index < w->numActions);

}

void pnWidget_addCallback(struct PnWidget *w, uint32_t index,
        // The callback function prototype varies with particular widget
        // and index.  The widget maker must publish a list of function
        // prototypes and indexes; example: PN_BUTTON_CB_CLICK.
        void *callback, void *userData) {


}
