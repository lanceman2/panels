#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "debug.h"
#include  "display.h"


static inline
struct PnSurface *PopQueue(struct PnDrawQueue *q) {

    struct PnSurface *s = q->first;
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
        struct PnSurface *s) {

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
static void DequeueChildren(struct PnDrawQueue *q, struct PnSurface *s) {

    DASSERT(q);
    DASSERT(s);

    struct PnSurface *c;
    for(c = s->firstChild; c; c = c->nextSibling) {
        // If the surface "c" is culled it can get un-culled at any time
        // so we just work with it.  It could happen that things change
        // before we get to draw.  Drawing events are somewhat
        // asynchronous to this process.
        if(c->isQueued)
            RemoveFromDrawQueue(q, c);
        else if(s->firstChild)
            // If a child is not queued it could have queued children.
            DequeueChildren(q, c);
    }
}

void pnSurface_queueDraw(struct PnSurface *s) {

    DASSERT(s);
    struct PnWindow *win = s->window;
    DASSERT(win);

    if(s->isQueued) {
        // It's already queued
        DASSERT(win->dqWrite->first);
        DASSERT(win->dqWrite->last);
        DASSERT(win->wl_callback);
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
    // it queued than so is this surface, "s".

    // Remove any descendants from the draw queue.  They get queued
    // implicitly by this, "s", being queued.  If a parent surface gets
    // drawn the children always get drawn right after.  That just how we
    // designed this widgets inside container widgets thing.
    DequeueChildren(win->dqWrite, s);

    // Now queue this surface "s" at win->dqWrite->last.
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
    DASSERT(!win->needAllocate);
    DASSERT(!win->needDraw);

    struct PnBuffer *buffer = GetNextBuffer(win,
            win->surface.allocation.width,
            win->surface.allocation.height);

    DASSERT(buffer);
    DASSERT(win->surface.allocation.width == buffer->width);
    DASSERT(win->surface.allocation.height == buffer->height);

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
    struct PnSurface *s;

    wl_surface_attach(win->wl_surface, buffer->wl_buffer, 0, 0);

    while((s = PopQueue(q))) {
        pnSurface_draw(s, buffer);
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
