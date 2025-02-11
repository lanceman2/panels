root := ..

include $(root)/pkg-cairo.make
include $(root)/config.make


PN_LIB := -L../lib -lpanels -Wl,-rpath=\$$ORIGIN/../lib


010_debug_SOURCES := debug.c
010_debug_LDFLAGS := $(PN_LIB)

020_window_SOURCES := window.c
020_window_LDFLAGS := $(PN_LIB)

window_run_SOURCES := window.c
window_run_LDFLAGS := $(PN_LIB)
window_run_CPPFLAGS := -DRUN

023_windows_SOURCES := windows.c
023_windows_LDFLAGS := $(PN_LIB)

windows_run_SOURCES := windows.c
windows_run_LDFLAGS := $(PN_LIB)
windows_run_CPPFLAGS := -DRUN

027_popup_SOURCES := popup.c
027_popup_LDFLAGS := $(PN_LIB)

popup_run_SOURCES := popup.c
popup_run_LDFLAGS := $(PN_LIB)
popup_run_CPPFLAGS := -DRUN

033_widget_SOURCES := widget.c
033_widget_LDFLAGS := $(PN_LIB)

widget_run_SOURCES := widget.c
widget_run_LDFLAGS := $(PN_LIB)
widget_run_CPPFLAGS := -DRUN

widgets_run_SOURCES := widgets.c
widgets_run_LDFLAGS := $(PN_LIB)
widgets_run_CPPFLAGS := -DRUN

040_widgets_SOURCES := widgets.c
040_widgets_LDFLAGS := $(PN_LIB)

rand_widgets_run_SOURCES := rand_widgets.c
rand_widgets_run_LDFLAGS := $(PN_LIB)
rand_widgets_run_CPPFLAGS := -DRUN

050_rand_widgets_SOURCES := rand_widgets.c
050_rand_widgets_LDFLAGS := $(PN_LIB)

rand_SOURCES := rand.c ../lib/debug.c

060_align_widget_SOURCES := align_widget.c
060_align_widget_LDFLAGS := $(PN_LIB)

align_widget_run_SOURCES := align_widget.c
align_widget_run_LDFLAGS := $(PN_LIB)
align_widget_run_CPPFLAGS := -DRUN

065_draw_widget_SOURCES := draw_widget.c
065_draw_widget_LDFLAGS := $(PN_LIB)

draw_widget_run_SOURCES := draw_widget.c
draw_widget_run_LDFLAGS := $(PN_LIB)
draw_widget_run_CPPFLAGS := -DRUN

ifdef WITH_FONTCONFIG
081_findFont_SOURCES := 081_findFont.c
081_findFont_LDFLAGS := $(PN_LIB)
endif


ifdef CAIRO_LDFLAGS
091_cairoDrawWidget_SOURCES := cairoDrawWidget.c
091_cairoDrawWidget_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
091_cairoDrawWidget_CPPFLAGS := $(CAIRO_CFLAGS)

cairoDrawWidget_run_SOURCES := cairoDrawWidget.c
cairoDrawWidget_run_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
cairoDrawWidget_run_CPPFLAGS := -DRUN $(CAIRO_CFLAGS)
endif



CLEANFILES := $(wildcard tmp_*)

test: build
	./run_tests


include $(root)/quickbuild.make

