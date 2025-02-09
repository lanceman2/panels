

// It looks like most GUI (graphical user interface) based programs
// (wayland compositor included), as we shrink a window, cull out the
// parts of the window to the right and bottom, keeping the left and top
// most part of the window.  This is just convention, and "panels" follows
// this convention.  Hence, we cull out widgets to the right and bottom
// as windows shrink.  Widgets that cannot get there requested size due to
// this window shrink culling will not be drawn, and the background
// drawing of the window will be shown to the GUI user where widgets are
// culled out and there is not large enough room in the lower right
// corner of the window.

// It turns out that the Wayland client has per process global data that
// we are forced, by the wayland design, to have in this code.  This is
// not a limitation that we prefer but we did not want to make our own
// windowing standard.  We had to start somewhere.
//
// I just want to make a thing that lets me quickly color pixels in a
// window (and sub-windows) without writing ten thousand lines of compiled
// source code.  Just a stupid little wrapper library.  "panels" is an
// order of magnitude smaller than GTK and Qt (leaky bloat monsters).


// Consolidate what would be many globals into one structure so as make
// finding and understanding all these singleton objects easier.  We
// appear to be trying to make what was in libX11 was called a "display"
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

// Constrained by libwayland-client, we can only have one Pn Display
// object in a process.


// If we add more surface types we'll need to check all the code.
//
// TODO: Add a wayland subsurface to this type thingy?
//
enum PnSurfaceType {

    // Yes, a popup window is in a sense a toplevel window; but we call it
    // a popup, and not a toplevel, and so does wayland-client.

    // 2 Window Surface types:
    PnSurfaceType_toplevel = 0x156c271, // From xdg_surface_get_toplevel()
    PnSurfaceType_popup,    // From wl_shell_surface_set_popup()

    // 1 Widget Surface type.  Not a wayland thing.
    PnSurfaceType_widget    // a rectangular piece of a surface
};


struct PnAllocation {

    // These x, y values are measured relative to the window, NOT parent
    // widgets (like in GTK).
    uint32_t x, y, width, height;
};


// A widget or window (toplevel window or popup window) has a surface.
//
struct PnSurface {

    enum PnSurfaceType type;

    // A link in the draw queue.
    struct PnSurface *dqNext, *dqPrev;

    // API user requested size.  What they get may be different.
    //
    // width and height are CONSTANT after they are first set!!!
    //
    // For container windows and widgets width is left and right border
    // width, and height is top and bottom border thickness.  A container
    // with no children will have width and height.  In this sense the
    // container owns the pixels of the border, so it draws them before
    // the children are drawn.
    const uint32_t width, height;

    // What they really get for surface size:
    struct PnAllocation allocation;

    // API user passed in draw function:
    int (*draw)(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 byte chunks*/,
            void *userData);
    // To pass to draw():
    void *drawData;

    // user set event callbacks
    //
    // TODO: Is this too many pointers; should we allocate memory for it
    // so that it this memory is not used unless the widget sets these
    // callbacks?
    //
    // The enter callback is special, as it sets the surface (widget or
    // window) to "focus", which we define as to receive the other
    // events.  The "focused" widget can press up the "focused" events
    // to it's parent by returning false.
    //
    bool (*enter)(struct PnSurface *surface,
            uint32_t x, uint32_t y, void *userData);
    void *enterData;
    void (*leave)(struct PnSurface *surface, void *userData);
    void *leaveData;
    bool (*press)(struct PnSurface *surface,
            uint32_t which, uint32_t x, uint32_t y,
            void *userData);
    void *pressData;
    bool (*release)(struct PnSurface *surface,
            uint32_t which, uint32_t x, uint32_t y,
            void *userData);
    void *releaseData;
    bool (*motion)(struct PnSurface *surface,
            uint32_t x, uint32_t y,
            void *userData);
    void *motionData;


    // We keep a linked list (tree like) graph of surfaces starting at a
    // window with PnSurface::parent == 0.
    //
    // The toplevel parent windows are owned by and listed in PnDisplay
    // (display).
    //
    // Popup windows also have PnSurface::parent == 0 here and popup
    // windows are owned by their parent toplevel window, but listed in
    // the with PnWindow::prev,next at PnWindow::toplevel.popups.
    //
    // So, this parent=0 for toplevel windows and popup windows.
    //
    struct PnSurface *parent;
    struct PnSurface *firstChild, *lastChild;
    struct PnSurface *nextSibling, *prevSibling;

    struct PnWindow *window; // The top most surface is a this window.

    uint32_t backgroundColor;

    enum PnDirection direction;
    enum PnAlign align;
    const enum PnExpand expand;

    // "canExpand" is used to mark container widgets as expandable, due to
    // leaf widgets being expandable (and not directly the container being
    // expandable), while we calculate all the widget positions and sizes.
    // A container widget can be expandable because its children widgets
    // are expandable; even if it does not mark it's own expand flag.  How
    // else could the container widgets hold an arbitrary number of child
    // widgets?
    //
    enum PnExpand canExpand;

    // Windows are NOT showing after pnWindow_create().
    //
    // Widgets are showing after pnWidget_create().  The user sets this
    // with PnWidget_show().
    bool hidden;

    // "culled" is set to true if the top window surface is not large
    // enough (and hence the container is not large enough) to show this
    // surface.  It is not showing because we don't have the space for it,
    // not because the user deliberately chooses not to show it.
    //
    // top window surfaces do not use this variable.
    //
    bool culled;

    // If a container widget has 0 width borders and no size open between
    // children widgets we set this flag, so that we do not call a widget
    // draw function for a container widget that has none of its own area
    // showing, though it has children widgets showing and their draw
    // functions get called.
    //
    // The "culled" acts before this, "noDrawing" flag.
    //
    bool noDrawing; // true if zero size showing.

    // Is in the window draw queue.
    bool isQueued;
};


// surface type PnSurfaceType_widget
//
// Widgets do not have a wayland surface (wl_surface) or other related
// wayland client objects in them.  Widgets use the parent window (parent
// most) objects wayland client stuff to draw to.
struct PnWidget {

    // 1st inherit surface
    struct PnSurface surface;

    // The window this widget is part of.
    struct PnWindow *window;

    void (*destroy)(struct PnWidget *widget, void *userData);
    void *destroyData;
};

struct PnBuffer {

    struct wl_buffer   *wl_buffer;
    struct wl_shm_pool *wl_shm_pool;

    size_t size; // total bytes of mapped shared memory file

    // stride is the distance in uint32_t (4 byte) chunks.
    uint32_t width, height, stride;

    // pixels is a pointer to the share memory pixel data from mmap(2).
    uint32_t *pixels;

    int fd; // File descriptor to shared memory file
};

struct PnDrawQueue {
    struct PnSurface *first, *last;
};

// surface type can be a toplevel or a popup
struct PnWindow {

    // 1st inherit surface
    struct PnSurface surface;

    // We use two draw queues, one we read from (and dequeue) and one we
    // write draw requests to from the API user.  This code dequeues one
    // of the queues at the same time as the API user writes to the other.
    // We switch the two queues each time we draw via this draw queue
    // method.
    struct PnDrawQueue *dqRead, *dqWrite;
    struct PnDrawQueue drawQueues[2];

    // To keep:
    //    1. a list of toplevel windows in the display, or
    //    2. a list of popup windows in toplevel windows.
    struct PnWindow *next, *prev;

    // Wayland client objects:
    //
    struct wl_surface  *wl_surface;
    struct xdg_surface *xdg_surface;
    //
    union {
        // for surface window type toplevel
        struct {
            struct xdg_toplevel *xdg_toplevel;
            // list of child popups
            struct PnWindow *popups; // points to newest one
        } toplevel;
        // for surface window type popup
        struct {
            struct xdg_positioner *xdg_positioner;
            struct xdg_popup *xdg_popup;
            struct PnWindow *parent;
        } popup;
    };
    //
    struct zxdg_toplevel_decoration_v1 *decoration;
    struct wl_callback *wl_callback;
    struct PnBuffer buffer;


    void (*destroy)(struct PnWindow *window, void *userData);
    void *destroyData;

    // "needAllocate" is a flag to said that we need to recompute all
    // widget allocations, that is widget (and window) sizes and
    // positions.
    bool needAllocate;

    // It was (on KDE plasma 5.27.5 Wayland, 2025 Jan 30) found that the
    // xdg_surface_listener::configure gets called a lot, even when there
    // are no pixels to change.  Since we can't just change pixel shared
    // memory any time; we set this "needDraw" flag when we know there
    // is a need to change at least one pixel's color.  And, obviously we
    // set this "needDraw" if there is a window resize, because that makes
    // all the pixels in the window in need of recoloring.
    //
    bool needDraw;
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
    // events exist separately?  Maybe tab key press to change keyboard
    // focus needs them to be separate.
    //
    // Set the window with mouse pointer focus (from enter and leave):
    struct PnWindow *pointerWindow;
    // The child most surface that has the pointer in it.
    struct PnSurface *pointerSurface;
    // The child most surface that has the pointer in it and
    // has taken "panels focus" by returning true from it's enter
    // callback.
    struct PnSurface *focusSurface;
    // In panels all x,y coordinates are given relative to the window's
    // surface (Not like GTK's widget relative coordinates).
    //
    // We set this x,y with wayland mouse pointer enter and mouse pointer
    // motion events.
    uint32_t x, y; // pointer position.

    struct PnSurface *buttonGrabSurface;
    uint32_t buttonGrab;

    // Set the window with keyboard focus (from enter and leave):
    struct PnWindow *kbWindow;
    struct PnSurface *kbSurface;

    // List of windows.
    struct PnWindow *windows; // points to newest window made.
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

extern int create_shm_file(size_t size);
extern struct PnBuffer *GetNextBuffer(struct PnWindow *win,
        uint32_t width, uint32_t height);
extern void FreeBuffer(struct PnBuffer *buffer);

extern bool InitToplevel(struct PnWindow *win);
extern bool InitPopup(struct PnWindow *win,
        int32_t w, int32_t h,
        int32_t x, int32_t y);

extern bool InitSurface(struct PnSurface *s);
extern void DestroySurface(struct PnSurface *s);

extern void GetWidgetAllocations(struct PnWindow *win);

extern void pnSurface_draw(struct PnSurface *s,
        struct PnBuffer *buffer);

extern void FlushDrawQueue(struct PnWindow *win);
extern bool _pnWindow_addCallback(struct PnWindow *win);
extern void PostDraw(struct PnWindow *win, struct PnBuffer *buffer);
extern bool DrawFromQueue(struct PnWindow *win);

extern void DrawAll(struct PnWindow *win, struct PnBuffer *buffer);

extern void GetSurfaceWithXY(const struct PnWindow *win,
        wl_fixed_t x,  wl_fixed_t y);

extern void DoEnterAndLeave(void);

static inline void RemoveSurfaceFromDisplay(struct PnSurface *s) {

    if(d.buttonGrabSurface == s)
        d.buttonGrabSurface = 0;

    if(d.focusSurface == s)
        d.focusSurface = 0;

    if(d.pointerSurface == s)
        d.pointerSurface = 0;
}

static inline void ResetDisplaySurfaces(void) {

    // TODO: Do we really want to keep these marked surfaces?  Like after
    // a call to GetWidgetAllocations().

    if(d.buttonGrabSurface && d.buttonGrabSurface->culled)
        d.buttonGrabSurface = 0;

    if(d.focusSurface && d.focusSurface->culled)
        d.focusSurface = 0;

    if(d.pointerSurface && d.pointerSurface->culled)
        d.pointerSurface = 0;
}
