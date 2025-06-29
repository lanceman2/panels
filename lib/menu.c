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
#include "menuBar.h"


struct Popup {

    struct PnWindow popup; // inherit popup

};


static
void destroy(struct PnWidget *b, struct PnMenu *m) {
    DASSERT(b);
    DASSERT(m);
    DASSERT(b == m->button);
    DASSERT(IS_TYPE2(b->type, PnWidgetType_menu));

    DZMEM(m, sizeof(*m));
    free(m);
}

static inline void PopupDestroy(struct PnMenu *m) {

    if(!m) return;
    if(m->popup) {
        pnWidget_destroy(m->popup);
        m->popup = 0;
    }
}


static bool enterPopup(struct PnWidget *popup,
            uint32_t x, uint32_t y, struct PnMenu *m) {
    DASSERT(popup);
    DASSERT(m);
    DASSERT(m->button);

    fprintf(stderr, "\n    enter(%p)[%" PRIi32 ",%" PRIi32 "]\n",
            popup, x, y);

    // We don't care where the mouse pointer is, we just want the popup
    // to line-up as best it can with the menu widget.
    //
    struct PnAllocation a;
    pnWidget_getAllocation(m->button, &a);
    struct PnAllocation pa;
    pnWidget_getAllocation(popup, &pa);

    // The parent for the popup is the "panels" window that owns this
    // button (menu thingy), and not necessarily the menu button.  The
    // popup is a window with it's own pixel memory allocations and
    // mappings.
    struct PnWidget *p = pnWindow_create(m->button/*parent*/,
            150, 300,
            a.x + pa.width, a.y + a.height, 0, 0, 0);
    pnWindow_show(p, true);

    return true; // take focus
}

static void leavePopup(struct PnWidget *w, struct PnMenu *m) {
    DASSERT(w);

    fprintf(stderr, "    leave(%p)[]\n", w);
}


static inline void ReCreatePopup(struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m->button);
    DASSERT(m->button->window);

    PopupDestroy(m);

    // We don't care where the mouse pointer is, we just want the popup
    // to line-up as best it can with the menu widget.
    //
    struct PnAllocation a;
    pnWidget_getAllocation(m->button, &a);

    // The parent for the popup is the "panels" window that owns this
    // button (menu thingy), and not necessarily the menu button.  The
    // popup is a window with it's own pixel memory allocations and
    // mappings.
    m->popup = pnWindow_create(m->button/*parent*/,
            100/*width*/, 100/*height*/,
            a.x, a.y + a.height, PnLayout_TB, 0, 0);

    {
        // Test add buttons to it.
        pnButton_create(m->popup, 130, 50, 0, 0, 0, 0, false/*toggle*/);
        pnButton_create(m->popup, 130, 50, 0, 0, 0, 0, false/*toggle*/);
        pnButton_create(m->popup, 130, 50, 0, 0, 0, 0, false/*toggle*/);
        pnButton_create(m->popup, 130, 50, 0, 0, 0, 0, false/*toggle*/);
    }

    // Looks like the Wayland compositor puts this popup in a good place
    // even when the menu is in a bad place.
    //
    // TODO: We need to study what the failure modes of the Wayland popup
    // are.
    ASSERT(m->popup);
    pnWidget_setEnter(m->popup, (void *) enterPopup, m);
    pnWidget_setLeave(m->popup, (void *) leavePopup, m);

    pnWidget_setBackgroundColor(m->popup, 0xFA299981);
    pnWindow_show(m->popup, true);

fprintf(stderr, "\r    widget=%p popup at x,y=%" PRIu32 ",%" PRIu32 "    ",
        m->button, a.x, a.y + a.height);

}

static bool enterAction(struct PnWidget *b, uint32_t x, uint32_t y,
            struct PnMenu *m) {
    DASSERT(b);
    DASSERT(m);
    DASSERT(b == m->button);
    DASSERT(IS_TYPE2(b->type, PnWidgetType_menu));

    if(m->button->parent &&
            IS_TYPE2(m->button->parent->type, PnWidgetType_menubar)) {
        // This menu is in a memu bar.
        struct PnMenuBar *mb = (void *) m->button->parent;
        if(m->popup) {
            WARN();
            pnWindow_show(m->popup, false);
        }
        // Change the active menu in the menuBar to "m".
        mb->menu = m;
    }

    ReCreatePopup(m);

    return true; // eat it.
}

// TODO: Not sure we need this callback.
//
static bool leaveAction(struct PnWidget *b, struct PnMenu *m) {
    DASSERT(b);
    DASSERT(m);
    DASSERT(b == m->button);
    DASSERT(IS_TYPE2(b->type, PnWidgetType_menu));

fprintf(stderr, "    leave widget=%p \n", b);

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
