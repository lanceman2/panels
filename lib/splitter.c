// This file is linked into libpanels.so if WITH_CAIRO is defined
// in ../config.make, otherwise it's not.
//
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cairo/cairo.h>
#include <linux/input-event-codes.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"
#include "splitter.h"


static bool press(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnSplitter *s) {
    DASSERT(s == (void *) w);
    DASSERT(s);
    if(which != BTN_LEFT) return false;


    return true;
}

static bool release(struct PnWidget *w,
            uint32_t which, int32_t x, int32_t y,
            struct PnSplitter *s) {
    DASSERT(s == (void *) w);
    DASSERT(s);
    if(which != BTN_LEFT) return false;


    return true;
}

static bool enter(struct PnWidget *w,
            uint32_t x, uint32_t y, struct PnSplitter *s) {
    DASSERT(s);
    s->cursorSet = pnWindow_pushCursor(s->cursorName)?true:false;
    return true; // take focus
}

static void leave(struct PnWidget *w, struct PnSplitter *s) {
    DASSERT(s);
    if(s->cursorSet)
        pnWindow_popCursor();
}

static
void destroy(struct PnWidget *w, struct PnSplitter *s) {

    DASSERT(s);
    DASSERT(s == (void *) w);
}

#define DEFAULT_LEN  (17)

struct PnWidget *pnSplitter_create(struct PnWidget *parent,
        struct PnWidget *first, struct PnWidget *second,
        bool isHorizontal /*or it's vertical*/,
        size_t size) {

    DASSERT(first);
    DASSERT(second);


    if(size < sizeof(struct PnSplitter))
        size = sizeof(struct PnSplitter);
    //
    
    enum PnExpand expand = PnExpand_V;

    if(!isHorizontal)
        expand = PnExpand_H;

    struct PnSplitter *s = (void *) pnWidget_create(parent,
            DEFAULT_LEN/*width*/, DEFAULT_LEN/*height*/,
            PnLayout_Splitter, PnAlign_CC, expand, size);
    if(!s)
        return 0; // Failure.

    if(isHorizontal)
        s->cursorName = "e-resize";
    else
        s->cursorName = "n-resize";

    // Setting the widget surface type.  We decrease the data, but
    // increase the complexity.  See enum PnSurfaceType in display.h.
    // It's so easy to forget about all these bits, but DASSERT() is my
    // hero.
    DASSERT(s->widget.type == PnSurfaceType_widget);
    s->widget.type = PnSurfaceType_splitter;
    DASSERT(s->widget.type & WIDGET);

    s->first = first;
    s->second = second;

    // Add widget callback functions:
    pnWidget_setEnter(&s->widget, (void *) enter, s);
    pnWidget_setLeave(&s->widget, (void *) leave, s);
    pnWidget_setPress(&s->widget, (void *) press, s);
    pnWidget_setRelease(&s->widget, (void *) release, s);
    pnWidget_addDestroy(&s->widget, (void *) destroy, s);

    pnWidget_setBackgroundColor(&s->widget, 0xFFFF0000);

    return &s->widget;
}
