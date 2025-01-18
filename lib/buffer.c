#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include  "display.h"


static void release(struct PnBuffer *buffer,
        struct wl_buffer *wl_buffer) {

    //WARN();
    buffer->busy = false;
}

static const struct wl_buffer_listener buffer_listener = {
    .release = (void *) release,
};


// Return false on success.
//
static bool ResizeBuffer(struct PnWindow *win, struct PnBuffer *buffer,
        size_t size) {

    DASSERT(buffer);
    DASSERT(buffer->wl_buffer);
    DASSERT(buffer->wl_shm_pool);
    DASSERT(buffer->size);
    DASSERT(!buffer->busy);
    DASSERT(buffer->fd > -1);
    DASSERT(buffer->size < size);
    DASSERT(buffer->width * buffer->height * PN_PIXEL_SIZE == size);

    if(buffer->pixels != MAP_FAILED) {
        if(munmap(buffer->pixels, buffer->size))
            ASSERT(0, "munmap() failed");
        buffer->pixels = MAP_FAILED;
    }

    if(ftruncate(buffer->fd, size) == -1) {
	ERROR("ftruncate(%d,%zu) failed", buffer->fd, size);
	goto fail;
    }
    buffer->size = size;

    buffer->pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
		buffer->fd, 0);
    if(buffer->pixels == MAP_FAILED) {
        ERROR("mmap() failed");
        goto fail;
    }

    wl_buffer_destroy(buffer->wl_buffer);
    buffer->wl_buffer = 0;

    // returns void.  Is there a way to check for error?
    wl_shm_pool_resize(buffer->wl_shm_pool, size);

    buffer->wl_buffer = wl_shm_pool_create_buffer(
            buffer->wl_shm_pool, 0,
	    buffer->width, buffer->height,
            buffer->width * PN_PIXEL_SIZE/*stride*/,
            WL_SHM_FORMAT_ARGB8888);
    if(!buffer->wl_buffer) {
        ERROR("wl_shm_pool_create_buffer() failed");
        goto fail;
    }
    
    if(wl_buffer_add_listener(buffer->wl_buffer,
                &buffer_listener, buffer)) {
        ERROR("wl_buffer_add_listener() failed");
        goto fail;
    }

    return false;

fail:

    FreeBuffer(buffer);

    return true;
}

// Return false on success.
//
static bool CreateBuffer(struct PnWindow *win, struct PnBuffer *buffer,
        size_t size) {

    DASSERT(buffer);
    DASSERT(!buffer->busy);
    DASSERT(size);

    buffer->fd = create_shm_file(size);
    if(buffer->fd <= -1)
        goto fail;

    buffer->pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
            buffer->fd, 0);
    if(buffer->pixels == MAP_FAILED) {
        ERROR("mmap() failed");
        goto fail;
    }
    buffer->size = size;

    buffer->wl_shm_pool = wl_shm_create_pool(d.wl_shm, buffer->fd, size);
    if(!buffer->wl_shm_pool) {
        ERROR("wl_shm_create_pool() failed");
        goto fail;
    }
    buffer->wl_buffer = wl_shm_pool_create_buffer(buffer->wl_shm_pool, 0,
			buffer->width, buffer->height,
                        buffer->width*PN_PIXEL_SIZE /*stride*/,
                        WL_SHM_FORMAT_ARGB8888);
    if(!buffer->wl_buffer) {
        ERROR("wl_shm_pool_create_buffer() failed");
        goto fail;
    }

    if(wl_buffer_add_listener(buffer->wl_buffer,
                &buffer_listener, buffer)) {
        ERROR("wl_buffer_add_listener() failed");
        goto fail;
    }

    return false;

fail:

    FreeBuffer(buffer);

    return true;
}

// Returns the next buffer (pixels) the we can draw to.
//
// Returns 0 if both buffers are busy, that is the wayland compositor
// is reading them (??).
//
struct PnBuffer *GetNextBuffer(struct PnWindow *win,
        uint32_t width, uint32_t height) {

    DASSERT(win);
    DASSERT(d.wl_display);
    DASSERT(win->wl_surface);
    DASSERT(win->xdg_surface);
    DASSERT(width > 0);
    DASSERT(height > 0);

    struct PnBuffer *buffer = win->buffer;
    // We only have 2 elements in this buffer[] array
    if(buffer->busy) ++buffer;
    if(buffer->busy) {
        // The two buffers are busy.  It is thought that there will be a
        // later draw-like event that will make the windows pixels
        // current.  It appears that now there is an overflow of draw-like
        // events.
        //INFO("Buffers busy");
        return 0;
    }

    DASSERT(PN_PIXEL_SIZE == 4);
    size_t size = width * height * PN_PIXEL_SIZE;

    if(buffer->wl_buffer && buffer->size > size) {
        // We can't decrease the size of the buffer without
        // creating a different buffer.
        FreeBuffer(buffer);
        DASSERT(!buffer->wl_buffer);
    }

    // We could change the width and height without changing the size.
    // That would be okay, except we just need the values for any
    // redraws.
    if(buffer->width != width) {
        win->needAllocate = true;
        buffer->width = width;
        buffer->stride = width * PN_PIXEL_SIZE;
    }
    if(buffer->height != height) {
        win->needAllocate = true;
        buffer->height = height;
    }

    if(buffer->wl_buffer && buffer->size == size)
        // No buffer resize needed.
        return buffer;

    if(!buffer->wl_buffer) {
        if(CreateBuffer(win, buffer, size))
            return 0;
    } else if(ResizeBuffer(win, buffer, size))
        return 0;

    return buffer;
}


void FreeBuffer(struct PnBuffer *buffer) {

    DASSERT(buffer);
    //DASSERT(!buffer->busy);

    if(buffer->wl_buffer)
        wl_buffer_destroy(buffer->wl_buffer);

    if(buffer->wl_shm_pool)
        wl_shm_pool_destroy(buffer->wl_shm_pool);

    if(buffer->pixels != MAP_FAILED) {
        DASSERT(buffer->size);
        munmap(buffer->pixels, buffer->size);
    }

    if(buffer->fd > -1)
        close(buffer->fd);

    memset(buffer, 0, sizeof(*buffer));
    buffer->pixels = MAP_FAILED;
    buffer->fd = -1;
}

