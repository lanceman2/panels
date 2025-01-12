root := ..


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



CLEANFILES := $(wildcard tmp_*)

test: build
	./run_tests


include $(root)/quickbuild.make

