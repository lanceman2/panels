#define _GNU_SOURCE
#include <link.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <wayland-client.h>
#include <cairo.h>
#include <fontconfig/fontconfig.h>

#include "../include/panels.h"

#include "debug.h"
#include  "display.h"


static void __attribute__((constructor)) constructor(void) {

    // TODO: forcing us to need these two libraries at compile time; at
    // least for DEBUG builds.
    DSPEW("FcPatternGetString=%p", FcPatternGetString);
    DSPEW("cairo_create=%p", cairo_create);

    DSPEW();
}


static void __attribute__((destructor)) destructor(void) {

    if(d.wl_display)
        pnDisplay_destroy();

    DSPEW("libpanels.so done");
}
