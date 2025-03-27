include ../config.make


INSTALL_DIR = $(PREFIX)/include


ifdef WITH_FONTCONFIG
define_WITH_FONTCONFIG := \#define PN_WITH_FONTCONFIG
else
define_WITH_FONTCONFIG := //\#define PN_WITH_FONTCONFIG
endif
ifdef WITH_CAIRO
define_WITH_CAIRO := \#define PN_WITH_CAIRO
else
define_WITH_CAIRO := //\#define PN_WITH_CAIRO
endif

IN_VARS :=\
 define_WITH_FONTCONFIG\
 define_WITH_CAIRO


INSTALLED :=\
 panels.h\
 panel_drawingUtils.h



include ../quickbuild.make
