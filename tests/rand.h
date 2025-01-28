
#include <stdlib.h>
#include <inttypes.h>

#include "../lib/debug.h"


// We don't need thread safety or a great amount of entropy (good pseudo
// randomness) here.  So, rand(3) is good enough.
//
static inline
int32_t Rand(int32_t min, int32_t max) {

    DASSERT(max >= min);
    DASSERT(max - min >= 0);
    // My brain cannot think through integer arithmetic today so
    // I'll just use floating point and convert back to integer.
    double x = max - min;
    x /= RAND_MAX;
    x *= rand();
    x += min;
    if(x > 0)
        x += 0.5;
    else if(x < 0)
        x -= 0.5;
    // That's like:
    //
    //   x = min + (max - min) * rand()/RAND_MAX + 0.5;
    //
    // we added 0.5 to fix the round off shift.  I know it's not quite
    // right at the edges but it's close enough.  The distribution of
    // values will be just a little off near the min and max values,
    // unless max - min is a power of 2 (so I think).
    int32_t ret = x;
    if(ret > max)
        return max;
    else if(ret < min)
        return min;
    return ret;
}

// Random color
static inline
uint32_t Color(void) {

    uint32_t c = (rand() >> 1) & 0xFFFFFF;
    return c | 0xFF000000;
}


// Return a random number from min to max with more min values.  We need
// this so we can have more of the common edge case.  Usually min is 1.
//
static inline
int32_t SkewMinRand(int32_t min, int32_t max, int32_t n) {

    DASSERT(max + n >= min, "Not: %" PRIi32 " > %" PRIi32, max+n, min);
    DASSERT(n >= 0);

    int32_t r = Rand(min, max + n); // r = [min, max + n]

    if(r > max)
        return min; // more min values
    return r;
}
