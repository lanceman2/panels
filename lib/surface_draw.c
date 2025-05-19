#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>

#include "../include/panels.h"

#include "xdg-shell-protocol.h"
#include "xdg-decoration-protocol.h"

#include "debug.h"
#include "display.h"

#include "../include/panels_drawingUtils.h"



// This function calls itself.
//
void pnSurface_draw(const struct PnWidget *s, const struct PnBuffer *buffer) {

    DASSERT(s);
    DASSERT(!s->culled);
    DASSERT(s->window);
 
    if(s->window->needAllocate && s->config)
        s->config((void *) s,
                buffer->pixels +
                s->allocation.y * buffer->stride +
                s->allocation.x,
                s->allocation.x, s->allocation.y,
                s->allocation.width, s->allocation.height,
                buffer->stride, s->configData);

    // All parent surfaces, including the window, could have nothing to draw
    // if their children widgets completely cover them.
    if(s->noDrawing) goto drawChildren;

#ifdef WITH_CAIRO
    if(s->cairoDraw) {
        DASSERT(s->cr);
        if(s->cairoDraw((void *) s, s->cr, s->cairoDrawData) == 1)
            pnWidget_queueDraw((void *) s);
    } else if(!s->draw) {
        DASSERT(s->cr);
        uint32_t c = s->backgroundColor;
        cairo_set_source_rgba(s->cr,
               ((0x00FF0000 & c) >> 16)/255.0, // R
               ((0x0000FF00 & c) >> 8)/255.0,  // G
               ((0x000000FF & c))/255.0,       // B
               ((0xFF000000 & c) >> 24)/255.0  // A
        );
        cairo_paint(s->cr);
    }
#else // without Cairo
    if(!s->draw)
        pn_drawFilledRectangle(buffer->pixels,
                s->allocation.x, s->allocation.y, 
                s->allocation.width, s->allocation.height,
                buffer->stride,
                s->backgroundColor /*color in ARGB*/);
#endif
    else
        if(s->draw((void *) s,
                buffer->pixels +
                s->allocation.y * buffer->stride +
                s->allocation.x,
                s->allocation.width, s->allocation.height,
                buffer->stride, s->drawData) == 1)
            pnWidget_queueDraw((void *) s);

drawChildren:

    switch(s->layout) {

        case PnLayout_None:
            break;

        case PnLayout_Cover:
        case PnLayout_One:
        case PnLayout_LR:
        case PnLayout_RL:
        case PnLayout_BT:
        case PnLayout_TB:
            // The s container uses some kind of listed list.
            for(struct PnWidget *c = s->l.firstChild; c;
                    c = c->pl.nextSibling) {
                if(!c->culled)
                    pnSurface_draw(c, buffer);
            }
            return;

        case PnLayout_Grid:
        {
            if(!s->g.grid)
                // No children yet.
                return;
            // s is a grid container.
            struct PnWidget ***child = s->g.grid->child;
            DASSERT(child);
            DASSERT(s->g.numRows);
            DASSERT(s->g.numColumns);
            for(uint32_t y=s->g.numRows-1; y != -1; --y)
                for(uint32_t x=s->g.numColumns-1; x != -1; --x) {
                    struct PnWidget *c = child[y][x];
                    if(!c || c->culled) continue;
                    // rows and columns can share widgets; like for row
                    // span N and/or column span N; that is adjacent cells
                    // that share the same widget.
                    if(IsUpperLeftCell(c, child, x, y))
                        pnSurface_draw(c, buffer);
                }
            return;
        }
    }
}
