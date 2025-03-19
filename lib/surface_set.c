#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"


static bool MotionEnter(struct PnWidget *w,
            uint32_t x, uint32_t y, void *userData) {
    DASSERT(w);
    return true; // Take focus so we can get motion events.
}

static void MotionLeave(struct PnWidget *w, void *userData) {
    DASSERT(w);
}


void pnWidget_setDraw(
        struct PnWidget *widget,
        int (*draw)(struct PnWidget *widget, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride,
            void *userData), void *userData) {
    DASSERT(widget);
    widget->surface.draw = draw;
    widget->surface.drawData = userData;
}

void pnWidget_setConfig(struct PnWidget *widget,
        void (*config)(struct PnWidget *widget, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, uint32_t stride/*4 byte chunks*/,
            void *userData), void *userData) {
    DASSERT(widget);
    widget->surface.config = config;
    widget->surface.configData = userData;
}

void pnWidget_getAllocation(const struct PnWidget *w,
        struct PnAllocation *a) {
    DASSERT(w);
    memcpy(a, &w->surface.allocation, sizeof(*a));
}

void pnWidget_setBackgroundColor(
        struct PnWidget *w, uint32_t argbColor) {
    w->surface.backgroundColor = argbColor;
}

uint32_t pnWidget_getBackgroundColor(struct PnWidget *w) {
    DASSERT(w);
    return w->surface.backgroundColor;
}

void pnWidget_setEnter(struct PnWidget *w,
        bool (*enter)(struct PnWidget *surface,
            uint32_t x, uint32_t y, void *userData),
        void *userData) {

    DASSERT(w);
    w->surface.enter = enter;
    w->surface.enterData = userData;

    if(!enter) {
        // Motion, Enter, and Leave are closely related.  See
        // pnWidget_setMotion().
        if(w->surface.motion)
            w->surface.enter = MotionEnter;
    }
}

void pnWidget_setLeave(struct PnWidget *w,
        void (*leave)(struct PnWidget *surface, void *userData),
        void *userData) {

    DASSERT(w);
    w->surface.leave = leave;
    w->surface.leaveData = userData;

    if(!leave) {
        // Motion, Enter, and Leave are closely related.  See
        // pnWidget_setMotion().
        if(w->surface.motion)
            w->surface.leave = MotionLeave;
    }

}

void pnWidget_setPress(struct PnWidget *w,
        bool (*press)(struct PnWidget *surface,
            uint32_t which,
            int32_t x, int32_t y, void *userData),
        void *userData) {

    DASSERT(w);
    w->surface.press = press;
    w->surface.pressData = userData;
}

void pnWidget_setRelease(struct PnWidget *w,
        bool (*release)(struct PnWidget *surface,
            uint32_t which,
            int32_t x, int32_t y, void *userData),
        void *userData) {

    DASSERT(w);
    w->surface.release = release;
    w->surface.releaseData = userData;

    if(!release && d.buttonGrabSurface == &w->surface) {
        // If we are removing the release callback (setting to 0), we need
        // to make sure that there is no pending button grab, and if there
        // is a button grab we must release the grab.
        d.buttonGrabSurface = 0;
        d.buttonGrab = 0;
    }
}

void pnWidget_setMotion(struct PnWidget *w,
        bool (*motion)(struct PnWidget *surface,
            int32_t x, int32_t y, void *userData),
        void *userData) {

    DASSERT(w);
    w->surface.motion = motion;
    w->surface.motionData = userData;

    // We need to get focus (from the enter event) if we want motion
    // events.  Enter, Leave, and Motion are closely related; that's just
    // the way things are.

    if(motion) {
        if(!w->surface.enter)
            w->surface.enter = MotionEnter;
        if(!w->surface.leave)
            w->surface.leave = MotionLeave;
    } else {
        if(w->surface.enter == MotionEnter)
            w->surface.enter = 0;
        if(w->surface.leave == MotionLeave)
            w->surface.leave = 0;
    }
}

#if 0
void pnWidget_setMinWidth(struct PnWidget *w, uint32_t width) {
    DASSERT(w);
    if(w->surface.width != width)
        *((uint32_t *) &w->surface.width) = width;

    // TODO: Let the user queue the draw??
}
#endif
