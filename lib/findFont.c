// This started from code at:
// https://stackoverflow.com/questions/10542832/how-to-use-fontconfig-to-get-font-list-c-c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fontconfig/fontconfig.h>

#include "debug.h"
#include "../include/panels.h"


#define FAIL(to, fmt, ...) \
    do {\
        ERROR(fmt, ##__VA_ARGS__);\
        goto to;\
    } while(0)


// This returns a file name path as a string pointer that must be
// free(3)ed.
//
// The question here is should we be keeping around some of the
// intermediate libfontconfig state data?  If this is just called once in
// a program process than the answer is most likely, No.  Maybe it will be
// called 3 times, and in that case the answer could still be, no.
//
// TODO: libfontconfig.so uses signed char for strings, so should I be
// checking and setting the all the sign (8th) bits to zero?  I always
// wondered why the sign bit was ignored in strings.  ASCII is a 7 bit
// code.  So, what is the 8th bit supposed to be?  Many 8-bit codes (e.g.,
// ISO 8859-1) contain ASCII as their lower half.
//
char *pnFindFont(const char *exp) {

    RET_ERROR(exp, 0, "slFindFont(exp=0) failed exp can't be 0");

    FcConfig *config;
    FcPattern *pat;
    FcPattern *font;
    char *path = 0;

    RET_ERROR(FcInit(), 0, "FcInit() failed");

    config = FcInitLoadConfigAndFonts();
    if(!config)
        FAIL(config, "FcInitLoadConfigAndFonts() failed");

    pat = FcNameParse((unsigned char *) exp);
    if(!pat)
        FAIL(pat, "FcNameParse(\"%s\") failed", exp);

    FcBool ret = FcConfigSubstitute(config, pat, FcMatchPattern);
    if(!ret)
        FAIL(pat, "FcConfigSubstitute() failed");

    FcDefaultSubstitute(pat);

    FcResult result;
    font = FcFontMatch(config, pat, &result);
    if(!font)
        FAIL(font, "FcFontMatch() failed");

    if(FcPatternGetString(font, FC_FILE, 0,
                (unsigned char **) &path) != FcResultMatch) {
        DASSERT(path == 0);
        FAIL(all, "FcPatternGetString() failed");
    }

    DASSERT(path);
    path = strdup(path);
    ASSERT(path, "strdup() failed");

    // Cleanup in reverse order on your way out of this function:
all:
    FcPatternDestroy(font);
font:
    FcPatternDestroy(pat);
pat:
    FcConfigDestroy(config);
config:
    FcFini();

    return path;
}
