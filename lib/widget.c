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
        enum PnLayout layout, enum PnAlign align,
        enum PnExpand expand, size_t size) {

    ASSERT(parent);
    DASSERT(parent->layout != PnLayout_None);
    DASSERT(parent->layout != PnLayout_One || !parent->firstChild);

    if(parent->layout == PnLayout_None) {
        ERROR("Parent surface cannot have children");
        return 0;
    }

    if(parent->layout == PnLayout_One &&
            parent->firstChild) {
        ERROR("Parent surface cannot have more than one child");
        return 0;
    }

    // If there will not be children in this new widget than
    // we need width, w, and height, h to be nonzero.
    if(layout == PnLayout_None) {
        if(w == 0)
            w = PN_DEFAULT_WIDGET_WIDTH;
        if(h == 0)
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
    widget->surface.layout = layout;
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

static inline
void RemoveCallback(struct PnAction *a, struct PnCallback *c) {

    DASSERT(c);

    if(c->next) {
        DASSERT(c != a->last);
        c->next->prev = c->prev;
    } else {
        DASSERT(c == a->last);
        a->last = c->prev;
    }

    if(c->prev) {
        DASSERT(c != a->first);
        c->prev->next = c->next;
    } else {
        DASSERT(c == a->first);
        a->first = c->next;
        //c->prev = 0; // DZMEM() does this.
    }
    //c->next = 0; // DZMEM() does this.

    // Free c
    DZMEM(c, sizeof(*c));
    free(c);
}

void pnWidget_destroy(struct PnWidget *w) {

    DASSERT(w);
    DASSERT(w->surface.parent);
    ASSERT(w->surface.type & WIDGET);

    // Free actions
    if(w->actions) {
        DASSERT(w->numActions);
        struct PnAction *a = w->actions;
        struct PnAction *end = a + w->numActions;
        for(; a != end; ++a)
            while(a->first)
                RemoveCallback(a, a->first);
        // Free the actions[] array.
        DZMEM(w->actions, w->numActions*sizeof(*w->actions));
        free(w->actions);
    }

    // If there is state in the display that refers to this surface (w)
    // take care to not refer to it.  Like if this widget (w) had focus,
    // for example.
    RemoveSurfaceFromDisplay((void *) w);

    while(w->destroys) {
        struct PnWidgetDestroy *destroy = w->destroys;
        destroy->destroy(w, destroy->destroyData);
        // pop one off the stack
        w->destroys = destroy->next;
        // Free the struct PnWidgetDestroy element.
        DZMEM(destroy, sizeof(*destroy));
        free(destroy);
    }

    while(w->surface.firstChild)
        pnWidget_destroy((void *) w->surface.firstChild);

    DestroySurface((void *) w);
    DZMEM(w, w->size);
    free(w);
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
        bool (*action)(struct PnWidget *widget,
            // The callback() can be any function prototype.  It's just a
            // pointer to any kind of function.  We'll pass this pointer
            // to the action function that knows what to do with it.
            void *callback, void *userData,
            void *actionData),
        void *actionData) {

    DASSERT(w);
    DASSERT(action);
    // Actions must be added in order.
    ASSERT(actionIndex == w->numActions,
            "Callback action number %" PRIu32
            " added out of order: !=%" PRIu32 ")",
            actionIndex, w->numActions);
    // Add an action to the actions[] array.
    w->actions = realloc(w->actions, (++w->numActions)*sizeof(*w->actions));
    ASSERT(w->actions, "realloc(,%zu) failed",
            w->numActions*sizeof(*w->actions));
    // Set values in the end element of the actions[].
    struct PnAction *a = w->actions + w->numActions-1;
    a->action = action;
    a->actionData = actionData;
    a->first = 0;
    a->last = 0;
}

void pnWidget_callAction(struct PnWidget *w, uint32_t index) {
    DASSERT(w);
    ASSERT(index < w->numActions);
    DASSERT(w->actions);

    struct PnCallback *c = w->actions[index].first;
    if(!c)
        // The widget API programmer did not call pnWidget_addCallback()
        // for this index.
        return;

    // Get the widget builder action() function.
    bool (*action)(struct PnWidget *widget, void *callback,
            void *userData, void *actionData) = w->actions[index].action;
    void *actionData = w->actions[index].actionData;
    for(; c; c = c->next)
        if(action(w, c->callback, c->callbackData, actionData))
            break;
}

void pnWidget_addCallback(struct PnWidget *w, uint32_t index,
        // The callback function prototype varies with particular widget
        // and index.  The widget maker must publish a list of function
        // prototypes and indexes; example: PN_BUTTON_CB_CLICK.
        void *callback, void *userData) {
    DASSERT(w);
    ASSERT(index < w->numActions);
    DASSERT(w->actions);
    ASSERT(callback);

    struct PnAction *a = w->actions + index;
    struct PnCallback *c = calloc(1, sizeof(*c));
    ASSERT(c, "calloc(1,%zu) failed", sizeof(*c));
    c->callback = callback;
    c->callbackData = userData;

    // Add this callback to the action callback as the last one.
    if(a->last) {
        DASSERT(a->first);
        DASSERT(!a->last->next);
        DASSERT(!a->first->prev);
        a->last->next = c;
    } else {
        DASSERT(!a->first);
        a->first = c;
    }
    c->prev = a->last;
    a->last = c;
}
