#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "debug.h"
#include "display.h"


// We use lots of function recursion to get widget positions, sizes, and
// culling.  This code may hurt your head.

static bool ResetChildrenCull(const struct PnWidget *s);


// Return true if at least one child is not culled.
//
static inline bool GridResetChildrenCull(const struct PnWidget *s) {

    DASSERT(s->layout == PnLayout_Grid);
    DASSERT(s->g.grid->numChildren);
    DASSERT(s->g.numColumns);
    DASSERT(s->g.numRows);
    DASSERT(s->g.grid);
    struct PnWidget ***child = s->g.grid->child;
    DASSERT(child);

    bool culled = true;

    for(uint32_t y=s->g.numRows-1; y != -1; --y)
        for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
            struct PnWidget *c = child[y][x];
            if(!c) continue;
            c->culled = ResetChildrenCull(c);

            if(!c->culled && culled)
                // We have at least one child not culled and the
                // container "s" is not hidden.
                culled = false;
            // We will set all children's children.
        }

    return culled;
}


static inline
bool pnList_ResetChildrenCull(const struct PnWidget *s) {

    bool culled = true;

    for(struct PnWidget *c = s->l.firstChild; c;
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


// This function calls itself.
//
// This culling got with this PnWidget::culled flag is just effecting the
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
// PnWidget::culled is set when we are allocating widget sizes and
// positions.
//
static bool ResetChildrenCull(const struct PnWidget *s) {

    bool culled = s->hidden;

    if(culled || !HaveChildren(s))
        return culled;

    // "s" is not culled yet, but it may get culled if no children
    // are not culled.

    // It's culled unless a child is not culled.

    switch(s->layout) {

        case PnLayout_Grid:
            return GridResetChildrenCull(s);

        default:
            return pnList_ResetChildrenCull(s);
    }
}


// This expandability attribute passes from child to parent, unlike
// culling that works from parent to child.  That is, if a child can
// expand than so can its parent (unrelated to the parent container widget
// surface user set expand flag).
//
// The parent container widget user set expand flag is ignored until the
// widget becomes a non-container widget (with no children, a leaf).
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
// Think of PnWidget::canExpand as a temporary flag that is only used
// when we are allocating widget sizes and positions.
//
// Note: enum PnExpand is a bits flag with two bits, it's not a bool.
// A widget can't expand if no bits are set in the "expand" flag.
//
// Returns the temporary correct canExapnd attribute of "s".
//
static uint32_t ResetCanExpand(struct PnWidget *s) {

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
        DASSERT(s->g.grid);
        struct PnWidget ***child = s->g.grid->child;
        DASSERT(child);
        for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
            for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                struct PnWidget *c = child[yi][xi];
                if(!c || c->culled) continue;
                if(!IsUpperLeftCell(c, child, xi, yi)) continue;
                // Call this for every un-culled child in the grid.
                s->canExpand |= ResetCanExpand(c);
            }
        }
        return s->canExpand;
    }

    DASSERT(s->layout >= PnLayout_LR ||
            s->layout <= PnLayout_BT, "WRITE MORE CODE");

    for(struct PnWidget *c = s->l.firstChild; c; c = c->pl.nextSibling) {
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
// Okay: Here's where it gets fucked up; we make it so containers with
// children and layout set to PnLayout_Cover have width and height of
// there children.  The culled flag will not effect this because the
// container and covering children are either all culled or none are
// culled.  The width and heights of all the widgets in a "cover" family
// must all have the same width and height.  For this code to be simple
// the width and height then will be the width and height of the top
// (leaf) child.
//
// If it's a container widget (surface), get this left and right widget
// border width, else if it's a leaf widget (surface) return the minimum
// width of the widget (surface).
//
static inline uint32_t GetBWidth(const struct PnWidget *s) {

    if(GET_WIDGET_TYPE(s->type) == W_SPLITTER)
        // The splitter container has no child separator borders.
        return 0;

    if(s->layout == PnLayout_Cover && HaveChildren(s))
        // The container with PnLayout_Cover can't have borders
        // the are not covered by the children.
        return 0;

    if(HaveChildren(s) || s->reqWidth)
        // It can be zero.  It's just a border that's 0.
        return s->reqWidth;

    if(s->type & WIDGET)
        return PN_MIN_WIDGET_WIDTH;

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
//
static inline uint32_t GetBHeight(const struct PnWidget *s) {

    if(s->layout == PnLayout_Cover && HaveChildren(s))
        // The container with PnLayout_Cover can't have borders
        // the are not covered by the children.
        return 0;

    if(HaveChildren(s) || s->reqHeight)
        // It can be zero. It's just a border that's 0.
        return s->reqHeight;

    if(s->type & WIDGET)
        return PN_MIN_WIDGET_HEIGHT;

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
// This does not get the positions of the widgets.  We must get that
// after we have the sizes.
//
static void TallyRequestedSizes(const struct PnWidget *s,
        struct PnAllocation *a) {

    DASSERT(s);
    DASSERT(!s->culled);
    DASSERT(&s->allocation == a);

    // We start at 0 and sum from there.
    //
    a->width = 0;
    a->height = 0;
    a->x = 0;
    a->y = 0;

    uint32_t borderX = GetBWidth(s);
    uint32_t borderY = GetBHeight(s);
    bool gotOne = false;
    struct PnWidget *c;

    switch(s->layout) {
        case PnLayout_None:
            break;
        case PnLayout_One:
        case PnLayout_Cover:
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
            if(!s->g.grid)
                // The grid is not set up yet.
                break;
            // Good luck following this shit.  I think it's inherently
            // complex.  I suck at writing, but I think the codes okay.
            //
            // There is not an obvious way to distribute the space in the
            // grid cells for the case when widgets can span more than one
            // cell space in the grid.  We ignore the cell space of the
            // multi-cell widget until we get to tallying the cell where
            // the multi-cell widget starts.  The !IsUpperLeftCell()
            // ignores the widget until it is in the upper and left corner
            // of the widget is where the cell we are testing is.  This
            // will tend to make the cells farthest from the upper left
            // corner smaller, but the total container grid size is
            // obvious and unique (the distribution of space in each cell
            // in the grid is not obvious because widgets can span
            // multiple cells).  This seems to be the simplest way to do
            // it.
            struct PnWidget ***child = s->g.grid->child;
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
                        // below in this column xi; that is, this widget
                        // "c" spans in the vertical direction, so we
                        // remove the last rows height if we can.
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
// The widget surface "s" size and position is correct at this point.  The
// children's size is correct.
//
// Just getting the children's x, y.
//
static void GetChildrenXY(const struct PnWidget *s,
        const struct PnAllocation *a) {

    DASSERT(!s->culled);
    DASSERT(HaveChildren(s));
    DASSERT(a->width, "s->type=%d", s->type);
    DASSERT(a->height);

    uint32_t borderX = GetBWidth(s);
    uint32_t borderY = GetBHeight(s);
    uint32_t x = a->x + borderX;
    uint32_t y = a->y + borderY;

    struct PnWidget *c;

    switch(s->layout) {

        case PnLayout_One:
        case PnLayout_Cover:
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
            struct PnWidget ***child = s->g.grid->child;
            DASSERT(child);
            uint32_t *X = s->g.grid->x;
            DASSERT(X);
            uint32_t *Y = s->g.grid->y;
            DASSERT(Y);
            X[s->g.numColumns] = a->x + a->width;
            Y[s->g.numRows] = a->y + a->height;

            for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
                Y[yi] = Y[yi+1];
                if(s->g.grid->heights[yi])
                    Y[yi] -= (borderY + s->g.grid->heights[yi]);
                x = a->x + a->width; // x max, end of row
                for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                    if(s->g.grid->widths[xi])
                        // Go to the x position of the start of the child
                        // widget cell.
                        x -= (borderX + s->g.grid->widths[xi]);
                    c = child[yi][xi];
                    if(!c || c->culled) continue;
                    if(!IsUpperLeftCell(c, child, xi, yi)) continue;
                    // Start with widget aligned left (horizontally).  We
                    // must start aligned left so culling (we do next)
                    // works correctly.  We must do aligning after
                    // culling.
                    DASSERT(s->g.grid->widths[xi] >= c->allocation.width);
                    c->allocation.x = x;
                }
            }
            for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                X[xi] = X[xi+1];
                if(s->g.grid->widths[xi])
                    X[xi] -= (borderX + s->g.grid->widths[xi]);
                y = a->y + a->height; // y max, bottom of grid
                for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
                    if(s->g.grid->heights[yi])
                        // Go to the y position of the start of the child
                        // widget cell.  Remember: y increases down.
                        y -= (borderY + s->g.grid->heights[yi]);
                    c = child[yi][xi];
                    if(!c || c->culled) continue;
                    if(!IsUpperLeftCell(c, child, xi, yi)) continue;
                    // Start with widget aligned up (top vertically).  We
                    // must start aligned top so the culling (we do next)
                    // works correctly.  We must do aligning after
                    // culling.
                    DASSERT(s->g.grid->heights[yi] >= c->allocation.height);
                    c->allocation.y = y;
                    if(HaveChildren(c))
                        GetChildrenXY(c, &c->allocation);
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


static uint32_t ClipOrCullChildren(const struct PnWidget *s,
        const struct PnAllocation *a);


#define GOT_CULL             (01)
// In order to a change to no child showing there must be a clip or cull.
#define TO_NO_SHOWING_CHILD  (GOT_CULL|02)


// Returns 0 or GOT_CULL or TO_NO_SHOWING_CHILD
//
// non-zero -> c or a descendent was culled or clipped
// 0        -> nothing was culled
//
static inline uint32_t RecurseClipOrCullX(
        /*parent space allocation*/
        const struct PnAllocation *a,
        struct PnWidget *c, struct PnAllocation *ca) {

    DASSERT(c->parent->layout != PnLayout_Grid);
    DASSERT(!c->culled);

    if(ca->x >= a->x + a->width) {
        // The inner edge does not even fit.
        // Cull this whole widget.
        c->culled = true;
        if(c->l.firstChild)
            return TO_NO_SHOWING_CHILD;
        return GOT_CULL;
    }

    if(ca->x + ca->width > a->x + a->width) {
        DASSERT(a->x + a->width > ca->x);
        // The outer edge does not fit.

        if(!c->l.firstChild) {
            if(c->clip) {
                // Clip it.
                ca->width = a->x + a->width - ca->x;
                return 0;
            }
            // This is a leaf that does not fit so we just cull it.
            c->culled = true;
            return GOT_CULL;
        }

        // First make c fit.  Clip it.
        ca->width = a->x + a->width - ca->x;
        // We have children in "c".
        uint32_t ret = ClipOrCullChildren(c, ca);
        if(ret == TO_NO_SHOWING_CHILD)
            // "c" has no children showing any more so cull it.
            c->culled = true;
        return ret;
    }

    return 0; // no culling or clipping
}

// Returns 0 or GOT_CULL or TO_NO_SHOWING_CHILD
//
// non-zero -> c or a descendent was culled or clipped
// 0        -> nothing was culled
//
static inline uint32_t RecurseClipOrCullY(
        /*parent space allocation*/
        const struct PnAllocation *a,
        struct PnWidget *c, struct PnAllocation *ca) {

    DASSERT(c->parent->layout != PnLayout_Grid);
    DASSERT(!c->culled);

    if(ca->y >= a->y + a->height) {
        // The inner edge does not even fit.
        // Cull this whole widget.
        c->culled = true;
        if(c->l.firstChild)
            return TO_NO_SHOWING_CHILD;
        return GOT_CULL;
    }

    if(ca->y + ca->height > a->y + a->height) {
        DASSERT(a->y + a->height > ca->y);
        // The outer edge does not fit.

        if(!c->l.firstChild) {
            if(c->clip) {
                // Clip it.
                ca->height = a->y + a->height - ca->y;
                return 0;
            }
            // This is a leaf that does not fit so we just cull it.
            c->culled = true;
            return GOT_CULL;
        }

        // First make c fit.  Clip it.
        ca->height = a->y + a->height - ca->y;
        // We have children in "c".
        uint32_t ret = ClipOrCullChildren(c, ca);
        if(ret == TO_NO_SHOWING_CHILD)
            // "c" has no children showing any more so cull it.
            c->culled = true;
        return ret;
    }

    return 0; // no culling or clipping
}

// For when the layout of the packing for the container of "c"
// is vertical.
//
// Returns 0 or GOT_CULL or TO_NO_SHOWING_CHILD
//
// non-zero -> c or a descendent was culled or clipped
// 0        -> nothing was culled
//
static inline uint32_t
ClipOrCullX(const struct PnAllocation *a, struct PnWidget *c) {

    DASSERT(c);
    DASSERT(!c->culled);
    DASSERT(c->parent);
    DASSERT(c->parent->layout == PnLayout_TB ||
            c->parent->layout == PnLayout_BT ||
            c->parent->layout == PnLayout_One ||
            c->parent->layout == PnLayout_Cover);

    uint32_t ret = 0;
    bool showingChild = false;

    // Now cull in the x layout.
    for(; c; c = c->pl.nextSibling) {
        if(c->culled) continue;
        if(RecurseClipOrCullX(a, c, &c->allocation))
            ret |= GOT_CULL;
        if(!c->culled)
            // At least one child still shows.
            showingChild = true;
    }
    if(showingChild) return ret;
    return TO_NO_SHOWING_CHILD;
}

// For when the layout of the packing for the container of "c"
// is horizontal.
//
// Returns 0 or GOT_CULL or TO_NO_SHOWING_CHILD
//
// non-zero -> c or a descendent was culled or clipped
// 0        -> nothing was culled
//
static inline uint32_t
ClipOrCullY(const struct PnAllocation *a, struct PnWidget *c) {

    DASSERT(c);
    DASSERT(!c->culled);
    DASSERT(c->parent);
    DASSERT(c->parent->layout == PnLayout_LR ||
            c->parent->layout == PnLayout_RL ||
            c->parent->layout == PnLayout_One ||
            c->parent->layout == PnLayout_Cover);

    uint32_t ret = 0;
    bool showingChild = false;

    // Now cull in the y layout.
    for(; c; c = c->pl.nextSibling) {
        if(c->culled) continue;
        if(RecurseClipOrCullY(a, c, &c->allocation))
            ret |= GOT_CULL;
        if(!c->culled)
            // At least one child still shows.
            showingChild = true;
    }

    if(showingChild) return ret;
    return TO_NO_SHOWING_CHILD;
}


// Cull widgets that do not fit. This is the hard part. If we (newly) cull
// a widget we have to go back and reposition all widgets.  This window
// can contain arbitrarily complex shapes, only limited to how random
// rectangles can be packed.
//
// Returns 0 or GOT_CULL or TO_NO_SHOWING_CHILD
//
// Returns true if there is a descendent of "s" is trimmed or culled.
//
// // Basic idea: Clip borders (of container widgets) and cull until we
// can't cull (or clip containers).
//
// At this point in the code all the widgets are pushed to the upper left
// and packed tight; i.e. there is no user selected widget aligning or
// expanding done yet.
//
// So, if there is a new cull or clip in a parent node we return (pop the
// ClipOrCullChildren() call stack).  If we cull a leaf, we keep clipping
// and culling that family of all leaf children until we can't cull and
// then pop the ClipOrCullChildren() call stack.
//
// Parent widgets without any showing children are culled as we pop the
// function call stack.
//
// This function indirectly calls itself.
//
static uint32_t
ClipOrCullChildren(const struct PnWidget *s,
        const struct PnAllocation *a) {

    DASSERT(s);
    DASSERT(&s->allocation == a);
    DASSERT(a->width);
    DASSERT(a->height);
    DASSERT(!s->culled);
    DASSERT(HaveChildren(s));

    struct PnWidget *c; // child iterator.

    // Initialize return value.
    uint32_t ret = 0;


    switch(s->layout) {

        case PnLayout_One:
        case PnLayout_Cover:
        case PnLayout_TB:
            c = s->l.firstChild;
            while(c && c->culled)
                c = c->pl.nextSibling;
            // We must have at least one child in this "s" widget
            // container, otherwise it would be culled.
            DASSERT(c);
            ret = ClipOrCullX(a, c);
            if(ret)
                // We'll be back to clip or cull more.
                return ret;

            c = s->l.lastChild;
            while(c && c->culled)
                c = c->pl.prevSibling;
            // We must have at least one child in this "s" widget
            // container, otherwise it would be culled.
            DASSERT(c);
            ret = RecurseClipOrCullY(a, c, &c->allocation) ?
                    GOT_CULL : 0;
            while(c->culled && c->pl.prevSibling) {
                c = c->pl.prevSibling;
                if(!c->culled)
                    RecurseClipOrCullY(a, c, &c->allocation);
            }
            if(c->culled) ret = TO_NO_SHOWING_CHILD;
            return ret;

        case PnLayout_BT:
            c = s->l.firstChild;
            while(c && c->culled)
                c = c->pl.nextSibling;
            // We must have at least one child in this "s" widget
            // container, otherwise it would be culled.
            DASSERT(c);
            ret = ClipOrCullX(a, c);
            if(ret)
                // We'll be back to clip or cull more.
                return ret;

            c = s->l.firstChild;
            while(c && c->culled)
                c = c->pl.nextSibling;
            // We must have at least one child in this "s" widget
            // container, otherwise it would be culled.
            DASSERT(c);
            ret = RecurseClipOrCullY(a, c, &c->allocation) ?
                    GOT_CULL : 0;
            while(c->culled && c->pl.nextSibling) {
                c = c->pl.nextSibling;
                if(!c->culled)
                    RecurseClipOrCullY(a, c, &c->allocation);
            }
            if(c->culled) ret = TO_NO_SHOWING_CHILD;
            return ret;

        case PnLayout_LR:
            c = s->l.firstChild;
            while(c && c->culled)
                c = c->pl.nextSibling;
            // We must have at least one child in this "s" widget
            // container, otherwise it would be culled.
            DASSERT(c);
            ret = ClipOrCullY(a, c);
            if(ret)
                // We'll be back to clip or cull more.
                return ret;

            c = s->l.lastChild;
            while(c && c->culled)
                c = c->pl.prevSibling;
            // We must have at least one child in this "s" widget
            // container, otherwise it would be culled.
            DASSERT(c);
            ret = RecurseClipOrCullX(a, c, &c->allocation) ?
                    GOT_CULL : 0;
            while(c->culled && c->pl.prevSibling) {
                c = c->pl.prevSibling;
                if(!c->culled)
                    RecurseClipOrCullX(a, c, &c->allocation);
            }
            if(c->culled) ret = TO_NO_SHOWING_CHILD;
            return ret;

        case PnLayout_RL:
            c = s->l.firstChild;
            while(c && c->culled)
                c = c->pl.nextSibling;
            // We must have at least one child in this "s" widget
            // container, otherwise it would be culled.
            DASSERT(c);
            ret = ClipOrCullY(a, c);
            if(ret)
                // We'll be back to clip or cull more.
                return ret;

            c = s->l.firstChild;
            while(c && c->culled)
                c = c->pl.nextSibling;
            // We must have at least one child in this "s" widget
            // container, otherwise it would be culled.
            DASSERT(c);
            ret = RecurseClipOrCullX(a, c, &c->allocation) ?
                    GOT_CULL : 0;
            while(c->culled && c->pl.nextSibling) {
                c = c->pl.nextSibling;
                if(!c->culled)
                    RecurseClipOrCullX(a, c, &c->allocation);
            }
            if(c->culled) ret = TO_NO_SHOWING_CHILD;
            return ret;

        case PnLayout_Grid: {

            // The hard part of this is when we have multi-span rows
            // and/or columns; anytime we cull a multi-span widget we
            // may cause the grid to dramatically change shape.
            //
            // If there are no multi-spanning widgets being cull tested
            // this can be much faster.
            //
            // The grid container must have an un-culled child in a row
            // (column) in order for that row (column) to be drawn.
            struct PnWidget ***child = s->g.grid->child;
            DASSERT(child);
            // At this point "a" is not necessarily consistent with
            // the children.
            uint32_t xMax = a->x + a->width;

            // 1. First cull due to X size.
            ///////////////////////////////////////////////////////////
            //
            for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
                    c = child[yi][xi];
                    if(!c || c->culled) continue;
                    if(c->allocation.x >= xMax) {
                        // No chance to fit.  If it spans more columns it
                        // does not matter (we checked it's starting x
                        // position), we can just cull it and continue
                        // culling more.  All children in this child "c"
                        // are culled too.
                        c->culled = true;
                        ret |= GOT_CULL;
                        continue;
                    }
                    if(c->allocation.x + c->allocation.width > xMax) {
                        bool haveCull = false;
                        // Squish "c".
                        c->allocation.width = xMax - c->allocation.x;
                        // See if it gets widget culls and it should.
                        if(HaveChildren(c)) {
                            haveCull = (ret |=
                                ClipOrCullChildren(c, &c->allocation));
                            DASSERT(ret);
                        }  else if(!c->clip) {
                            // This is a leaf widget.
                            haveCull = c->culled = true;
                            ret |= GOT_CULL;
                        }
                        if(c->pg.cSpan > 1 && haveCull)
                            // We just culled a multi-column spanning
                            // child than we need to finish this culling
                            // pass.  The shape of the whole grid can
                            // change a lot from this.
                            //
                            // Multi-span in grids (tables) are a pain in
                            // the ass.
                            //
                            // TODO: We may add more checks here to see if
                            // the whole grid changed a lot, and if not,
                            // do not return now.  Letting it cull more in
                            // the next culling pass does not make this
                            // wrong, but maybe just a little slower.
                            return ret;
                    }
                }
            }

            // If we got any culls in culling along X we do the Y culling
            // on the next pass, if there are any.
            if(ret) return ret;

            // 2. Second cull due to Y size.
            ///////////////////////////////////////////////////////////
            uint32_t yMax = a->y + a->height;
            //
            for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
                for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                    c = child[yi][xi];
                    if(!c || c->culled) continue;
                    if(c->allocation.y >= yMax) {
                        // No chance to fit.  If it spans more rows it
                        // does not matter (we checked it's starting y
                        // position), we can just cull it and continue
                        // culling more.  All children in this child "c"
                        // are culled too.
                        c->culled = true;
                        ret |= GOT_CULL;
                        continue;
                    }
                    if(c->allocation.y + c->allocation.height > yMax) {
                        bool haveCull = false;
                        // Squish "c".
                        c->allocation.height = yMax - c->allocation.y;
                        // See if it gets widget culls and it should.
                        if(HaveChildren(c)) {
                            haveCull = (ret |=
                                ClipOrCullChildren(c, &c->allocation));
                            DASSERT(ret);
                        } else if(!c->clip) {
                            haveCull = c->culled = true;
                            ret |= GOT_CULL;
                        }
                        if(c->pg.rSpan > 1 && haveCull)
                            // We just culled a multi-row spanning
                            // child than we need to finish this culling
                            // pass.  The shape of the whole grid can
                            // change a lot from this.
                            //
                            // Multi-span in grids (tables) are a pain in
                            // the ass.
                            //
                            // TODO: We may add more checks here to see if
                            // the whole grid changed a lot, and if not,
                            // do not return now.  Letting it cull more in
                            // the next culling pass does not make this
                            // wrong, but maybe just a little slower.
                            return ret;
                    }
                }
            }

            return ret;
        }

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
struct PnWidget *Next(struct PnWidget *s) {
    DASSERT(s);
    return s->pl.nextSibling;
}

static inline
struct PnWidget *Prev(struct PnWidget *s) {
    DASSERT(s);
    return s->pl.prevSibling;
}


static inline
void AlignXJustified(struct PnWidget *first,
        struct PnWidget *(*next)(struct PnWidget *c),
        uint32_t extraWidth) {

    DASSERT(first);
    DASSERT(extraWidth);

    uint32_t num = 0;
    struct PnWidget *c;

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

static inline void AlignYJustified(struct PnWidget *first,
        struct PnWidget *(*next)(struct PnWidget *c),
        uint32_t extraHeight) {

    DASSERT(first);
    DASSERT(extraHeight);

    uint32_t num = 0;
    struct PnWidget *c;

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
// And, the widget surface "s" positions and sizes, "a", are correct and
// hence constant in this function prototype.  "Correct" in this case, is
// only a matter of how we define it here, correct is relative to this
// context, whatever the hell that is.
//
static inline
void ExpandHShared(const struct PnWidget *s,
        const struct PnAllocation *a,
        struct PnWidget *first,
        struct PnWidget *(*next)(struct PnWidget *s)) {

    DASSERT(first);
    DASSERT(next);
    DASSERT(s);
    DASSERT(a == &s->allocation);

    uint32_t border = GetBWidth(s);
    uint32_t numExpand = 0;
    uint32_t neededWidth = 0;
    struct PnWidget *c;
    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        neededWidth += border + c->allocation.width;
        DASSERT(c->allocation.width);
        if(c->canExpand & PnExpand_H)
            ++numExpand;
    }
    // There got to be something, otherwise "s" should have been culled
    // and this would not be called.
    DASSERT(neededWidth);
    neededWidth += border;

    // When we expand we do not fill the container right edge border.
    // But, it may be filled in a little already, via what we called
    // "trimming".

    // It may be that none of the widgets increase their width because
    // widget surface "s" is squeezing them to their limit; but we still
    // need to recalculate the children x positions.

    // We will change these children and then go on to expand these
    // children's children.

    uint32_t extra = 0;
    if(neededWidth < a->width)
        extra = a->width - neededWidth;
    uint32_t padPer = 0;
    uint32_t endPiece = 0;
    if(numExpand) {
        padPer = extra/numExpand;
        // endPiece is the leftover pixels from extra not being a integer
        // multiple of padPer.  We add them one per widget until we run
        // out.
        endPiece = extra - numExpand * padPer;
    }
    // The starting left side widget has the smallest x position.
    uint32_t x = a->x;

    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        struct PnAllocation *ca = &c->allocation;
        ca->x = (x += border);
        // Add to the width if we can.
        if(c->canExpand & PnExpand_H) {
            ca->width += padPer;
            if(endPiece) {
                ++ca->width;
                --endPiece;
            }
        } // else ca->width does not change.
        x += ca->width;
    }
    DASSERT(x <= a->x + a->width);
    DASSERT(a->x + a->width >= x);
    // The end piece should be used up.
    DASSERT(!endPiece);

    uint32_t rightBorderWidth = a->x + a->width - x;

    if(((s->align & PN_ALIGN_X) == PN_ALIGN_X_LEFT) ||
            rightBorderWidth <= border)
        // It's already aligned to the left or there is no extra
        // room to move the widgets into alignment with.
        return;

    // Now we move the children based on the "Align" attribute of "s".
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


// Child positions are no longer valid and must be computed from the parent
// "s" positions in "a" (which are correct).
//
// See comments in and above ExpandHShared().  There's symmetry here.
//
static inline
void ExpandVShared(const struct PnWidget *s,
        const struct PnAllocation *a,
        struct PnWidget *first,
        struct PnWidget *(*next)(struct PnWidget *s)) {

    DASSERT(first);
    DASSERT(next);
    DASSERT(s);
    DASSERT(a == &s->allocation);

    uint32_t border = GetBHeight(s);
    uint32_t numExpand = 0;
    uint32_t neededHeight = 0;
    struct PnWidget *c;
    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        DASSERT(c->allocation.height);
        neededHeight += border + c->allocation.height;
        if(c->canExpand & PnExpand_V)
            ++numExpand;
    }
    // There got to be something, otherwise "s" should have been culled
    // and this would not be called.
    DASSERT(neededHeight);
    neededHeight += border;

    // When we expand we do not fill the container bottom edge border.
    // But, it may be filled in a little already, via what we called
    // "trimming".

    // It may be that none of the widgets increase their height because
    // widget surface "s" is squeezing them to their limit; but we still
    // need to recalculate the children y positions.

    // We will change these children and then go on to expand these
    // children's children.

    uint32_t extra = 0;
    if(neededHeight < a->height)
        extra = a->height - neededHeight;
    uint32_t padPer = 0;
    uint32_t endPiece = 0;
    if(numExpand) {
        padPer = extra/numExpand;
        // endPiece is the leftover pixels from extra not being a integer
        // multiple of padPer.  It's added to the widget height one at a
        // time.
        endPiece = extra - numExpand * padPer;
    }
    // The starting top side widget has the smallest y position.
    uint32_t y = a->y;

    for(c = first; c; c = next(c)) {
        if(c->culled) continue;
        struct PnAllocation *ca = &c->allocation;
        ca->y = (y += border);
        // Add to the height if we can.
        if(c->canExpand & PnExpand_V) {
            ca->height += padPer;
            if(endPiece) {
                ++ca->height;
                --endPiece;
            }
        } // else ca->height does not change.
        y += ca->height;
    }
    DASSERT(y <= a->y + a->height);
    DASSERT(a->y + a->height >= y);
    // The end piece should be used up.
    DASSERT(!endPiece);

    uint32_t bottomBorderHeight = a->y + a->height - y;

    if(((s->align & PN_ALIGN_Y) == PN_ALIGN_Y_TOP) ||
            bottomBorderHeight <= border)
        // It's already aligned to the top or there is no extra
        // room to move the widgets into alignment with.
        return;

    // Now we move the children based on the "Align" attribute of "s".
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
void ExpandH(const struct PnWidget *s,
        const struct PnAllocation *a,
        struct PnWidget *c, struct PnAllocation *ca,
        uint32_t rightBorderWidth) {
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
    // Set the position.
    ca->x = a->x + border;

    DASSERT(ca->width);
    DASSERT(a->width >= border + rightBorderWidth + ca->width);

    if(!(c->canExpand & PnExpand_H))
        return;

    // Set the width.
    ca->width = a->width - border - rightBorderWidth;
}

// "s" is the parent of "c".  "a" is the allocation of "s".
// "a" is correct.
//
// Fix the y and height of "ca" which is the allocation of "c".
// "c" fills all the space of "s" in this y direction.
//
static inline
void ExpandV(const struct PnWidget *s,
        const struct PnAllocation *a,
        struct PnWidget *c, struct PnAllocation *ca,
        uint32_t bottomBorderHeight) {
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
    // Set the position.
    ca->y = a->y + border;

    DASSERT(ca->height);
    DASSERT(a->height >= border + bottomBorderHeight + ca->height);

    if(!(c->canExpand & PnExpand_V))
        return;

    // Set the height.
    ca->height = a->height - border - bottomBorderHeight;
}


// This is a case where we are aligning one widget.
//
static inline
void AlignX(const struct PnWidget *s,
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
void AlignY(const struct PnWidget *s,
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


// Expand the grid to fit in "s" and "a".
//
static inline void ExpandGrid(const struct PnWidget *s,
        const struct PnAllocation *a) {

    DASSERT(s->g.numColumns);
    DASSERT(s->g.numRows);
    DASSERT(s->g.grid);
    DASSERT(s->g.grid->child);
    DASSERT(s->g.grid->numChildren);
    struct PnWidget ***child = s->g.grid->child;
    DASSERT(child);
    uint32_t *widths = s->g.grid->widths;
    uint32_t *heights = s->g.grid->heights;
    DASSERT(widths);
    DASSERT(heights);
    struct PnWidget *c;

    {
        // Fix the grid widths[] due to the grid container being too small
        // to hold all the child widgets; or do nothing.
        uint32_t xMax = a->x + a->width;
        uint32_t x = 0;
        uint32_t xi;
        uint32_t border = GetBWidth(s);
        for(xi=0; xi < s->g.numColumns; ++xi) {
            if(!widths[xi]) continue;
            x += border + widths[xi];
            if(x <= xMax) continue;
            widths[xi] -= x - xMax;
            break;
        }
        for(++xi; xi < s->g.numColumns; ++xi)
            widths[xi] = 0;
    }
    {
        // Fix the grid heights[], due to the grid container being too
        // small to hold all the child widgets; or do nothing.
        uint32_t yMax = a->y + a->height;
        uint32_t y = 0;
        uint32_t yi;
        uint32_t border = GetBHeight(s);
        for(yi=0; yi < s->g.numRows; ++yi) {
            if(!heights[yi]) continue;
            y += border + heights[yi];
            if(y <= yMax) continue;
            heights[yi] -= y - yMax;
            break;
        }
        for(++yi; yi < s->g.numRows; ++yi)
            heights[yi] = 0;
    }


    uint32_t border = GetBWidth(s);
    uint32_t numExpand = 0;
    uint32_t needed = 0;
    uint32_t sectionCount = 0;

    for(uint32_t xi=s->g.numColumns-1; xi!=-1; --xi) {
        if(!widths[xi]) continue;
        // sectionCount is the number of columns that show.
        ++sectionCount;
        // Tally the total width needed.
        needed += border + widths[xi];
        for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
            c = child[yi][xi];
            if(!c || c->culled) continue;
            if(!IsUpperLeftCell(c, child, xi, yi))
                // We have to count each child once.  Elements can span
                // more than one cell, so an element can be in more than
                // one cell.
                continue;
            if(c->canExpand & PnExpand_H) {
                ++numExpand;
                break;
            }
        }
    }

    if(needed)
        needed += border;

    uint32_t extra = 0;
    if(a->width > needed)
        extra = a->width - needed;

    uint32_t padPer = 0;
    uint32_t endPad = 0;

    if(numExpand) {
        padPer = extra/numExpand;
        endPad = extra - padPer * numExpand;
        sectionCount = 0;
    } else if(s->expand & PnExpand_H && sectionCount) {
        // Special case the grid expands but no children expand.
        // We expand all the cells uniformly.
        padPer = extra/sectionCount;
        endPad = extra - padPer * sectionCount;
    } else
        sectionCount = 0;

    uint32_t *X = s->g.grid->x;
    DASSERT(X);
    uint32_t numColumns = s->g.numColumns;

    // At this point we assume that the arrays
    // s->g.grid->widths[] and s->g.grid->heights[] have 
    // correct values for the unexpanded and culled grid.
    // We need to add to all these widths[] and heights[]
    // just to expand by extra = a->width - needed.

    // Start adding space in x:
    //
    uint32_t x = border;

    for(uint32_t xi=0; xi<numColumns; ++xi) {
        X[xi] = x;
        if(!widths[xi])
            // This column is not showing.
            continue;
        // Can this column, xi, expand? 
        bool canExpand = sectionCount?true:false;
        for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
            c = child[yi][xi];
            if(!c || c->culled) continue;
            if(!IsUpperLeftCell(c, child, xi, yi)) continue;
            if(c->canExpand & PnExpand_H) {
                canExpand = true;
                break;
            }
        }
        if(canExpand) {
            widths[xi] += padPer;
            if(endPad) {
                // Add 1 more until endPad is gone.
                ++widths[xi];
                --endPad;
            }
        }
        x += widths[xi] + border;
        if(x > a->x + a->width)
            // The cell was clipped.
            x = a->x + a->width;
    }
    X[numColumns] = x;
    DASSERT(!endPad);

    // Now we have all the widths[] and X[] for the whole grid.
    //
    // Here we set the children x positions and widths.
    // Children that can't expand have a good width already.
    //
    for(uint32_t xi=0; xi<numColumns; ++xi) {
        for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
            c = child[yi][xi];
            if(!c || c->culled) continue;
            if(!IsUpperLeftCell(c, child, xi, yi)) continue;
            // Even if we did not expand this column we still needed this
            // widget "c" x position.
            // We start with the x position of "c" on the left side of the
            // cell.
            c->allocation.x = X[xi];
            if(c->canExpand & PnExpand_H) {
                c->allocation.width = widths[xi];
                DASSERT(c->pg.cSpan);
                for(uint32_t span=c->pg.cSpan-1; span; --span)
                    if(widths[xi+span])
                        c->allocation.width += (widths[xi+span] + border);
            } else { // "c" width does not change.
                // Align it horizontally with the given empty horizontal
                // space in the cell.
                //
                // Let w be the x space available to use for the widget
                // "c".
                uint32_t w = widths[xi];
                // If the widget spans more than one column:
                for(uint32_t span=c->pg.cSpan-1; span; --span)
                    if(widths[xi+span])
                        w += (widths[xi+span] + border);
                // We better have at least enough space to fix the widget
                // in the cell.
                DASSERT(w >= c->allocation.width);
                switch(c->align & PN_ALIGN_X) {
                    case PN_ALIGN_X_CENTER:
                    case PN_ALIGN_X_JUSTIFIED:
                        c->allocation.x = X[xi] + (w - c->allocation.width)/2;
                        break;
                    case PN_ALIGN_X_RIGHT:
                        c->allocation.x = X[xi] + w - c->allocation.width;
                        break;
                }
            }
        }
    }


    // Now do it for y:
    ////////////////////////////////////////////////////////////////////
    border = GetBHeight(s);
    numExpand = 0;
    needed = 0;
    sectionCount = 0;

    for(uint32_t yi=s->g.numRows-1; yi!=-1; --yi) {
        if(!heights[yi]) continue;
        // sectionCount is the number of rows that show.
        ++sectionCount;
        // Tally the total width needed.
        needed += border + heights[yi];
        for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
            c = child[yi][xi];
            if(!c || c->culled) continue;
            if(!IsUpperLeftCell(c, child, xi, yi))
                // We have to count each child once.
                continue;
            if(c->canExpand & PnExpand_V) {
                ++numExpand;
                break;
            }
        }
    }
    if(needed)
        needed += border;

    extra = 0;
    if(a->height > needed)
        extra = a->height - needed;

    padPer = 0;
    endPad = 0;

    if(numExpand) {
        padPer = extra/numExpand;
        endPad = extra - padPer * numExpand;
        sectionCount = 0;
    } else if(s->expand & PnExpand_H && sectionCount) {
        // Special case the grid expands but no children expand.
        // We expand all the cells uniformly.
        padPer = extra/sectionCount;
        endPad = extra - padPer * sectionCount;
    } else
        sectionCount = 0;


    uint32_t *Y = s->g.grid->y;
    DASSERT(Y);
    uint32_t numRows = s->g.numRows;
    //
    // Start adding space in y:
    /////////////////////////////////////////////////////////
    uint32_t y = border;

    for(uint32_t yi=0; yi<numRows; ++yi) {
        Y[yi] = y;
        if(!heights[yi])
            // This column is not showing.
            continue;
        // Can this column, xi, expand? 
        bool canExpand = sectionCount?true:false;
        for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
            c = child[yi][xi];
            if(!c || c->culled) continue;
            if(!IsUpperLeftCell(c, child, xi, yi)) continue;
            if(c->canExpand & PnExpand_V) {
                canExpand = true;
                break;
            }
        }
        if(canExpand) {
            heights[yi] += padPer;
            if(endPad) {
                // Add 1 more until endPad is gone.
                ++heights[yi];
                --endPad;
            }
        }
        y += heights[yi] + border;
        if(y > a->y + a->height)
            // The cell was clipped.
            y = a->y + a->height;
    }
    Y[numRows] = y;
    DASSERT(!endPad);

    // Now we have all the heights[] and Y[] for the whole grid.
    //
    // Here we set the children y positions and heights.
    // Children that can't expand have a good width already.
    //
    for(uint32_t yi=0; yi<numRows; ++yi) {
        for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
            c = child[yi][xi];
            if(!c || c->culled) continue;
            if(!IsUpperLeftCell(c, child, xi, yi)) continue;
            // Even if we did not expand this row we still needed this
            // widget "c" y position.
            // We start with the y position of "c" at the top of the cell.
            c->allocation.y = Y[yi];
            if(c->canExpand & PnExpand_V) {
                c->allocation.height = heights[yi];
                DASSERT(c->pg.rSpan);
                for(uint32_t span=c->pg.rSpan-1; span; --span)
                    if(heights[yi+span])
                        c->allocation.height += (heights[yi+span] + border);
            } else { // "c" height does not change.
                // Align it vertically with the given empty vertical space
                // in the cell.
                //
                // Let h be the y space available to use for the widget
                // "c".
                uint32_t h = heights[yi];
                // If the widget spans more than one column:
                for(uint32_t span=c->pg.cSpan-1; span; --span)
                    if(heights[yi+span])
                        h += (heights[yi+span] + border);
                // We better have at least enough space to fix the widget
                // in the cell.
                DASSERT(h >= c->allocation.height);
                switch(c->align & PN_ALIGN_Y) {
                    case PN_ALIGN_Y_CENTER:
                    case PN_ALIGN_Y_JUSTIFIED:
                        c->allocation.y = Y[yi] + (h - c->allocation.height)/2;
                        break;
                    case PN_ALIGN_Y_BOTTOM:
                        c->allocation.y = Y[yi] + h - c->allocation.height;
                        break;
                    //case PN_ALIGN_Y_LEFT:
                }
            }
        }
    }
}


// Get bottom border width of a horizontal layout.
//
static inline uint32_t GetHBottomBorder(const struct PnWidget *s,
        const struct PnAllocation *a) {

    DASSERT(s);
    DASSERT(s->l.firstChild);
    DASSERT(a->height);
    // a horizontal layout
    DASSERT(s->layout == PnLayout_LR ||
        s->layout == PnLayout_RL ||
        s->layout == PnLayout_Cover ||
        s->layout == PnLayout_One);

    uint32_t maxHeight = 0;
    struct PnWidget *c;

    for(c = s->l.firstChild; c; c = c->pl.nextSibling) {
        if(c->culled) continue;
        DASSERT(c->allocation.height);
        if(maxHeight < c->allocation.height)
            maxHeight = c->allocation.height;
    }
    // There had to be at least one child widget not culled.
    DASSERT(maxHeight);
    DASSERT(a->height >= maxHeight);

    uint32_t border = GetBHeight(s);
    DASSERT(a->height > border);
    DASSERT(maxHeight + border <= a->height);

    if(maxHeight + border > a->height - border)
        // This contain widget "s" was clipped.
        return a->height - maxHeight - border;
    return border;
}

// Get right border width of a vertical layout.
//
static inline uint32_t GetVRightBorder(const struct PnWidget *s,
        const struct PnAllocation *a) {

    DASSERT(s);
    DASSERT(s->l.firstChild);
    // a horizontal layout
    DASSERT(s->layout == PnLayout_TB ||
        s->layout == PnLayout_BT ||
        s->layout == PnLayout_Cover ||
        s->layout == PnLayout_One);

    uint32_t maxWidth = 0;
    struct PnWidget *c;

    for(c = s->l.firstChild; c; c = c->pl.nextSibling) {
        if(c->culled) continue;
        DASSERT(c->allocation.width);
        if(maxWidth < c->allocation.width)
            maxWidth = c->allocation.width;
    }
    // There had to be at least one child widget not culled.
    DASSERT(maxWidth);
    DASSERT(a->width >= maxWidth);

    uint32_t border = GetBWidth(s);
    DASSERT(a->width > border);
    DASSERT(maxWidth + border <= a->width);

    if(maxWidth + border > a->width - border)
        // This contain widget "s" was clipped.
        return a->width - maxWidth - border;
    return border;
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
// const struct PnWidget *s  is mostly constant.  Oh well.
//
static void ExpandChildren(const struct PnWidget *s,
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

    struct PnWidget *c;

    switch(s->layout) {

        case PnLayout_One:
        case PnLayout_Cover:
            DASSERT(s->l.firstChild == s->l.lastChild);
        case PnLayout_LR:
            ExpandHShared(s, a, s->l.firstChild, Next);
            endBorder = GetHBottomBorder(s, a);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    ExpandV(s, a, c, &c->allocation, endBorder);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled && !(c->canExpand & PnExpand_V))
                    AlignY(s, a, &c->allocation, endBorder);
            break;

        case PnLayout_TB:
            ExpandVShared(s, a, s->l.firstChild, Next);
            endBorder = GetVRightBorder(s, a);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    ExpandH(s, a, c, &c->allocation, endBorder);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled && !(c->canExpand & PnExpand_H))
                    AlignX(s, a, &c->allocation, endBorder);
            break;

        case PnLayout_BT:
            ExpandVShared(s, a, s->l.lastChild, Prev);
            endBorder = GetVRightBorder(s, a);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    ExpandH(s, a, c, &c->allocation, endBorder);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled && !(c->canExpand & PnExpand_H))
                    AlignX(s, a, &c->allocation, endBorder);
            break;

        case PnLayout_RL:
            ExpandHShared(s, a, s->l.lastChild, Prev);
            endBorder = GetHBottomBorder(s, a);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled)
                    ExpandV(s, a, c, &c->allocation, endBorder);
            for(c = s->l.firstChild; c; c = c->pl.nextSibling)
                if(!c->culled && !(c->canExpand & PnExpand_V))
                    AlignY(s, a, &c->allocation, endBorder);
            break;

        case PnLayout_Grid:
            ExpandGrid(s, &s->allocation);
            break;

        case PnLayout_None:
        default:
            ASSERT(0);
            break;
    }

    if(s->layout == PnLayout_Grid) {
        DASSERT(s->g.grid);
        DASSERT(s->g.grid->child);
        DASSERT(s->g.grid->numChildren);
        
        struct PnWidget ***child = s->g.grid->child;
        DASSERT(child);
        for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
            for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                c = child[yi][xi];
                if(!c || c->culled) continue;
                if(HaveChildren(c))
                    ExpandChildren(c, &c->allocation);
            }
        }
        return;
    }

    // Now that all the children of "s" have correct allocations we can
    // expand (fix) the "s" children's children allocations.

    for(c = s->l.firstChild; c; c = c->pl.nextSibling)
        if(!c->culled && c->l.firstChild)
            ExpandChildren(c, &c->allocation);
}


// NOT A FULL PASS through the widget tree.
//
bool HaveShowingChild(const struct PnWidget *s) {

    DASSERT(s);

    if(s->layout != PnLayout_Grid) {
        for(s = s->l.firstChild; s; s = s->pl.nextSibling)
            if(!s->culled) return true;
        return false;
    }

    DASSERT(s->g.grid);
    struct PnWidget ***child = s->g.grid->child;
    DASSERT(child);
    for(uint32_t yi=s->g.numRows-1; yi != -1; --yi)
        for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
            struct PnWidget *c = child[yi][xi];
            if(c && !c->culled) return true;
        }
    return false;
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
void GetWidgetAllocations(struct PnWidget *s) {

    DASSERT(s);
    struct PnAllocation *a = &s->allocation;
    DASSERT(s->type == PnSurfaceType_toplevel ||
            s->type == PnSurfaceType_popup);
    DASSERT(!a->x);
    DASSERT(!a->y);
    DASSERT(!s->hidden);
    DASSERT(!s->parent);
    DASSERT(!s->culled);

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

    if(!HaveChildren(s)) {
        // This is the case where an API user wants to draw on a simple
        // window, without dumb-ass widgets.  Fuck ya!  The main point of
        // this API is to do simple shit like draw pixels.
        //INFO("w,h=%" PRIi32",%" PRIi32, a->width, a->height);
        return;
    }

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
    // this shit.  Trying to "re-form" a proven given solution is a way
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

                case PnLayout_Grid: {
                    struct PnWidget ***child = s->g.grid->child;
                    DASSERT(child);
                    for(uint32_t yi=s->g.numRows-1; yi != -1; --yi) {
                        for(uint32_t xi=s->g.numColumns-1; xi != -1; --xi) {
                            struct PnWidget *c = child[yi][xi];
                            if(!c || c->culled) continue;
                            ASSERT(0, "All children should be culled");
                        }
                    }
                    break;
                }
                default:
                    ASSERT(s->l.firstChild);
                    ASSERT(s->l.firstChild->culled);
                    for(struct PnWidget *c=s->l.firstChild; c;
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

        // Still shrink wrapped and the top widget surface width and
        // height are set to the "shrink wrapped" width and height.
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
        // through its paces.  Try (edit and recompile) it with 1000
        // widgets.

        if(!HaveShowingChild(s)) break;

        ++loopCount;

        if(!(loopCount % 14)) {
            // TODO: change this to DSPEW() or remove loopCount.
            //
            // If this prints a lot it likely we have a bug in this code.
            INFO("calling ClipOrCullChildren() %" PRIu32 " times",
                    loopCount);
        }

    } while(// PASS 4 plus loop repeats times 3
            ClipOrCullChildren(s, a));


    if(HaveShowingChild(s)) {

        // TOTAL PASSES (so far) = 1 + 3 * loopCount

        // Recursively resets "canExpand" flags used in ExpandChildren().
        // We had to do widget culling stuff before this stage.
        //
        // PASS 5
        ResetCanExpand(s);

        // Expand widgets that can be expanded.  Note: this only fills in
        // otherwise blank container spaces; but by doing so has to move the
        // child widgets; they push each other around when they expand; so
        // both the positions and sizes of the widgets may change.
        //
        // Also aligns the widgets if they to not fill their container space.
        //
        // PASS 6
        ExpandChildren(s, a);

        // TOTAL PASSES = 3 + 3 * loopCount
    }

    // TODO: Mash more widget (surface) passes together?  I'd expect that
    // would be prone to introducing bugs.  It's currently doing a very
    // ordered logical progression.  Switching the order of any of the
    // operations will break it (without lots more changes).

    // Reset the "focused" widget surface and like things.  At this point
    // we may be culling out that "focused" surface.  I'm not sure if
    // there is a case that lets the focused and other marked widget
    // surfaces stay marked; but if there is, for example, a "focused"
    // widget surface that just got culled, we need to make it not
    // "focused" now.  A case may be when the panels API users code uses
    // code to do a resize of the window.  A very common example may be:
    // the user clicks a widget that closes it.
    ResetDisplaySurfaces();

    //INFO("w,h=%" PRIi32",%" PRIi32, a->width, a->height);
}
