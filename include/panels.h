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

#define PN_PIXEL_SIZE  4 // bytes per pixel


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
// removed from in between views.  And, what happen when a view (widget)
// border is moved when there are a lot of views in a large grid of views.
//
// As in general relativity, gravity defines how space is distributed
// among its massive pieces.
//
enum PnGravity {

    // PnGravity is a attribute of a widget container (surface), be it a
    // widget or window.

    PnGravity_None = 0, // For non-container widgets or windows

    // The container surface can only have zero or one widget, so it puts
    // the child widget where ever it wants to on its surface.
    PnGravity_One,

    // T Top, B Bottom, L Left, R Right
    //
    // Vertically column aligning child widgets
    PnGravity_TB, // child widgets float/align top to bottom
    PnGravity_BT, // child widgets float/align bottom to top

    // Horizontally row aligning child widgets
    PnGravity_LR, // child widgets float/align left to right
    PnGravity_RL,  // child widgets float/align right to left

    PnGravity_Callback // The window or widget will define its own packing.
};


struct PnWindow;
struct PnWidget;
// both PnWindow and PnWidget are PnSurface
struct PnSurface;


PN_EXPORT bool pnDisplay_dispatch(void);
PN_EXPORT bool pnDisplay_haveXDGDecoration(void);

PN_EXPORT struct PnWindow *pnWindow_create(uint32_t w, uint32_t h,
        enum PnGravity gravity);
PN_EXPORT void pnWindow_destroy(struct PnWindow *window);
PN_EXPORT void pnWindow_show(struct PnWindow *window, bool show);
PN_EXPORT void pnWindow_setCBDestroy(struct PnWindow *window,
        void (*destroy)(struct PnWindow *window, void *userData),
        void *userData);

PN_EXPORT struct PnWidget *pnWidget_create(
        struct PnSurface *parent, uint32_t w, uint32_t h,
        enum PnGravity gravity);
PN_EXPORT void pnWidget_destroy(struct PnWidget *widget);

//PN_EXPORT struct PnSurface *pnWindow_getSurface(struct PnWindow *window);
//PN_EXPORT struct PnSurface *pnWidget_getSurface(struct PnWidget *window);




#ifdef __cplusplus
}
#endif


#endif
