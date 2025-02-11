#include <stdio.h>
#include <stdlib.h>

#include "../lib/debug.h"

#include "../include/panels.h"

#define FONT  "Sans"

int main(void) {

    char *path = pnFindFont(FONT);

    ASSERT(path);

    fprintf(stderr, "pnFindFont(\"%s\")=%s\n", FONT, path);

    free(path);

    return 0;
}

