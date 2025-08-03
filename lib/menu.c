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
    //
    // TODO: Change button to not a pointer, but the struct PnButton.
    // This opaquely inheritance shit sucks.   pnWidget_setUserData() and
    // pnWidget_getUserData() is a bad idea, you can't do higher
    // inheritance levels with it.   Or is there another way?  Higher
    // levels of user data?
    //
    struct PnWidget *button; // inherit (opaquely).

    struct PnMenu *parent; // for sub-menus that have a parent menu

    struct PnWidget *popup; // Or no popup
};


// We only allow one leaf menu (and it's popups) to be active at a time;
// per process.  "top" is the last menu (popup) to be showing (triggered).
//
// So: we use a stack with the child most menu at the top.  We pop it by
// changing the top to the parent.  We push a child menu to the top.
static struct PnMenu *top = 0; // active showing menu popup


bool PopupEnter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnMenu *m) {
    DASSERT(w);
    DASSERT(m);
    DASSERT(m->popup);

WARN();
    return false;
}

static inline
void PopupCreate(struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m->button);
    DASSERT(!m->popup);
    struct PnWidget *popup = pnWindow_create(m->button/*parent*/,
            0, 0, 0/*x*/, 0/*y*/, PnLayout_TB, 0, 0);
    ASSERT(popup);
    pnWidget_setBackgroundColor(popup,
            pnWidget_getBackgroundColor(m->button), 0);
    m->popup = popup;
    pnWidget_setEnter(popup, (void *) PopupEnter, m);
}

static inline
struct PnWidget *PopupGet(struct PnMenu *m) {

    DASSERT(m);
    if(!m->popup)
        PopupCreate(m);
    return m->popup;
}

static void PopupHide(struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m->popup);
    DASSERT(m == top);
    pnPopup_hide(m->popup);
    top = m->parent;
}

static inline void PopupShow(struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m->button);
    
    if(m == top) return;

    // First: hide menus that are showing, that are not m's parent.
    while(top && m->parent != top)
        PopupHide(top);

    if(!m->popup)
        // If there was no menu items created than there is no
        // popup created.
        return;

    // Current/last menu to show.
    top = m;

    int32_t x, y;
    struct PnAllocation a;

    if(m->parent) {
        DASSERT(m->parent->popup);
        DASSERT(m->button->window->widget.type & POPUP);
        pnWidget_getAllocation(m->button, &a);
        x = m->button->window->popup.x + a.x + a.width;
        y = m->button->window->popup.y + a.y;
    } else {
        pnWidget_getAllocation(m->button, &a);
        x = a.x;
        y = a.y + a.height;
    }

    // WTF do we do if this fails:
    ASSERT(!pnPopup_show(m->popup, x, y));
}

static inline
void PopupDestroy(struct PnMenu *m) {
    DASSERT(m);
    struct PnWidget *popup = m->popup;
    if(!m->popup) return;

    pnWidget_destroy(popup);
    m->popup = 0;
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

fprintf(stderr, "    leave widget=%p \n", b);

    return true; // eat it.
}


struct PnWidget *pnMenu_addItem(struct PnWidget *button,
    const char *label) {

    DASSERT(button);

    struct PnMenu *m = pnWidget_getUserData(button);
    struct PnWidget *popup = PopupGet(m);
    DASSERT(popup);
    struct PnWidget *w = pnMenu_create(popup,
            4/*width*/, 2/*height*/,
            PnLayout_LR, PnAlign_CC, PnExpand_H, label);
    ASSERT(w);
    struct PnMenu *newMenu = pnWidget_getUserData(w);
    newMenu->parent = m;

    return w;
}


struct PnWidget *pnMenu_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand, const char *label) {

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
