#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"


static bool MotionEnter(struct PnSurface *surface,
            uint32_t x, uint32_t y, void *userData) {
    DASSERT(surface);
    return true; // Take focus so we can get motion events.
}

static void MotionLeave(struct PnSurface *surface, void *userData) {
    DASSERT(surface);
}


void pnSurface_setDraw(
        struct PnSurface *s,
        int (*draw)(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride,
            void *userData), void *userData) {
    DASSERT(s);

    s->draw = draw;
    s->drawData = userData;
}

void pnSurface_setConfig(struct PnSurface *s,
        void (*config)(struct PnSurface *surface, uint32_t *pixels,
            uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, uint32_t stride/*4 byte chunks*/,
            void *userData), void *userData) {
    DASSERT(s);
    s->config = config;
    s->configData = userData;
}

void pnSurface_getAllocation(const struct PnSurface *s,
        struct PnAllocation *a) {
    DASSERT(s);
    memcpy(a, &s->allocation, sizeof(*a));
}

void pnSurface_setBackgroundColor(
        struct PnSurface *s, uint32_t argbColor) {
    s->backgroundColor = argbColor;
}

uint32_t pnSurface_getBackgroundColor(struct PnSurface *s) {
    DASSERT(s);
    return s->backgroundColor;
}

void pnSurface_setEnter(struct PnSurface *s,
        bool (*enter)(struct PnSurface *surface,
            uint32_t x, uint32_t y, void *userData),
        void *userData) {

    DASSERT(s);
    s->enter = enter;
    s->enterData = userData;

    if(!enter) {
        // Motion, Enter, and Leave are closely related.  See
        // pnSurface_setMotion().
        if(s->motion)
            s->enter = MotionEnter;
    }
}

void pnSurface_setLeave(struct PnSurface *s,
        void (*leave)(struct PnSurface *surface, void *userData),
        void *userData) {

    DASSERT(s);
    s->leave = leave;
    s->leaveData = userData;

    if(!leave) {
        // Motion, Enter, and Leave are closely related.  See
        // pnSurface_setMotion().
        if(s->motion)
            s->leave = MotionLeave;
    }

}

void pnSurface_setPress(struct PnSurface *s,
        bool (*press)(struct PnSurface *surface,
            uint32_t which,
            int32_t x, int32_t y, void *userData),
        void *userData) {

    DASSERT(s);
    s->press = press;
    s->pressData = userData;
}

void pnSurface_setRelease(struct PnSurface *s,
        bool (*release)(struct PnSurface *surface,
            uint32_t which,
            int32_t x, int32_t y, void *userData),
        void *userData) {

    DASSERT(s);
    s->release = release;
    s->releaseData = userData;

    if(!release && d.buttonGrabSurface == s) {
        // If we are removing the release callback (setting to 0), we need
        // to make sure that there is no pending button grab, and if there
        // is a button grab we must release the grab.
        d.buttonGrabSurface = 0;
        d.buttonGrab = 0;
    }
}

void pnSurface_setMotion(struct PnSurface *s,
        bool (*motion)(struct PnSurface *surface,
            int32_t x, int32_t y, void *userData),
        void *userData) {

    DASSERT(s);
    s->motion = motion;
    s->motionData = userData;

    // We need to get focus (from the enter event) if we want motion
    // events.  Enter, Leave, and Motion are closely related; that's just
    // the way things are.

    if(motion) {
        if(!s->enter)
            s->enter = MotionEnter;
        if(!s->leave)
            s->leave = MotionLeave;
    } else {
        if(s->enter == MotionEnter)
            s->enter = 0;
        if(s->leave == MotionLeave)
            s->leave = 0;
    }
}

#if 0
void pnSurface_setMinWidth(struct PnSurface *s, uint32_t width) {
    DASSERT(s);
    if(s->width != width)
        *((uint32_t *) &s->width) = width;

    // TODO: Let the user queue the draw??
}
#endif
