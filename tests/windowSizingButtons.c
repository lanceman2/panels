#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>

#include "../include/panels.h"

#include "../lib/debug.h"

#include "rand.h"


static void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static struct PnWidget *win;

static bool click(struct PnWidget *b,  void (*func)(void)) {

    ASSERT(b);

    if(func)
        func();
    else
        fprintf(stderr, "CLICKED button %p\n", b);
    return false;
}

static void Minimize(void) {
    pnWindow_setMinimized(win);
}
static void Maximize(void) {
    pnWindow_setMaximized(win);
}
static void unsetMaximize(void) {
    pnWindow_unsetMaximized(win);
}
static void Fullscreen(void) {
    pnWindow_setFullscreen(win);
}
static void unsetFullscreen(void) {
    pnWindow_unsetFullscreen(win);
}
static void setShrinkWrapped(void) {
    pnWindow_setShrinkWrapped(win);
}
static void quit(void) {
    win = 0;
}

static void Button(
        void (*func)(void), const char *label,
        uint32_t i, uint32_t j,
        uint32_t rowSpan, uint32_t colSpan) {

    struct PnWidget *button = pnButton_create(0/*parent*/,
            0/*width*/, 0/*height*/,
            0/*layout*/, 0/*align*/,
            PnExpand_VH, 0/*label*/, false/*toggle*/);
    ASSERT(button);
    pnWidget_addChildToGrid(win, button, i, j, rowSpan, colSpan);

    struct PnWidget *l = (void *) pnLabel_create(
            button/*parent*/,
            0/*width*/, 30/*height*/,
            4/*xPadding*/, 4/*yPadding*/,
            0/*align*/, PnExpand_HV/*expand*/, label);
    ASSERT(l);
    pnLabel_setFontColor(l, 0xF0000000);
    uint32_t color = 0xDA000000 | (0x00FFFFFF & Color());
    pnWidget_setBackgroundColor(l, color);
    pnWidget_setBackgroundColor(button, color);
    pnWidget_addCallback(button, PN_BUTTON_CB_CLICK, click, func);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    srand(5);

    win = pnWindow_createAsGrid(
        0 /*parent*/,
        5/*width*/, 5/*height*/, 0/*x*/, 0/*y*/,
        0/*align*/, PnExpand_HV/*expand*/,
        0/*numColumns*/, 0/*numRows*/);
    ASSERT(win);
    pnWidget_setBackgroundColor(win, 0xFF000000);

    Button(Minimize, "Minimize", 0, 0, 2, 1);
    Button(Maximize, "Maximize", 0, 1, 1, 1);
    Button(unsetMaximize, "Unset Maximize", 1, 1, 1, 1);
    Button(Fullscreen, "Fullscreen", 0, 2, 1, 1);
    Button(unsetFullscreen, "Unset Fullscreen", 1, 2, 1, 1);
    Button(setShrinkWrapped, "Set Shrink Wrapped", 0, 3, 2, 1);
    Button(0, "Hello", 0, 4, 1, 1);
    Button(quit, "Quit", 1, 4, 1, 1);

    pnWindow_show(win);

    while(win && pnDisplay_dispatch());

    return 0;
}
