// This is just a step (study) toward setting widget packing and event
// search callbacks for the widget layout type PnSplitter.
//
// PnSplitter is a container widget with 3 child widgets.  The widget
// in-between the other two is the panel slider.
//
// Except for when the separator panel is moved, this container widget
// acts like, and is mostly coded with the PnLayout_LR or PnLayout_TB
// widgets layout.  Unlike PnLayout_LR or PnLayout_TB it can only have
// 3 child widgets (not more or less).
//
struct PnSplitter {

    // We put the separator leaf widget in this widget.
    // We use the struct PnWidget::l widget list.
    //
    struct PnWidget widget; // inherit first

    char *cursorName;

    // This is the second child in the widget.l.firstChild list.
    // That is widget.l.firstChild->nextSibling when the first child is
    // present.
    struct PnWidget *slider;

    bool cursorSet; // The special pointer cursor is set or not.
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


// The steps used to allocate the size and position of all widgets in the
// window is (from allocation.c):
//
//   1  pnSurface_draw()
//   2  findSurface()
//   3  ResetChildrenCull()
//   4  TallyRequestedSizes()
//   5  GetChildrenXY()
//   6  ClipOrCullChildren()
//   7  ResetCanExpand()
//   8  ExpandChildren()
//   9   There is also a change to GetWidth() in allocation.c.
//   10  HaveChildren() needed no changes, but in general could have.
//   11  DestroySurfaceChildren() looks at s->layout to destroy all children.
//       It needed no changes, and used the children list like all but the
//       grid layout, PnLayout_Grid.
//   12  Any "add child" widget functions we need to treat the splitter
//       container widget as a special case (number of children == 3) of
//       PnLayout_LR or PnLayout_TB widget layout type.
//   13  CreateCairos()
//
// All these functions recurse and go through the all the parts of the
// widget tree that are showing (not culled).  To do that the callback
// functions for a particular panel widget layout must recurse, that is
// call the starting allocation function for it's children.
//
// For example: pnSplitter_resetChildrenCull() will call
// ResetChildrenCull() passing the children's child widgets.
//

// This is called in ExpandChildren() in allocation.c
void Splipper_preExpand(const struct PnWidget *w,
        const struct PnAllocation *a);
