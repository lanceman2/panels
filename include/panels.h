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
#define PN_WINDOW_BGCOLOR (0xFF999900)
#define PN_DEFAULT_WINDOW_WIDTH  (400)
#define PN_DEFAULT_WINDOW_HEIGHT (400)
#define PN_DEFAULT_WIDGET_WIDTH  (40)
#define PN_DEFAULT_WIDGET_HEIGHT (40)


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
    // container packing method. The window or widget will define its own
    // packing.
    PnDirection_Callback
};


// Expansiveness (space greed) is an attribute of a widget and not the
// container it is in.  A container that contains a widget that can
// expand, can also expand given space to the contained widget while not
// expanding it's border sizes.  The expand attribute of a container
// widget does not effect the container widgets border size unless all
// the children widgets do not expand (in the direction of interest).
//
// Sibling widgets (widgets in the same container) that can expand share
// the extra space.
//
// Exception: A top level window that cannot expand (resize) unless the
// user set its expand flag; in pnWindow_create() or other function.
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
// The default is PnAlign_LT will push the widgets to the top left corner
// which is were the window manager (compositor) puts the x and y
// positions at zero and zero.  That's just a given convention that we go
// along with.  All popular operating systems with windowing follow that
// convention.
//
// If any of the widgets in the container can expand, than the Align
// value of the container is not considered.
//
// Put another way: the "align" widget attribute does not adjust the child
// widget sizes, it can just change the child widgets positions; but only
// if none of the sibling widgets expand to take all the "extra" space.

// The default "align" is:
//     Left X-direction (horizontal) and Top Y-direction (vertical).
//

// x -> first 2 bits  y -> next 2 bits

#define PN_ALIGN_X  (03)
#define PN_ALIGN_Y  (03 << 2)


#define PN_ALIGN_X_LEFT      (00) // default
#define PN_ALIGN_X_RIGHT     (01) 
#define PN_ALIGN_X_CENTER    (02)
#define PN_ALIGN_X_JUSTIFIED (03)

#define PN_ALIGN_Y_TOP       (00) // default
#define PN_ALIGN_Y_BOTTOM    (01 << 2)
#define PN_ALIGN_Y_CENTER    (02 << 2)
#define PN_ALIGN_Y_JUSTIFIED (03 << 2)

enum PnAlign {

    // x -> first 2 bits  y -> next 2 bits

    PnAlign_LT = (PN_ALIGN_X_LEFT      | PN_ALIGN_Y_TOP), // = 0
    PnAlign_RT = (PN_ALIGN_X_RIGHT     | PN_ALIGN_Y_TOP),
    PnAlign_CT = (PN_ALIGN_X_CENTER    | PN_ALIGN_Y_TOP),
    PnAlign_JT = (PN_ALIGN_X_JUSTIFIED | PN_ALIGN_Y_TOP),

    PnAlign_LB = (PN_ALIGN_X_LEFT      | PN_ALIGN_Y_BOTTOM),
    PnAlign_RB = (PN_ALIGN_X_RIGHT     | PN_ALIGN_Y_BOTTOM),
    PnAlign_CB = (PN_ALIGN_X_CENTER    | PN_ALIGN_Y_BOTTOM),
    PnAlign_JB = (PN_ALIGN_X_JUSTIFIED | PN_ALIGN_Y_BOTTOM),

    PnAlign_LC = (PN_ALIGN_X_LEFT      | PN_ALIGN_Y_CENTER),
    PnAlign_RC = (PN_ALIGN_X_RIGHT     | PN_ALIGN_Y_CENTER),
    PnAlign_CC = (PN_ALIGN_X_CENTER    | PN_ALIGN_Y_CENTER),
    PnAlign_JC = (PN_ALIGN_X_JUSTIFIED | PN_ALIGN_Y_CENTER),

    PnAlign_LJ = (PN_ALIGN_X_LEFT      | PN_ALIGN_Y_JUSTIFIED),
    PnAlign_RJ = (PN_ALIGN_X_RIGHT     | PN_ALIGN_Y_JUSTIFIED),
    PnAlign_CJ = (PN_ALIGN_X_CENTER    | PN_ALIGN_Y_JUSTIFIED),
    PnAlign_JJ = (PN_ALIGN_X_JUSTIFIED | PN_ALIGN_Y_JUSTIFIED)
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
        enum PnAlign align,
        enum PnExpand expand);

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

PN_EXPORT void pnSurface_setDraw(struct PnSurface *s,
        int (*draw)(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 byte chunks*/,
            void *userData), void *userData);
static inline void pnWindow_setDraw(struct PnWindow *window,
        int (*draw)(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 byte chunks*/,
            void *userData), void *userData) {
     pnSurface_setDraw((void *) window, draw, userData);
}
static inline void pnWidget_setDraw(struct PnWidget *widget,
        int (*draw)(struct PnSurface *surface, uint32_t *pixels,
            uint32_t w, uint32_t h, uint32_t stride/*4 byte chunks*/,
            void *userData), void *userData) {
     pnSurface_setDraw((void *) widget, draw, userData);
}


//PN_EXPORT struct PnSurface *pnWindow_getSurface(struct PnWindow *window);
//PN_EXPORT struct PnSurface *pnWidget_getSurface(struct PnWidget *widget);




#ifdef __cplusplus
}
#endif


#endif
