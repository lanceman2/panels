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

067_draw_widget_SOURCES := draw_widget.c
067_draw_widget_LDFLAGS := $(PN_LIB)

070_orphans_SOURCES := orphans.c
070_orphans_LDFLAGS := $(PN_LIB)

orphans_run_SOURCES := orphans.c
orphans_run_LDFLAGS := $(PN_LIB)
orphans_run_CPPFLAGS := -DRUN

073_orphansNoWin_SOURCES := orphansNoWin.c
073_orphansNoWin_LDFLAGS := $(PN_LIB)

ifdef WITH_FONTCONFIG
081_findFont_SOURCES := 081_findFont.c
081_findFont_LDFLAGS := $(PN_LIB)
endif


ifdef CAIRO_LDFLAGS
# ../lib/libpanel.so is linked with libcairo.so
091_myCairoDrawWidget_SOURCES := myCairoDrawWidget.c
091_myCairoDrawWidget_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
091_myCairoDrawWidget_CPPFLAGS := $(CAIRO_CFLAGS)

myCairoDrawWidget_run_SOURCES := myCairoDrawWidget.c
myCairoDrawWidget_run_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
myCairoDrawWidget_run_CPPFLAGS := -DRUN $(CAIRO_CFLAGS)

112_cairo_joy_SOURCES := cairo_joy.c
112_cairo_joy_LDFLAGS := $(CAIRO_LDFLAGS)
112_cairo_joy_CPPFLAGS := $(CAIRO_CFLAGS)

# figuring out how to get the size of text without
# creating a panel widget.
110_CairoTextSizeFit_SOURCES := CairoTextSizeFit.c ../lib/debug.c
110_CairoTextSizeFit_LDFLAGS := $(CAIRO_LDFLAGS)
110_CairoTextSizeFit_CPPFLAGS := $(CAIRO_CFLAGS)
endif


ifdef WITH_CAIRO
# ../lib/libpanel.so is not required to be linked with libcairo.so
097_cairoDrawWidget_SOURCES := cairoDrawWidget.c
097_cairoDrawWidget_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
097_cairoDrawWidget_CPPFLAGS := $(CAIRO_CFLAGS)

cairoDrawWidget_run_SOURCES := cairoDrawWidget.c
cairoDrawWidget_run_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
cairoDrawWidget_run_CPPFLAGS := -DRUN $(CAIRO_CFLAGS)

105_button_SOURCES := button.c
105_button_LDFLAGS := $(PN_LIB)

button_run_SOURCES := button.c
button_run_LDFLAGS := $(PN_LIB)
button_run_CPPFLAGS := -DRUN

115_label_SOURCES := label.c
115_label_LDFLAGS := $(PN_LIB)

label_run_SOURCES := label.c
label_run_LDFLAGS := $(PN_LIB)
label_run_CPPFLAGS := -DRUN

labeledButton_run_SOURCES := labeledButton.c
labeledButton_run_LDFLAGS := $(PN_LIB)
labeledButton_run_CPPFLAGS := -DRUN

120_labeledButton_SOURCES := labeledButton.c
120_labeledButton_LDFLAGS := $(PN_LIB)
endif


scribble_run_SOURCES := scribble.c
scribble_run_LDFLAGS := $(PN_LIB)
scribble_run_CPPFLAGS := -DRUN

102_scribble_run_SOURCES := scribble.c
102_scribble_run_LDFLAGS := $(PN_LIB)


130_gridWindow_SOURCES := gridWindow.c
130_gridWindow_LDFLAGS := $(PN_LIB)

gridWindow_run_SOURCES := gridWindow.c
gridWindow_run_LDFLAGS := $(PN_LIB)
gridWindow_run_CPPFLAGS := -DRUN

134_gridSpan_SOURCES := gridSpan.c
134_gridSpan_LDFLAGS := $(PN_LIB)

gridSpan_run_SOURCES := gridSpan.c
gridSpan_run_LDFLAGS := $(PN_LIB)
gridSpan_run_CPPFLAGS := -DRUN

136_GridWithNoChildren_SOURCES := GridWithNoChildren.c
136_GridWithNoChildren_LDFLAGS := $(PN_LIB)

GridWithNoChildren_run_SOURCES := GridWithNoChildren.c
GridWithNoChildren_run_LDFLAGS := $(PN_LIB)
GridWithNoChildren_run_CPPFLAGS := -DRUN



CLEANFILES := $(wildcard tmp_*)

test: build
	./run_tests


include $(root)/quickbuild.make

