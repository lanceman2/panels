# panels

If your not a C developer, go away.  This project is just starting.

This software project is strictly for a Wayland compositor based windowing
desktop systems, like on a GNU/Linux computer system; Wayland being the
new replacement to the X11 window desktop system on UNIX like operating
systems.  Currently being developed on Debian GNU/Linux 12 with KDE Plasma
Wayland.

Panels provides a C API (application programming interface) library
and utility programs.  It can be used with C++.

Mostly "panels" is minimalistic.  We wanted to:

1. divide windows into rectangular sub-sections called widgets,
2. draw and color pixels in the widgets,
3. receive keyboard events associated with these widgets,
4. the libpanels.so can be loaded and unloaded without leaking
   system resources, and
5. keep it small.


## Dependencies

Panels only depends on libraries which we have found to be robust
(not leaky), and with very little bloat:

- wayland-client

- cairo which depends on pixman and fontconfig, it's nice to build it
  without X11 (but it works with the extra X11 cruft)

- fontconfig which depends on freetype

We have made the cairo and fontconfig dependencies optional.

The use of cairo and fontconfig provide very basic 2D drawing, but we also
keep the "panels" users ability to draw without using cairo (and
fontconfig).  Though cairo draws in a pretty optimal way it can never be
faster than drawing by just changing the pixel values by hand (as they
say), in a large class of cases.  So, panels provides access to "raw"
pixels.  Not doing a thing is always faster than doing a thing.

There are other libraries that these depend on, but they are very
robust and stable.  More below...


## Reference Examples

 * [hello wayland](https://github.com/emersion/hello-wayland.git)
 * [wleird](https://github.com/emersion/wleird.git)


## Rambling and Ranting

We have found that it is possible to get non-leaky code with these
libraries: cairo, fontconfig, and wayland-client.  You just have to find
the required "destroy" functions and call them when needed.

There are lots of very common secondary library dependences that tend to
not be buggy, not changing rapidly, and hence do not seem to need
attention, like:
    linux-vdso.so, libpthread.so, libm.so, libffi.so, libexpat.so,
    libc.so, libz.so, libpng16.so, libbrotlidec.so, libbrotlicommon.so,
    and ld-linux-x86-64.so.
I just think of them as compiler and linker magic.

I wonder how hard it is to write C code that can load and unload libc.
I made a try at it, and it turns out that it's not an ordinary shared
library.  If I live too long, I'll to do that at a later time.
I'm guessing that linux-vdso.so, libpthread.so, and ld-linux-x86-64.so are
special libraries too.

We have found that pixman leaks memory, but got a patch excepted to pixman
to fix it.  It should make it to everyone soon enough.  It appears that
all the other dependencies pass Valgrind memory and other resource
cleaning tests.

Things like GTK and Qt can't be used because they leak lots of system
resources, and I've talked with the developers and they are not interested
in changing the design of their code. The GTK and Qt code are leaky by
design.  They assume that users will run processes that never unlink
from their code.  The GTK and Qt libraries will always leak lots of system
resources, i.e. you can't unload them.

I've heard said that we should write code that for every object created
there should be a way to destroy that object, but not in Qt and GTK.  Qt
and GTK are so magical that they do not have to follow very basic
standards of quality.  It bugs the fuck out of me, because I know if they
just took a little more time in design and development these libraries
could be unloaded.
