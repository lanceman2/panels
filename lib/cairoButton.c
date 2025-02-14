// This file is linked into libpanels.so if WITH_CAIRO is defined
// in ../config.make, otherwise it's not.
//
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cairo/cairo.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"
#include "cairoWidget.h"


struct PnWidget *pnButton_create(struct PnSurface *parent,
        const char *label) {

    struct PnButton *b = (void *) pnWidget_create(parent,
              200/*width*/, 100/*height*/,
            0/*direction*/, 0/*align*/,
            PnExpand_HV/*expand*/, sizeof(*b));
    ASSERT(b);

    return (void *) b;
}
