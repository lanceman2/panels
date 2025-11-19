root := ..

include $(root)/config.make
include $(root)/pkg-cairo.make


INSTALL_DIR = $(PREFIX)/bin
PN_LIB := -L../lib -lpanels -Wl,-rpath=\$$ORIGIN/../lib

BUILD_NO_INSTALL :=

# WITH_CAIRO means libpanels.so is linked with libcairo.so
ifdef WITH_CAIRO
ifdef CAIRO_LDFLAGS # We need to link to libcairo.so directly too.

MicToScope_SOURCES := MicToScope.c
MicToScope_LDFLAGS := $(PN_LIB) $(CAIRO_LDFLAGS)
MicToScope_CPPFLAGS := $(CAIRO_CFLAGS)

endif
endif


include $(root)/quickbuild.make

