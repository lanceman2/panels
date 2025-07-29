// This started from code at:
// https://stackoverflow.com/questions/10542832/how-to-use-fontconfig-to-get-font-list-c-c


// We added code that was needed so this passed the valgrind test and so
// it did not spew so much.

#include <stdio.h>
#include <stdlib.h>
#include <fontconfig/fontconfig.h>
#include <cairo/cairo.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "run.h"

int runLabels(void) {

    FcBool result = FcInit();
    RET_ERROR(result, 1, "FcInit() failed");

    FcConfig *config = FcConfigGetCurrent();
    RET_ERROR(config, 1, "FcConfigGetCurrent() failed");

    result = FcConfigSetRescanInterval(config, 0);
    RET_ERROR(result, 1, "FcConfigSetRescanInterval() failed");

    // show the fonts
    FcPattern *pat = FcPatternCreate();
    RET_ERROR(pat, 1, "FcPatternCreate() failed");

    FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_LANG, NULL);
    RET_ERROR(os, 1, "FcObjectSetBuild() failed");

    FcFontSet *fs = FcFontList(config, pat, os);
    RET_ERROR(fs, 1, "FcFontList() failed");

    printf("Total fonts: %d\n", fs->nfont);
    for(int i=0; fs && i < fs->nfont; i++) {
        FcPattern *font = fs->fonts[i];//FcFontSetFont(fs, i);
        //FcPatternPrint(font);
        FcChar8 *s = FcNameUnparse(font);
        FcChar8 *file;
        if(FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
            //printf("Filename: %s", file);
        }
        //printf("Font: %s", s);
        free(s);
    }

    FcFontSetDestroy(fs);
    FcObjectSetDestroy(os);
    FcPatternDestroy(pat);
    //FcConfigDestroy(config);
    // Trying to brake it:
    FcFini();
    FcFini();
    FcFini();
    FcFini();


cairo_debug_reset_static_data();
    FcFini();


#if 1
    struct PnWidget *win = pnWindow_create(0, 10, 10,
            0, 0, 0, 0, PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xFF000000, 0);

    struct PnWidget *label = (void *) pnLabel_create(
            win/*parent*/,
            0/*width*/, 30/*height*/,
            10/*xPadding*/, 10/*yPadding*/,
            PnAlign_CC/*align*/,
            PnExpand_HV/*expand*/, "Hello World");
    ASSERT(label);
    pnWidget_setBackgroundColor(label, 0xFFF0F0F0, 0);
    pnLabel_setFontColor(label, 0xFF000000);


cairo_debug_reset_static_data();

    label = (void *) pnLabel_create(
            win/*parent*/,
            0/*width*/, 30/*height*/,
            10/*xPadding*/, 10/*yPadding*/,
            PnAlign_CC/*align*/,
            PnExpand_HV/*expand*/, "Goodbye World");
    ASSERT(label);
    pnWidget_setBackgroundColor(label, 0xFFF0F0F0, 0);
    pnLabel_setFontColor(label, 0xFF000000);

cairo_debug_reset_static_data();
 FcFini();
cairo_debug_reset_static_data();
cairo_debug_reset_static_data();
cairo_debug_reset_static_data();


    pnWindow_show(win);

    Run(win);
#endif

    // Adding this two lines brakes it with valgrind.
    //cairo_debug_reset_static_data();
    //FcFini();

    return 0;
}

#ifndef DSO
int main(void) {

    return runLabels();
}
#endif
