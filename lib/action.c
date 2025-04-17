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
        // add(), if set, is called when pnWidget_addCallback() is called.
        // The passed callback pointer is a unique ID and opaque pointer
        // to the struct PnCallback.
        void (*add)(struct PnWidget *w, uint32_t actionIndex,
                    struct PnCallback *callback, void *actionData),
        void *actionData, size_t callbackSize) {

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
    a->add = add;
    a->first = 0;
    a->last = 0;
    a->actionIndex = actionIndex;
    if(callbackSize > sizeof(*a->first))
        a->callbackSize = callbackSize;
    else
        a->callbackSize = sizeof(*a->first);
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
    struct PnCallback *c = calloc(1, a->callbackSize);
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

    if(a->add)
        // Tell the particular widget that we are adding a callback.
        a->add(w, a->actionIndex, c, a->actionData);
}
