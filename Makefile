# This file is for building and installing software with quickbuild and
# GNU make.

SUBDIRS :=\
 include\
 lib


# There are two scripts that need to run to make this GNU makefile
# based software build system to be usable from a git clone download:
#
#   1.  ./bootstrap -- downloads some file(s)
#   2.  ./configure -- generate some file(s)
#
#   "make clean" will not remove any of these files. 
#
# We add checks for the needed files in this make file making this
# make file somewhat dummy proof.
#
ifneq ($(wildcard quickbuild.make),quickbuild.make)
$(error "First run './bootstrap'")
endif


# Add a "clean"-like target to remove files downloaded by bootstrap:
gitclean: cleaner
	rm -f quickbuild.make lib/debug.c lib/debug.h
# quickbuild knows that this means add this to "make cleaner"
CLEANERFILES := config.make


#$(warning thingy=$(strip $(filter %clean clean%, $(MAKECMDGOALS)) ))

# If the make targets include a "clean" like thing we clean
# the tests directory too.
ifneq ($(strip $(filter %clean clean%, $(MAKECMDGOALS))),)
SUBDIRS +=\
 tests
else
ifneq ($(wildcard config.make),config.make)
$(error "Now run './configure'")
endif
endif


test:
	$(MAKE) && cd tests && $(MAKE) test


include quickbuild.make
