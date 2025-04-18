#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>

#include <cairo/cairo.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"
#include "graph.h"


void pnPlot_setLineColor(struct PnPlot *p, uint32_t color) {
    DASSERT(p);
    p->lineColor = color;
}

void pnPlot_setPointColor(struct PnPlot *p, uint32_t color) {
    DASSERT(p);
    p->pointColor = color;
}

void pnPlot_setLineWidth(struct PnPlot *p, double width) {
    DASSERT(p);
    DASSERT(width >= 0);
    p->lineWidth = width;
}

void pnPlot_setPointSize(struct PnPlot *p, double size) {
    DASSERT(p);
    DASSERT(size >= 0);
    p->pointSize = size;
}
