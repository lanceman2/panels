#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"

#include "../include/panels_drawingUtils.h"


void DestroySurfaceChildren(struct PnSurface *s) {

    if(s->layout != PnLayout_Grid)
        while(s->l.firstChild)
            pnWidget_destroy((void *) s->l.firstChild);
    else {
        struct PnSurface ***child = s->g.child;
        for(uint32_t y=s->g.numRows-1; y != -1; --y)
            for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
                struct PnSurface *c = child[y][x];
                // rows and columns can share widgets; like for row span 2
                // and column span 2; that is adjacent cells that share
                // the same widget.
                if(c && (!x || c != child[y][x-1]) &&
                        (!y || c != child[y-1][x])) {
                    pnWidget_destroy((void *) c);
                    --s->g.numChildren;
                }
                child[y][x] = 0;
            }
        DASSERT(!s->g.numChildren);
    }
}


void GetSurfaceDamageFunction(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->wl_surface);

    if(!d.surface_damage_func) {
        // WTF (what the fuck): Why do they change the names of functions
        // in an API, and still keep both accessible even when one is
        // broken.
        //
        // This may not be setting the correct function needed to see what
        // surface_damage_func should be set to.  We just make it work on
        // the current system (the deprecated function
        // wl_surface_damage()).  When this fails you'll see (from the
        // stderr tty spew):
        // 
        //  wl_display@1: error 1: invalid method 9 (since 1 < 4), object wl_surface@3
        //
        // ...or something like that.
        //
        // Given it took me a month (at least) to find this (not so great
        // fix) we're putting lots of comment-age here.  I had a hard time
        // finding the point in the code where that broke it after it
        // keeps running with no error some time after.
        //
        // This one may be correct. We would have hoped that
        // wl_proxy_get_version() would have used argument prototype const
        // struct wl_proxy * so we do not change memory at/in
        // win->wl_surface, but alas they do not:
        uint32_t version = wl_proxy_get_version(
                (struct wl_proxy *) win->wl_surface);
        //
        // These two may be the wrong function to use (I leave here for
        // the record):
        //uint32_t version = xdg_toplevel_get_version(win->xdg_toplevel);
        //uint32_t version = xdg_surface_get_version(win->xdg_surface);

        switch(version) {
            case 1:
                // Older deprecated version (see:
                // https://wayland-book.com/surfaces-in-depth/damaging-surfaces.html)
                DSPEW("Using deprecated function wl_surface_damage()"
                        " version=%" PRIu32, version);
                d.surface_damage_func = wl_surface_damage;
                break;

            case 4: // We saw a version 4 in another compiled program that used
                    // wl_surface_damage_buffer()
            default:
                // newer version:
                DSPEW("Using newer function wl_surface_damage_buffer() "
                        "version=%" PRIu32, version);
                d.surface_damage_func = wl_surface_damage_buffer;
        }
    }
}

static inline
void AddChildSurfaceList(struct PnSurface *parent, struct PnSurface *s) {

    DASSERT(parent);
    DASSERT(s);
    DASSERT(s->parent);
    DASSERT(s->parent == parent);
    DASSERT(!s->l.firstChild);
    DASSERT(!s->l.lastChild);
    DASSERT(!s->pl.nextSibling);
    DASSERT(!s->pl.prevSibling);

    if(parent->l.firstChild) {
        DASSERT(parent->l.lastChild);
        DASSERT(!parent->l.lastChild->pl.nextSibling);
        DASSERT(!parent->l.firstChild->pl.prevSibling);

        s->pl.prevSibling = parent->l.lastChild;
        parent->l.lastChild->pl.nextSibling = s;
    } else {
        DASSERT(!parent->l.lastChild);
        parent->l.firstChild = s;
    }
    parent->l.lastChild = s;
}

// We act on just the farthest upper and left cell for a given
// surface (widget).  Widgets can span more than one cell.
//
static inline
bool IsUpperLeftCell(struct PnSurface *c,
        struct PnSurface ***cells, uint32_t x, uint32_t y) {
    return (c && (!x || c != cells[y][x-1]) &&
            (!y || c != cells[y-1][x]));
}


// This can destroy, create, and reallocate the grid.  Just choose your
// numColumns and numRows.  Widgets in removed cells get destroyed.
//
// Oh what fun.
//
static inline
void RecreateGrid(struct PnSurface *s, uint32_t numColumns, uint32_t numRows) {

    DASSERT(s->layout == PnLayout_Grid);
    struct PnSurface ***child = s->g.child;
    ASSERT(s->g.numColumns || !child);
    ASSERT(s->g.numRows || !child);
    ASSERT((numColumns && numRows) || (!numColumns && !numRows));

    // 1. First do possible cleanup due to number of rows decreasing.
    ////////////////////////////////////////////////////////////////////
    uint32_t y=s->g.numRows-1;
    //
    for(; y >= numRows && y != -1; --y) {
        // Remove row child[y].
        for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
            struct PnSurface *c = child[y][x];
            // rows and columns can share widgets; like for row span 2
            // and column span 3; that is adjacent cells that share
            // the same widget.
            //
            // Note: the element spans may change, if the spans go across
            // cells that are kept and cells that are removed.
            if(IsUpperLeftCell(c, child, x, y)) {
                pnWidget_destroy((void *) c);
                --s->g.numChildren;
            }
            child[y][x] = 0; // DEBUG memset 0 thingy
        }
        child[y] = 0;
        free(child[y]);
    }

    // 2. Do removing or growing of the cells at the end of the existing
    //    rows.
    ////////////////////////////////////////////////////////////////////
    // For the remaining rows, shrink the number of columns per row as
    // needed.
    //
    // y is numRows-1 or -1
    for(; y != -1; --y) {
        // Note we iterate just on the end of the row if it needs
        // trimming.
        for(uint32_t x=s->g.numColumns-1; x>=numColumns && x!=-1; --x) {
            struct PnSurface *c = child[y][x];
            if(IsUpperLeftCell(c, child, x, y)) {
                pnWidget_destroy((void *) c);
                --s->g.numChildren;
            }
            child[y][x] = 0; // DEBUG memset 0 thingy
        }
        if(s->g.numColumns != numColumns) continue;
        child[y] = realloc(child[y], numColumns*sizeof(*child[y]));
        ASSERT(child[y], "realloc(,%zu) failed",
                numColumns*sizeof(*child[y]));
        if(numColumns > s->g.numColumns)
            // Zero the new memory.
            memset(child[y] + s->g.numColumns, 0,
                    (numColumns - s->g.numColumns)*sizeof(*child[y]));
    }

    // 3. Grow or shrink the number of rows.
    /////////////////////////////////////////////////////////////////
    if(numRows != s->g.numRows) {
        child = s->g.child = realloc(child, numRows*sizeof(*child));
        ASSERT(child, "realloc(,%zu) failed", numRows*sizeof(*child));
        if(numRows > s->g.numRows)
            // Zero the new memory.
            memset(child + s->g.numRows, 0,
                    (numRows - s->g.numRows)*sizeof(*child));
    }

    // 4. Add new rows.
    /////////////////////////////////////////////////////////////////
    for(y=s->g.numRows; y < numRows; ++y) {
        child[y] = calloc(numColumns, sizeof(*child[y]));
        ASSERT(child[y], "calloc(%" PRIu32 ",%zu) failed",
                numColumns, sizeof(*child[y]));
        // calloc() zeros memory.
    }

    s->g.numRows = numRows;
    s->g.numColumns = numColumns;
}


static inline
void AddChildSurfaceGrid(struct PnSurface *grid, struct PnSurface *s,
        uint32_t column, uint32_t row, uint32_t cSpan, uint32_t rSpan) {

    DASSERT(cSpan);
    DASSERT(rSpan);

    if(column + cSpan > grid->g.numColumns ||
            row + rSpan > grid->g.numRows) {
        uint32_t c = column + cSpan;
        if(c < grid->g.numColumns)
            c = grid->g.numColumns;
        uint32_t r = row + rSpan;
        if(r < grid->g.numRows)
            r = grid->g.numRows;
        RecreateGrid(grid, c, r);
    }

    DASSERT(grid->g.numColumns > column);
    DASSERT(grid->g.numRows > row);
    DASSERT(grid->g.child);

    struct PnSurface ***child = s->g.child;
    for(uint32_t y=row+rSpan-1; y>=row; --y)
        for(uint32_t x=column+cSpan-1; x>=column; --x)
            // The added child can span more cells.
            child[y][x] = s;

    s->pg.row = row;
    s->pg.column = column;
}

// Add the child, s, to the parent.
static inline
void AddChildSurface(struct PnSurface *parent, struct PnSurface *s,
        uint32_t column, uint32_t row, uint32_t cSpan, uint32_t rSpan) {

    DASSERT(parent);
    DASSERT(s);
    DASSERT(s->parent);
    DASSERT(s->parent == parent);

    if(parent->layout != PnLayout_Grid)
        AddChildSurfaceList(parent, s);
    else
        AddChildSurfaceGrid(parent, s, column, row, cSpan, rSpan);
}
    
static inline
void RemoveChildSurfaceList(struct PnSurface *parent, struct PnSurface *s) {

    if(s->pl.nextSibling) {
        DASSERT(parent->l.lastChild != s);
        s->pl.nextSibling->pl.prevSibling = s->pl.prevSibling;
    } else {
        DASSERT(parent->l.lastChild == s);
        parent->l.lastChild = s->pl.prevSibling;
    }

    if(s->pl.prevSibling) {
        DASSERT(parent->l.firstChild != s);
        s->pl.prevSibling->pl.nextSibling = s->pl.nextSibling;
        s->pl.prevSibling = 0;
    } else {
        DASSERT(parent->l.firstChild == s);
        parent->l.firstChild = s->pl.nextSibling;
    }

    s->pl.nextSibling = 0;
    s->parent = 0;
}

static inline
void RemoveChildSurfaceGrid(struct PnSurface *parent, struct PnSurface *s) {

    ASSERT(0, "WRITE CODE FOR GRID CASE HERE");
}

static inline
void RemoveChildSurface(struct PnSurface *parent, struct PnSurface *s) {

    DASSERT(s);
    DASSERT(parent);
    DASSERT(s->parent == parent);

    if(parent->layout != PnLayout_Grid)
        RemoveChildSurfaceList(parent, s);
    else
        RemoveChildSurfaceGrid(parent, s);
}

void pnSurface_setDraw(
        struct PnSurface *s,
        int (*draw)(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride,
            void *userData), void *userData) {
    DASSERT(s);

    s->draw = draw;
    s->drawData = userData;
}

// This function calls itself.
//
void pnSurface_draw(struct PnSurface *s, struct PnBuffer *buffer) {

    DASSERT(s);
    DASSERT(!s->culled);
    DASSERT(s->window);

    if(s->window->needAllocate && s->config)
        s->config(s,
                buffer->pixels +
                s->allocation.y * buffer->stride +
                s->allocation.x,
                s->allocation.x, s->allocation.y,
                s->allocation.width, s->allocation.height,
                buffer->stride, s->configData);

    // All parent surfaces, including the window, could have nothing to draw
    // if their children widgets completely cover them.
    if(s->noDrawing) goto drawChildren;

#ifdef WITH_CAIRO
    if(s->cairoDraw) {
        DASSERT(s->cr);
        if(s->cairoDraw(s, s->cr, s->cairoDrawData) == 1)
            pnSurface_queueDraw(s);
    } else if(!s->draw) {
        DASSERT(s->cr);
        uint32_t c = s->backgroundColor;
        cairo_set_source_rgba(s->cr,
               ((0x00FF0000 & c) >> 16)/255.0, // R
               ((0x0000FF00 & c) >> 8)/255.0,  // G
               ((0x000000FF & c))/255.0,       // B
               ((0xFF000000 & c) >> 24)/255.0  // A
        );
        cairo_paint(s->cr);
    }
#else // without Cairo
    if(!s->draw)
        pn_drawFilledRectangle(buffer->pixels,
                s->allocation.x, s->allocation.y, 
                s->allocation.width, s->allocation.height,
                buffer->stride,
                s->backgroundColor /*color in ARGB*/);
#endif
    else
        if(s->draw(s,
                buffer->pixels +
                s->allocation.y * buffer->stride +
                s->allocation.x,
                s->allocation.width, s->allocation.height,
                buffer->stride, s->drawData) == 1)
            pnSurface_queueDraw(s);

drawChildren:

    // Now draw children (widgets).
    if(s->layout != PnLayout_Grid)
        for(struct PnSurface *c = s->l.firstChild; c; c = c->pl.nextSibling) {
            if(!c->culled)
                pnSurface_draw(c, buffer);
        }
    else if(s->g.numChildren) {
        struct PnSurface ***child = s->g.child;
        for(uint32_t y=s->g.numRows-1; y != -1; --y)
            for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
                struct PnSurface *c = child[y][x];
                if(c->culled) continue;
                // rows and columns can share widgets; like for row span N
                // and/or column span N; that is adjacent cells that share
                // the same widget.
                if(c && (!x || c != child[y][x-1]) &&
                        (!y || c != child[y-1][x]))
                    pnSurface_draw(c, buffer);
                }
    }
}

static bool MotionEnter(struct PnSurface *surface,
            uint32_t x, uint32_t y, void *userData) {
    DASSERT(surface);
    return true; // Take focus so we can get motion events.
}

static void MotionLeave(struct PnSurface *surface, void *userData) {
    DASSERT(surface);
}


// Return false on success.
//
// Both window and widget are a surface.  Some of this surface data
// structure is already set up.
//
// column and row are -1 for the unused case.
//
bool InitSurface(struct PnSurface *s, uint32_t column, uint32_t row,
        uint32_t cSpan, uint32_t rSpan) {

    DASSERT(s);
    DASSERT(s->type);

    if(s->layout == PnLayout_Grid) {
        DASSERT(row = s->g.numRows);
        DASSERT(column = s->g.numColumns);
        RecreateGrid(s, column, row);
    }

    if(!s->parent) {
        DASSERT(!(s->type & WIDGET));
        // this is a window.
        return false;
    }
    // this is a widget
    DASSERT(s->type & WIDGET);

    // Default widget background color is that of it's
    // parent.
    s->backgroundColor = s->parent->backgroundColor;

    AddChildSurface(s->parent, s, column, row, cSpan, rSpan);

    return false;
}


void DestroySurface(struct PnSurface *s) {

    DASSERT(s);

#ifdef WITH_CAIRO
    DestroyCairo(s);
#endif

    if(!(s->type & WIDGET)) {
        DASSERT(!s->parent);
        return;
    }
    DASSERT(s->parent);

    RemoveChildSurface(s->parent, s);

    if(s->layout == PnLayout_Grid) {
        // A Grid container surface has memory allocated for it's
        // container 2D array thingy: child[numRows][numColumns]
        struct PnSurface ***child = s->g.child;
        for(uint32_t y=s->g.numRows-1; y != -1; --y) {
            // Free a row array.
            DZMEM(child[y], sizeof(*child[y])*s->g.numColumns);
            free(child[y]);
        }
        DZMEM(child, sizeof(*child)*s->g.numRows);
        free(child);
    }
}


void pnSurface_setConfig(struct PnSurface *s,
        void (*config)(struct PnSurface *surface, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, uint32_t stride/*4 byte chunks*/,
            void *userData), void *userData) {
    DASSERT(s);
    s->config = config;
    s->configData = userData;
}

void pnSurface_getAllocation(const struct PnSurface *s,
        struct PnAllocation *a) {
    DASSERT(s);
    memcpy(a, &s->allocation, sizeof(*a));
}

void pnSurface_setBackgroundColor(
        struct PnSurface *s, uint32_t argbColor) {
    s->backgroundColor = argbColor;
}

uint32_t pnSurface_getBackgroundColor(struct PnSurface *s) {
    DASSERT(s);
    return s->backgroundColor;
}


void pnSurface_setEnter(struct PnSurface *s,
        bool (*enter)(struct PnSurface *surface,
            uint32_t x, uint32_t y, void *userData),
        void *userData) {

    DASSERT(s);
    s->enter = enter;
    s->enterData = userData;

    if(!enter) {
        // Motion, Enter, and Leave are closely related.  See
        // pnSurface_setMotion().
        if(s->motion)
            s->enter = MotionEnter;
    }
}

void pnSurface_setLeave(struct PnSurface *s,
        void (*leave)(struct PnSurface *surface, void *userData),
        void *userData) {

    DASSERT(s);
    s->leave = leave;
    s->leaveData = userData;

    if(!leave) {
        // Motion, Enter, and Leave are closely related.  See
        // pnSurface_setMotion().
        if(s->motion)
            s->leave = MotionLeave;
    }

}

void pnSurface_setPress(struct PnSurface *s,
        bool (*press)(struct PnSurface *surface,
            uint32_t which,
            int32_t x, int32_t y, void *userData),
        void *userData) {

    DASSERT(s);
    s->press = press;
    s->pressData = userData;
}

void pnSurface_setRelease(struct PnSurface *s,
        bool (*release)(struct PnSurface *surface,
            uint32_t which,
            int32_t x, int32_t y, void *userData),
        void *userData) {

    DASSERT(s);
    s->release = release;
    s->releaseData = userData;

    if(!release && d.buttonGrabSurface == s) {
        // If we are removing the release callback (setting to 0), we need
        // to make sure that there is no pending button grab, and if there
        // is a button grab we must release the grab.
        d.buttonGrabSurface = 0;
        d.buttonGrab = 0;
    }
}

void pnSurface_setMotion(struct PnSurface *s,
        bool (*motion)(struct PnSurface *surface,
            int32_t x, int32_t y, void *userData),
        void *userData) {

    DASSERT(s);
    s->motion = motion;
    s->motionData = userData;

    // We need to get focus (from the enter event) if we want motion
    // events.  Enter, Leave, and Motion are closely related; that's just
    // the way things are.

    if(motion) {
        if(!s->enter)
            s->enter = MotionEnter;
        if(!s->leave)
            s->leave = MotionLeave;
    } else {
        if(s->enter == MotionEnter)
            s->enter = 0;
        if(s->leave == MotionLeave)
            s->leave = 0;
    }
}

bool pnSurface_isInSurface(const struct PnSurface *s,
        uint32_t x, uint32_t y) {
    DASSERT(s);
    return (s->allocation.x <= x && s->allocation.y <= y &&
            x < s->allocation.x + s->allocation.width && 
            y < s->allocation.y + s->allocation.height);
}

#if 0
void pnSurface_setMinWidth(struct PnSurface *s, uint32_t width) {
    DASSERT(s);
    if(s->width != width)
        *((uint32_t *) &s->width) = width;

    // TODO: Let the user queue the draw??
}
#endif
