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
// That link does not have useable code, but with lots of trial and error
// I got this to work using that link as a hint.  The wayland cursor code
// in glfw did not work for me, but also gave me hints.


static struct wl_cursor_theme *theme = 0;
// TODO: Destroy this after use.
static struct wl_surface *surface = 0;

void CleanupCursorTheme(void) {

    if(surface) {
        wl_surface_destroy(surface);
        surface = 0;
    }
    if(theme) {
        wl_cursor_theme_destroy(theme);
        theme = 0;
    }
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

    //const char *name = "nw-resize";

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


// cursorName is for example: "n-resize" "nw-resize" 
//
bool pnWindow_setCursor(struct PnWidget *w, const char *name) {

    DASSERT(w);
    DASSERT(d.wl_pointer);
    DASSERT(theme);
    DASSERT(d.wl_shm);
    DASSERT(name);
    DASSERT(name[0]);
    DASSERT(surface);
    DASSERT(d.surface_damage_func);
    DASSERT((w->type & TOPLEVEL) ||
            (w->type & POPUP) ||
            (w->type & WIDGET));

    struct PnWindow *win;
    switch(w->type & (TOPLEVEL|POPUP|WIDGET)) {
        case TOPLEVEL:
        case POPUP:
            win = (struct PnWindow *) w;
            break;
        case WIDGET:
            win = w->window;
            DASSERT(win);
            break;
        default:
            ASSERT(0);
    }

    if(win != d.pointerWindow) {
        DASSERT(!win->lastSerial);
        ERROR("PnWindow not in focus");
        goto fail;
    }

    DASSERT(win->lastSerial);

    struct wl_cursor *cursor = wl_cursor_theme_get_cursor(theme, name);
    DASSERT(cursor);
    struct wl_cursor_image *image = cursor->images[0];
    DASSERT(image);

    wl_pointer_set_cursor(d.wl_pointer, win->lastSerial, surface,
            image->hotspot_x, image->hotspot_y);

    struct wl_buffer *buffer = wl_cursor_image_get_buffer(image);
    if(!buffer) {
        ERROR("wl_cursor_image_get_buffer() for cursor \"%s\"",
                name);
        goto fail;
    }

    d.surface_damage_func(surface, 0, 0, image->width, image->height);
    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);

    return false;

fail:

    return true;
}

