root := ..

include $(root)/config.make
include $(root)/pkg-fontconfig.make
include $(root)/pkg-cairo.make

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

025_reinit_singleton_SOURCES := reinit_singleton.c
025_reinit_singleton_LDFLAGS := $(PN_LIB)

reinit_singleton_run_SOURCES := reinit_singleton.c
reinit_singleton_run_LDFLAGS := $(PN_LIB)
reinit_singleton_run_CPPFLAGS := -DRUN

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

widgets_cull_run_SOURCES := widgets.c
widgets_cull_run_LDFLAGS := $(PN_LIB)
widgets_cull_run_CPPFLAGS := -DRUN -DCULL

042_widgets_cull_SOURCES := widgets.c
042_widgets_cull_LDFLAGS := $(PN_LIB)
042_widgets_cull_CPPFLAGS := -DCULL

rand_widgets_run_SOURCES := rand_widgets.c
rand_widgets_run_LDFLAGS := $(PN_LIB)
rand_widgets_run_CPPFLAGS := -DRUN

050_rand_widgets_SOURCES := rand_widgets.c
050_rand_widgets_LDFLAGS := $(PN_LIB)

rand_widgets_cull_run_SOURCES := rand_widgets.c
rand_widgets_cull_run_LDFLAGS := $(PN_LIB)
rand_widgets_cull_run_CPPFLAGS := -DRUN -DCULL

052_rand_widgets_cull_SOURCES := rand_widgets.c
052_rand_widgets_cull_LDFLAGS := $(PN_LIB)
052_rand_widgets_cull_CPPFLAGS := -DCULL

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

cursor_run_SOURCES := cursor.c
cursor_run_LDFLAGS := $(PN_LIB)
cursor_run_CPPFLAGS := -DRUN

075_cursor_SOURCES := cursor.c
075_cursor_LDFLAGS := $(PN_LIB)

hsplitter_run_SOURCES := splitter.c
hsplitter_run_LDFLAGS := $(PN_LIB)
hsplitter_run_CPPFLAGS := -DRUN

077_hsplitter_SOURCES := splitter.c
077_hsplitter_LDFLAGS := $(PN_LIB)

vsplitter_run_SOURCES := splitter.c
vsplitter_run_LDFLAGS := $(PN_LIB)
vsplitter_run_CPPFLAGS := -DRUN -DVERTICAL

078_vsplitter_SOURCES := splitter.c
078_vsplitter_LDFLAGS := $(PN_LIB)
078_vsplitter_CPPFLAGS := -DVERTICAL

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

108_cairo_joy_SOURCES := cairo_joy.c
108_cairo_joy_LDFLAGS := $(CAIRO_LDFLAGS) $(FONTCONFIG_LDFLAGS)
108_cairo_joy_CPPFLAGS := $(CAIRO_CFLAGS) $(FONTCONFIG_CFLAGS)

# figuring out how to get the size of text without
# creating a panel widget.
110_CairoTextSizeFit_SOURCES := CairoTextSizeFit.c ../lib/debug.c
110_CairoTextSizeFit_LDFLAGS := $(CAIRO_LDFLAGS) $(FONTCONFIG_LDFLAGS)
110_CairoTextSizeFit_CPPFLAGS := $(CAIRO_CFLAGS) $(FONTCONFIG_CFLAGS)
endif


ifdef WITH_CAIRO

125_reinit_singleton_Cairo_SOURCES := reinit_singleton.c
125_reinit_singleton_Cairo_LDFLAGS := $(PN_LIB)
125_reinit_singleton_Cairo_CPPFLAGS := -DWITH_CAIRO

reinit_singleton_run_Cairo_SOURCES := reinit_singleton.c
reinit_singleton_run_Cairo_LDFLAGS := $(PN_LIB)
reinit_singleton_run_Cairo_CPPFLAGS := -DRUN -DWITH_CAIRO

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

111_fontconfig_SOURCES := fontconfig.c
111_fontconfig_LDFLAGS := $(PN_LIB) $(FONTCONFIG_LDFLAGS) $(CAIRO_LDFLAGS)
111_fontconfig_CPPFLAGS := $(FONTCONFIG_CFLAGS) $(CAIRO_CFLAGS)
fontconfig_run_SOURCES := fontconfig.c
fontconfig_run_LDFLAGS := $(PN_LIB) $(FONTCONFIG_LDFLAGS) $(CAIRO_LDFLAGS)
fontconfig_run_CPPFLAGS := $(FONTCONFIG_CFLAGS) $(CAIRO_CFLAGS) -DRUN

fontconfig.so_SOURCES := fontconfig.c
fontconfig.so_LDFLAGS := $(PN_LIB) $(FONTCONFIG_LDFLAGS) $(CAIRO_LDFLAGS)
fontconfig.so_CPPFLAGS := $(FONTCONFIG_CFLAGS) $(CAIRO_CFLAGS) -DDSO

fontconfig_run.so_SOURCES := fontconfig.c
fontconfig_run.so_LDFLAGS := $(PN_LIB) $(FONTCONFIG_LDFLAGS) $(CAIRO_LDFLAGS)
fontconfig_run.so_CPPFLAGS := $(FONTCONFIG_CFLAGS) $(CAIRO_CFLAGS) -DDSO -DRUN

112_load_fontconfig_SOURCES := load_fontconfig.c ../lib/debug.c
112_load_fontconfig_CPPFLAGS :=\
 -DDSO_FILE=\"./fontconfig.so\"\
 $(FONTCONFIG_CFLAGS)\
 $(CAIRO_CFLAGS)
112_load_fontconfig_LDFLAGS := $(FONTCONFIG_LDFLAGS) $(CAIRO_LDFLAGS)

load_fontconfig_run_SOURCES := load_fontconfig.c  ../lib/debug.c
load_fontconfig_run_CPPFLAGS :=\
 -DDSO_FILE=\"./fontconfig_run.so\"\
 $(FONTCONFIG_CFLAGS)\
 $(CAIRO_CFLAGS)
load_fontconfig_run_LDFLAGS := $(FONTCONFIG_LDFLAGS) $(CAIRO_LDFLAGS)

114_loadAndUnloadCheckMaps_SOURCES := loadAndUnloadCheckMaps.c ../lib/debug.c
114_loadAndUnloadCheckMaps_CPPFLAGS :=\
 -DDSO_FILE=\"./fontconfig.so\"

loadAndUnloadCheckMaps_run_SOURCES := loadAndUnloadCheckMaps.c  ../lib/debug.c
loadAndUnloadCheckMaps_run_CPPFLAGS :=\
 -DDSO_FILE=\"./fontconfig_run.so\"

graph_run_SOURCES := graph.c
graph_run_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS) -lm
graph_run_CPPFLAGS := -DRUN $(CAIRO_CFLAGS)

221_graph_SOURCES := graph.c
221_graph_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS) -lm
221_graph_CPPFLAGS := $(CAIRO_CFLAGS)

graph4_run_SOURCES := graph4.c
graph4_run_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS) -lm
graph4_run_CPPFLAGS := -DRUN $(CAIRO_CFLAGS)

222_graph4_SOURCES := graph4.c
222_graph4_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS) -lm
222_graph4_CPPFLAGS := $(CAIRO_CFLAGS)

graph2_hsplitter_run_SOURCES := graph2_hsplitter.c
graph2_hsplitter_run_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS) -lm
graph2_hsplitter_run_CPPFLAGS := -DRUN $(CAIRO_CFLAGS)

223_graph2_hsplitter_SOURCES := graph2_hsplitter.c
223_graph2_hsplitter_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS) -lm
223_graph2_hsplitter_CPPFLAGS := $(CAIRO_CFLAGS)


graphCairoDrawOver_run_SOURCES := graphCairoDrawOver.c
graphCairoDrawOver_run_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
graphCairoDrawOver_run_CPPFLAGS := -DRUN $(CAIRO_CFLAGS)

224_graphCairoDrawOver_SOURCES := graphCairoDrawOver.c
224_graphCairoDrawOver_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
224_graphCairoDrawOver_CPPFLAGS := $(CAIRO_CFLAGS)

graphDrawOver_run_SOURCES := graphDrawOver.c
graphDrawOver_run_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
graphDrawOver_run_CPPFLAGS := -DRUN $(CAIRO_CFLAGS)

234_graphDrawOver_SOURCES := graphDrawOver.c
234_graphDrawOver_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
234_graphDrawOver_CPPFLAGS := $(CAIRO_CFLAGS)

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

image_run_SOURCES := image.c
image_run_LDFLAGS := $(PN_LIB)
image_run_CPPFLAGS := -DRUN

133_image_SOURCES := image.c
133_image_LDFLAGS := $(PN_LIB)

155_generic_SOURCES := generic.c
155_generic_LDFLAGS := $(PN_LIB)

generic_run_SOURCES := generic.c
generic_run_LDFLAGS := $(PN_LIB)
generic_run_CPPFLAGS := -DRUN

145_menuItem_SOURCES := menuItem.c
145_menuItem_LDFLAGS := $(PN_LIB)

menuItem_run_SOURCES := menuItem.c
menuItem_run_LDFLAGS := $(PN_LIB)
menuItem_run_CPPFLAGS := -DRUN

endif # ifdef WITH_CAIRO


scribble_run_SOURCES := scribble.c
scribble_run_LDFLAGS := $(PN_LIB)
scribble_run_CPPFLAGS := -DRUN

102_scribble_SOURCES := scribble.c
102_scribble_LDFLAGS := $(PN_LIB)

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

135_gridSpan_cull_SOURCES := gridSpan.c
135_gridSpan_cull_LDFLAGS := $(PN_LIB)
135_gridSpan_cull_run_CPPFLAGS := -DCULL

gridSpan_cull_run_SOURCES := gridSpan.c
gridSpan_cull_run_LDFLAGS := $(PN_LIB)
gridSpan_cull_run_CPPFLAGS := -DRUN -DCULL

136_GridWithNoChildren_SOURCES := GridWithNoChildren.c
136_GridWithNoChildren_LDFLAGS := $(PN_LIB)

GridWithNoChildren_run_SOURCES := GridWithNoChildren.c
GridWithNoChildren_run_LDFLAGS := $(PN_LIB)
GridWithNoChildren_run_CPPFLAGS := -DRUN


CLEANFILES := $(wildcard tmp_*)

test: build
	./run_tests


include $(root)/quickbuild.make

