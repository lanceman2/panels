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

    struct PnWidget *button;

    // More data here..
};


static
void destroy(struct PnWidget *w, struct PnMenu *m) {

    DASSERT(w);
    DASSERT(w == m->button);

    DZMEM(m, sizeof(*m));
    free(m);
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
            layout, align, expand, label, false/*toggle*/,
            0/*size*/);
    if(!m->button) {
        DZMEM(m, sizeof(*m));
        free(m);
        return 0; // Failure.
    }

    DASSERT(m->button->type == PnSurfaceType_button);
    DASSERT(m->button->type & WIDGET);
    m->button->type = PnSurfaceType_menu;

    pnWidget_addDestroy(m->button, (void *) destroy, m);
    pnWidget_setBackgroundColor(m->button, 0xFFCDCDCD);

    return m->button;
}
