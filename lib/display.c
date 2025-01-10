#define _GNU_SOURCE
#include <link.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../include/panels.h"

#include  "display.h"
#include "debug.h"


// struct PnDisplay encapsulate the wayland per process singleton objects,
// mostly.
//
// There should be no performance hit for adding a structure name space to
// a bunch of global variables, given the struct names get compiled.  Just
// think of this struct as a bunch of global declarations.  struct
// PnDisplay is mostly pointers to singleton wayland structures (objects)
// and lists of libpanel objects that can't continue to exist if
// libpanel.so is unloaded.
//
struct PnDisplay pnDisplay = {0};


// We only get zero or one display per process.
//
// This is a little weird because we follow the wayland client that makes
// many singleton objects for a given process, which is not our preferred
// design.  We don't make these objects if they never get used (i.e. if
// this never gets called).  So, this is not called in the libpanels.so
// constructor.  We are assuming that we will not need an if statement
// body in a "tight loop" (resource hog) to call this, if so, then
// aw-shit.  We'd guess that windows do not get created in a tight loop.
// Note: this idea of limiting the interfaces to the libpanels.so API is
// very counter to the design of the libwayland-client.so API.  That's the
// point of libpanels.so.
//
// Return 0 on success and not zero on error.  TODO error return codes?
//
int pnDisplay_create(void) {

    // So many things in this function can fail.


    pnDisplay.exists = true;
    return 0; // return 0 => success.
}


// This will cleanup all things from libpanels.  We wish to make this
// libpanels library robust under sloppy user cases, by calling this in
// the library destructor if pnDisplay_create() was ever called.
void pnDisplay_destroy(void) {

    pnDisplay.exists = false;
}


bool npDisplay_run(void) {

    return 0;
}

