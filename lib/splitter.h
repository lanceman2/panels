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

    // The relative size of the first widget.  So 1.0 -> means that the
    // first showing (widget.l.firstChild) widget takes all the space.  If
    // it's zero it's culled.
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
//
// All these functions recurse and go through the all the parts of the
// widget tree that are showing (not culled).  To do that the callback
// functions for a particular panel widget layout must recurse, that is
// call the starting allocation function for it's children.
//
// For example: pnSplitter_resetChildrenCull() will call
// ResetChildrenCull() passing the children's child widgets.
//

static inline void Splipper_preExpandH(const struct PnWidget *w,
        const struct PnAllocation *a,
        struct PnSplitter *s) {

    if(a->width <= s->slider->reqWidth) {
        w->l.firstChild->culled = true;
        w->l.lastChild->culled = true;
        s->slider->culled = false;
        s->slider->allocation.x = a->x;
        s->slider->allocation.width = a->width;
        return;
    }

    DASSERT(a->width > s->slider->reqWidth);

    if(w->l.firstChild->reqWidth >= w->l.lastChild->reqWidth) {

        if(w->l.firstChild->reqWidth + s->slider->reqWidth >= a->width) {
            w->l.lastChild->culled = true;
            w->l.lastChild->reqWidth = 1;
            w->l.firstChild->reqWidth = a->width - s->slider->reqWidth;
        } else {
            w->l.lastChild->culled = false;
            w->l.lastChild->reqWidth = a->width
                - w->l.firstChild->reqWidth - s->slider->reqWidth;
        }

        w->l.firstChild->culled = false;
        s->slider->culled = false;
        w->l.firstChild->allocation.x = a->x;
        w->l.firstChild->allocation.width = w->l.firstChild->reqWidth;
        s->slider->allocation.x = a->x + w->l.firstChild->reqWidth;
        s->slider->allocation.width = s->slider->reqWidth;
        w->l.lastChild->allocation.x = s->slider->allocation.x +
            s->slider->allocation.width;
        w->l.lastChild->allocation.width = w->l.lastChild->reqWidth;
        return;
    }
    {
        if(w->l.lastChild->reqWidth + s->slider->reqWidth >= a->width) {
            w->l.firstChild->culled = true;
            w->l.firstChild->reqWidth = 1;
            w->l.lastChild->reqWidth = a->width - s->slider->reqWidth;
            w->l.firstChild->allocation.x = a->x;
            w->l.firstChild->allocation.width = 0;
        } else {
            w->l.firstChild->culled = false;
            w->l.firstChild->reqWidth = a->width
                - w->l.lastChild->reqWidth - s->slider->reqWidth;
            w->l.firstChild->allocation.x = a->x;
            w->l.firstChild->allocation.width = w->l.firstChild->reqWidth;
        }

        w->l.lastChild->culled = false;
        s->slider->culled = false;
        s->slider->allocation.x = w->l.firstChild->allocation.x +
            w->l.firstChild->allocation.width;
        s->slider->allocation.width = s->slider->reqWidth;
        w->l.lastChild->allocation.x = s->slider->allocation.x +
            s->slider->reqWidth;
        w->l.lastChild->allocation.width = w->l.lastChild->reqWidth;
    }
}

static inline void Splipper_preExpandV(const struct PnWidget *w,
        const struct PnAllocation *a,
        const struct PnSplitter *s) {

    if(a->height <= s->slider->reqHeight) {
        w->l.firstChild->culled = true;
        w->l.lastChild->culled = true;
        s->slider->culled = false;
        s->slider->allocation.y = a->y;
        s->slider->allocation.height = a->height;
        return;
    }

    DASSERT(a->height > s->slider->reqHeight);

    if(w->l.firstChild->reqHeight >= w->l.lastChild->reqHeight) {

        if(w->l.firstChild->reqHeight + s->slider->reqHeight >= a->height) {
            w->l.lastChild->culled = true;
            w->l.lastChild->reqHeight = 1;
            w->l.firstChild->reqHeight = a->height - s->slider->reqHeight;
        } else {
            w->l.lastChild->culled = false;
            w->l.lastChild->reqHeight = a->height
                - w->l.firstChild->reqHeight - s->slider->reqHeight;
        }

        w->l.firstChild->culled = false;
        s->slider->culled = false;
        w->l.firstChild->allocation.y = a->y;
        w->l.firstChild->allocation.height = w->l.firstChild->reqHeight;
        s->slider->allocation.y = a->y + w->l.firstChild->reqHeight;
        s->slider->allocation.height = s->slider->reqHeight;
        w->l.lastChild->allocation.y = s->slider->allocation.y +
            s->slider->allocation.height;
        w->l.lastChild->allocation.height = w->l.lastChild->reqHeight;
        return;
    }
    {
        if(w->l.lastChild->reqHeight + s->slider->reqHeight >= a->height) {
            w->l.firstChild->culled = true;
            w->l.firstChild->reqHeight = 1;
            w->l.lastChild->reqHeight = a->height - s->slider->reqHeight;
            w->l.firstChild->allocation.y = a->y;
            w->l.firstChild->allocation.height = 0;
        } else {
            w->l.firstChild->culled = false;
            w->l.firstChild->reqHeight = a->height
                - w->l.lastChild->reqHeight - s->slider->reqHeight;
            w->l.firstChild->allocation.y = a->y;
            w->l.firstChild->allocation.height = w->l.firstChild->reqHeight;
        }

        w->l.lastChild->culled = false;
        s->slider->culled = false;
        s->slider->allocation.y = w->l.firstChild->allocation.y +
            w->l.firstChild->allocation.height;
        s->slider->allocation.height = s->slider->reqHeight;
        w->l.lastChild->allocation.y = s->slider->allocation.y +
            s->slider->reqHeight;
        w->l.lastChild->allocation.height = w->l.lastChild->reqHeight;
    }

}


static inline void Splipper_preExpand(const struct PnWidget *w,
        const struct PnAllocation *a) {

    DASSERT(GET_WIDGET_TYPE(w->type) == W_SPLITTER);
    DASSERT(w->layout == PnLayout_LR ||
            w->layout == PnLayout_TB);
    struct PnSplitter *s = (void *) w;

    if(w->layout == PnLayout_LR)
        Splipper_preExpandH(w, a, s);
    else
        Splipper_preExpandV(w, a, s);
}

