#ifndef __PN_H__
#define __PN_H__

#include <stdbool.h>

#ifndef PN_EXPORT
#  define PN_EXPORT extern
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct PnWindow;

PN_EXPORT struct PnWindow *pnWindow_create(void);
PN_EXPORT void pnWindow_destroy(struct PnWindow *window);

PN_EXPORT bool npDisplay_run(void);

#ifdef __cplusplus
}
#endif


#endif
