#define _GNU_SOURCE
#include <link.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <cairo.h>
#include <fontconfig/fontconfig.h>

#include "../include/panels.h"

#include  "display.h"
#include "debug.h"


static void __attribute__((constructor)) constructor(void) {

    // TODO: forcing us to need these two libraries at compile time; at
    // least for DEBUG builds.
    DSPEW("FcPatternGetString=%p", FcPatternGetString);
    DSPEW("cairo_create=%p", cairo_create);

    DSPEW();
}


static void __attribute__((destructor)) destructor(void) {

    if(pnDisplay.exists)
        pnDisplay_destroy();

    DSPEW("libpanels.so done");
}
