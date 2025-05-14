#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "../include/panels.h"
#include "debug.h"
#include  "display.h"
#include "../include/panels_drawingUtils.h"


static inline
struct PnWidget *_pnWidget_createFull(
        struct PnWidget *parent, uint32_t w, uint32_t h,
        enum PnLayout layout, enum PnAlign align,
        enum PnExpand expand, 
        uint32_t column, uint32_t row,
        uint32_t cSpan, uint32_t rSpan,
        size_t size) {

    // We can make widgets without having a window yet, so we need to make
    // sure that we have a PnDisplay (process singleton).
    if(CheckDisplay()) return 0;

    if(!parent)
        // Use the god parent.  This will start existent as an unseen
        // widget without a top level window (window widget).
        parent = (void *) &d.widget;

    // Miss-use cases.  The code assumes this to be true.  You can't just
    // remove these assumptions without trashing the running program.  If
    // you don't like it, rewrite all the code.  The code is currently
    // highly dependent on this being so.
    //
    DASSERT(parent->layout != PnLayout_None,
            "Widget can't have children");
    DASSERT(!parent->l.firstChild ||
            (parent->layout != PnLayout_One &&
            parent->layout != PnLayout_Cover),
            "Widget can only have one child");
    //
    if(parent->layout == PnLayout_None) {
        ERROR("Parent widget cannot have children");
        return 0;
    }
    //
    if((parent->layout == PnLayout_One ||
            parent->layout == PnLayout_Cover) &&
            parent->l.firstChild) {
        ERROR("Parent widget cannot have more than one child");
        return 0;
    }

    // If there will not be children in this new widget than
    // we need width, w, and height, h to be nonzero.
    if(layout == PnLayout_None) {
        if(w == 0)
            w = PN_MIN_WIDGET_WIDTH;
        if(h == 0)
            h = PN_MIN_WIDGET_HEIGHT;
    }
    struct PnWidget *widget;

    if(size < sizeof(*widget))
        size = sizeof(*widget);

    widget = calloc(1, size);
    ASSERT(widget, "calloc(1,%zu) failed", size);
#ifdef DEBUG
    widget->size = size;
#endif
    widget->parent = parent;
    widget->layout = layout;
    widget->align = align;
    // Setting the default leaf widget clip setting here (see comment in
    // display.c).  If you change this; run tests to see how widget sizing
    // and culling changes on a window resize.
    //
    // TODO: Add an interface to the API for changing this.
    widget->clip = true; // true or false

    * (enum PnExpand *) &widget->expand = expand;
    widget->type = PnSurfaceType_widget;
    if(widget->layout == PnLayout_Grid) {
        //DASSERT(column);
        //DASSERT(row);
        DASSERT(column != -1);
        DASSERT(row != -1);
        widget->g.numColumns = column;
        widget->g.numRows = row;
    }

    if(parent->type & WIDGET)
        widget->window = parent->window;
    else
        widget->window = (void *) parent;

    widget->reqWidth = w;
    widget->reqHeight = h;

    InitSurface(widget, column, row, cSpan, rSpan);

    return widget;
}

struct PnWidget *pnWidget_createAsGrid(
        struct PnWidget *parent, uint32_t w, uint32_t h,
        enum PnAlign align, enum PnExpand expand,
        uint32_t numColumns, uint32_t numRows,
        size_t size) {
    return _pnWidget_createFull(parent, w, h, PnLayout_Grid,
            align, expand, numColumns, numRows,
            0/*cSpan*/, 0/*rSpan*/, size);
}

struct PnWidget *pnWidget_create(
        struct PnWidget *parent, uint32_t w, uint32_t h,
        enum PnLayout layout, enum PnAlign align,
        enum PnExpand expand, size_t size) {
    ASSERT(layout != PnLayout_Grid);
    return _pnWidget_createFull(parent, w, h, layout, align, expand, 
            -1/*numColumns*/, -1/*numRows*/,
            0/*cSpan*/, 0/*rSpan*/, size);
}

struct PnWidget *pnWidget_createInGrid(
        struct PnWidget *grid, uint32_t w, uint32_t h,
        enum PnLayout layout,
        enum PnAlign align, enum PnExpand expand, 
        uint32_t columnNum, uint32_t rowNum,
        uint32_t columnSpan, uint32_t rowSpan,
        size_t size) {
    ASSERT(grid);
    ASSERT(grid->layout == PnLayout_Grid);
    return _pnWidget_createFull(grid, w, h, layout, align, expand, 
            columnNum, rowNum, columnSpan, rowSpan, size);
}

static inline
void RemoveCallback(struct PnAction *a, struct PnCallback *c) {

    DASSERT(c);

    if(c->next) {
        DASSERT(c != a->last);
        c->next->prev = c->prev;
    } else {
        DASSERT(c == a->last);
        a->last = c->prev;
    }

    if(c->prev) {
        DASSERT(c != a->first);
        c->prev->next = c->next;
    } else {
        DASSERT(c == a->first);
        a->first = c->next;
        //c->prev = 0; // DZMEM() does this.
    }
    //c->next = 0; // DZMEM() does this.

    // Free c
    DZMEM(c, sizeof(*c));
    free(c);
}

void pnWidget_destroy(struct PnWidget *w) {

    DASSERT(w);
    DASSERT(w->parent);
    ASSERT(w->type & WIDGET);

    // Free actions
    if(w->actions) {
        DASSERT(w->numActions);
        struct PnAction *a = w->actions;
        struct PnAction *end = a + w->numActions;
        for(; a != end; ++a)
            while(a->first)
                RemoveCallback(a, a->first);
        // Free the actions[] array.
        DZMEM(w->actions, w->numActions*sizeof(*w->actions));
        free(w->actions);
    }

    // If there is state in the display that refers to this widget surface
    // (w) take care to not refer to it.  Like if this widget (w) had
    // focus, for example.
    RemoveSurfaceFromDisplay((void *) w);

    while(w->destroys) {
        struct PnWidgetDestroy *destroy = w->destroys;
        destroy->destroy(w, destroy->destroyData);
        // pop one off the stack
        w->destroys = destroy->next;
        // Free the struct PnWidgetDestroy element.
        DZMEM(destroy, sizeof(*destroy));
        free(destroy);
    }

    // Destroy children.
    // Destroy all child widgets in the list from w->l.firstChild
    // or w->g.child[][].
    DestroySurfaceChildren(w);

    DestroySurface(w);
    DZMEM(w, w->size);
    free(w);
}

void pnWidget_show(struct PnWidget *widget, bool show) {

    DASSERT(widget);
    ASSERT(widget->type & WIDGET);

    // Make it be one of two values.  Because we can have things like (3)
    // == true
    //
    // I just do not want to assume how many bits are set in the value of
    // true.  Maybe true is 0xFF of maybe it's 0x01.  Hell, I don't
    // necessarily know number of byte in the value of true and false.  I
    // don't even know if C casting fixes its value to just true and false
    // (I doubt it does).
    //
    show = show ? true : false;

    if(widget->hidden == !show)
        // No change.
        return;

    widget->hidden = !show;
    // This change may change the size of the window and many of the
    // widgets in the window.

    widget->window->needAllocate = true; 
}

void pnWidget_addChild(struct PnWidget *parent,
        struct PnWidget *child) {
    DASSERT(parent);
    DASSERT(child);
    DASSERT(parent->layout != PnLayout_Grid);

    if(child->parent)
        RemoveChildSurface(child->parent, child);
    child->parent = parent;
    InitSurface(child, -1, -1, 0, 0);
}

void pnWidget_addChildToGrid(struct PnWidget *parent,
        struct PnWidget *child,
        uint32_t columnNum, uint32_t rowNum,
        uint32_t columnSpan, uint32_t rowSpan) {
    DASSERT(parent);
    DASSERT(child);
    if(columnNum == -1 || rowNum == -1) {
        DASSERT(parent->layout != PnLayout_Grid);
        columnNum = -1;
        rowNum = -1;
        columnSpan = 0;
        rowSpan = 0;
    }

    if(child->parent)
        RemoveChildSurface(child->parent, child);
    child->parent = parent;
    InitSurface(child, columnNum, rowNum, columnSpan, rowSpan);
}
