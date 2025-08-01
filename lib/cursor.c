#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <wayland-client.h>
#include <wayland-cursor.h>


#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "../include/panels.h"
#include "debug.h"
#include "display.h"

// reference:
//https://bugaevc.gitbooks.io/writing-wayland-clients/content/beyond-the-black-square/cursors.html
//
// That link does not have useable code, but with lots of trial and
// error.  I got this to work using that link as a hint.  The wayland
// cursor code in glfw did not work for me, but it also gave me hints.

// This cursor stuff is a singleton, one per process;
// just like the wl_display and et al.
//
// TODO: We could make more than one, but that's not how this is written.
// I don't think we can have more than one cursor at a time, so having
// having one (or none) instance(s) of this cursor_theme seems to make
// sense.  It could be that KWin is just buggy, so that code may not
// work with other wayland compositors.


static struct wl_cursor_theme *theme = 0;
// TODO: Destroy this after use.
static struct wl_surface *surface = 0;

static struct Stack {
    char *name;
    struct Stack *prev;
    uint32_t refcount;
} *stack = 0;

static uint32_t count = 0;


// cursorName is for example: "n-resize" "nw-resize" 
//
static
bool pnWindow_setCursor(struct PnWidget *w, const char *name) {

    DASSERT(w);
    DASSERT(d.wl_pointer);
    DASSERT(theme);
    DASSERT(d.wl_shm);
    DASSERT(name);
    DASSERT(name[0]);
    DASSERT(surface);
    //DASSERT(d.pointerWindow);
    DASSERT(d.surface_damage_func);

    struct PnWindow *win;
    switch(w->type & (TOPLEVEL|POPUP)) {
        case TOPLEVEL:
        case POPUP:
            win = (struct PnWindow *) w;
            break;
        case 0:
            // Not a TOPLEVEL or POPUP
            win = w->window;
            DASSERT(win);
            break;
        default:
            ASSERT(0);
    }

    DASSERT(win);
    DASSERT(win->lastSerial);

    struct wl_cursor *cursor = wl_cursor_theme_get_cursor(theme, name);
    if(!cursor) {
        ERROR("wl_cursor_theme_get_cursor(theme, \"%s\") failed",
                name);
        return true;
    }
    struct wl_cursor_image *image = cursor->images[0];
    ASSERT(image);

    struct wl_buffer *buffer = wl_cursor_image_get_buffer(image);
    if(!buffer) {
        ERROR("wl_cursor_image_get_buffer() for cursor \"%s\" failed",
                name);
        return true;
    }


    // TODO: This is the part that make the cursor dependent on the window
    // with win->lastSerial.  I do not understand this.
    //
    wl_pointer_set_cursor(d.wl_pointer, win->lastSerial, surface,
            image->hotspot_x, image->hotspot_y);
    d.surface_damage_func(surface, 0, 0, image->width, image->height);
    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);

    return false;
}

// Returns the name of the previous cursor, zero if it stays the same, or
// zero if there is none left.
//
static inline char *Pop(void) {

    DASSERT(stack);
    DASSERT(stack->name);
    DASSERT(stack->name[0]);
    DASSERT(count);

    --count;

    if(--stack->refcount) return 0;

    struct Stack *last = stack;
    stack = stack->prev;
    DZMEM(last->name, strlen(last->name));
    free(last->name);
    DZMEM(last, sizeof(*last));
    free(last);

    if(!stack) return 0;

    DASSERT(stack->name);

    return stack->name;
}

static inline void Push(const char *name) {

    DASSERT(name);
    DASSERT(name[0]);

#ifdef DEBUG
    if(stack) {
        DASSERT(stack->name);
        DASSERT(stack->name[0]);
    }
#endif

    ++count;

    if(stack && strcmp(name, stack->name) == 0) {
        // The name is the same as the last one.
        ++stack->refcount;
        return;
    }

    struct Stack *s = calloc(1, sizeof(*s));
    ASSERT(s, "calloc(1,%zu) failed", sizeof(*s));
    s->name = strdup(name);
    ASSERT(s->name, "strdup() failed");
    ++s->refcount;
    s->prev = stack;
    stack = s;
}


void CleanupCursorTheme(void) {

    if(surface) {
        wl_surface_destroy(surface);
        surface = 0;
    }
    if(theme) {
        wl_cursor_theme_destroy(theme);
        theme = 0;
    }

    while(stack) Pop();

    DASSERT(count == 0);
}


void LoadCursorTheme(void) {

    // Credit:
    //
    // GLFW 3.5 Wayland - www.glfw.org
    // Lots of code copying from https://github.com/glfw/glfw.git

    DASSERT(theme == 0);
    DASSERT(d.wl_shm);
    DASSERT(d.wl_compositor);

    int cursorSize = 16;

    const char* sizeString = getenv("XCURSOR_SIZE");
    if(sizeString) {
        errno = 0;
        const long cursorSizeLong = strtol(sizeString, NULL, 10);
        if (errno == 0 && cursorSizeLong > 0 && cursorSizeLong < INT_MAX)
            cursorSize = (int) cursorSizeLong;
    }

    const char* themeName = getenv("XCURSOR_THEME");

    theme = wl_cursor_theme_load(themeName, cursorSize, d.wl_shm);
    if(!theme) {
        ERROR("Failed to load default cursor theme");
        goto fail;
    }
    DSPEW("CURSOR size=%d and theme name=\"%s\"", cursorSize, themeName);

    surface = wl_compositor_create_surface(d.wl_compositor);
    if(!surface) {
        ERROR("wl_compositor_create_surface() failed");
        goto fail;
    }

    return;


fail:

    CleanupCursorTheme();
    ASSERT(0);
}



// Return 0 on failure.
// Return stack height on success.
//
uint32_t pnWindow_pushCursor(struct PnWidget *w, const char *name) {

    DASSERT(w);
    DASSERT(w->window);

    if(!stack) {
        DASSERT(!count);
        // We need to initialize this thing.
        if(pnWindow_setCursor(w, PN_DEFAULT_CURSOR))
            return 0; // shit.
        Push(PN_DEFAULT_CURSOR);
    }
    DASSERT(count);

    if(pnWindow_setCursor(w, name))
        return 0; // fail.

    Push(name);

    return count;
}

// Pared with a successful pnWindow_pushCursor()
//
void pnWindow_popCursor(struct PnWidget *w) {

    DASSERT(w);
    DASSERT(w->window);


    char *name = Pop();

    if(!stack || !name) return;

    pnWindow_setCursor(w, name);
}
