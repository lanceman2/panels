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

    // Perhaps it's called composition, but the API user will not know or
    // see the difference, so I think it's fair to call it inheritance.
    struct PnWidget *button; // inherit (opaquely).

    struct PnWidget *popup;
    bool popupShowing;
};

static inline
void PopupCreate(struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m->button);
    DASSERT(!m->popupShowing);
    DASSERT(!m->popup);

    struct PnWidget *popup = pnWindow_create(m->button/*parent*/,
            100, 300, 200/*x*/, 0/*y*/, 0, 0, 0);
    ASSERT(popup);
    pnWidget_setBackgroundColor(popup, 0xFA299981, 0);
    m->popup = popup;
}

static inline
void PopupShow(struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m->button);
    DASSERT(!m->popupShowing);

    if(!m->popup)
        PopupCreate(m);

    // WTF do we do if this fails:
    ASSERT(!pnWindow_show(m->popup));
    m->popupShowing = true;
}

static inline
void PopupDestroy(struct PnMenu *m) {
    DASSERT(m);
    struct PnWidget *popup = m->popup;
    if(!popup) return;

    pnWidget_destroy(popup);
    m->popup = 0;
}

static inline
void PopupHide(struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m->popup);
    DASSERT(m->popupShowing);

    pnPopup_hide(m->popup);
    m->popupShowing = false;
}

static
void destroy(struct PnWidget *b, struct PnMenu *m) {
    DASSERT(b);
    DASSERT(m);
    DASSERT(b == m->button);
    DASSERT(IS_TYPE2(b->type, PnWidgetType_menu));
    PopupDestroy(m);
    DZMEM(m, sizeof(*m));
    free(m);
}

static bool enterAction(struct PnWidget *b, uint32_t x, uint32_t y,
            struct PnMenu *m) {
    DASSERT(b);
    DASSERT(m);
    DASSERT(b == m->button);
    DASSERT(IS_TYPE2(b->type, PnWidgetType_menu));

    PopupShow(m);
    fprintf(stderr, "    enter widget=%p \n", b);
    return true; // eat it.
}

static bool leaveAction(struct PnWidget *b, struct PnMenu *m) {
    DASSERT(b);
    DASSERT(m);
    DASSERT(b == m->button);
    DASSERT(IS_TYPE2(b->type, PnWidgetType_menu));

    PopupHide(m);
//fprintf(stderr, "    leave widget=%p \n", b);

    return true; // eat it.
}


struct PnWidget *pnMenu_addItem(struct PnWidget *menu,
    const char *label) {

    DASSERT(menu); // pointer to the button

    //struct PnMenu *m = pnWidget_getUserData(menu);

    // LEFT OFF HERE ..................

    return 0;
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
    pnWidget_setBackgroundColor(m->button, 0xFFCDCDCD, true);
    pnWidget_addCallback(m->button,
            PN_BUTTON_CB_ENTER, enterAction, m);
    pnWidget_addCallback(m->button,
            PN_BUTTON_CB_LEAVE, leaveAction, m);
    pnWidget_setUserData(m->button, m);
    // The user gets the button, but it's augmented.
    return m->button;
}
