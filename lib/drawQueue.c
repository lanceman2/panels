#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "debug.h"
#include  "display.h"


static inline
struct PnWidget *PopQueue(struct PnDrawQueue *q) {

    struct PnWidget *s = q->first;
    if(!s) return s;

    DASSERT(s->isQueued);
    DASSERT(!s->dqPrev);

    s->isQueued = false;
    q->first = s->dqNext;

    if(q->first) {
        DASSERT(q->first->dqPrev == s);
        q->first->dqPrev = 0;
        s->dqNext = 0;
    } else
        q->last = 0;

    return s;
}


// Remove "s" from a drawQueue doubly linked list.
//
static inline void RemoveFromDrawQueue(struct PnDrawQueue *q,
        struct PnWidget *s) {

    DASSERT(s);
    DASSERT(s->isQueued);
    DASSERT(q->first);
    DASSERT(q->last);

    s->isQueued = false;

    if(s->dqNext) {
        DASSERT(s != q->last);
        DASSERT(s->dqNext->isQueued);
        s->dqNext->dqPrev = s->dqPrev;
    } else {
        DASSERT(s == q->last);
        q->last = s->dqPrev;
    }

    if(s->dqPrev) {
        DASSERT(s != q->first);
        DASSERT(s->dqPrev->isQueued);
        s->dqPrev->dqNext = s->dqNext;
        s->dqPrev = 0;
    } else {
        DASSERT(s == q->first);
        q->first = s->dqNext;
    }
    s->dqNext = 0;
}

// This function calls itself.
//
// Check and dequeue all children from a window draw queue.
static void DequeueChildren(struct PnDrawQueue *q, struct PnWidget *s) {

    DASSERT(q);
    DASSERT(s);

    struct PnWidget *c;
    if(s->layout != PnLayout_Grid) {
        for(c = s->l.firstChild; c; c = c->pl.nextSibling) {
            // If the widget surface "c" is culled it can get un-culled at
            // any time so we just work with it.  It could happen that
            // things change before we get to draw.  Drawing events are
            // somewhat asynchronous to this process.
            if(c->isQueued)
                RemoveFromDrawQueue(q, c);
            else if(HaveChildren(s))
                // If a child is not queued it could have queued children.
                DequeueChildren(q, c);
        }
        return;
    }

    // else // This widget "s" is a grid container.
    if(!s->g.grid)
            // "s" has no children.
            return;
    struct PnWidget ***child = s->g.grid->child;
    DASSERT(child);
    for(uint32_t y=s->g.numRows-1; y != -1; --y)
        for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
            struct PnWidget *c = child[y][x];
            // rows and columns can share widgets; like for row span 2 and
            // column span 2; that is adjacent cells that share the same
            // widget.
            if(IsUpperLeftCell(c, child, x, y)) {
                if(c->isQueued)
                    RemoveFromDrawQueue(q, c);
                else if(HaveChildren(s))
                    // If a child is not queued it could have queued children.
                    DequeueChildren(q, c);
            }
        }
}

void pnWidget_queueDraw(struct PnWidget *s, bool allocate) {

    DASSERT(s);
    struct PnWindow *win = s->window;
    DASSERT(win);

    if(s->isQueued) {
        // It's already queued
        DASSERT(win->dqWrite->first);
        DASSERT(win->dqWrite->last);
        //DASSERT(win->wl_callback);// This popped.
        return;
    }

    if(s->culled || s->hidden)
        // This widget will not be showing and none of its' children are
        // showing either.  It would be okay if it got to the wayland 
        // callback code when it's culled; it could get culled before the
        // draw happens.
        return;

    if(_pnWindow_addCallback(win))
        // Failure.  That sucks.
        return;

    // The rule is that if a parent (or higher parentage) of this surface
    // it queued than so is this widget surface, "s".

    // Remove any descendants from the draw queue.  They get queued
    // implicitly by this, "s", being queued.  If a parent surface gets
    // drawn the children always get drawn right after.  That just how we
    // designed this widgets inside container widgets thing.
    DequeueChildren(win->dqWrite, s);

    // Now queue this widget surface "s" at win->dqWrite->last.
    DASSERT(!s->dqNext);
    DASSERT(!s->dqPrev);

    struct PnDrawQueue *q = win->dqWrite;
    DASSERT(q);

    if(q->last) {
        DASSERT(!q->last->dqNext);
        DASSERT(q->first);
        q->last->dqNext = s;
        s->dqPrev = q->last;
    } else {
        DASSERT(!q->first);
        q->first = s;
    }
    DASSERT(!q->first->dqPrev);
    q->last = s;
    s->isQueued = true;
    // TODO: It may be more performant to call _pnWidget_getAllocations(s)
    // now; but we still need to wait for the compositor to tell us when
    // the wl_buffer is ready so we can re-setup the Cairo surfaces on the
    // wl_buffer (for the case when allocate == true).

    if(allocate)
        s->needAllocate = allocate;
}

// Return false if the queue was drawn.
//
// Return true is the buffer was busy so we did not draw.
//
bool DrawFromQueue(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->dqWrite);
    DASSERT(win->dqWrite->first);
    DASSERT(win->dqWrite->last);
    DASSERT(!win->widget.needAllocate);
    DASSERT(!win->needDraw);

    struct PnBuffer *buffer = GetNextBuffer(win,
            win->widget.allocation.width,
            win->widget.allocation.height);

    DASSERT(buffer);
    DASSERT(win->widget.allocation.width == buffer->width);
    DASSERT(win->widget.allocation.height == buffer->height);

    // Switch the write and read draw queues.
    struct PnDrawQueue *q = win->dqWrite;
    DASSERT(q);
    DASSERT(win->dqRead);
    win->dqWrite = win->dqRead;
    DASSERT(win->dqWrite != q);
    win->dqRead = q;
    // Dequeue the old write queue, which this now the read queue, q.

    // Note: we call it the "read" queue but we are reading and dequeueing
    // (writing) it here.  It's more like reading what to draw, so ya,
    // read.
    struct PnWidget *s;

    while((s = PopQueue(q))) {

        if(s->needAllocate) {
            // TODO: maybe call _pnWidget_getAllocations()
            // in pnWidget_queueDraw()?
            //
            // TODO: add config() callbacks....

            if(s->parent)
                // The things calculated in _pnWidget_getAllocations() are
                // values in the children of the passed argument widget,
                // so we pass in the parent widget to get the new sizes
                // and positions of it's children.  The sizes and
                // positions of the parent are not changed.
                _pnWidget_getAllocations(s->parent);
            else
                _pnWidget_getAllocations(s);
#ifdef WITH_CAIRO
            RecreateCairos(win, s);
#endif
        }

        pnSurface_draw(s, buffer, s->needAllocate);

        s->needAllocate = false;
        // The draw() function may have queued that surface again
        // but in the write (other) queue now.
        //
        // Looks like we can add together rectangles of damage.
        d.surface_damage_func(win->wl_surface,
                s->allocation.x, s->allocation.y,
                s->allocation.width, s->allocation.height);
     }

    // The "read" queue should be empty now.
    DASSERT(!q->last);

    wl_surface_attach(win->wl_surface, buffer->wl_buffer, 0, 0);
    wl_surface_commit(win->wl_surface);

    return false;
}

// Flush (empty) without drawing anything.  Also removes the wayland
// client wl_callback for the window.
//
// We need to do this if we get a window "draw all" event.  The "draw all"
// will draw all window's widgets, so the draw queue is not needed before
// it draws all.
//
void FlushDrawQueue(struct PnWindow *win) {

    DASSERT(win);
    DASSERT(win->dqWrite);
    DASSERT(win->dqRead);
    DASSERT(win->dqWrite != win->dqRead);
    DASSERT(!win->dqRead->first);
    DASSERT(!win->dqRead->last);

    struct PnDrawQueue *q = win->dqWrite;

    while(q->first)
        RemoveFromDrawQueue(q, q->first);

    DASSERT(!q->last);

    if(win->wl_callback) {
        wl_callback_destroy(win->wl_callback);
        win->wl_callback = 0;
    }
}
