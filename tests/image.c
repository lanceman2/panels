#include <stdlib.h>
#include <signal.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "rand.h"
#include "run.h"

#define FILENAME "/usr/share/icons/hicolor/256x256/"\
    "apps/libreoffice-writer.png"


static struct PnWidget *win;


static void W(const char *filename, uint32_t w, uint32_t h) {

    struct PnWidget *widget = pnImage_create(win/*parent*/,
            filename,
            w/*width*/, h/*height*/,
            PnAlign_RC/*align image relative to the widget surface*/,
            PnExpand_HV/*expand*/);
    ASSERT(widget);
    //TODO: What does image widget do with background color.
    pnWidget_setBackgroundColor(widget, Color(), 0);
}

static void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    srand(123);

    win = pnWindow_create(0, 10/*width*/, 10/*height*/,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/,
            PnAlign_CC/*align*/, PnExpand_HV);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xFF000000, 0);

    W(FILENAME, 0, 0);
    W(FILENAME, 0, 100);
    W(FILENAME, 100, 100);
    W(FILENAME, 100, 0);

    pnWindow_show(win);

    Run(win);

    return 0;
}
