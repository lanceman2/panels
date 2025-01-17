#ifndef __PN_H__
#define __PN_H__

#include <stdbool.h>
#include <stdint.h>

#ifndef PN_EXPORT
#  define PN_EXPORT extern
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define PN_PIXEL_SIZE     (4) // bytes per pixel
// DEFAULTS
#define PN_BORDER_WIDTH   (6) // default border pixels wide
#define PN_WINDOW_BGCOLOR (0x09999900)

// Child widget packing gravity (for lack of a better word)
//
// Is there a "field/branch" in mathematics that we can know that will
// give use a more optimal way to parametrise widget rectangle packing?
// We need to keep in mind that our rectangles can grown and shrink as
// needed to make the resulting window fully packed (with no gaps).
//
// Studying the behavior of gvim (the program) with changing multiple edit
// views is very helpful in studying widget containerization.  Like for
// example how it splits a edit wiew.  And, what happens when a view is
// removed from in between views.  And, what happens when a view (widget)
// border is moved when there are a lot of views in a large grid of views.
//
// As in general relativity, gravity defines how space is distributed
// among its massive pieces.
//
// A better term might be "alignment", in place of "gravity".
// Or how about "direction".
//
enum PnGravity {

    // PnGravity is a attribute of a widget container (surface), be it a
    // widget or window.

    // PnGravity_None and PnGravity_One just add failure modes for when
    // the user adds extra children.

    // no gravity to hold widgets.  Cannot have any children.
    PnGravity_None = -2, // For non-container widgets or windows

    // The container surface can only have zero or one widget, so it puts
    // the child widget where ever it wants to on its surface.
    PnGravity_One = -1,

    // Windows (and widgets) with all the below gravities can have zero
    // or more children.

    // T Top, B Bottom, L Left, R Right
    //
    // Horizontally row aligning child widgets:
    //
    // We use 0 so that PnGravity_LR is the default; it certainly made
    // writing test code easier.
    //
    PnGravity_LR = 0, // child widgets float/align left to right
    PnGravity_RL = 1,  // child widgets float/align right to left

    // Vertically column aligning child widgets
    // PnGravity_BT is like real world gravity pulling down where the
    // starting widget gets squished on the bottom.
    PnGravity_BT = 2, // child widgets float/align bottom to top
    PnGravity_TB = 3, // child widgets float/align top to bottom

    // We could use this gravity to make a grid, a notebook, or other
    // container packing method.
    PnGravity_Callback // The window or widget will define its own packing.
};


// Widgets can be greedy for different kinds of space.
// The greedy widget will take the space it can get, but it has
// to share that space with other sibling greedy widgets.
//
// Q: If not, and no children in the tree are greedy in X (or Y too), then
// does that mean that the window will not be expandable in the X (Y)
// direction?  I'm thinking the answer is no; but there may be a need to
// have the idea of a windows natural (X and Y) size for this case (a
// TODO).
//
enum PnGreed {
    // H Horizontal first bit, V Vertical second bit
    PnGreed_None = 00, // The widget is not greedy for any space
    PnGreed_H    = 01, // The widget is greedy for Horizontal space
    PnGreed_V    = 02, // The widget is greedy for Vertical space
    PnGreed_HV   = (01 & 02), // The widget is greedy for all 2D space
    PnGreed_VH   = (01 & 02)  // The widget is greedy for all 2D space
};


struct PnWindow;
struct PnWidget;
// both PnWindow and PnWidget are PnSurface
struct PnSurface;


PN_EXPORT bool pnDisplay_dispatch(void);
PN_EXPORT bool pnDisplay_haveXDGDecoration(void);

PN_EXPORT struct PnWindow *pnWindow_create(
        // parent = 0 for toplevel
        // parent != 0 for popup
        struct PnWindow *parent,
        /* For containers, width is left/right border thickness, height is
         * top/bottom border thickness. */
        uint32_t width, uint32_t height,
        int32_t x, int32_t y,
        enum PnGravity gravity);

PN_EXPORT void pnWindow_destroy(struct PnWindow *window);
PN_EXPORT void pnWindow_show(struct PnWindow *window, bool show);
PN_EXPORT void pnWindow_setCBDestroy(struct PnWindow *window,
        void (*destroy)(struct PnWindow *window, void *userData),
        void *userData);
PN_EXPORT void pnWindow_queueDraw(struct PnWindow *window);

PN_EXPORT struct PnWidget *pnWidget_create(
        struct PnSurface *parent,
        uint32_t width, uint32_t height,
        enum PnGravity gravity, enum PnGreed greed);
PN_EXPORT void pnWidget_show(struct PnWidget *widget, bool show);
PN_EXPORT void pnWidget_queueDraw(struct PnWidget *widget);
PN_EXPORT void pnWidget_destroy(struct PnWidget *widget);



PN_EXPORT void pnSurface_setBackgroundColor(
        struct PnSurface *s, uint32_t argbColor);

static inline void pnWindow_setBackgroundColor(
        struct PnWindow *window, uint32_t argbColor) {
    pnSurface_setBackgroundColor((void *) window, argbColor);
}
static inline void pnWidget_setBackgroundColor(
        struct PnWidget *widget, uint32_t argbColor) {
    pnSurface_setBackgroundColor((void *) widget, argbColor);
}


//PN_EXPORT struct PnSurface *pnWindow_getSurface(struct PnWindow *window);
//PN_EXPORT struct PnSurface *pnWidget_getSurface(struct PnWidget *window);




#ifdef __cplusplus
}
#endif


#endif
