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


struct PnWidget *pnMenuBar_create(struct PnWidget *parent,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand) {

    struct PnMenuBar *m = (void *) pnWidget_create(parent,
            width, height,
            layout, align, expand, sizeof(*m));
    if(!m)
        return 0; // Failure.

    DASSERT(m->widget.type == PnWidgetType_widget);
    m->widget.type = PnWidgetType_menubar;

    pnWidget_setBackgroundColor(&m->widget, 0xFFCDCDCD);

    return &m->widget;
}
