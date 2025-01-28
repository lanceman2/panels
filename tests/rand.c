#include <signal.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../lib/debug.h"

#include "rand.h"


#define SEED  10

int main(void) {

    srand(SEED);

    for(uint32_t i=0; i<2000; ++i)
        printf("%" PRIi32 " ", SkewMinRand(0, 9, 0));
        //printf("%" PRIi32 " ", Rand(1, 9));
    printf("\n");
    return 0;

    return 0;
}
