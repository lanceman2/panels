// Inline functions that define the packing and event search for the
// panels splitter widget.

struct PnSplitter {

    // We put the separator leaf widget in this widget.
    struct PnWidget widget; // inherit first

    // Horizontal    left    right
    // Vertical      top     bottom
    struct PnWidget *first, *second;

    char *cursorName;

    bool isHorizontal; // true ==> horizontal, not true ==> vertical

    bool cursorSet;

    // The relative size of the first widget.  So 1 -> means that the
    // first widget takes all the space.
    float firstSize;
};


static inline struct PnSplitter *GetSplitter(struct PnWidget *w) {

    DASSERT(w);
    ASSERT(GET_WIDGET_TYPE(w->type) == W_SPLITTER);
    struct PnSplitter *s = (void *) w;
    DASSERT(s->first);
    DASSERT(s->second);
    return s;
}


static inline void PnSplitter_drawChildren(struct PnWidget *w,
        struct PnBuffer *buffer) {

    struct PnSplitter *s = GetSplitter(w);

    // Note: we must have the widget space (first and second) allocation
    // keep this floating point firstSize variable consistent with this:
    if(s->firstSize) {
        DASSERT(s->first->allocation.width);
        DASSERT(s->first->allocation.height);
        pnSurface_draw(s->first, buffer);
    }
    if(s->firstSize != 1.0) {
        DASSERT(s->second->allocation.width);
        DASSERT(s->second->allocation.height);
        pnSurface_draw(s->second, buffer);
    }
}
