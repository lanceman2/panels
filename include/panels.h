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


// We make default enumeration values be 0.  That saves a ton of
// development time.


// Child widget direction packing order
//
// Container widget attribute PnDirection.
//
// Is there a "field/branch" in mathematics that we can know that will
// give use a more optimal way to parametrise widget rectangle packing?
// We need to keep in mind that our rectangles can grown and shrink as
// needed to make the resulting window fully packed (with no gaps).
//
// Studying the behavior of gvim (the program) with changing multiple edit
// views is very helpful in studying widget containerization.  Like for
// example how it splits a edit view.  And, what happens when a view is
// removed from in between views.  And, what happens when a view (widget)
// border is moved when there are a lot of views in a large grid of views.
//
// Like text flow direction.  Example: in English words of on the page
// from left to right, the first word read is on the left.
//
// Packing order direction: example PnDirection_RL means the first added
// goes to the right side, second added goes next to the first just to the
// left of the first.
//
enum PnDirection {

    // PnDirection is a attribute of a widget container (surface), be it a
    // widget or window.

    // PnDirection_None and PnDirection_One just add failure modes for when
    // the user adds extra children.

    // no gravity to hold widgets.  Cannot have any children.
    PnDirection_None = -2, // For non-container widgets or windows

    // The container surface can only have zero or one widget, so it puts
    // the child widget where ever it wants to on its surface.
    PnDirection_One = -1,

    // Windows (and widgets) with all the below gravities can have zero
    // or more children.

    // T Top, B Bottom, L Left, R Right
    //
    // Horizontally row aligning child widgets:
    //
    // We use 0 so that PnDirection_LR is the default; it certainly made
    // writing test code easier.
    //
    PnDirection_LR = 0, // child widgets assemble from left to right
    PnDirection_RL = 1, // child widgets assemble from right to left

    // Vertically column aligning child widgets:
    //
    PnDirection_TB = 2, // child widgets assemble top to bottom
    //
    // PnDirection_BT is like real world gravity pulling down where the
    // starting widget gets squished on the bottom.
    PnDirection_BT = 3, // child widgets assemble bottom to top

    // We could use this direction to make a grid, a notebook, or other
    // container packing method.
    PnDirection_Callback // The window or widget will define its own packing.
};


// Expansiveness (space greed) is an attribute of a widget and not the
// container it is in.  Hence, expansiveness (expand) is not an attribute
// of a window since windows are never contained (in this API).
//
// Sibling widgets (widgets in the same container) that can expand share
// the extra space.
//
enum PnExpand {
    // H Horizontal first bit, V Vertical second bit
    PnExpand_None = 00, // The widget is not greedy for any space
    PnExpand_H    = 01, // The widget is greedy for Horizontal space
    PnExpand_V    = 02, // The widget is greedy for Vertical space
    PnExpand_HV   = (01 | 02), // The widget is greedy for all 2D space
    PnExpand_VH   = (01 | 02)  // The widget is greedy for all 2D space
};


// Container widget attribute Align:
//
// If there is extra space for a widget that will not be filled, we
// align (float) the widget into position.
//
// If any of the widgets in the container can expand, than the Align
// value of the container is not considered.
enum PnAlign {

    PnAlign_L = 0, // Left (default horizontal)
    PnAlign_R = 1, // Right
    PnAlign_C = 2, // Center for both horizontal and vertical
    PnAlign_T = 0, // Top (default vertical)
    PnAlign_B = 1, // Bottom
    PnAlign_J = 3  // Justified
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
        enum PnDirection direction,
        enum PnAlign align);

PN_EXPORT void pnWindow_destroy(struct PnWindow *window);
PN_EXPORT void pnWindow_show(struct PnWindow *window, bool show);
PN_EXPORT void pnWindow_setCBDestroy(struct PnWindow *window,
        void (*destroy)(struct PnWindow *window, void *userData),
        void *userData);
PN_EXPORT void pnWindow_queueDraw(struct PnWindow *window);

PN_EXPORT struct PnWidget *pnWidget_create(
        struct PnSurface *parent,
        uint32_t width, uint32_t height,
        enum PnDirection direction,
        enum PnAlign align,
        enum PnExpand expand);
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
