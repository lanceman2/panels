#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "debug.h"
#include  "display.h"



static bool inline HaveChildren(const struct PnSurface *s) {
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

// We use lots of function recursion to get widget positions, sizes, and
// culling.  This code may hurt your head.

static bool ResetChildrenCull(struct PnSurface *s);


// Return true if at least one child is not culled.
//
static inline bool GridResetChildrenCull(struct PnSurface *s) {

    DASSERT(s->layout == PnLayout_Grid);
    DASSERT(s->g.grid->numChildren);
    DASSERT(s->g.numColumns);
    DASSERT(s->g.numRows);
    DASSERT(s->g.grid);
    struct PnSurface ***child = s->g.grid->child;
    DASSERT(child);

    bool culled = true;

    for(uint32_t y=s->g.numRows-1; y != -1; --y)
        for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
            struct PnSurface *c = child[y][x];
            c->culled = ResetChildrenCull(c);

            if(!c->culled && culled)
                // We have at least one child not culled and the
                // container "s" is not hidden.
                culled = false;
            // We will set all children's children.
        }

    return culled;
}


// This function calls itself.
//
// This culling got with this PnSurface::culled flag is just effecting the
// showing (and not showing) of widgets: due to the window size not being
// large enough, or the panel's API (application programming interface)
// user hiding the widget.
//
// If a parent is culled: then we do not need to think about culling
// the children, they are implied to be culled without setting the
// "culled" flag for the children.
//
// Returns true if "s" is culled.
//
// Here we are resetting the culled flags so we just use the hidden
// flags until after we compare widget sizes with the window size.
//
// PnSurface::culled is set when we are allocating widget sizes and
// positions.
//
static bool ResetChildrenCull(struct PnSurface *s) {

    bool culled = s->hidden;

    if(culled || !HaveChildren(s))
        return culled;

    // "s" is not culled yet, but it may get culled if no children
    // are not culled.

    culled = true;
    // It's culled unless a child is not culled.

    switch(s->layout) {

        case PnLayout_Grid:

            return GridResetChildrenCull(s);

        default:
            for(struct PnSurface *c = s->l.firstChild; c;
                    c = c->pl.nextSibling) {
                c->culled = ResetChildrenCull(c);

                if(!c->culled && culled)
                    // We have at least one child not culled and the
                    // container "s" is not hidden.
                    culled = false;
                // We will set all children's children.
            }
            return culled;
    }
}


// This expandability attribute passes from child to parent, unlike
// culling that works from parent to child.  That is, if a child can
// expand than so can its parent (unrelated to the parent container
// surface user set expand flag).
//
// The parent container surface user set expand flag is ignored until the
// surface becomes a non-container widget (with no children, a leaf).
//
// "panels" does not so much distinguish between container widgets and
// non-container widgets.  Panels widgets can change between being a
// container and non-container widget just by removing or adding child
// widgets from them, without recreating the widget.
//
// Note: a container widget does not change to being a leaf widget by
// having all its children culled.  If a container widget has all
// its children culled, then the container will be culled too.
//
// Think of PnSurface::canExpand as a temporary flag that is only used
// when we are allocating widget sizes and positions.
//
// Note: enum PnExpand is a bits flag with two bits, it's not a bool.
// A widget can't expand if no bits are set in the "expand" flag.
//
// Returns the temporary correct canExapnd attribute of "s".
//
static uint32_t ResetCanExpand(struct PnSurface *s) {

    DASSERT(s);
    DASSERT(!s->culled);

    if(HaveChildren(s))
        // We may add changes to this later:
        s->canExpand = s->expand;
    else {
        // This is a leaf node.  We are at an end of the function call
        // stack.  s->expand is a user set attribute.
        s->canExpand = s->expand;
        return s->canExpand;
    }

    // Now "s" has children.

    if(s->layout == PnLayout_Grid) {

        ASSERT(0, "WRITE GRID CASE");

        return s->canExpand;
    }

    // TODO: deal with containers that are not PnLayout_LR,
    // PnLayout_RL, PnLayout_TB, or PnLayout_BT.
    ASSERT(s->layout >= PnLayout_LR ||
            s->layout <= PnLayout_BT, "WRITE MORE CODE");

    for(struct PnSurface *c = s->l.firstChild; c; c = c->pl.nextSibling) {
        if(c->culled)
            // Skip culled widgets.
            continue;
        // If any of the children can expand than "s" can expand, so long
        // as there's no culling to get to them.
        s->canExpand |= ResetCanExpand(c);
    }

    // Return if we have any leaf children that can expand.
    return s->canExpand;
}


// This lets us have container widgets (and windows) with zero width
// and/or height.  We only let it be zero if it's a container (has
// children).  If it's a leaf widget we force it to have non-zero width.
//
// If it's a container widget (surface), get this left and right widget
// border width, else if it's a leaf widget (surface) return the minimum
// width of the widget (surface).
//
static inline uint32_t GetBWidth(const struct PnSurface *s) {

    if(HaveChildren(s) || s->width)
        // It can be zero.
        return s->width;

    if(s->type & WIDGET)
        return PN_DEFAULT_WIDGET_WIDTH;

    DASSERT(s->type == PnSurfaceType_popup ||
            s->type == PnSurfaceType_toplevel);
    return PN_DEFAULT_WINDOW_WIDTH;
}

// Get this upper (lower) widget border width, of a container.  If it's a
// leaf widget we force it to have non-zero height.
//
// If it's a container widget (surface), get this upper and lower widget
// border width, else if it's a leaf widget (surface) this returns the
// minimum height of the widget.

static inline uint32_t GetBHeight(const struct PnSurface *s) {

    if(HaveChildren(s) || s->height)
        // It can be zero.
        return s->height;

    if(s->type & WIDGET)
        return PN_DEFAULT_WIDGET_HEIGHT;

    DASSERT(s->type == PnSurfaceType_popup ||
            s->type == PnSurfaceType_toplevel);
    return PN_DEFAULT_WINDOW_HEIGHT;
}


// This function calls itself.  I don't think we'll blow the function call
// stack here.  How many layers of widgets do we have?
//
// A not showing widget has it's "culling" flag set to true (before this
// function call), if not it's false (before this function call).
//
static
void TallyRequestedSizes(const struct PnSurface *s,
        struct PnAllocation *a) {

    DASSERT(!s->culled);

    // We start at 0 and sum from there.
    //
    a->width = 0;
    a->height = 0;
    a->x = 0;
    a->y = 0;

    uint32_t borderX = GetBWidth(s);
    uint32_t borderY = GetBHeight(s);
    bool gotOne = false;
    struct PnSurface *c;

    switch(s->layout) {
        case PnLayout_None:
            break;
        case PnLayout_One:
            DASSERT((!s->l.firstChild && !s->l.lastChild)
                    || s->l.firstChild == s->l.lastChild);
        case PnLayout_LR:
        case PnLayout_RL:
            for(c = s->l.firstChild; c; c = c->pl.nextSibling) {
                if(c->culled) continue;
                if(!gotOne) gotOne = true;
                TallyRequestedSizes(c, &c->allocation);
                // Now we have all "s" children and descendent sizes.
                a->width += c->allocation.width + borderX;
                if(a->height < c->allocation.height)
                    a->height = c->allocation.height;
            }
            if(gotOne)
                a->height += borderY;
            break;
        case PnLayout_BT:
        case PnLayout_TB:
            for(c = s->l.firstChild; c; c = c->pl.nextSibling) {
                if(c->culled) continue;
                if(!gotOne) gotOne = true;
                TallyRequestedSizes(c, &c->allocation);
                // Now we have all "s" children and descendent sizes.
                a->height += c->allocation.height + borderY;
                if(a->width < c->allocation.width)
                    a->width = c->allocation.width;
            }
            if(gotOne)
                a->width += borderX;
            break;
        case PnLayout_Grid: {
            // Good luck following this shit.  I think it's inherently
            // complex.
            //
            // There is not an obvious way to distribute the space in the
            // grid cells for the case when widgets can span more than one
            // cell space in the grid.  This is a simple way to do it.  We
            // ignore the cell space of the multi-cell widget until we
            // get to tallying the cell where the multi-cell widget
            // starts.  The !IsUpperLeftCell() ignores the widget until it
            // is in the upper and left corner of the widget is where the
            // cell we are testing is.  This will tend to make the cells
            // farthest from the upper left corner smaller, but the total
            // container grid size is obvious and unique (the distribution
            // of space in each cell in the grid is not obvious).  This
            // seems to be the simplest way to do it.
            struct PnSurface ***child = s->g.grid->child;
            for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
                uint32_t cellHeight = 0;
                for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                    c = child[yi][xi];
                    if(!c || c->culled) continue;
                    if(!IsUpperLeftCell(c, child, xi, yi)) continue;
                    TallyRequestedSizes(c, &c->allocation);
                    // Now we have all "c" children and descendent sizes.
                    // Find the tallest child.
                    uint32_t h = c->allocation.height;
                    DASSERT(h);
                    uint32_t y = yi + 1;
                    while(y < s->g.numRows && child[y][xi] == c) {
                        // This, "c", is the same widget as the widget
                        // below in this column; that is, this widget "c"
                        // spans in the vertical direction, so we remove
                        // the last rows height if we can.
                        if(h > s->g.grid->heights[y])
                            h -= s->g.grid->heights[y];
                        else {
                            // This widget will not contribute to the
                            // height, because it's too small to need more
                            // height than what was in row yi without it.
                            h = 0;
                            break;
                        }
                        ++y;
                    }
                    if(cellHeight < h)
                        cellHeight = h;
                }
                // Record the height of the row, which could be zero
                // if there were no widgets found in this row.
                s->g.grid->heights[yi] = cellHeight;

                if(cellHeight)
                    a->height += borderY + cellHeight;
            }
            for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                uint32_t cellWidth = 0;
                for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
                    c = child[yi][xi];
                    if(!c || c->culled) continue;
                    if(!IsUpperLeftCell(c, child, xi, yi)) continue;
                    // We already called TallyRequestedSizes(c,) in the
                    // above double for() loop.
                    uint32_t w = c->allocation.width;
                    DASSERT(w);
                    uint32_t x = xi + 1;
                    while(x < s->g.numColumns && child[yi][x] == c) {
                        // This, "c", is the same widget as the widget
                        // to the right; that is, this widget "c"
                        // spans in the horizontal direction, so we remove
                        // the last columns width if we can.
                        if(w > s->g.grid->widths[x])
                            w -= s->g.grid->widths[x];
                        else {
                            // This widget will not contribute to the
                            // width, because it's too small to need more
                            // width than what was in column xi without it.
                            w = 0;
                            break;
                        }
                        ++x;
                    }
                    if(cellWidth < w)
                        cellWidth = w;
                }
                // Record the width of the column, which could be zero
                // if there were no widgets found in this column.
                s->g.grid->widths[xi] = cellWidth;

                if(cellWidth)
                    a->width += borderX + cellWidth;
            }
            break;
        }
        default:
    }

    // Add the last border or first size if there are no children.
    a->width += borderX;
    a->height += borderY;
}

// At this point we have the widths and heights of all widgets
// (surfaces) for the case where the window is shrink wrapped.
//
// The surface "s" size and position is correct at this point.  The
// children's size is correct.
//
// Just getting the children's x, y.
//
static void GetChildrenXY(const struct PnSurface *s,
        const struct PnAllocation *a) {

    DASSERT(!s->culled);
    DASSERT(HaveChildren(s));
    DASSERT(a->width, "s->type=%d", s->type);
    DASSERT(a->height);

    uint32_t borderX = GetBWidth(s);
    uint32_t borderY = GetBHeight(s);
    uint32_t x = a->x + borderX;
    uint32_t y = a->y + borderY;

    struct PnSurface *c;

    switch(s->layout) {

        case PnLayout_One:
        case PnLayout_TB:
            for(c = s->l.firstChild; c; c = c->pl.nextSibling) {
                if(c->culled) continue;
                c->allocation.x = x;
                c->allocation.y = y;
                if(c->l.firstChild)
                    GetChildrenXY(c, &c->allocation);
                y += c->allocation.height + borderY;
            }
            break;

        case PnLayout_BT:
            for(c = s->l.lastChild; c; c = c->pl.prevSibling) {
                if(c->culled) continue;
                c->allocation.x = x;
                c->allocation.y = y;
                if(c->l.firstChild)
                    GetChildrenXY(c, &c->allocation);
                y += c->allocation.height + borderY;
            }
            break;

        case PnLayout_LR:
            for(c = s->l.firstChild; c; c = c->pl.nextSibling) {
                if(c->culled) continue;
                c->allocation.x = x;
                c->allocation.y = y;
                if(c->l.firstChild)
                    GetChildrenXY(c, &c->allocation);
                x += c->allocation.width + borderX;
            }
            break;

        case PnLayout_RL:
            for(c = s->l.lastChild; c; c = c->pl.prevSibling) {
                if(c->culled) continue;
                c->allocation.x = x;
                c->allocation.y = y;
                if(c->l.firstChild)
                    GetChildrenXY(c, &c->allocation);
                x += c->allocation.width + borderX;
            }
            break;

        case PnLayout_Grid: {
            struct PnSurface ***child = s->g.grid->child;
            for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
                x = a->x + a->width; // x max, end of row
                for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                    c = child[yi][xi];
                    if(c->culled) continue;
                    if(!IsUpperLeftCell(c, child, xi, yi)) continue;
                    x -= (c->allocation.width + borderX);
                    c->allocation.x = x;
                }
            }
            for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                y = a->y + a->height; // y max, bottom of grid
                for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
                    c = child[yi][xi];
                    if(c->culled) continue;
                    if(!IsUpperLeftCell(c, child, xi, yi)) continue;
                    y -= (c->allocation.height + borderY);
                    c->allocation.y = y;
                }
            }
            break;
        }

        case PnLayout_None:
        default:
            ASSERT(0);
            break;
    }
}


static uint32_t ClipOrCullChildren(const struct PnSurface *s,
        const struct PnAllocation *a);

#define GOT_CULL            (01)
#define NO_SHOWING_CHILD    (GOT_CULL|02)


// Returns true or false  // had a cull or not
//
// true -> c or a descendent was culled
// false -> nothing was culled
//
static inline bool RecurseCullX(
        /*parent space allocation*/
        const struct PnAllocation *a,
        struct PnSurface *c, struct PnAllocation *ca) {

    DASSERT(c->layout != PnLayout_Grid);
    DASSERT(!c->culled);

    if(ca->x > a->x + a->width) {
        // The inner edge does not even fit.
        // Cull this whole widget.
        c->culled = true;
        return true;
    }

    if(ca->x + ca->width > a->x + a->width) {
        // The outer edge does not fit.

        if(!c->l.firstChild) {
            // This is a leaf that does not fit so we just cull it.
            c->culled = true;
            return true;
        }
        // We may cull some of this widget.
        // First make c fit.
        ca->width = a->x + a->width - ca->x;

        uint32_t ret = ClipOrCullChildren(c, ca);

        // This child "c" may not be culled if it has children that fit.
        if(ret & GOT_CULL) {
            // We culled some children from "c".
            if(ret == NO_SHOWING_CHILD)
                // "c" has no children showing any more so cull it.
                c->culled = true;
            //else: "c" has children that still fit.
            //      We had a culling of some of the children of "c".

            return true;

        } //else {
            // "c" had no children culled, so we must have just trimmed
            // the end border of "c".  So this is not a culling, it's just
            // a container border trimming.  We'll come here again if cull
            // searching the other layout (Y) makes this loop again.
            // }
    }

    return false; // no culling.
}

// Returns true or false  // had a cull or not
//
// true -> c or a descendent was culled
// false -> nothing was culled
//
static inline bool RecurseCullY(
        /*parent space allocation*/
        const struct PnAllocation *a,
        struct PnSurface *c, struct PnAllocation *ca) {

    DASSERT(c->layout != PnLayout_Grid);
    DASSERT(!c->culled);

    if(ca->y > a->y + a->height) {
        // The inner edge does not even fit.
        // Cull this whole widget.
        c->culled = true;
        return true;
    }

    if(ca->y + ca->height > a->y + a->height) {
        // The outer edge does not fit.

        if(!c->l.firstChild) {
            // This is a leaf that does not fit so we just cull it.
            c->culled = true;
            return true;
        }

        // We may cull some of this widget.
        // First make c fit.
        ca->height = a->y + a->height - ca->y;

        uint32_t ret = ClipOrCullChildren(c, ca);

        // This child "c" may not be culled if it has children that fit.
        if(ret & GOT_CULL) {
            // We culled some children from "c".
            if(ret == NO_SHOWING_CHILD)
                // "c" has no children showing any more so cull it.
                c->culled = true;
            //else: "c" has children that still fit.
            //      We had a culling of some of the children of "c".

            return true;

        } //else {
            // "c" had no children culled, so we must have just trimmed
            // the end border of "c".  So this is not a culling, it's just
            // a container border trimming.  We'll come here again if cull
            // searching the other layout (X) makes this loop again.
            // }
    }

    return false; // no culling.
}

// For when the layout of the packing for the container of "c"
// is vertical.
//
static inline
uint32_t CullX(const struct PnAllocation *a, struct PnSurface *c) {

    DASSERT(c);
    DASSERT(c->parent);
    DASSERT(c->parent->layout == PnLayout_TB ||
            c->parent->layout == PnLayout_BT);

    bool haveChildShowing = false;
    uint32_t haveCullRet = 0;

    // Now cull in the x layout.
    for(; c; c = c->pl.nextSibling) {
        if(c->culled) continue;
        if(RecurseCullX(a, c, &c->allocation))
            haveCullRet |= GOT_CULL;
        if(!c->culled)
            // At least one child still shows.
            haveChildShowing = true;
    }
    if(!haveChildShowing) return NO_SHOWING_CHILD;
    return haveCullRet;
}

// For when the layout of the packing for the container of "c"
// is horizontal.
//
static inline
uint32_t CullY(const struct PnAllocation *a, struct PnSurface *c) {

    DASSERT(c);
    DASSERT(c->parent);
    DASSERT(c->parent->layout == PnLayout_LR ||
            c->parent->layout == PnLayout_RL);

    bool haveChildShowing = false;
    uint32_t haveCullRet = 0;

    // Now cull in the y layout.
    for(; c; c = c->pl.nextSibling) {
        if(c->culled) continue;
        if(RecurseCullY(a, c, &c->allocation))
            haveCullRet |= GOT_CULL;
        if(!c->culled)
            // At least one child still shows.
            haveChildShowing = true;
    }
    if(!haveChildShowing) return NO_SHOWING_CHILD;
    return haveCullRet;
}


// Cull widgets that do not fit. This is the hard part. If we (newly) cull
// a widget we have to go back and reposition all widgets.  This window
// can have arbitrarily complex (can of worms) shapes.  Maybe after
// culling each widget we go up just one parent level and reshape the
// widgets; and iterate like that until there are no more new culls in the
// tree traversal.
//
// Returns 0 or GOT_CULL or NO_SHOWING_CHILD
//
//
// Returns true if there is a descendent of "s" is trimmed or culled.
//
// // Clip borders and cull until we can't cull.
//
// So, if there is a new cull in a parent node we return (pop the
// ClipOrCullChildren() call stack).  If we cull a leaf, we keeping
// clipping and culling that family of all leaf children until we can't
// cull and then pop the ClipOrCullChildren() call stack.
//
// Parent widgets without any showing children are culled as we pop the
// function call stack.
//
// The windows widgets are no longer shrink wrapped.
//
// This function indirectly calls itself.
//
static uint32_t ClipOrCullChildren(const struct PnSurface *s,
        const struct PnAllocation *a) {

    DASSERT(!s->culled);
    DASSERT(HaveChildren(s));

    struct PnSurface *c; // child iterator.

    // Initialize return value.
    uint32_t haveCullRet = 0;


    switch(s->layout) {

        case PnLayout_One:
        case PnLayout_TB:
            haveCullRet = CullX(a, s->l.firstChild);
            if(haveCullRet) return haveCullRet;
            c = s->l.lastChild;
            while(c && c->culled)
                c = c->pl.prevSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = RecurseCullY(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->pl.prevSibling) {
                c = c->pl.prevSibling;
                if(!c->culled)
                    RecurseCullY(a, c, &c->allocation);
            }
            if(c->culled) haveCullRet = NO_SHOWING_CHILD;
            return haveCullRet;

        case PnLayout_BT:
            haveCullRet = CullX(a, s->l.firstChild);
            if(haveCullRet) return haveCullRet;
            c = s->l.firstChild;
            while(c && c->culled)
                c = c->pl.nextSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = RecurseCullY(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->pl.nextSibling) {
                c = c->pl.nextSibling;
                if(!c->culled)
                    RecurseCullY(a, c, &c->allocation);
            }
            if(c->culled) haveCullRet = NO_SHOWING_CHILD;
            return haveCullRet;

        case PnLayout_LR:
            haveCullRet = CullY(a, s->l.firstChild);
            if(haveCullRet) return haveCullRet;
            c = s->l.lastChild;
            while(c && c->culled)
                c = c->pl.prevSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = RecurseCullX(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->pl.prevSibling) {
                c = c->pl.prevSibling;
                if(!c->culled)
                    RecurseCullX(a, c, &c->allocation);
            }
            if(c->culled) haveCullRet = NO_SHOWING_CHILD;
            return haveCullRet;

        case PnLayout_RL:
            haveCullRet = CullY(a, s->l.firstChild);
            if(haveCullRet) return haveCullRet;
            c = s->l.firstChild;
            while(c && c->culled)
                c = c->pl.nextSibling;
            // We must have at least one child in this widget container,
            // otherwise it would be culled.
            DASSERT(c);
            haveCullRet = RecurseCullX(a, c, &c->allocation) ? GOT_CULL : 0;
            while(c->culled && c->pl.nextSibling) {
                c = c->pl.nextSibling;
                if(!c->culled)
                    RecurseCullX(a, c, &c->allocation);
            }
            if(c->culled) haveCullRet = NO_SHOWING_CHILD;
            return haveCullRet;

        case PnLayout_Grid:
            ASSERT(0, "MORE CODE HERE");
            break;

        case PnLayout_None:
        default:
            ASSERT(0, "s->type=%d s->layout=%d", s->type, s->layout);
            break;
    }

    // We should not get here.
    ASSERT(0);
    return 0;
}

static inline
struct PnSurface *Next(struct PnSurface *s) {
    DASSERT(s);
    return s->pl.nextSibling;
}

static inline
struct PnSurface *Prev(struct PnSurface *s) {
    DASSERT(s);
    return s->pl.prevSibling;
}


static inline
void AlignXJustified(struct PnSurface *first,
        struct PnSurface *(*next)(struct PnSurface *c),
        uint32_t extraWidth) {

    DASSERT(first);
    DASSERT(extraWidth);

    uint32_t num = 0;
    struct PnSurface *c;

    for(c = first; c; c = next(c))
        if(!c->culled)
            ++num;
    DASSERT(num);

    if(num == 1) {
        extraWidth /= 2;
        for(c = first; c; c = next(c))
            if(!c->culled) {
                first->allocation.x += extraWidth;
                break;
            }
        return;
    }

    uint32_t delta = extraWidth/(num - 1);
    uint32_t end = extraWidth - delta * (num - 1);
    uint32_t i = 0;

    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        --num;
        c->allocation.x += i;
        if(!num && end)
            c->allocation.x += end;
        i += delta;
    }
}

static inline void AlignYJustified(struct PnSurface *first,
        struct PnSurface *(*next)(struct PnSurface *c),
        uint32_t extraHeight) {

    DASSERT(first);
    DASSERT(extraHeight);

    uint32_t num = 0;
    struct PnSurface *c;

    for(c = first; c; c = next(c))
        if(!c->culled)
            ++num;
    DASSERT(num);

    if(num == 1) {
        extraHeight /= 2;
        for(c = first; c; c = next(c))
            if(!c->culled) {
                first->allocation.y += extraHeight;
                break;
            }
        return;
    }

    uint32_t delta = extraHeight/(num - 1);
    uint32_t end = extraHeight - delta * (num - 1);
    uint32_t i = 0;

    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        --num;
        c->allocation.y += i;
        if(!num && end)
            c->allocation.y += end;
        i += delta;
    }
}


// Expand children of "s" while taking space to the right.
//
// Shrink a window and if that program lets you (shrink it) you'll see it
// culls widgets to the right and bottom; on any common operating system.
// That's why (or consistent with) +x is to the right and +y is in the
// down direction.
//
// This will pad (border) the right edge by GetBWidth(s).  If it can't pad
// the right edge (put the container full right border in), it will not
// change anything, except maybe the x positions.
//
// For a horizontal container widget "trimming" is when the window is not
// large enough to show all the right border of the container widget.
// Leaf widgets do not get trimmed, if they do not have their minimum
// required size they get culled.  Culling happens before this function
// call.
//
// So now child positions are no longer valid and must be computed from
// the parent "s" positions in "a".  The child widths may increase here,
// but do not decrease.  Hence the term Expand.
//
// And, the surface "s" positions and sizes, "a", are correct and hence
// constant in this function prototype.  "Correct" in this case, is only a
// matter of how we define it here, correct is relative to this context,
// whatever the hell that is.
//
static inline
void ExpandHShared(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnSurface *first,
        struct PnSurface *(*next)(struct PnSurface *s)) {

    DASSERT(first);
    DASSERT(next);
    DASSERT(s);
    DASSERT(a == &s->allocation);

    uint32_t border = GetBWidth(s);
    uint32_t numExpand = 0;
    uint32_t neededWidth = 0;
    struct PnSurface *c;
    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        neededWidth += border + c->allocation.width;
        if(c->canExpand & PnExpand_H)
            ++numExpand;
    }

    // When we expand we do not fill the container right edge border.
    // But, it may be filled in a little already, via what we called
    // "trimming".

    // It may be that none of the widgets increase their width because
    // surface "s" is squeezing them to their limit; but we still need to
    // recalculate the children x positions.

    // We will change these children and then go on to expand these
    // children's children.

    uint32_t extra = 0;
    if(neededWidth + border < a->width)
        extra = a->width - (neededWidth + border);
    uint32_t padPer = 0;
    if(numExpand)
        padPer = extra/numExpand;
    // endPiece is the leftover pixels from extra not being a integer
    // multiple of padPer.  It's added to the last widget width.
    uint32_t endPiece = extra - numExpand * padPer;
    // The starting left side widget has the smallest x position.
    uint32_t x = a->x;

    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        struct PnAllocation *ca = &c->allocation;
        ca->x = (x += border);
        // Add to the width if we can.
        if(c->canExpand & PnExpand_H) {
            ca->width += padPer;
            --numExpand;
            if(!numExpand)
                // endPiece can be zero.
                ca->width += endPiece;
        } // else ca->width does not change.
        x += ca->width;
    }
    DASSERT(x <= a->x + a->width);
    DASSERT(a->x + a->width >= x);

    uint32_t rightBorderWidth = a->x + a->width - x;

    if(s->noDrawing && extra)
        // There will be part of "s" container showing to draw.
        ((struct PnSurface *)s)->noDrawing = false;

    if(!(s->align & PN_ALIGN_X) || rightBorderWidth <= border)
        return;

    // Now we move the children based on the "Align" attribute of "s".
    // The default "align" was PN_ALIGN_X_LEFT which is 0.
    // The container "s" is now aligned left.

    rightBorderWidth -= border;

    DASSERT(!padPer);
    DASSERT(extra);

    switch(s->align & PN_ALIGN_X) {

        case PN_ALIGN_X_RIGHT:
            for(c = first; c; c = next(c)) {
                if(c->culled) continue;
                c->allocation.x += rightBorderWidth;
            }
            break;

        case PN_ALIGN_X_CENTER:
            rightBorderWidth /= 2;
            for(c = first; c; c = next(c)) {
                if(c->culled) continue;
                c->allocation.x += rightBorderWidth;
            }
            break;

        case PN_ALIGN_X_JUSTIFIED:
            AlignXJustified(first, next, rightBorderWidth);
    }
}


// Child positions are no longer valid an must be computed from the parent
// "s" positions in "a" (which are correct).
//
// See comments in and above ExpandHShared().  There's symmetry here.
//
static inline
void ExpandVShared(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnSurface *first,
        struct PnSurface *(*next)(struct PnSurface *s)) {

    DASSERT(first);
    DASSERT(next);
    DASSERT(s);
    DASSERT(a == &s->allocation);

    uint32_t border = GetBHeight(s);
    uint32_t numExpand = 0;
    uint32_t neededHeight = 0;
    struct PnSurface *c;
    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        neededHeight += border + c->allocation.height;
        if(c->canExpand & PnExpand_V)
            ++numExpand;
    }

    // When we expand we do not fill the container bottom edge border.
    // But, it may be filled in a little already, via what we called
    // "trimming".

    // It may be that none of the widgets increase their height because
    // surface "s" is squeezing them to their limit; but we still need to
    // recalculate the children y positions.

    // We will change these children and then go on to expand these
    // children's children.

    uint32_t extra = 0;
    if(neededHeight + border < a->height)
        extra = a->height - (neededHeight + border);
    uint32_t padPer = 0;
    if(numExpand)
        padPer = extra/numExpand;
    // endPiece is the leftover pixels from extra not being a integer
    // multiple of padPer.  It's added to the last widget height.
    uint32_t endPiece = extra - numExpand * padPer;
    // The starting top side widget has the smallest y position.
    uint32_t y = a->y;

    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        struct PnAllocation *ca = &c->allocation;
        ca->y = (y += border);
        // Add to the height if we can.
        if(c->canExpand & PnExpand_V) {
            ca->height += padPer;
            --numExpand;
            if(!numExpand)
                // endPiece can be zero.
                ca->height += endPiece;
        } // else ca->height does not change.
        y += ca->height;
    }
    DASSERT(y <= a->y + a->height);
    DASSERT(a->y + a->height >= y);

    uint32_t bottomBorderHeight = a->y + a->height - y;

    if(s->noDrawing && extra)
        // There will be part of "s" container showing to draw.
        ((struct PnSurface *)s)->noDrawing = false;

    if(!(s->align & PN_ALIGN_Y) || bottomBorderHeight <= border)
        return;

    // Now we move the children based on the "Align" attribute of "s".
    // The default "align" was PN_ALIGN_Y_TOP which is 0.
    // The container "s" is now aligned top.

    bottomBorderHeight -= border;

    DASSERT(!padPer);
    DASSERT(extra);

    switch(s->align & PN_ALIGN_Y) {

        case PN_ALIGN_Y_BOTTOM:
            for(c = first; c; c = next(c)) {
                if(c->culled) continue;
                c->allocation.y += bottomBorderHeight;
            }
            break;

        case PN_ALIGN_Y_CENTER:
            bottomBorderHeight /= 2;
            for(c = first; c; c = next(c)) {
                if(c->culled) continue;
                c->allocation.y += bottomBorderHeight;
            }
            break;

        case PN_ALIGN_Y_JUSTIFIED:
            AlignYJustified(first, next, bottomBorderHeight);
    }
}


// allocation a is the parent allocation and it is correct.
//
// Fix the x and width of ca.
//
static inline
void ExpandH(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnSurface *c, struct PnAllocation *ca,
        uint32_t *rightBorderWidth) {
    DASSERT(a);
    DASSERT(c);
    DASSERT(ca == &c->allocation);
    DASSERT(s);
    DASSERT(s->layout < PnLayout_Grid);
    DASSERT(s->l.firstChild);
    DASSERT(s->l.lastChild);
    DASSERT(&s->allocation == a);
    DASSERT(!c->culled);

    uint32_t border = GetBWidth(s);
    ca->x = a->x + border;

    DASSERT(ca->width);
    DASSERT(a->width >= border + ca->width);

    uint32_t rightBorder = a->width - border - ca->width;
    if(*rightBorderWidth > rightBorder)
        *rightBorderWidth = rightBorder;

    if(!(c->canExpand & PnExpand_H)) {
        // If we have not unset s->noDrawing already and
        // there is part of the "s" showing to draw then.
        if(s->noDrawing && ca->width < a->width)
            // There will be part of "s" container showing to draw.
            ((struct PnSurface *)s)->noDrawing = false;
        return;
    }

    // Now see how to set the width, or not.

    DASSERT(ca->width <= a->width + border);
    DASSERT(a->width > border);
    // The width of "ca" could be correct already, and may have
    // this "correct" size by "trimming", so we do not change it
    // if it already has some width based on this logic.

    if(a->width < 2 * border)
        // "s" was trimmed.  The end container border (at the right edge
        // of the window) was cut; so there is not a possibly of expanding
        // "c".  Only containers get trimmed their right and bottom
        // borders trimmed, leaf widgets get culled (not trimmed).
        return;

    if(ca->width < a->width - 2 * border)
        ca->width = a->width - 2 * border;

    DASSERT(a->width >= border + ca->width);

    rightBorder = a->width - border - ca->width;
    if(*rightBorderWidth > rightBorder)
        *rightBorderWidth = rightBorder;
}

// "s" is the parent of "c".  "a" is the allocation of "s".
// "a" is correct.
//
// Fix the y and height of "ca" which is the allocation of "c".
// "c" fills all the space of "s" in this y direction.
//
static inline
void ExpandV(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnSurface *c, struct PnAllocation *ca,
        uint32_t *bottomBorderHeight) {
    DASSERT(a);
    DASSERT(c);
    DASSERT(ca == &c->allocation);
    DASSERT(s);
    DASSERT(s->layout < PnLayout_Grid);
    DASSERT(s->l.firstChild);
    DASSERT(s->l.lastChild);
    DASSERT(&s->allocation == a);
    DASSERT(!c->culled);

    uint32_t border = GetBHeight(s);
    ca->y = a->y + border;

    DASSERT(ca->height);
    DASSERT(a->height >= border + ca->height);

    uint32_t bottomBorder = a->height - border - ca->height;
    if(*bottomBorderHeight > bottomBorder)
        *bottomBorderHeight = bottomBorder;

    if(!(c->canExpand & PnExpand_V)) {
        // If we have not unset s->noDrawing already and
        // there is part of the "s" showing to draw then.
        if(s->noDrawing && ca->height < a->height)
            // There will be part of "s" container showing to draw.
            ((struct PnSurface *)s)->noDrawing = false;
        return;
    }

    // Now see how to set the height, or not.

    DASSERT(ca->height <= a->height + border);
    DASSERT(a->height > border);
    // The height of "ca" could be correct already, and may have
    // this "correct" size by "trimming", so we do not change it
    // if it already has some height based on this logic.

    if(a->height < 2 * border)
        // "s" was trimmed.  The end border (at the bottom edge of the
        // window) was cut; so there is not a possibly of expanding "c".
        // Only containers get trimmed their right and bottom borders
        // trimmed, leaf widgets get culled (not trimmed).
        return;

    if(ca->height < a->height - 2 * border)
        ca->height = a->height - 2 * border;

    DASSERT(a->height >= border + ca->height);

    bottomBorder = a->height - border - ca->height;
    if(*bottomBorderHeight > bottomBorder)
        *bottomBorderHeight = bottomBorder;
}


// This is a case where we are aligning one widget.
//
static inline
void AlignX(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnAllocation *ca,
        uint32_t endBorder) {

    uint32_t border = GetBWidth(s);

    if(endBorder > border)
        endBorder = border;

    DASSERT(a->width >= border + endBorder + ca->width);

    uint32_t extra = a->width - (border + endBorder + ca->width);
    if(!extra) return;

    switch(s->align & PN_ALIGN_X) {

        case PN_ALIGN_X_RIGHT:
            ca->x += extra;
            break;

        case PN_ALIGN_X_CENTER:
        case PN_ALIGN_X_JUSTIFIED:
            ca->x += extra/2;
        }
}

// This is a case where we are aligning one widget.
//
static inline
void AlignY(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnAllocation *ca,
        uint32_t endBorder) {

    uint32_t border = GetBHeight(s);

    if(endBorder > border)
        endBorder = border;

    DASSERT(a->height >= border + endBorder + ca->height);

    uint32_t extra = a->height - (border + endBorder + ca->height);
    if(!extra) return;

    switch(s->align & PN_ALIGN_Y) {

        case PN_ALIGN_Y_BOTTOM:
            ca->y += extra;
            break;

        case PN_ALIGN_Y_CENTER:
        case PN_ALIGN_Y_JUSTIFIED:
            ca->y += extra/2;
        }
}

// This function calls itself.
//
// At this call "s" has it's position and size set "correctly" and so do
// all the parent levels above "s" (grand parents, great grand parents,
// and so on).
//
// We are changing the position and size of the children of "s", and
// the children's children, the children's children's children, and so
// on.
//
// const struct PnSurface *s  is mostly constant.  Oh well.
//
static void ExpandChildren(const struct PnSurface *s,
        const struct PnAllocation *a) {

    // "s" has a correct allocation, "a".
    //
    // The children has unexpanded widths and heights.

    DASSERT(s);
    DASSERT(HaveChildren(s));
    DASSERT(!s->culled);

    // This will be either the container "s" right edge border padding
    // size, or the bottom edge border padding size (in pixels).
    uint32_t endBorder = UINT32_MAX;

    if(GetBWidth(s) || GetBHeight(s))
        // This container has a top and/or left border showing, so it has
        // non-zero area showing; so it will need some drawing for sure.
        ((struct PnSurface *)s)->noDrawing = false;
    else
        // No showing surface yet, but it may get set later in this
        // function call, in Expand?Shared() or in Expand?() where we
        // check if any part of this container "s" is showing between its
        // children.
        ((struct PnSurface *)s)->noDrawing = true;

    struct PnSurface *c;

    switch(s->layout) {

        case PnLayout_One:
            DASSERT(s->l.firstChild == s->l.lastChild);
        case PnLayout_LR:
            ExpandHShared(s, a, s->l.firstChild, Next);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    ExpandV(s, a, c, &c->allocation, &endBorder);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    AlignY(s, a, &c->allocation, endBorder);
            break;

        case PnLayout_TB:
            ExpandVShared(s, a, s->l.firstChild, Next);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    ExpandH(s, a, c, &c->allocation, &endBorder);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    AlignX(s, a, &c->allocation, endBorder);
            break;

        case PnLayout_BT:
            ExpandVShared(s, a, s->l.lastChild, Prev);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    ExpandH(s, a, c, &c->allocation, &endBorder);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    AlignX(s, a, &c->allocation, endBorder);
            break;

        case PnLayout_RL:
            ExpandHShared(s, a, s->l.lastChild, Prev);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    ExpandV(s, a, c, &c->allocation, &endBorder);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    AlignY(s, a, &c->allocation, endBorder);
            break;

        case PnLayout_Grid: {
            struct PnSurface ***child = s->g.grid->child;
            uint32_t yi=s->g.numRows-1;
            // Expand columns with any extra space.
            uint32_t extraWidth = a->width;
            uint32_t num = 0;
            for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                c = child[yi][xi];
                if(c->culled) continue;
                if(!IsUpperLeftCell(c, child, xi, yi)) continue;
                DASSERT(extraWidth >= c->allocation.width);
                extraWidth -= c->allocation.width;
                ++num;
            }
            // Now extraWidth is the extra width we will distribute.
            uint32_t addPer = 0;
            if(num) addPer = extraWidth/num;
            uint32_t end = extraWidth - num * addPer;

            for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
                    c = child[yi][xi];
                    if(c->culled) continue;
                    if(!IsUpperLeftCell(c, child, xi, yi)) continue;
                    ///////////SHIT CODE BELOW
                    --end;
                }
            }
            break;
        }

        case PnLayout_None:
        default:
            ASSERT(0);
            break;
    }

    if(s->layout == PnLayout_Grid) {
        DASSERT(s->g.grid);
        DASSERT(s->g.grid->child);
        DASSERT(s->g.grid->numChildren);

        ASSERT(0, "WRITE MORE CODE HERE");

        return;
    }

    // Now that all the children of "s" have correct allocations we can
    // expand (fix) the "s" children's children allocations.

    for(c = s->l.firstChild; c; c = c->pl.nextSibling)
        if(!c->culled && c->l.firstChild)
            ExpandChildren(c, &c->allocation);
}

// We must set the positions even if the relative positions do not appear
// to change, because if a grand parent (or greater) moves for a past
// call, these children need to reposition too, even if the alignment in
// this container does not change.  So:
//
// Position the children.
//
// The positions and size of the container is fixed and correct.
//
static inline
void AlignXRight(const struct PnSurface *s,
        const struct PnAllocation *a,
        struct PnSurface *first,
        struct PnSurface *(*next)(struct PnSurface *s)) {

    uint32_t border = GetBWidth(s);
    // childrenWidth is the sum of children width with a border added.
    uint32_t childrenWidth = 0;
    struct PnSurface *c;

    for(c = first; c ; c = next(c)) {
        if(c->culled) continue;
        childrenWidth += c->width + border;
    }
}


// Get the positions and sizes of all the widgets in the window.
//
// This does not allocate any memory.  The "Allocations" part of the name
// it borrowed from the GDK part of the GTK API, which has a rectangular
// space "allocation" class which gets setup when the widgets are
// "allocated" (find the position and size of widgets).
//
// TODO: A better function name may be GetWidgetPositionAndSize().  And
// the struct PnAllocation should be struct PositionAndSize.
//
void GetWidgetAllocations(struct PnWindow *win) {

    DASSERT(win);
    struct PnSurface *s = &win->surface;
    struct PnAllocation *a = &s->allocation;
    DASSERT(s->type == PnSurfaceType_toplevel ||
            s->type == PnSurfaceType_popup);
    DASSERT(!a->x);
    DASSERT(!a->y);
    DASSERT(!s->hidden);
    DASSERT(!s->parent);
    DASSERT(!s->culled);
    DASSERT(win->needAllocate);

    if(!a->width && !HaveChildren(s)) {
        DASSERT(!a->height);
        a->width = GetBWidth(s);
        a->height = GetBHeight(s);
    }

    // We already handled not letting the window resize if the user set
    // the expand flags so it cannot expand, in toplevel.c.
    //
    // TODO: Do this for popup windows in popup.c?
    //

    if(!HaveChildren(s))
        // This is the case where an API user wants to draw on a simple
        // window, without dumb-ass widgets.  Fuck ya!  The main point of
        // this API is to do simple shit like draw pixels.
        //INFO("w,h=%" PRIi32",%" PRIi32, a->width, a->height);
        return;

    // We do many passes through the widget (surface) tree data structure.
    // Each pass does a different thing, read on.  I think it's impossible
    // to do all these calculations in one pass through the widget tree
    // data structure.
    //
    // A simple example of the conundrum we are solving is: a parent
    // container widget can't know it's size until it adds up the sizes of
    // its showing children; but also a parent container widget can't know
    // how many of its children are showing until it knows what its size
    // is.  So, it's kind of a bootstrap like thing.
    //
    // This may not be the most optimal solution, but I can follow it and
    // it seems to work.
    //
    // Yes. No shit, this is a solved problem, but there is no fucking way
    // anyone can extract someone else's solution into a form that codes
    // this shit.  Trying to "reform" a proven given solution is a way
    // harder problem than writing this code from scratch.  That is the
    // nature of software... 
    //
    // So far, I see no performance issues.  Tested thousands of widgets
    // randomly laid out in a window, and randomly varying parameters.
    // See ../tests/rand_widgets.c
    //
    // We count widget tree passes, but note: widget passes can get
    // quicker as widget passes cull and so on.

    ResetChildrenCull(s); // PASS 1

    uint32_t loopCount = 0;

    // We set haveChildShowing after many calculations.
    bool haveChildShowing = false;


    do {
        // Save the width and height.
        uint32_t width = a->width;
        uint32_t height = a->height;

        // This shrink wraps the widgets, only getting the widgets widths
        // and heights, without the culled widgets.
        TallyRequestedSizes(s, a); // PASS 2 plus loop repeats
        // So now a->width and a->height may have changed.

        if(a->width == 0 || a->height == 0) {
            // This has to be the special case where the window has
            // children that are all culled; because we do not allow a
            // window to have zero width or height if it has no
            // child widgets in it.
            DASSERT(s->layout > PnLayout_None);
#ifdef DEBUG
            switch(s->layout) {

                case PnLayout_Grid:
                    ASSERT(0, "WRITE CODE FOR GRID CASE");
                    break;

                default:
                    ASSERT(s->l.firstChild);
                    ASSERT(s->l.firstChild->culled);
                    for(struct PnSurface *c=s->l.firstChild; c;
                            c = c->pl.nextSibling)
                        ASSERT(c->culled);
            }
#endif
            DASSERT(width);
            DASSERT(height);
            if(a->width == 0)
                a->width = width;
            if(a->height == 0)
                a->height = height;
            break;
        }

        // No x and y allocations yet.  We have widths and heights.
        GetChildrenXY(s, a); // PASS 3 plus loop repeats
        // Now we have x and y allocations (and widths and heights).

        // Still shrink wrapped and the top surface width and height are
        // set to the "shrink wrapped" width and height.
        //
        // If this is the first call to GetWidgetAllocations(), width and
        // height will be zero.  The shrink wrapped value of a->width and
        // a->height will always be greater than zero (that's just due to
        // how we define things).   Widget containers can have no space
        // showing, but leaf widgets are required to take up space if they
        // are not culled (for some reason).

        if(width >= a->width && height >= a->height) {
            a->width = width;
            a->height = height;
            // Done culling widgets, we have enough space in the window
            // for all showing widgets.  The window may be a little large
            // now, but after this loop we may expand some of the widgets
            // to fill the extra space in addition to "aligning" them.
            break;

        } else if(width) {
            // If one is set then both width and height are set.
            DASSERT(height);
            a->width = width;
            a->height = height;
        } else {
            // This is the first call to GetWidgetAllocations() for this
            // window and a->width and a->height where both zero at the
            // start of this "do" loop.
            //
            // If one is not set than both are not set.
            DASSERT(!width && !height);
        }

        DASSERT(a->width && a->height);

        // Clip and cull until we can't clip or cull.  After a clip or
        // cull we need to fix the widget packing; shrink wrapping and
        // re-positioning widgets.  Hence this loops until widgets do not
        // change how they are shown.
        //
        // The widgets can have a very complex structure. Basically the
        // window is a pile of worms (widgets).  If we resize or remove a
        // worm we need to shake the worms into a new positions refilling
        // new empty spaces as they come to be.  This looping will
        // converge (terminate) so long as there is a finite number of
        // widgets in the window.  The worst case would be that we cull or
        // resize one widget per loop; but it's likely that many widgets
        // will cull or resize in each loop, if there is any culling.
        //
        // tests/random_widgets.c is a test that should work this loop
        // through its paces.  Try it with 1000 widgets.

        ++loopCount;

        if(loopCount >= 4) {
            // TODO: change this to DSPEW() or remove loopCount.
            INFO("calling ClipOrCullChildren() %" PRIu32 " times",
                    loopCount);
        }

        haveChildShowing = false;
        // It can happen that the window is so small that no widget can
        // fix in it: all the widgets get culled.
        //
        // NOT A FULL PASS through the widget tree.
        if(s->layout < PnLayout_Grid) {
            for(struct PnSurface *c = s->l.firstChild; c;
                    c = c->pl.nextSibling)
                if(!c->culled) {
                    haveChildShowing = true;
                    break;
                }
        } else {
            ASSERT(0, "WRITE CODE FOR GRID CASE");
        }

    } while(haveChildShowing &&
            // PASS 4 plus loop repeats times 3
            ClipOrCullChildren(s, a));

    // TOTAL PASSES (so far) = 1 + 3 * loopCount

    // Recursively resets "canExpand" flags used in ExpandChildren().
    // We had to do widget culling stuff before this stage.
    //
    // PASS 5
    ResetCanExpand(s);

    // Expand widgets that can be expanded.  Note: this only fills in
    // otherwise blank container spaces; but by doing so has to move the
    // child widgets; they push each other around when they expand.
    //
    // PASS 6
    ExpandChildren(s, a);


    // TOTAL PASSES = 3 + 3 * loopCount

    // TODO: Mash more widget (surface) passes together?  I'd expect that
    // would be prone to introducing bugs.  It's currently doing a very
    // ordered logical progression.  Switching the order of any of the
    // operations will break it (without more changes).

    // Reset the "focused" surface and like things.  At this point we may
    // be culling out that "focused" surface.  I'm not sure if there is a
    // case that lets the focused and other marked surfaces stay marked;
    // but if there is, for example, a "focused" surface that just got
    // culled, we need to make it not "focused" now.  A case may be when
    // the panels API users code uses code to do a resize of the window.
    // Very common example may be: the user clicks a widget that closes
    // it.
    ResetDisplaySurfaces();


//INFO("w,h=%" PRIi32",%" PRIi32, a->width, a->height);
}
