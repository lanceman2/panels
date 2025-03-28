// reference:
// https://specifications.freedesktop.org/icon-theme-spec/latest/#directory_layout

// In libpanels the image widget is intended to be used for widget icons,
// so the image widget has utility to find icon image files.  We thought
// of calling it the icon widget, but functionally it's more of just a
// tool to open and read an image file and put a representation of that
// file in the widget's pixels, it creates an image in a widget.  So, we
// named it based on function and not the more abstract notion of icon.
//
// Hence: in libpanels the image widget is an icon widget.
//
// There's composite widgets that have image widgets in them, where the
// images come from a theme (group of image and other-related files).

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include  "display.h"


struct PnWidget *pnImage_create(struct PnWidget *parent,
        const char *filename,
        uint32_t width, uint32_t height,
        enum PnLayout layout,
        enum PnAlign align,
        enum PnExpand expand, size_t size) {

    return 0;
}
