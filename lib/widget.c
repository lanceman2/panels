#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include  "display.h"

#include "../include/panels.h"
#include "../include/panels_drawingUtils.h"


struct PnWidget *pnWidget_create(
        struct PnSurface *parent, uint32_t w, uint32_t h) {

    return 0;
}


void pnWidget_destroy(struct PnWidget *widget) {

}
