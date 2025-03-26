#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>

#include <cairo/cairo.h>

#include "gridLines.h"

#include "../include/panels.h"
#include "debug.h"
#include "display.h"




static GdkWindow *window = 0;


gboolean graph_buttonPress_cb(GtkWidget *drawingArea,
        GdkEventButton *e, struct QsGraph *g) {

  DASSERT(g);
  DASSERT(drawingArea);
  DASSERT(g->drawingArea == drawingArea);

  if(g->zoom_action)
    // Do nothing if there are actions already underway.  Example:
    // pressing another button while one is already pressed.
    return TRUE;

  if(e->button == SLIDE_BUTTON) {
      // Mark the action as running.
      g->zoom_action = SLIDE_ACTION;
      window = gtk_widget_get_window(drawingArea);
      DASSERT(window);
      gdk_window_set_cursor(window, hand_cursor);
      // record starting position
      g->x0 = e->x;
      g->y0 = e->y;
  }

  if(e->button == BOX_BUTTON) {
      // Mark the action as running.
      g->zoom_action = BOX_ACTION;
      // record starting position
      g->x0 = e->x;
      g->y0 = e->y;
  }


  return TRUE; // TRUE -> eat the event
}

gboolean graph_buttonRelease_cb(GtkWidget *drawingArea,
        GdkEventButton *e, struct QsGraph *g) {

  DASSERT(g);
  DASSERT(drawingArea);
  DASSERT(g->drawingArea == drawingArea);

  if(!g->zoom_action)
    // Do nothing if there no actions currently underway.  It may be that
    // we got two buttons pressed and this is the release of the button
    // that got no action because another button was pressed while that
    // button was pressed.
    return TRUE;

  if(e->button == SLIDE_BUTTON && (g->zoom_action & SLIDE_ACTION)) {
      DASSERT(window);
      gdk_window_set_cursor(window, 0);
      window = 0;
      // Mark the action as done.
      g->zoom_action &= ~SLIDE_ACTION;

      double dx, dy;
      dx = g->x0 - e->x;
      dy = g->y0 - e->y;

      if(dx > g->padX)
          dx = g->padX;
      else if(dx < - g->padX)
          dx = - g->padX;
      if(dy > g->padY)
          dy = g->padY;
      else if(dy < - g->padY)
          dy = - g->padY;

      g->x0 = g->y0 = g->slideX = g->slideY = 0;

      PushZoom(g, // make a new zoom in the zoom stack
              pixToX(g->padX + dx, g->zoom),
              pixToX(g->width + g->padX + dx, g->zoom),
              pixToY(g->height + g->padY + dy, g->zoom),
              pixToY(g->padY + dy, g->zoom));
      DASSERT(!g->zoom_action, "We have multi button press actions??");
  }

    if(e->button == BOX_BUTTON && (g->zoom_action & BOX_ACTION)) {
        DSPEW("Zoom box finished");
        g->zoom_action &= ~BOX_ACTION;
        if(g->x0 == g->x || g->y0 == g->y)
            return TRUE;

        if(g->x0 > g->x) {
            double d = g->x;
            g->x = g->x0;
            g->x0 = d;
        }
        if(g->y0 > g->y) {
            double d = g->y;
            g->y = g->y0;
            g->y0 = d;
        }

        // Now g->x > g->x0 and g->y > g->y0
 
        PushZoom(g, // make a new zoom in the zoom stack
              pixToX(g->padX + g->x0, g->zoom),
              pixToX(g->padX + g->x, g->zoom),
              pixToY(g->padY + g->y, g->zoom),
              pixToY(g->padY + g->y0, g->zoom));
      }
 
  return TRUE;
}

gboolean graph_pointerMotion_cb(GtkWidget *drawingArea, GdkEventMotion *e,
        struct QsGraph *g) {

  DASSERT(g);
  DASSERT(drawingArea);
  DASSERT(g->drawingArea == drawingArea);
  DASSERT(e->type == GDK_MOTION_NOTIFY);

  //DSPEW("x,y=%g,%g", e->x, e->y);

    if(g->zoom_action & SLIDE_ACTION) {

        g->slideX = e->x - g->x0;
        g->slideY = e->y - g->y0;
  
        if(g->slideX > g->padX)
            g->slideX = g->padX;
        else if(g->slideX < - g->padX)
            g->slideX = - g->padX;
        if(g->slideY > g->padY)
            g->slideY = g->padY;
        else if(g->slideY < - g->padY)
            g->slideY = - g->padY;

        gtk_widget_queue_draw(drawingArea);
        return TRUE;
    }

    if(g->zoom_action & BOX_ACTION) {

        g->x = e->x;
        g->y = e->y;

        // Record the pointer plot x y values in the status bar:
        PrintStatusbar(g, g->x + g->padX, g->y + g->padY);

        // Draw the zoom box.  This is the only place we draw the zoom
        // box.

        gtk_widget_queue_draw(drawingArea);
        return TRUE;
    }

 
    // Record the pointer plot x y values in the status bar:
    PrintStatusbar(g, e->x + g->padX, e->y + g->padY);

  return TRUE;
}

gboolean graph_pointerEnter_cb(GtkWidget *drawingArea, GdkEvent *e,
        struct QsGraph *g) {

  DASSERT(g);
  DASSERT(drawingArea);
  DASSERT(g->drawingArea == drawingArea);

DSPEW();

  return TRUE;
}

gboolean graph_pointerLeave_cb(GtkWidget *drawingArea, GdkEvent *e,
        struct QsGraph *g) {

  DASSERT(g);
  DASSERT(drawingArea);
  DASSERT(g->drawingArea == drawingArea);

DSPEW();

  return TRUE;
}
