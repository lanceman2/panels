// Inline functions that define the packing and event search for the
// panels splitter widget.
//
// This is just a step toward setting widget packing and event search
// callbacks for the widget layout type PnSplitter.   This is slower
// running code, but more structured; likely not measurable run-time
// difference.  It's just adding one function call per widget allocation
// cycles thingy (whatever the fuck that is), as opposed to not calling a
// function.  So in general more structure traded for less speed, when
// adding widget layout types.  But, it's likely faster than any other
// widget API code.
//
// PnSplitter is a container widget with 3 child widgets.  The widget
// in-between the other two is the panel slider.
//
struct PnSplitter {

    // We put the separator leaf widget in this widget.
    // We use the struct PnWidget::l widget list.
    //
    struct PnWidget widget; // inherit first

    char *cursorName;

    // This is also widget.l.firstChild.
    struct PnWidget *slider;

    bool cursorSet; // The special pointer cursor is set or not.

    // The relative size of the first widget.  So 1.0 -> means that the
    // first showing (widget.l.firstChild->nextSibling) widget takes all
    // the space.
    float firstSize;
};


static inline struct PnSplitter *GetSplitter(const struct PnWidget *w) {

    DASSERT(w);
    ASSERT(GET_WIDGET_TYPE(w->type) == W_SPLITTER);
    struct PnSplitter *s = (void *) w;
    DASSERT(s->widget.l.firstChild);
    DASSERT(s->slider);
    DASSERT(s->widget.l.firstChild == s->slider);
    DASSERT(s->widget.l.lastChild);

    return s;
}

// 1
//
static inline void PnSplitter_drawChildren(struct PnWidget *w,
        struct PnBuffer *buffer) {

    struct PnSplitter *s = GetSplitter(w);

    struct PnWidget *c = s->widget.l.firstChild;
    for(; c; c = c->pl.nextSibling) {
        DASSERT(c->allocation.width);
        DASSERT(c->allocation.height);
        pnSurface_draw(c, buffer);
    }
}

// 2
//
static inline struct PnWidget *PnSplitter_findSurface(
        const struct PnWindow *win,
        struct PnWidget *s, uint32_t x, uint32_t y) {

    // Just for the assertions.
    GetSplitter(s);
    DASSERT(!s->culled);

    struct PnWidget *c = s->l.firstChild;
    for(; c; c = c->pl.nextSibling)
        if(c->allocation.x <= x &&
                    x < c->allocation.x + c->allocation.width &&
                    c->allocation.y <= y &&
                    y < c->allocation.y + c->allocation.height) {
            if(c->culled) continue;
            if(HaveChildren(c))
                return FindSurface(win, c, x, y);
            return c;
        }

    return s;
}


// The steps used to allocate the size and position of all widgets in the
// window is (from allocation.c):
//
//   3  ResetChildrenCull()
//   4  TallyRequestedSizes()
//   5  GetChildrenXY()
//   6  ClipOrCullChildren()
//   7  ResetCanExpand()
//   8  ExpandChildren()
//
//   9  There is also a change to GetWidth() in allocation.c.
//   10 HaveChildren() needed no changes, but in general could have.
//
// All these functions recurse and go through the all the parts of the
// widget tree that are showing (not culled).  To do that the callback
// functions for a particular panel widget layout must recurse, that is
// call the starting allocation function for it's children.
//
// For example: pnSplitter_resetChildrenCull() will call
// ResetChildrenCull() passing the children's child widgets.
//

// 3
//
static inline bool pnSplitter_resetChildrenCull(const struct PnWidget *s) {

    GetSplitter(s);

    // This will call ResetChildrenCull() in it.
    return pnList_ResetChildrenCull(s);
}

// 4
//
static inline void pnSplitter_tallyRequestedSizes(const struct PnWidget *s,
        struct PnAllocation *a) {


}
