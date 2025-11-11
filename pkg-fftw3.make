
# Get the fftw3 specific compiler options if we can.

libdir := $(shell pkg-config --variable=libdir fftw3)

# At run-time the environment variable LD_LIBRARY_PATH can override the
# DSO library file that it used at build-time by using the
# --enable-new-dtags linker option below:
#
FFTW3_LDFLAGS := $(shell pkg-config --libs fftw3)\
 -Wl,--enable-new-dtags,-rpath,$(libdir)
FFTW3_CFLAGS := $(shell pkg-config --cflags fftw3)

ifeq ($(libdir),)
$(warning software package fftw3 was not found)
undefine FFTW3_LDFLAGS
endif

# Spew what fftw3 compiler options we have found
#$(info FFTW3_CFLAGS="$(FFTW3_CFLAGS)" FFTW3_LDFLAGS="$(FFTW3_LDFLAGS)")

undefine libdir
