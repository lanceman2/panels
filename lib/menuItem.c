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


struct PnMenuItem {

    struct PnWidget *button;

    // Optional
    struct PnWidget *popup;
};


static
void destroy(struct PnWidget *b, struct PnMenuItem *m) {
    DASSERT(b);
    DASSERT(m);
    DASSERT(b == m->button);

    DZMEM(m, sizeof(*m));
    free(m);
}


struct PnWidget *pnMenuItem_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand) {

    struct PnMenuItem *m = calloc(1, sizeof(*m));
    ASSERT(m, "calloc(1,%zu) failed", sizeof(*m));

    m->button = pnButton_create(parent, width, height,
            layout, align, expand, 0/*label*/, false/*toggle*/);
    if(!m->button) {
        DZMEM(m, sizeof(*m));
        free(m);
        return 0; // Failure.
    }

    DASSERT(m->button->type == PnWidgetType_button);
    DASSERT(m->button->type & WIDGET);
    m->button->type = PnWidgetType_menuitem;

    pnWidget_addDestroy(m->button, (void *) destroy, m);
    pnWidget_setBackgroundColor(m->button, 0xFFCDCDCD);
    // The user gets the button, but it's augmented.
    return m->button;
}
