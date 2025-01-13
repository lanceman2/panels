
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
// fucking pixel.
//
// Constrained by libwayland-client, we can only have one Pn Display
// object in a process.


// If we add surface types we'll need to check all the code.
//
// TODO: Add a wayland subsurface to this type thingy?
//
enum PnSurfaceType {

    // 2 Window Surface types:
    PnSurfaceType_toplevel = 1, // From xdg_surface_get_toplevel()
    PnSurfaceType_popup,    // From wl_shell_surface_set_popup()

    // 1 Widget Surface type
    PnSurfaceType_widget    // a piece of a surface
};


struct PnAllocation {

    uint32_t x, y, width, height;
};


// A widget or window has a surface.
//
struct PnSurface {

    enum PnSurfaceType type;

    // API user requested size.  What they get may be different.
    // These are constant after they are first set.
    const uint32_t width, height;

    struct PnAllocation allocation;

    // API user passed in draw function:
    int (*draw)(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride,
            void *user_data);

    // To pass to draw():
    void *user_data;

    // We keep a linked list (tree like) graph of surfaces starting at a
    // window with parent == 0.  The top level parent windows are owned by
    // a PnDisplay (display).
    struct PnSurface *parent;
    struct PnSurface *firstChild, *lastChild;
    struct PnSurface *nextSibling, *prevSibling;
};


// type PnSurfaceType_widget
struct PnWidget {

    // 1st inherit surface
    struct PnSurface surface;
};

struct PnBuffer {

    struct wl_buffer   *wl_buffer;
    struct wl_shm_pool *wl_shm_pool;
    size_t size; // total bytes of mapped shared memory file

    int fd; // File descriptor to shared memory file

 
    bool busy; // the wayland compositor may be reading the buffer
};


// type can be a toplevel or a popup
struct PnWindow {

    // 1st inherit surface
    struct PnSurface surface;

    // To keep:
    //    1. a list of toplevel windows in the display, or
    //    2. a list of children popup windows in toplevel windows.
    struct PnWindow *next, *prev;

    size_t sharedBufferSize;

    // Wayland client objects:
    //
    struct wl_surface  *wl_surface;
    struct xdg_surface *xdg_surface;
    //
    union {
        // for surface type toplevel
        struct {
            struct xdg_toplevel *xdg_toplevel;
            // list of child popups
            struct PnWindow *popups; // points to newest one
        } toplevel;
        // for surface type popup
        struct {
            struct xdg_positioner *xdg_positioner;
            struct xdg_popup *xdg_popup;
            //struct PnWindow *parent; // toplevel parent
        } popup;
    };
    //
    struct zxdg_toplevel_decoration_v1 *decoration;
    struct wl_callback *wl_callback;
    struct PnBuffer buffer[2];

};


// Making wayland client objects into one big ass display thing, like
// a X11 display.
//
struct PnDisplay {

    // 9 singleton wayland client objects:
    //
    // We have not confirmed that there can only be one of the following
    // objects (some we have), but there may be no point in making more
    // than one of these per process.  They are somewhat in order of
    // creation.
    //
    struct wl_display *wl_display;                              // 1
    struct wl_registry *wl_registry;                            // 2
    struct wl_shm *wl_shm;                                      // 3
    struct wl_compositor *wl_compositor;                        // 4
    struct xdg_wm_base *xdg_wm_base;                            // 5
    struct wl_seat *wl_seat;                                    // 6
    struct wl_pointer *wl_pointer;                              // 7
    struct wl_keyboard *wl_keyboard;                            // 8
    struct zxdg_decoration_manager_v1 *zxdg_decoration_manager; // 9

    uint32_t handle_global_error;

    // TODO: There may be a more standard way to do this.
    //
    // We find this function pointer at runtime, it could be at least one
    // of two different functions.
    void (*surface_damage_func)(struct wl_surface *wl_surface,
            int32_t x, int32_t y, int32_t width, int32_t height);

    // GTK window border decoration shit:
    //
    // We're avoiding this for now.  libdecor.so leaks lots of system
    // resources, but it dynamically loads it's leaky libraries (like
    // libglib.so et al.) only if you initialize it.
    //struct libdecor *libdecor = 0;

    // Does the mouse pointer focus and the keyboard focus always follow
    // each other?  I'm guessing that, not always.  Why else do the two
    // events exist separately?
    //
    // Set the window with mouse pointer focus (from enter and leave):
    struct SlWindow *pointerWindow;
    // Set the window with keyboard focus (from enter and leave):
    struct SlWindow *kbWindow;

    // List of windows.
    struct PnWindow *windows; // points to newest window made.

    // TODO: widget (sub-window) focus?
};


// One big ass global display thingy that has 9 wayland display thingys in
// it:
extern struct PnDisplay d;

extern int pnDisplay_create(void);
extern void pnDisplay_destroy(void);

// Returns false on success
static inline bool CheckDisplay(void) {
    if(!d.wl_display) pnDisplay_create();
    // At this point this may be called from a window create function
    // so we have to assume that we have a working display, if not
    // this returns true.
    return (bool) !d.wl_display;
}


extern void GetSurfaceDamageFunction(struct PnWindow *win);
