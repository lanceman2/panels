// This file is linked into libpanels.so if WITH_CAIRO is defined
// in ../config.make, otherwise it's not.
//
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

    // We are trying to keep the struct PnButton, which we build this
    // PnMenu widget on, opaque; so we just have a pointer to it.
    // We'll append the widget button type with menu, W_MENU.
    //
    // This is a different way to inherit a button widget.  We know
    // the widget part of it as it has a struct PnWidget in it.
    //
    // Funny how we can inherit without knowing the full button data
    // structure.  We'll use this pointer as the thing that the user
    // gets.  offsetof(3) is our friend to find other data in this.
    //
    // Does C++ have this kind of opaque inheritance built into the
    // language?  I grant you, it's not as efficient (plus one pointer)
    // but we don't need to expose the full class (struct) data structure
    // in a header file.
    struct PnWidget *button;


    struct PnWidget *popup;
    // More data here..
};


static
void destroy(struct PnWidget *b, struct PnMenu *m) {
    DASSERT(b);
    DASSERT(m);
    DASSERT(b == m->button);

    DZMEM(m, sizeof(*m));
    free(m);
}

static bool enterAction(struct PnWidget *b, uint32_t x, uint32_t y,
            struct PnMenu *m) {
    DASSERT(b);
    DASSERT(m);
    DASSERT(b == m->button);
    DASSERT(b->window);

    struct PnAllocation a;
    pnWidget_getAllocation(b, &a);

    if(m->popup) {
        pnWidget_destroy(m->popup);
        m->popup = 0;
    }

    m->popup = pnWindow_create(&b->window->widget/*parent*/, 100, 300,
            a.x, a.y + a.height, 0, 0, 0);
    // Looks like the Wayland compositor puts this popup in a good place
    // even when the menu is in a bad place.
    //
    // TODO: We need to study what the failure modes of the Wayland popup
    // are.
    ASSERT(m->popup);
    pnWidget_setBackgroundColor(m->popup, 0xFA299981);
    pnWindow_show(m->popup, true);

fprintf(stderr, "\r    widget=%p x,y=%" PRIu32 ",%" PRIu32 "    ", b, x, y);

    return true; // eat it.
}

// TODO: Not sure we need this callback.
//
static bool leaveAction(struct PnWidget *b, struct PnMenu *m) {
    DASSERT(b);
    DASSERT(m);
    DASSERT(b == m->button);

fprintf(stderr, "\r leave widget=%p                ", b);

    return true; // eat it.
}


struct PnWidget *pnMenu_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand, const char *label) {

    ASSERT(!label, "WRITE MORE CODE");

    struct PnMenu *m = calloc(1, sizeof(*m));
    ASSERT(m, "calloc(1,%zu) failed", sizeof(*m));

    m->button = pnButton_create(parent, width, height,
            layout, align, expand, label, false/*toggle*/);
    if(!m->button) {
        DZMEM(m, sizeof(*m));
        free(m);
        return 0; // Failure.
    }

    DASSERT(m->button->type == PnWidgetType_button);
    DASSERT(m->button->type & WIDGET);
    m->button->type = PnWidgetType_menu;

    pnWidget_addDestroy(m->button, (void *) destroy, m);
    pnWidget_setBackgroundColor(m->button, 0xFFCDCDCD);
    pnWidget_addCallback(m->button,
            PN_BUTTON_CB_ENTER, enterAction, m);
    pnWidget_addCallback(m->button,
            PN_BUTTON_CB_LEAVE, leaveAction, m);
    // The user gets the button, but it's augmented.
    return m->button;
}
