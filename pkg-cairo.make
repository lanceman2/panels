
# Get the cairo specific compiler options if we can.

libdir := $(shell pkg-config --variable=libdir cairo)

# At run-time the environment variable LD_LIBRARY_PATH can override the
# DSO library file that it used at build-time by using the
# --enable-new-dtags linker option below:
#
CAIRO_LDFLAGS := $(shell pkg-config --libs cairo)\
 -Wl,--enable-new-dtags,-rpath,$(libdir)
CAIRO_CFLAGS := $(shell pkg-config --cflags cairo)

ifeq ($(libdir),)
ifdef WITH_CAIRO
$(error software package cairo was not found)
else
$(warning software package cairo was not found)
endif
undefine CAIRO_LDFLAGS
endif

# Spew what cairo compiler options we have found
#$(info CAIRO_CFLAGS="$(CAIRO_CFLAGS)" CAIRO_LDFLAGS="$(CAIRO_LDFLAGS)")

undefine libdir

