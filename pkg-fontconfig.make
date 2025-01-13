
ifdef WITH_CAIRO

# Get the fontconfig specific compiler options.

libdir := $(shell pkg-config --variable=libdir fontconfig)

# At run-time the environment variable LD_LIBRARY_PATH can override the
# DSO library file that it used at build-time by using the
# --enable-new-dtags linker option below:
#
FONTCONFIG_LDFLAGS := $(shell pkg-config --libs fontconfig)\
 -Wl,--enable-new-dtags,-rpath,$(libdir)
FONTCONFIG_CFLAGS := $(shell pkg-config --cflags fontconfig)

ifeq ($(libdir),)
# Use of fontconfig is required.
$(error software package fontconfig was not found)
endif

# Spew what fontconfig compiler options we have found
#$(info FONTCONFIG_CFLAGS="$(FONTCONFIG_CFLAGS)" FONTCONFIG_LDFLAGS="$(FONTCONFIG_LDFLAGS)")

undefine libdir

endif
