
#ifdef WITH_CAIRO
#include <cairo/cairo.h>
#endif

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
// fucking pixel seems silly.

// Constrained by libwayland-client, we can only have one Pn Display
// object in a process.  Not our fucking choose.  We're lucky that
// it's not leaky (memory) like most libraries.

#define TOPLEVEL         (01) // first bit set
#define POPUP            (02) // second bit set
#define WIDGET           (04) // third bit set
#define GET_WIDGET_TYPE(x)  (~(TOPLEVEL|POPUP|WIDGET) & (x))
// WIDGET TYPES: are a number in the (skip first 3 bits) higher bits.
#define W_LABEL          (1 << 3)
#define W_BUTTON         (2 << 3)
#define W_TOGGLE_BUTTON  (3 << 3)
#define W_MENU           (4 << 3)
#define W_MENUITEM       (5 << 3)


// If we add more surface types we'll need to check all the code.
//
// TODO: Add a wayland subsurface to this type thingy?
//
enum PnSurfaceType {

    // Yes, a popup window is in a sense a toplevel window; but we call it
    // a popup, and not a toplevel, and so does wayland-client.

    // 2 Window Surface types:
    PnSurfaceType_toplevel = TOPLEVEL, // From xdg_surface_get_toplevel()
    PnSurfaceType_popup = POPUP,    // From wl_shell_surface_set_popup()

    // Widget Surface types:  Not a wayland client thing.
    PnSurfaceType_widget     = WIDGET, // a rectangular piece of a surface
    PnSurfaceType_label      = (WIDGET | W_LABEL), // a label widget
    PnSurfaceType_button     = (WIDGET | W_BUTTON), // a button widget
    PnSurfaceType_menu       = (WIDGET | W_MENU), // ...
    PnSurfaceType_menuitem   = (WIDGET | W_MENUITEM)
};


struct PnWidget;


struct PnGrid {

    // child[y][x] points to a widget.
    struct PnWidget ***child;

    // Positions of the grid lines starting at the top left of the grid
    // surface (widget).  For any m and n, x[m], y[n] is to the upper left
    // corner of a cell where a contained widget can be positioned if it
    // is aligned top left.  We must remember that there can be a border
    // between cells and widgets in the cells may not necessarily fill the
    // whole cell space.
    //
    // We can quickly find the cell n and m at a given pixel position x
    // and y using 2D bisection O(2*log2N), and then test if we are in a
    // widget that is in the cell.
    //
    uint32_t *x, *y; // array sizes: x[numColumns+1], y[numRows+1]

    // At widget size allocation time we record the size and spacing of
    // all the grid-lines in these two allocated arrays:
    // widths[numColumns] and heights[numRows].  These are only needed in
    // the "bootstrapping" in getting the widget packing (widget size and
    // position allocations).  These could have been allocated and freed
    // each time the widgets are packed, but are not because it's not much
    // memory (or is it?).
    //
    // These are widths and heights without cell borders of the grid.
    // Just the room available for a child widget.
    //
    // TODO: Are these redundant given x and y above?  Maybe not given
    // that we can mark unused rows and columns with them.
    //
    // widths[m] == 0 means that there is no showing cells for column m.
    // heights[n] == 0 means that there is no showing cells for row n.
    //
    uint32_t *widths, *heights;

    uint32_t numChildren;
};


// This makes a stack list of widget destroy functions.  Destroy()
// functions are called in the reverse order that they are added.
//
// TODO: This destroy functions linked list seem a little wasteful.  We
// are allocating more memory just to add a destroy() function, in order
// to simplify the interface to add destroy functions.  Another way to do
// this would be to have the PnWidget inheriting struct add this
// PnWidgetDestroy thing to it.  We need these functions in a list so
// users can make widgets based on other widgets, that are based on yet
// other widgets; and so on to N widgets deep.  Without this (or something
// like this) the end widget user can't in this "simple way" inherit and
// expose the PnWidget interfaces to it's end programming user.  They'd
// have to get and store the previous destroy function from the last
// widget object it is inheriting (which may be an okay idea).
//
struct PnWidgetDestroy {

    void (*destroy)(struct PnWidget *widget, void *userData);
    void *destroyData;
    struct PnWidgetDestroy *next; // next in the list.
};


struct PnCallback {
    // The callbacks are called in order until one of them returns
    // true.  The widget object knows that the actual function prototype
    // is.  We are marshaling (I think that's the correct term) the action
    // callbacks.
    //
    // The particular widget's API user set callback.
    void *callback;
    void *callbackData;
    struct PnCallback *prev, *next;
};

struct PnAction {

    // The action function called to indirectly call the particular
    // widget's API user set callback(s).
    bool (*action)(struct PnWidget *widget, void *callback,
            void *userData, void *actionData);

    void *actionData;

    // Ordered list of callbacks:
    struct PnCallback *first, *last;
};

// surface type PnSurfaceType_widget
//
// Widgets do not have a Wayland surface (wl_surface) or other related
// Wayland client objects in them.  Widgets use the parent window (parent
// most) objects Wayland client stuff to draw to.
struct PnWidget {

    enum PnSurfaceType type;

    // A link in the draw queue.
    struct PnWidget *dqNext, *dqPrev;

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

    // What they really get for surface size.  Changes when the parent
    // window is resized.
    struct PnAllocation allocation;

    // user set event callbacks
    //
    // TODO: Is this too many pointers; should we allocate memory for it
    // so that it this memory is not used unless the widget sets these
    // callbacks?
    //
    // API user passed in draw function:
    int (*draw)(struct PnWidget *widget, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 byte chunks*/,
            void *userData);
    // To pass to draw():
    void *drawData;
    //
    void (*config)(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, uint32_t stride/*4 byte chunks*/,
            void *userData);
    void *configData;
    //
    // The enter callback is special, as it sets the widget to "focus",
    // which we define as to receive the other events.  The "focused"
    // widget can press up the "focused" events to it's parent by
    // returning false.
    //
    bool (*enter)(struct PnWidget *w,
            uint32_t x, uint32_t y, void *userData);
    void *enterData;
    void (*leave)(struct PnWidget *w, void *userData);
    void *leaveData;
    bool (*press)(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            void *userData);
    void *pressData;
    bool (*release)(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            void *userData);
    void *releaseData;
    bool (*motion)(struct PnWidget *w,
            int32_t x, int32_t y,
            void *userData);
    void *motionData;

#ifdef WITH_CAIRO
    int (*cairoDraw)(struct PnWidget *w,
            cairo_t *cr, void *userData);
    void *cairoDrawData;
    cairo_t *cr;
    cairo_surface_t *cairo_surface;
#endif

    // We keep a linked list (tree like) graph of widget starting at a
    // window with PnWidget::parent == 0.
    //
    // The toplevel parent windows are owned by and listed in PnDisplay
    // (display).
    //
    // Popup windows also have PnWidget::parent == 0 here and popup
    // windows are owned by their parent toplevel window, but listed in
    // the with PnWindow::prev,next at PnWindow::toplevel.popups.
    //
    // So, this parent=0 for toplevel windows and popup windows.
    //
    struct PnWidget *parent;

    union {
        // For containers to see their children.
        ////////////////////////////////////////////////////////
        struct {
            struct PnWidget *firstChild, *lastChild;
        } l; // widget is a container list (l)

        struct {
            // We make this "g" the same size as PnWidget::l by
            // allocating grid.  Otherwise it would be much larger than
            // PnWidget::l above.
            struct PnGrid *grid;
            uint32_t numColumns, numRows;
        } g; // widget is a container grid (g)
    };

    union {
        // For children to go through the lists they are in.
        ////////////////////////////////////////////////////////
        struct {
            struct PnWidget *nextSibling, *prevSibling;
        } pl; // my parent container is a list (pl)
        struct {
            // row y -> child[y]  child[y][x]
            uint32_t row, column, cSpan, rSpan;
        } pg; // my parent container is a grid (pg)
    };

    struct PnWindow *window; // The top most widget is a this window.

    struct PnWidgetDestroy *destroys;

    // An allocated array of marshaller functions that keep user set
    // callback functions for panel widgets.
    //
    // These functions call API user callbacks.  See
    // pnWidget_addCallback().  The object class that inherits PnWidget
    // must define indexes into this array.  Like for example
    // PN_BUTTON_CB_ACTION being 0, the action callback of a button
    // click.
    //
    // Fuck that name string lookup stuff (GTK), or that signal/slot new
    // language (Qt MOC) stuff.  When setup, (I think) this is much faster
    // than either.  Each action is referred to by an index using a CPP
    // MACROs like PN_BUTTON_CB_CLICK.  The correct indexes are
    // solidified by constructing the base objects in order on the way up
    // to the last widget inheritance level.  We assume that each given
    // widget type has a predetermined and fixed value and index order of
    // actions just after they are created.  We do not need a fucking data
    // base or other complex data structure.  An array does the job well.
    // Users that make widget objects just need to find their starting
    // index from their base widget object's last created index.  They do
    // it just once when the code is written.  Any time a base widget adds
    // or removes an action all the widget indexes need to adjust their
    // fixed values (a compile time change).  It appears that the setup of
    // signals/slots is a compile time thing in Qt, so I think it's very
    // hard to justify all that added complexity.  In GTK it looks like it
    // could be done at run-time, but I never see it used that way.
    //
    // So: the particular indexes (example PN_BUTTON_CB_CLICK) into this
    // array are set at compile time.
    //
    // Qt says: "callbacks can be unintuitive and may suffer from problems
    // in ensuring the type-correctness of callback arguments."  We
    // require that widget makers and users don't fuck up their function
    // pointer prototypes.  If custom widget function prototypes change
    // with panel callbacks you're screwed, i.e. you must be careful. The
    // compiler will check the function types for you.  If the ABI
    // (applcation binary interface) changes, ya no fucking shit, it will
    // crash.  GTK uses CPP MACRO madness to check types.  I find that GTK
    // CPP macro gobject code is impossible to follow, code scoping CPP
    // macros is a pain.  Keep the power of C and: don't read a double as
    // an int; unless you know you are decoding it.
    //
    // This is the shit!  Simple and fast.
    //
    // Allocated actions[] array.  Realloc(3).
    struct PnAction *actions;
    uint32_t numActions;

#ifdef DEBUG
    size_t size;
#endif

    // 32 color bits in the byte order Alpha Red Green Blue:
    uint32_t backgroundColor;

    enum PnLayout layout;
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
    // widget surface.  It is not showing because we don't have the space
    // for it, not because the user deliberately chooses not to show it.
    //
    // Top window widgets do not use this variable.
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
    struct PnWidget *first, *last;
};

// surface type (widget.type) can be a toplevel or a popup
struct PnWindow {

    // 1st inherit widget
    struct PnWidget widget;

    // Windows are widgets with additions for drawing pixels on Wayland
    // surfaces.  The widget tree contained in windows use the Wayland
    // surfaces in the top most widget which is the window.

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


    void (*destroy)(struct PnWidget *window, void *userData);
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

    // This is a flag that is set in a wl_callback that lets us know that
    // the window is very likely being shown to the user.  We know that
    // the pixel buffer was drawn to, and we have waited for one
    // wl_callback event.  I don't think that there's a way to really know
    // that the window is showing, but this may be close enough.
    uint32_t haveDrawn;
};


// Making wayland client objects into one big ass display thing, like
// a X11 display.
//
struct PnDisplay {

    // Inherit widget for orphaned children widgets.  That is,
    // widgets without parents.  This display thingy is the god parent of
    // all widgets without parents.  The widgets in this "list" do not get
    // drawn unless they get new parents that belong to a PnWindow at a
    // top parenting level.
    //
    // TODO: This does not ever use a lot of the data in this structure,
    // given this widget surface is never drawn and so on.  But, it's not
    // a huge amount of data.  Maybe brake PnWidget into two smaller
    // structures?  And use just the parenting part of a widget here.
    //
    struct PnWidget widget;


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
    // The child most widget that has the pointer in it.
    struct PnWidget *pointerWidget;
    // The child most widget that has the pointer in it and
    // has taken "panels focus" by returning true from it's enter
    // callback.
    struct PnWidget *focusWidget;
    // In panels all x,y coordinates are given relative to the window's
    // widget (Not like GTK's widget relative coordinates).
    //
    // We set this x,y with wayland mouse pointer enter and mouse pointer
    // motion events.
    int32_t x, y; // pointer position.

    struct PnWidget *buttonGrabWidget;
    uint32_t buttonGrab;

    // Set the window with keyboard focus (from enter and leave):
    struct PnWindow *kbWindow;
    struct PnWidget *kbWidget;

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

// We act on just the farthest upper and left cell for a given
// surface (widget).  Widgets can span more than one cell; so this
// selects one of a particular widget per grid looping of all cells
// in the grid; like in RecreateGrid() in surface.c.
//
static inline
bool IsUpperLeftCell(struct PnWidget *c,
        struct PnWidget ***cells, uint32_t x, uint32_t y) {
    return (c && (!x || c != cells[y][x-1]) &&
            (!y || c != cells[y-1][x]));
}


static bool inline HaveChildren(const struct PnWidget *s) {
    DASSERT(s);
    ASSERT(s->layout < PnLayout_Callback, "WRITE MORE CODE");

    if(s->layout < PnLayout_Grid) {
        DASSERT((s->l.firstChild && s->l.lastChild) ||
                (!s->l.firstChild && !s->l.lastChild));
        return (s->l.firstChild);
    }

    // else
    DASSERT(s->layout == PnLayout_Grid);

    return (s->g.grid->numChildren);
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

extern bool InitSurface(struct PnWidget *s,
        uint32_t column, uint32_t row, uint32_t cSpan, uint32_t rSpan);
extern void DestroySurface(struct PnWidget *s);
extern void DestroySurfaceChildren(struct PnWidget *s);

extern void GetWidgetAllocations(struct PnWindow *win);

extern void pnSurface_draw(struct PnWidget *s,
        struct PnBuffer *buffer);

extern void FlushDrawQueue(struct PnWindow *win);
extern bool _pnWindow_addCallback(struct PnWindow *win);
extern void PostDraw(struct PnWindow *win, struct PnBuffer *buffer);
extern bool DrawFromQueue(struct PnWindow *win);

extern void DrawAll(struct PnWindow *win, struct PnBuffer *buffer);

extern void GetSurfaceWithXY(const struct PnWindow *win,
        wl_fixed_t x,  wl_fixed_t y, bool isEnter);

#ifdef WITH_CAIRO
extern void RecreateCairos(struct PnWindow *win);
extern void DestroyCairos(struct PnWidget *win);
extern void DestroyCairo(struct PnWidget *s);
#endif


// Sets d.focusWidget
extern void DoEnterAndLeave(void);
// Sets d.pointerWidget
extern void GetPointerSurface(const struct PnWindow *win);
// Calls motion() callback until a widget callback returns true.
extern void DoMotion(struct PnWidget *s);


static inline void RemoveSurfaceFromDisplay(struct PnWidget *s) {

    if(d.buttonGrabWidget == s)
        d.buttonGrabWidget = 0;

    if(d.focusWidget == s)
        d.focusWidget = 0;

    if(d.pointerWidget == s)
        d.pointerWidget = 0;
}

static inline void ResetDisplaySurfaces(void) {

    // TODO: Do we really want to keep these marked surfaces?  Like after
    // a call to GetWidgetAllocations().

    if(d.buttonGrabWidget && d.buttonGrabWidget->culled)
        d.buttonGrabWidget = 0;

    if(d.focusWidget && d.focusWidget->culled)
        d.focusWidget = 0;

    if(d.pointerWidget && d.pointerWidget->culled)
        d.pointerWidget = 0;
}
