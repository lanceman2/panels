root := ..

include $(root)/config.make
include $(root)/pkg-cairo.make
include $(root)/pkg-fontconfig.make
include $(root)/pkg-waylandclient.make


ifdef WITH_CAIRO
CPPFLAGS += -DWITH_CAIRO
else
$(warning Building WITHOUT cairo)
endif
ifdef WITH_FONTCONFIG
CPPFLAGS += -DWITH_FONTCONFIG
else
$(warning Building WITHOUT fontconfig)
endif


INSTALL_DIR = $(PREFIX)/lib

SUBDIRS := pkgconfig



xdg-shell-protocol.c:
	$(WL_SCANNER) private-code $(WL_PROTOCOL) $@
xdg-shell-protocol.h:
	$(WL_SCANNER) client-header $(WL_PROTOCOL) $@

xdg_decoration_xml := unstable/xdg-decoration/xdg-decoration-unstable-v1.xml

xdg-decoration-protocol.c:
	$(WL_SCANNER) private-code $(WL_PROTOCOL_DIR)/$(xdg_decoration_xml) $@
xdg-decoration-protocol.h:
	$(WL_SCANNER) client-header $(WL_PROTOCOL_DIR)/$(xdg_decoration_xml) $@


# We need to build xdg-shell-client-protocol.h before we create depend
# files (we made edits to ../quickbuild.make for this case):
PRE_BUILD :=\
 xdg-shell-protocol.h\
 xdg-decoration-protocol.h

BUILD_NO_INSTALL :=\
 xdg-shell-protocol.c\
 xdg-decoration-protocol.c

libpanels.so_SOURCES :=\
 xdg-shell-protocol.c\
 xdg-decoration-protocol.c\
 debug.c\
 constructor.c\
 display.c\
 window.c\
 surface.c\
 shm.c\
 buffer.c\
 widget.c\
 toplevel.c\
 popup.c\
 allocation.c\
 drawQueue.c\
 eventFindXY.c\
 surface_draw.c\
 widget_set.c\
 action.c\
 monitor.c\
 cursor.c\
 splitter.c\
 generic.c\
 menu.c\
 window_set.c

ifdef WITH_FONTCONFIG
libpanels.so_SOURCES +=\
 findFont.c
endif

ifdef WITH_CAIRO
libpanels.so_SOURCES +=\
 cairo.c\
 button.c\
 label.c\
 image.c\
 graph.c\
 graphCallbacks.c\
 graph.h\
 staticPlot.c\
 plot.c\
 graphSet.c
endif


libpanels.so_CPPFLAGS :=\
 $(WL_CFLAGS)\
 $(WLCU_CFLAGS)\
  $(CAIRO_CFLAGS)\
 $(FONTCONFIG_CFLAGS)\
 -DPN_LIB_CODE



libpanels.so_CFLAGS :=\
 -fvisibility=hidden\
 -DPN_EXPORT="__attribute__((visibility(\"default\"))) extern"\
 -DEXPORT="PN_EXPORT"
#
# good read:
#   https://amir.rachum.com/shared-libraries/
#
# The order of these LDFLAGS can matter if we add more library
# dependencies.
#
# At some point we need an operating system package manager that
# fixes this kind of (DSO dependency hell) shit, like NixOS.
#
# Handy commands to run in a shell:
#
#   readelf -d libpanels.so
#
#   nm --extern-only libpanels.so
#   nm -D libpanels.so
#
libpanels.so_LDFLAGS :=\
 $(CAIRO_LDFLAGS)\
 $(WL_LDFLAGS)\
 $(WLCU_LDFLAGS)\
 $(FONTCONFIG_LDFLAGS)\
 -Wl,--retain-symbols-file=retain-symbols.txt
libpanels.so: retain-symbols.txt


include ../quickbuild.make

