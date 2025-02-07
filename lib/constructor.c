#define _GNU_SOURCE
#include <link.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <wayland-client.h>

#ifdef WITH_CAIRO
#  include <cairo.h>
#endif
#ifdef WITH_FONTCONFIG
#  include <fontconfig/fontconfig.h>
#endif

#include "../include/panels.h"

#include "debug.h"
#include  "display.h"


static void __attribute__((constructor)) constructor(void) {

#ifdef WITH_FONTCONFIG
    // TODO: forcing us to need these two libraries at compile time; at
    // least for DEBUG builds.  At least until other code that uses
    // fontconfig and cairo is written.
    fprintf(stderr, "FcPatternGetString=%p\n", FcPatternGetString);
#endif
#ifdef WITH_CAIRO
    fprintf(stderr, "cairo_create=%p\n", cairo_create);
#endif

    DSPEW();
}


static void __attribute__((destructor)) destructor(void) {

    if(d.wl_display)
        pnDisplay_destroy();

    DSPEW("libpanels.so done");
}
