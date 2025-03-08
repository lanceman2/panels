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


// Return false on success.
//
static bool ResizeBuffer(struct PnWindow *win, struct PnBuffer *buffer,
        size_t size) {

    DASSERT(buffer);
    DASSERT(buffer->wl_buffer);
    DASSERT(buffer->wl_shm_pool);
    DASSERT(buffer->size);
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

    if(buffer->wl_shm_pool)
        wl_shm_pool_destroy(buffer->wl_shm_pool);

    buffer->wl_shm_pool = wl_shm_create_pool(d.wl_shm, buffer->fd, size);
    if(!buffer->wl_shm_pool) {
        ERROR("wl_shm_create_pool() failed");
        goto fail;
    }

    // I tried using wl_shm_pool_resize() and it's a fucking nightmare of
    // endless wayland errors/bugs.  Time spent I can't get back.  I got
    // it working with the use of two wl_buffers switching between the
    // two.  But using two buffers is not efficient when you have widgets
    // that draw each other in an independent fashion; there's no way to
    // keep the two buffers correct without a lot of memory copying,
    // making it much more OS (operating system) resource intensive than
    // using one buffer.

    buffer->wl_buffer = wl_shm_pool_create_buffer(
            buffer->wl_shm_pool, 0,
	    buffer->width, buffer->height,
            buffer->width * PN_PIXEL_SIZE/*stride in bytes*/,
            WL_SHM_FORMAT_ARGB8888);
    if(!buffer->wl_buffer) {
        ERROR("wl_shm_pool_create_buffer() failed");
        goto fail;
    }

#ifdef WITH_CAIRO
    RecreateCairos(win);
#endif

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

    if(buffer->wl_shm_pool)
        wl_shm_pool_destroy(buffer->wl_shm_pool);

    buffer->wl_shm_pool = wl_shm_create_pool(d.wl_shm, buffer->fd, size);
    if(!buffer->wl_shm_pool) {
        ERROR("wl_shm_create_pool() failed");
        goto fail;
    }
    buffer->wl_buffer = wl_shm_pool_create_buffer(buffer->wl_shm_pool, 0,
			buffer->width, buffer->height,
                        buffer->width*PN_PIXEL_SIZE /*stride in bytes*/,
                        WL_SHM_FORMAT_ARGB8888);
    if(!buffer->wl_buffer) {
        ERROR("wl_shm_pool_create_buffer() failed");
        goto fail;
    }
#ifdef WITH_CAIRO
    RecreateCairos(win);
#endif
    return false;

fail:

    FreeBuffer(buffer);

    return true;
}

// Returns the buffer (pixels) that we can draw to
// with the corrected sizes.
//
struct PnBuffer *GetNextBuffer(struct PnWindow *win,
        uint32_t width, uint32_t height) {

    DASSERT(win);
    DASSERT(d.wl_display);
    DASSERT(win->wl_surface);
    DASSERT(win->xdg_surface);
    DASSERT(width > 0);
    DASSERT(height > 0);

    struct PnBuffer *buffer = &win->buffer;

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
        buffer->width = width;
        buffer->stride = width; // in 4 byte chunks (uint32_t)
    }
    if(buffer->height != height)
        buffer->height = height;

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
