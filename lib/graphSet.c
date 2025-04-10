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


static inline void Check(struct PnWidget *g) {
    DASSERT(g);
    ASSERT(GET_WIDGET_TYPE(g->type) == W_GRAPH);
}

void pnGraph_setSubGridColor(struct PnWidget *g, uint32_t color) {
    Check(g);
    ((struct PnGraph *) g)->subGridColor = color;
}
void pnGraph_setGridColor(struct PnWidget *g, uint32_t color) {
    Check(g);
    ((struct PnGraph *) g)->gridColor = color;
}
void pnGraph_setLabelsColor(struct PnWidget *g, uint32_t color) {
    Check(g);
    ((struct PnGraph *) g)->labelsColor = color;
}
void pnGraph_setZeroLineColor(struct PnWidget *g, uint32_t color) {
    Check(g);
    ((struct PnGraph *) g)->zeroLineColor = color;
}
