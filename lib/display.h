
// It turns out that the Wayland client has per process global data that
// we are forced, by the wayland design, to have in this code.  This is
// not what we prefer but we did not want to make our own windowing
// standard.
//
// The wayland C client interface exposes an order of magnitude more
// interfaces than libpanels.  I just want to make a thing that lets me
// quickly color pixels in a window (and sub-windows) without writing ten
// thousand lines of compiled source code.  Just a stupid little wrapper
// library.
//
// Consolidate what would be many globals into one structure so as make
// finding and understanding all these singleton objects easier.  We
// appear to be trying to make what was in libX11 called a "display"
// object; but in libX11 the display was not necessarily a singleton
// object: you could make as many X11 client display connections in a
// given process as you wanted (or so it appeared given the interface).
// This wayland limitation seems short sighted to me.  On the brighter
// side, wayland is much less complex than X11, and could in many cases
// have better performance (depends on a lot of shit, I suppose).
//
// X11 is a huge beast of complexity with a six pack abs.  Wayland is
// simple, but its guts are hanging out.
//
// RANT: Why can't the wayland client C API just hide all these global
// singleton objects, and make them as they become necessary in the
// process.  All of these potential singleton objects are either existing
// or not; and libwayland-client (the wayland client library) should be
// able to keep track the things that are needed, like the X11 client
// display connection object did.  I can't see how there is a big
// performance gain in not doing this.  It's just a list of dependences
// that could be automatically generated.  How can a user use a wl_surface
// without any fucking pixels?  Yes, yes, layers give you control, but
// come on, layering and exposing 5 objects down to get to coloring a
// fucking pixel, WTF.
//
// Because of libwayland-client, we can only have one Pn Display.
//
struct PnDisplay {

    bool exists; // if this struct is ever set up.

};

extern struct PnDisplay pnDisplay;
extern int pnDisplay_create(void);
extern void pnDisplay_destroy(void);
