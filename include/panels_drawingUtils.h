#include <stdint.h>

// "pix" is a (uint32_t *) pointer to the top left corner of the
// drawing surface.
//
// TODO: Note this does not know the size on the drawing surface, so
// drawing out of bounds may corrupt process memory.
//
// TODO: Does libpixman.so do this kind of thing better?  ...automatically
// using the hardware specific array operations (like SIMD).  The nice
// thing about this code is that it is short and easy to follow (at least
// for me).
//
static inline
void pn_drawFilledRectangle(uint32_t *pix/*surface starting pixel*/,
        uint32_t x, uint32_t y,
        uint32_t width, uint32_t height, uint32_t stride,
        uint32_t color) {

#define PIXEL_SIZE (4)

#ifdef PN_LIB_CODE
    DASSERT(pix);
    DASSERT(width);
    DASSERT(height);
    DASSERT(stride);
    DASSERT(width <= stride/PIXEL_SIZE);
    DASSERT(stride % PIXEL_SIZE == 0);
#endif


    // Note: we did not want to add many new variables, so we recycled the
    // function parameter variables.

    // convert stride from number of bytes to number of 4 byte unsigned
    // integer.
    stride /= PIXEL_SIZE;

    // We start drawing at this pixel:
    pix += x + y * stride;

    height *= stride; // how many pixels we advance to get past the end
    stride -= width;  // pixels in a row we do not draw

    // "end" is a pointer to a pixel after the last pixel (if it exists).
    // "end" is just a limiting pointer that we never dereference.
    //
    for(uint32_t *end = pix + height;
            pix < end; pix += stride/*skip drawing this many*/)
        for(uint32_t *rowEnd = pix + width; pix < rowEnd; ++pix)
            *pix = color;
}

#undef PIXEL_SIZE
