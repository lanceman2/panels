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


void DestroySurfaceChildren(struct PnWidget *s) {

    if(s->layout != PnLayout_Grid)
        while(s->l.firstChild)
            pnWidget_destroy((void *) s->l.firstChild);
    else {
        if(!s->g.grid)
            // "s" has no children.
            return;
        struct PnWidget ***child = s->g.grid->child;
        DASSERT(child);
        for(uint32_t y=s->g.numRows-1; y != -1; --y)
            for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
                struct PnWidget *c = child[y][x];
                // rows and columns can share widgets; like for row span 2
                // and column span 2; that is adjacent cells that share
                // the same widget.
                if(IsUpperLeftCell(c, child, x, y))
                    pnWidget_destroy((void *) c);

                child[y][x] = 0;
            }
        DASSERT(!s->g.grid->numChildren);
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
void AddChildSurfaceList(struct PnWidget *parent, struct PnWidget *s) {

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

// This can destroy, create, and reallocate a grid container.
//
// Remember realloc() is mostly your friend.  It copies (keeps) memory
// contents to new memory locations when memory is enlarged.  But, it does
// not zero new memory.
//
// Just choose your numColumns and numRows.  Widgets in removed cells get
// destroyed.  This also handles destroying any child widgets (surfaces)
// in the grid cells that may be destroyed.
//
// Oh what fun.
//
static inline
void RecreateGrid(struct PnWidget *s,
        uint32_t numColumns, uint32_t numRows) {
    DASSERT(s);
    DASSERT(s->layout == PnLayout_Grid);

    // -1. Special Case, the grid was never made.
    ////////////////////////////////////////////////////////////////////
    if(!s->g.grid) {
        if(!numColumns) {
            DASSERT(!numRows);
            // The grid was never used as a grid of widgets.
            return;
        } else {
            DASSERT(numRows);
        }
    }


    if(!s->g.grid) {
        s->g.grid = calloc(1, sizeof(*s->g.grid));
        ASSERT(s->g.grid, "calloc(1,%zu) failed", sizeof(*s->g.grid));
    }

    struct PnWidget ***child = s->g.grid->child;

    ASSERT(s->g.numColumns || !child);
    ASSERT(s->g.numRows || !child);
    ASSERT((numColumns && numRows) || (!numColumns && !numRows));


    // 0. Setup x[], y[], widths[] and heights[] arrays.
    ////////////////////////////////////////////////////////////////////
    if(numColumns && numColumns != s->g.numColumns) {
        DASSERT(numColumns);
        s->g.grid->widths = realloc(s->g.grid->widths,
                numColumns*sizeof(*s->g.grid->widths));
        ASSERT(s->g.grid->widths, "realloc(,%zu) failed",
                numColumns*sizeof(*s->g.grid->widths));

        s->g.grid->x = realloc(s->g.grid->x,
                (numColumns+1)*sizeof(*s->g.grid->x));
        ASSERT(s->g.grid->x, "realloc(,%zu) failed",
                (numColumns+1)*sizeof(*s->g.grid->x));

        // The values in this array do not matter now at this time.
    }
    if(numRows && numRows != s->g.numRows) {
            s->g.grid->heights = realloc(s->g.grid->heights,
                numRows*sizeof(*s->g.grid->heights));
        ASSERT(s->g.grid->heights, "realloc(,%zu) failed",
                numRows*sizeof(*s->g.grid->heights));

        s->g.grid->y = realloc(s->g.grid->y,
                (numRows+1)*sizeof(*s->g.grid->y));
        ASSERT(s->g.grid->y, "realloc(,%zu) failed",
                (numRows+1)*sizeof(*s->g.grid->y));

        // The values in this array do not matter now at this time.
    }

    // 1. Do possible cleanup due to number of rows decreasing.
    ////////////////////////////////////////////////////////////////////
    uint32_t y=s->g.numRows-1;
    for(; y >= numRows && y != -1; --y) {
        // Remove existing row child[y].
        for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
            struct PnWidget *c = child[y][x];
            // rows and columns can share widgets; like for row span 2
            // and column span 3; that is adjacent cells that share
            // the same widget.
            //
            // Note: the element spans may change, if the spans go across
            // cells that are kept and cells that are removed.
            if(IsUpperLeftCell(c, child, x, y))
                pnWidget_destroy(c);
            child[y][x] = 0; // DEBUG memset 0 thingy
        }
        // Remove unneeded row pointers.
        free(child[y]);
        child[y] = 0;
    }

    // 2. Grow or shrink the number of rows with the new numColunms.
    /////////////////////////////////////////////////////////////////
    if(numRows && numRows != s->g.numRows) {

#ifdef DEBUG
        if(s->g.numRows > numRows) {
            // Zero memory that is freed.
            DZMEM(child + numRows,
                    (s->g.numRows - numRows)*sizeof(*child));
        }
#endif
        s->g.grid->child = (child = realloc(child,
                    numRows*sizeof(*child)));
        ASSERT(child, "realloc(,%zu) failed", numRows*sizeof(*child));

        if(s->g.numRows < numRows) {
            // We added rows so:
            // Create new row (child[y]) pointers.  The old row pointers
            // will be fine by using realloc() above.
            for(y=s->g.numRows; y < numRows; ++y) {
                child[y] = calloc(numColumns, sizeof(*child[y]));
                    ASSERT(child[y], "calloc(%" PRIu32 ",%zu) failed",
                            numColumns, sizeof(*child[y]));
                // calloc() zeros memory.
            }
        }
        // If it shrank the number of rows we removed row pointers in 1.
    }

    // 3. Do removing or adding of the columns at the end of existing rows
    //    that will still exist.
    ////////////////////////////////////////////////////////////////////
    // For the remaining rows, shrink or grow the number of columns per
    // row as needed.
    if(numColumns && s->g.numRows &&
            s->g.numColumns && s->g.numColumns != numColumns) {

        DASSERT(numRows);
        // Now we iterate just on the end of pre-existing rows if it needs
        // trimming.
        y = s->g.numRows-1;
        if(y >= numRows-1)
            y = numRows-1;
        for(; y!=-1; --y) {
            for(uint32_t x=s->g.numColumns-1;
                    x>=numColumns && x!=-1; --x) {
                struct PnWidget *c = child[y][x];
                if(IsUpperLeftCell(c, child, x, y))
                    pnWidget_destroy((void *) c);
                child[y][x] = 0; // DEBUG memset 0 thingy
            }

            child[y] = realloc(child[y], numColumns*sizeof(*child[y]));
            ASSERT(child[y], "realloc(,%zu) failed",
                    numColumns*sizeof(*child[y]));
            if(numColumns > s->g.numColumns)
                // Zero the new memory.
                memset(child[y] + s->g.numColumns, 0,
                        (numColumns - s->g.numColumns)*sizeof(*child[y]));
        }
    }

    // 4. Cleanup all the grid rows.
    /////////////////////////////////////////////////////////////////
    if(!numRows) {
        DASSERT(!numColumns);
        DASSERT(child);
        DASSERT(s->g.numRows);
        DASSERT(s->g.numColumns);

        // Free the whole grid.
        DASSERT(s->g.grid->x);
        DASSERT(s->g.numColumns);
        DZMEM(s->g.grid->x, (s->g.numColumns+1)*
                sizeof(*s->g.grid->x));
        free(s->g.grid->x);

        DASSERT(s->g.grid->widths);
        DZMEM(s->g.grid->widths, s->g.numColumns*sizeof(*s->g.grid->widths));
        free(s->g.grid->widths);

        DASSERT(s->g.grid->y);
        DASSERT(s->g.numRows);
        DZMEM(s->g.grid->y, (s->g.numRows+1)*
                sizeof(*s->g.grid->y));
        free(s->g.grid->y);

        DASSERT(s->g.grid->heights);
        DZMEM(s->g.grid->heights, s->g.numRows*sizeof(*s->g.grid->heights));
        free(s->g.grid->heights);


        // All the row pointers child[y] are freed in (1.) above.
        // And all child widgets are destroyed in (1.) above.

        DZMEM(child, s->g.numRows*sizeof(*child));
        free(child);

        DZMEM(s->g.grid, sizeof(*s->g.grid));
        free(s->g.grid);

        s->g.grid = 0;
        s->g.numRows = 0;
        s->g.numColumns = 0;
    } else {
        DASSERT(numColumns);
        DASSERT(numRows);
    }

    // 5. Record the grid size.
    /////////////////////////////////////////////////////////////////
    s->g.numRows = numRows;
    s->g.numColumns = numColumns;
}


static inline
void AddChildSurfaceGrid(struct PnWidget *grid, struct PnWidget *s,
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

    DASSERT(grid->g.grid);
    DASSERT(grid->g.numColumns > column);
    DASSERT(grid->g.numRows > row);
    struct PnWidget ***child = grid->g.grid->child;
    DASSERT(child);

    // The added child can span more cells; so we loop over all the cells
    // it spans.
    //
    for(uint32_t y=row+rSpan-1; y>=row && y!=-1; --y)
        for(uint32_t x=column+cSpan-1; x>=column && x!=-1; --x) {
            struct PnWidget *c = child[y][x];
            if(c)
                // There is a widget in this cell already.
                //
                // This will also remove this widget "c" from the cell and
                // all the cells it spans.
                pnWidget_destroy(c);
            child[y][x] = s;
        }

    ++grid->g.grid->numChildren;

    s->pg.row = row;
    s->pg.column = column;
    s->pg.cSpan = cSpan;
    s->pg.rSpan = rSpan;
}

// Add the child, s, to the parent.
static inline
void AddChildSurface(struct PnWidget *parent, struct PnWidget *s,
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
void RemoveChildSurfaceList(struct PnWidget *parent, struct PnWidget *s) {

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
void RemoveChildSurfaceGrid(struct PnWidget *grid, struct PnWidget *s) {

    DASSERT(grid->g.grid);
    struct PnWidget ***child = grid->g.grid->child;
    DASSERT(child);
    uint32_t numColumns = grid->g.numColumns;
    uint32_t numRows = grid->g.numRows;
    DASSERT(numColumns);
    DASSERT(numRows);
    DASSERT(grid->g.grid->numChildren);
    uint32_t x = s->pg.column;
    uint32_t y = s->pg.row;

    DASSERT(numColumns > x);
    DASSERT(numRows > y);
    DASSERT(child[y][x] == s);

    // There can be any amount of cells that form a rectangle; given that
    // there can be a span (number) of columns and rows (not just a span
    // of 1).

    for(; y<numRows; ++y)
        for(x=s->pg.column; x<numColumns; ++x) {
            if(child[y][x] == s)
                child[y][x] = 0;
            else
                break;
        }

    --grid->g.grid->numChildren;

    // TODO: We could have initialized them to -1 (invalid value).
    //
    s->pg.row = 0;
    s->pg.column = 0;
}


void RemoveChildSurface(struct PnWidget *parent, struct PnWidget *s) {

    DASSERT(s);
    DASSERT(parent);
    DASSERT(s->parent == parent);

    if(parent->layout != PnLayout_Grid)
        RemoveChildSurfaceList(parent, s);
    else
        RemoveChildSurfaceGrid(parent, s);
}


// Return false on success.
//
// Both window and widget are a surface.  Some of this surface data
// structure is already set up.
//
// column and row are -1 for the unused case.
//
void InitSurface(struct PnWidget *s, uint32_t column, uint32_t row,
        uint32_t cSpan, uint32_t rSpan) {

    DASSERT(s);
    DASSERT(s->type);

    if(s->layout == PnLayout_Grid) {
        DASSERT(!s->g.numRows);
        DASSERT(!s->g.numColumns);
        if(column) {
            DASSERT(row);
            RecreateGrid(s, column, row);
        } else
            // The grid can be made and/or resized later when columns and
            // rows are added to add children (widgets) to it.  For now
            // it's just a widget with no children.
            DASSERT(!row);
    }

    if(!s->parent) {
        DASSERT(!(s->type & WIDGET));
        // this is a window.
        return;
    }
    // this is a widget
    DASSERT(s->type & WIDGET);

    // Default widget background color is that of it's
    // parent.
    s->backgroundColor = s->parent->backgroundColor;

    AddChildSurface(s->parent, s, column, row, cSpan, rSpan);
}


void DestroySurface(struct PnWidget *s) {

    DASSERT(s);

#ifdef WITH_CAIRO
    DestroyCairo(s);
#endif

    if(s->type & WIDGET)
        RemoveChildSurface(s->parent, s);

    if(s->layout == PnLayout_Grid)
        RecreateGrid(s, 0, 0);
}

bool pnWidget_isInSurface(const struct PnWidget *w,
        uint32_t x, uint32_t y) {
    DASSERT(w);
    return (w->allocation.x <= x && w->allocation.y <= y &&
            x < w->allocation.x + w->allocation.width && 
            y < w->allocation.y + w->allocation.height);
}
