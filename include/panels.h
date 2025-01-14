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

struct PnWindow;
struct PnWidget;
struct PnSurface;


PN_EXPORT bool pnDisplay_dispatch(void);
PN_EXPORT bool pnDisplay_haveXDGDecoration(void);

PN_EXPORT struct PnWindow *pnWindow_create(uint32_t w, uint32_t h);
PN_EXPORT void pnWindow_destroy(struct PnWindow *window);
PN_EXPORT void pnWindow_show(struct PnWindow *window, bool show);
PN_EXPORT void pnWindow_setCBDestroy(struct PnWindow *window,
        void (*destroy)(struct PnWindow *window, void *userData),
        void *userData);

PN_EXPORT struct PnWidget *pnWidget_create(
        struct PnSurface *parent, uint32_t w, uint32_t h);
PN_EXPORT void pnWidget_destroy(struct PnWidget *widget);

//PN_EXPORT struct PnSurface *pnWindow_getSurface(struct PnWindow *window);
//PN_EXPORT struct PnSurface *pnWidget_getSurface(struct PnWidget *window);




#ifdef __cplusplus
}
#endif


#endif
