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

    struct PnWidget *parentPopup; // For popup menus within menus.

    // Perhaps it's called composition, but the API user will not know or
    // see the difference, so I think it's fair to call it inheritance.
    struct PnWidget *button; // inherit (opaquely).

    struct PnWidget *popup;
};


// We only allow one menu (and it's popups) to be active at a time; per
// process.
static struct PnMenu *menu = 0; // active menu.


bool PopupEnter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnMenu *m) {
    DASSERT(w);
    DASSERT(m);
    DASSERT(m->popup);
    DASSERT(menu == m);

WARN();
    return false;
}

static inline
void PopupCreate(struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m->button);
    DASSERT(!m->popup);
    struct PnWidget *popup = pnWindow_create(m->button/*parent*/,
            4, 4, 0/*x*/, 0/*y*/, PnLayout_TB, 0, 0);
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
    if(!m->popup) return;

    DASSERT(m == menu);

    pnPopup_hide(m->popup);
    menu = 0;
}

static inline void PopupShow(struct PnMenu *m) {
    DASSERT(m);
    DASSERT(m->button);
    if(!m->popup)
        // If there was no menu items created than there is no
        // popup created.
        return;

    if(menu && menu != m)
        PopupHide(menu);

    menu = m;

    int32_t x, y;
    struct PnAllocation a;

    if(!m->parentPopup) {
        pnWidget_getAllocation(m->button, &a);
        x = a.x;
        y = a.y + a.height;
    } else {
        pnWidget_getAllocation(m->parentPopup, &a);
        x = a.x + 100;
        y = a.y + 100;
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

    //PopupHide(m);
fprintf(stderr, "    leave widget=%p \n", b);

    return true; // eat it.
}


struct PnWidget *pnMenu_addItem(struct PnWidget *menu,
    const char *label) {

    DASSERT(menu); // pointer to the button

    struct PnMenu *m = pnWidget_getUserData(menu);
    struct PnWidget *popup = PopupGet(m);
    DASSERT(popup);
    struct PnWidget *w = pnMenu_create(popup,
            4/*width*/, 2/*height*/,
            PnLayout_LR, PnAlign_CC, PnExpand_H, label);
    ASSERT(w);
    // This makes a menu but with a parent menu.
    // Reuse variable m.
    m = pnWidget_getUserData(w);
    m->parentPopup = popup;

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
