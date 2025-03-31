// code reference:
// https://github.com/glfw/glfw/
//
// I think that the license is compatible.  So, I'll start with cut and
// paste.  Saved we a few days.  Thank you GLFW.
//
// GLFW is an Open Source, multi-platform library for OpenGL, OpenGL ES
// and Vulkan application development. It provides a simple,
// platform-independent API for creating windows, contexts and surfaces,
// reading input, handling events, etc.
//
// GLFW uses lots of typedef.  I hate typedef.  It makes code opaque.  We
// don't need layers of indirection in variable struct name space.  It's
// makes code harder to scope out.  You're always asking yourself what the
// fuck is this, a pointer, a pointer to a pointer, or a basic C type.
// It's just code obfuscation.  I'm glad libwayland-client does not do
// that shit.

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "../include/panels.h"
#include "debug.h"
#include "display.h"


static void HandleMonitorGeometry(void *data,
        struct wl_output *wl_output, int32_t x, int32_t y,
        int32_t physical_width, int32_t physical_height, int32_t subpixel,
        const char *make, const char *model, int32_t transform) {

    INFO("Physical screen size= %" PRIi32 " mm, %" PRIi32 " mm",
            physical_width, physical_height);
}

static void HandleMonitorPixelContents(void *data,
                struct wl_output *wl_output,
                uint32_t flags,
                int32_t width, int32_t height, int32_t refresh) {

    INFO("Screen size=%" PRIi32 ",%" PRIi32
            " refresh=%" PRIi32 " milli Hz",
            width, height, refresh);

    d.screen_width = width;
    d.screen_height = height;
}

static void HandleMonitorInformationSent(void *data,
                struct wl_output *wl_output) {
    DASSERT(wl_output == d.wl_output);
}

static void HandleMonitorScale(void *data,
                struct wl_output *wl_output, int32_t factor) {
    //monitor_rendered_to.scale = factor;
}

static void HandleMonitorName(void *data, struct wl_output *wl_output,
                              const char *name) {
    INFO("Monitor name= \"%s\"", name);
}

static void HandleMonitorDescription(void* data,
                struct wl_output *wl_output,
                const char *description) {
    INFO("Monitor description= \"%s\"", description);
}

const struct wl_output_listener output_listener = {
    HandleMonitorGeometry,
    HandleMonitorPixelContents,
    HandleMonitorInformationSent,
    HandleMonitorScale,
    HandleMonitorName,
    HandleMonitorDescription
};
