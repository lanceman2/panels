#define _GNU_SOURCE
#include <link.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <wayland-client.h>

#ifdef WITH_CAIRO
#  include <cairo.h>
#endif

#include "../include/panels.h"

#include "debug.h"
#include  "display.h"


static void __attribute__((constructor)) constructor(void) {

    DSPEW();
}


static void __attribute__((destructor)) destructor(void) {

    if(d.wl_display)
        pnDisplay_destroy();

#ifdef WITH_CAIRO
    // TODO: There the question: is anyone still using cairo?
    cairo_debug_reset_static_data();
#endif

    DSPEW("libpanels.so done");
}
