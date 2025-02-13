include ../config.make


INSTALL_DIR = $(PREFIX)/include


ifdef WITH_FONTCONFIG
define_WITH_FONTCONFIG := \#define WITH_FONTCONFIG
else
define_WITH_FONTCONFIG := //\#define WITH_FONTCONFIG
endif
ifdef WITH_CAIRO
define_WITH_CAIRO := \#define WITH_CARIO
else
define_WITH_CAIRO := //\#define WITH_CARIO
endif

IN_VARS :=\
 define_WITH_FONTCONFIG\
 define_WITH_CAIRO


INSTALLED :=\
 panels.h\
 panel_drawingUtils.h



include ../quickbuild.make
