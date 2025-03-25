
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <dlfcn.h>
#include <cairo/cairo.h>
#include <fontconfig/fontconfig.h>

#include "../lib/debug.h"

static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

void LoadAndRun(void) {

    void *dlhandle = dlopen(DSO_FILE, RTLD_NOW);
    ASSERT(dlhandle);
    int (*runLabels)(void) = dlsym(dlhandle, "runLabels");
    DASSERT(runLabels);
    ASSERT(runLabels() == 0);
    ASSERT(0 == dlclose(dlhandle));
}

int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    for(int i=0; i<3; ++i) {
        LoadAndRun();
        cairo_debug_reset_static_data();
        FcFini();
        cairo_debug_reset_static_data();
        FcFini();
        cairo_debug_reset_static_data();
        FcFini();
      }

    return 0;
}
