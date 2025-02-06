#define _GNU_SOURCE
#include <link.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "../include/panels.h"
#include "debug.h"
#include  "display.h"


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
struct PnDisplay d = {0}; // very important to zero it.


static void xdg_wm_base_handle_ping(void *data,
		struct xdg_wm_base *xdg_wm_base, uint32_t serial) {

    //DSPEW("PING");
    // The compositor will send us a ping event to check that we're
    // responsive.  We need to send back a pong request immediately.
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_handle_ping
};


// Find the most childish surface that has the pointer position, d.x and
// d.y, in it.
//
// TODO: Do we need a faster surface search data structure?
//
// TODO: Maybe the widget (surface) order in the searching could be more
// optimal.
//
static struct PnSurface * FindSurface(const struct PnWindow *win,
        struct PnSurface *s, uint32_t x, uint32_t y) {

    DASSERT(win);
    DASSERT(s->firstChild);
    DASSERT(!s->culled);

    // Let's see if the x,y positions always make sense.
    DASSERT(s->allocation.x <= x);
    DASSERT(s->allocation.y <= y);
    if(x >= s->allocation.x + s->allocation.width)
        --x;
    if(y >= s->allocation.y + s->allocation.height) {
        WARN("y=%" PRIu32 "s->allocation.y + s->allocation.height=%" PRIu32,
                y, s->allocation.y + s->allocation.height);
    }

    DASSERT(x < s->allocation.x + s->allocation.width);
    DASSERT(y < s->allocation.y + s->allocation.height);

    // At this point in the code "s" contains the pointer position, x,y.
    // Now search the children to see if x,y is in one of them.

    struct PnSurface *c;

    switch(s->direction) {

        case PnDirection_One:
        case PnDirection_LR:
            for(c = s->firstChild; c; c = c->nextSibling) {
                if(c->culled) continue;
                if(x < c->allocation.x)
                    // x is to the left of surface "c"
                    break; // done in x
                if(x >= c->allocation.x + c->allocation.width)
                    // x is to the right of surface "c" so
                    // look at siblings to the right.
                    continue;
                // The x is in surface "c"
                if(y < c->allocation.y)
                    // y is above the surface "c"
                    break;
                if(y >= c->allocation.y + c->allocation.height)
                    // y is below the surface "c"
                    break;
                // this surface "c" contains x,y.
                if(c->firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        case PnDirection_RL:
            for(c = s->lastChild; c; c = c->prevSibling) {
                if(c->culled) continue;
                if(x < c->allocation.x)
                    // x is to the left of surface "c"
                    break; // done in x
                if(x >= c->allocation.x + c->allocation.width)
                    // x is to the right of surface "c" so
                    // look at siblings to the right.
                    continue;
                // The x is in surface "c"
                if(y < c->allocation.y)
                    // y is above the surface "c"
                    break;
                if(y >= c->allocation.y + c->allocation.height)
                    // y is below the surface "c"
                    break;
                // this surface "c" contains x,y.
                if(c->firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        case PnDirection_TB:
            for(c = s->firstChild; c; c = c->nextSibling) {
                if(c->culled) continue;
                if(y < c->allocation.y)
                    // y is to the above of surface "c"
                    break; // done in y
                if(y >= c->allocation.y + c->allocation.height)
                    // y is below surface "c" so
                    // look at siblings below.
                    continue;
                // The y is in surface "c"
                if(x < c->allocation.x)
                    // x is to the left the surface "c"
                    break;
                if(x >= c->allocation.x + c->allocation.width)
                    // x is to the right the surface "c"
                    break;
                // this surface "c" contains x,y.
                if(c->firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        case PnDirection_BT:
            for(c = s->lastChild; c; c = c->prevSibling) {
                if(c->culled) continue;
                if(y < c->allocation.y)
                    // y is to the above of surface "c"
                    break; // done in y
                if(y >= c->allocation.y + c->allocation.height)
                    // y is below surface "c" so
                    // look at siblings below.
                    continue;
                // The y is in surface "c"
                if(x < c->allocation.x)
                    // x is to the left the surface "c"
                    break;
                if(x >= c->allocation.x + c->allocation.width)
                    // x is to the right the surface "c"
                    break;
                // this surface "c" contains x,y.
                if(c->firstChild)
                    return FindSurface(win, c, x, y);
                // c is a leaf.
                return c;
            }
            break;

        default:
            ASSERT(0, "Write more code");
            return 0;
    }


    return s;
}

static void GetSurfaceWithXY(const struct PnWindow *win,
        wl_fixed_t x,  wl_fixed_t y) {

    // Maybe we round up wrong...??  We fix it below.
    d.x = (wl_fixed_to_double(x) + 0.5);
    d.y = (wl_fixed_to_double(y) + 0.5);

    // Sometimes the values are just shit.

    // Let's see if the x,y positions always make sense.
    // Hit these, WTF.
    //DASSERT(d.x < (uint32_t) -1);
    //DASSERT(d.y < (uint32_t) -1);

    if(d.x >= win->surface.allocation.width) {
        // I don't trust the wayland compositor to give me good numbers.
        // I've seen values of x larger than the width of the window.
        // Maybe we round up wrong...
        // I've seen this assertion activated, so what the hell:
        // why is the wayland compositor sending us position values that
        // are not in the window.
        //DASSERT(d.x - win->surface.allocation.width < 3);
        // x values go from 0 to width -1.
        d.x = win->surface.allocation.width - 1;
    }
    if(d.y >= d.pointerWindow->surface.allocation.height) {
        // I don't trust the wayland compositor to give me good numbers.
        // I've seen values of y larger than the height of the window.
        //DASSERT(d.y - d.pointerWindow->surface.allocation.height < 3);
        // y values go from 0 to height -1.
        d.y = d.pointerWindow->surface.allocation.height - 1;
    }

    d.pointerSurface = (void *) d.pointerWindow;

    if(d.pointerSurface->firstChild)
        d.pointerSurface = FindSurface(d.pointerWindow, d.pointerSurface,
                d.x, d.y);
//WARN("Got widget=%p at %" PRIu32 ",%" PRIu32, d.pointerSurface, d.x, d.y);
}

static void DoEnterAndLeave(void) {

    DASSERT(d.pointerSurface);

    for(struct PnSurface *s = d.pointerSurface; s; s = s->parent) {
        DASSERT(!s->culled);
        if(d.focusSurface == s)
            // This surface "s" already has focus.
            break;
        if(s->enter && s->enter(s, d.x, d.y, s->enterData)) {
            if(d.focusSurface && d.focusSurface->leave)
                d.focusSurface->leave(d.focusSurface,
                        d.focusSurface->leaveData);
            d.focusSurface = s;
            break;
        }
    }
}

// This is the window enter event
//
static void enter(void *data,
        struct wl_pointer *p, uint32_t serial,
        struct wl_surface *wl_surface, wl_fixed_t x,  wl_fixed_t y) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(p);
    DASSERT(d.wl_pointer == p);
 
    DASSERT(!d.pointerWindow);
    d.pointerWindow = wl_surface_get_user_data(wl_surface);
    DASSERT(d.pointerWindow->wl_surface == wl_surface);
    DASSERT(d.pointerWindow->surface.allocation.x == 0);
    DASSERT(d.pointerWindow->surface.allocation.y == 0);

    GetSurfaceWithXY(d.pointerWindow, x, y);

    DASSERT(d.pointerSurface);
    DoEnterAndLeave();
}

static void leave(void *data, struct wl_pointer *p,
        uint32_t serial, struct wl_surface *wl_surface) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(p);
    DASSERT(d.wl_pointer == p);

    DASSERT((void *) d.pointerWindow ==
            wl_surface_get_user_data(wl_surface));

    if(d.focusSurface && d.focusSurface->leave)
        d.focusSurface->leave(d.focusSurface, d.focusSurface->leaveData);

    d.pointerWindow = 0;
    d.pointerSurface = 0;
    d.focusSurface = 0;
}

// Window motion.
//
static void motion(void *, struct wl_pointer *p, uint32_t,
        wl_fixed_t x,  wl_fixed_t y) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(p);
    DASSERT(d.wl_pointer == p);
    DASSERT(d.pointerSurface);
    DASSERT(d.pointerWindow);

    struct PnSurface *s = d.pointerSurface;
    GetSurfaceWithXY(d.pointerWindow, x, y);

    if(s != d.pointerSurface)
        // The surface we are pointing to changed.
        // The focused widget may be changed.
        DoEnterAndLeave();
}

static void button(void *, struct wl_pointer *p,
        uint32_t serial,
        uint32_t time,
        uint32_t button, uint32_t state) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(p);
    DASSERT(d.wl_pointer == p);

    //DSPEW("button=%" PRIu32 " state=%" PRIu32, button, state);
}

static void axis(void *, struct wl_pointer *p, uint32_t,
        uint32_t,  wl_fixed_t) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(p);
    DASSERT(d.wl_pointer == p);

    //DSPEW();
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = enter,
    .leave = leave,
    .motion = motion,
    .button = button,
    .axis = axis
};

static void kb_map(void* data, struct wl_keyboard* kb,
        uint32_t frmt, int32_t fd, uint32_t sz) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(kb);
    DASSERT(d.wl_keyboard == kb);

    //DSPEW("fd=%d", fd);
}

static void kb_enter(void* data, struct wl_keyboard* kb,
        uint32_t ser, struct wl_surface* wl_surface,
        struct wl_array* keys) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(kb);
    DASSERT(d.wl_keyboard == kb);

    DASSERT(!d.kbWindow);

    d.kbWindow = wl_surface_get_user_data(wl_surface);
//DSPEW();
}

static void kb_leave(void* data, struct wl_keyboard* kb,
        uint32_t ser, struct wl_surface* wl_surface) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(kb);
    DASSERT(d.wl_keyboard == kb);

    d.kbWindow = 0;
    //DSPEW();
}

static void kb_key(void* data, struct wl_keyboard* kb,
        uint32_t ser, uint32_t t, uint32_t key,
        uint32_t stat) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(kb);
    DASSERT(d.wl_keyboard == kb);

    //DSPEW("key=%" PRIu32, key);
}

static void kb_mod(void* data, struct wl_keyboard* kb,
        uint32_t ser, uint32_t dep, uint32_t lat,
        uint32_t lock, uint32_t grp) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(kb);
    DASSERT(d.wl_keyboard == kb);

    //DSPEW();
}

static void kb_repeat(void* data, struct wl_keyboard* kb,
        int32_t rate, int32_t del) {

    DASSERT(d.wl_display);
    DASSERT(d.wl_seat);
    DASSERT(kb);
    DASSERT(d.wl_keyboard == kb);

    DSPEW();
}

static struct wl_keyboard_listener kb_listener = {
    .keymap = kb_map,
    .enter = kb_enter,
    .leave = kb_leave,
    .key = kb_key,
    .modifiers = kb_mod,
    .repeat_info = kb_repeat
};


static void seat_handle_capabilities(void *data, struct wl_seat *seat,
		uint32_t capabilities) {

    DASSERT(d.wl_display);
    DASSERT(seat == d.wl_seat);
    DASSERT(!d.wl_pointer);
    DASSERT(!d.wl_keyboard);
    DASSERT(capabilities & WL_SEAT_CAPABILITY_POINTER);
    DASSERT(capabilities & WL_SEAT_CAPABILITY_KEYBOARD);

    if(capabilities & WL_SEAT_CAPABILITY_POINTER) {
        d.wl_pointer = wl_seat_get_pointer(seat);
        ASSERT(d.wl_pointer);
	wl_pointer_add_listener(d.wl_pointer, &pointer_listener, seat);
    }

    if(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
	d.wl_keyboard = wl_seat_get_keyboard(seat);
        ASSERT(d.wl_keyboard);
	wl_keyboard_add_listener(d.wl_keyboard, &kb_listener, 0);
    }
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
};


static void handle_global(void *data, struct wl_registry *registry,
            uint32_t name, const char *interface, uint32_t version) {

    DASSERT(registry);
    DASSERT(d.wl_registry == registry);
    DASSERT(d.wl_display);

    //DSPEW("name=\"%s\" (%" PRIu32 ")", wl_shm_interface.name, name);

    if(!strcmp(interface, wl_shm_interface.name)) {
	d.wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
        DASSERT(d.wl_shm);
        if(!d.wl_shm) {
            ERROR("wl_registry_bind(,,) for shm failed");
            d.handle_global_error = 1;
        }
    } else if(!strcmp(interface, wl_seat_interface.name)) {
        // I'm guessing we can only get one wayland seat.
        DASSERT(d.wl_seat == 0);
        d.wl_seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
        if(!d.wl_seat) {
            ERROR("wl_registry_bind(,,) for seat failed");
            d.handle_global_error = 2;
            return;
        }
	if(wl_seat_add_listener(d.wl_seat, &seat_listener, NULL)) {
            ERROR("wl_seat_add_listener() failed");
            d.handle_global_error = 3;
        }
    } else if(!strcmp(interface, wl_compositor_interface.name)) {
        d.wl_compositor = wl_registry_bind(registry, name,
                &wl_compositor_interface, 1);
        if(!d.wl_compositor) {
            ERROR("wl_registry_bind(,,) for wayland compositor failed");
            d.handle_global_error = 4;
        }
    } else if(!strcmp(interface, xdg_wm_base_interface.name)) {
	d.xdg_wm_base = wl_registry_bind(registry, name,
                &xdg_wm_base_interface, 1);
        if(!d.xdg_wm_base) {
            ERROR("wl_registry_bind(,,) for xdg_wm_base failed");
            d.handle_global_error = 5;
            return;
        }
	if(xdg_wm_base_add_listener(d.xdg_wm_base,
                    &xdg_wm_base_listener, 0)) {
            ERROR("xdg_wm_base_add_listener(,,) failed");
            d.handle_global_error = 6;
        }
    } else if(!strcmp(interface, zxdg_decoration_manager_v1_interface.name)) {
        d.zxdg_decoration_manager = wl_registry_bind(registry, name,
	        &zxdg_decoration_manager_v1_interface, 1);
        if(!d.zxdg_decoration_manager) {
            ERROR("wl_registry_bind(,,) for zxdg_decoration_manager failed");
            d.handle_global_error = 7;
            return;
        }
    }
}


static void handle_global_remove(void *data,
        struct wl_registry *registry, uint32_t name) {
    // I care to see this.
    WARN("called for name=%" PRIu32, name);
}

static const struct wl_registry_listener registry_listener = {
    .global = handle_global,
    .global_remove = handle_global_remove
};

#if 0
static void display_error(void *data,
        struct wl_display *wl_display,
	void *object_id, uint32_t code,
	const char *message) {

    DASSERT(d.wl_display == wl_display);

    WARN("wl_display got error: \"%s\"", message);
}

static void display_delete_id(void *data,
        struct wl_display *wl_display, uint32_t id) {
 
    DASSERT(d.wl_display == wl_display);

    WARN("deleted wayland object %" PRIu32, id);
}

struct wl_display_listener display_listener = {
    .error = display_error,
    .delete_id = display_delete_id
};
#endif


#if 0
static void libdecor_error(struct libdecor *context,
		       enum libdecor_error error,
		       const char *message) {
    DSPEW("error=%d", error);
}

struct libdecor_interface libdecor_interface = {

    .error = libdecor_error
};
#endif



static int _pnDisplay_create(void) {

    DASSERT(!d.wl_display);

    d.wl_display = wl_display_connect(0);
    RET_ERROR(d.wl_display, 1, "wl_display_connect() failed");

#if 0 // This does not seem to work
    RET_ERROR(!wl_display_add_listener(d.wl_display,
                &display_listener, 0),
            2, "wl_display_add_listener() failed");
#endif

    d.wl_registry = wl_display_get_registry(d.wl_display);
    RET_ERROR(d.wl_registry, 3,
            "wl_display_get_registry(%p) failed", d.wl_display);

    RET_ERROR(!wl_registry_add_listener(d.wl_registry,
                    &registry_listener, 0), 4,
            "wl_registry_add_listener(%p, %p,) failed",
                d.wl_registry, &registry_listener);
    
    d.handle_global_error = 0;

    // I'm guessing wl_display_roundtrip() can block.
    RET_ERROR(wl_display_roundtrip(d.wl_display) != -1 &&
                    d.handle_global_error == 0, 5,
            "wl_display_roundtrip(%p) failed handle_global_error=%"
                PRIu32, d.wl_display, d.handle_global_error);
        // I needed a way to get an error value out of handle_global()
        // so I added variable handle_global_error.

    // Note: we are just letting the user see one error and are not
    // letting them see all the errors if there is more than one.
    // Showing more than one error doesn't seem to be a big gain.
    //
    // The user is pretty well screwed if we fail to get any of these:
    //
    RET_ERROR(d.wl_shm,        6, "cannot get wl_shm");
    RET_ERROR(d.wl_compositor, 7, "cannot get wl_compositor");
    RET_ERROR(d.xdg_wm_base,   8, "cannot get xdg_wm_base");
    RET_ERROR(d.wl_seat,       9, "cannot get wl_seat");

    if(!d.zxdg_decoration_manager)
        // This will happen on a GNOME 3 wayland desktop.
        NOTICE("cannot get zxdg_decoration_manager");

    // It looks like we do not get the pointer and the keyboard until we
    // call wl_display_dispatch().
    int err = 0;
    while(!d.wl_pointer || !d.wl_keyboard)
        if(wl_display_dispatch(d.wl_display) == -1) {
            ERROR("wl_display_dispatch() failed");
            err = 11;
            break;
        }
    RET_ERROR(d.wl_pointer,   10, "cannot get wl_pointer");
    RET_ERROR(d.wl_keyboard, 11, "cannot get wl_keyboard");

    return err; // return 0 => success.
}


static void _pnDisplay_destroy(void) {

    DASSERT(d.wl_display);

    // Destroy stuff in reverse order of creation, pretty much.

    // Destroy all the windows.
    while(d.windows)
        pnWindow_destroy(d.windows);

    if(d.zxdg_decoration_manager)
        zxdg_decoration_manager_v1_destroy(d.zxdg_decoration_manager);

    if(d.wl_keyboard) {
        DASSERT(d.wl_seat);
        wl_keyboard_release(d.wl_keyboard);
    }

    if(d.wl_pointer) {
        DASSERT(d.wl_seat);
        wl_pointer_release(d.wl_pointer);
    }

    if(d.wl_seat)
        wl_seat_release(d.wl_seat);

    if(d.xdg_wm_base)
        xdg_wm_base_destroy(d.xdg_wm_base);

    if(d.wl_compositor)
        wl_compositor_destroy(d.wl_compositor);

    if(d.wl_shm)
        wl_shm_destroy(d.wl_shm);

    if(d.wl_registry)
        wl_registry_destroy(d.wl_registry);

    wl_display_disconnect(d.wl_display);

    memset(&d, 0, sizeof(d));
}


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
// point of libpanels.so; make a window with 10 lines of code.
//
// Return 0 on success and not zero on error.
//
// TODO Better document error return codes?
//
int pnDisplay_create(void) {

    // So many things in this function can fail.

    ASSERT(!d.wl_display);

    int ret = _pnDisplay_create();

    if(ret)
        _pnDisplay_destroy();

    return ret;
}


// This will cleanup all things from libpanels.  We wish to make this
// libpanels library robust under sloppy user cases, by calling this in
// the library destructor if pnDisplay_create() was ever called.
//
void pnDisplay_destroy(void) {

    if(!d.wl_display) {
        // The whole struct PnDisplay should be zeros.  Lets check a few
        // parts.
        DASSERT(!d.wl_display);
        DASSERT(!d.wl_registry);
        DASSERT(!d.wl_shm);
        DASSERT(!d.wl_compositor);
        DASSERT(!d.xdg_wm_base);

        DASSERT(!d.windows);
        return;
    }

    _pnDisplay_destroy();
}


bool pnDisplay_haveXDGDecoration(void) {

    if(!d.wl_display)
        _pnDisplay_create();

    return (bool) d.zxdg_decoration_manager;
}


// This can block.
//
// Return true if we can keep running.
//
bool pnDisplay_dispatch(void) {

    if(!d.wl_display) {
        if(_pnDisplay_create())
            return false;
    }

    if(wl_display_dispatch(d.wl_display) != -1 &&
            d.windows/*we have at least one window*/)
        // We can keep going.
        return true;

    return false;
}
