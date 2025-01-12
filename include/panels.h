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

struct PnWindow;

PN_EXPORT bool pnDisplay_dispatch(void);
PN_EXPORT bool pnDisplay_haveXDGDecoration(void);

PN_EXPORT struct PnWindow *pnWindow_create(uint32_t w, uint32_t h);
PN_EXPORT void pnWindow_destroy(struct PnWindow *window);

#ifdef __cplusplus
}
#endif


#endif
