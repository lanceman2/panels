# panels

If your not a C developer, go away.  This project is just starting.

This software project is strictly for a Wayland compositor based windowing
desktop systems, like on a GNU/Linux computer system; Wayland being the
new replacement to the X11 window desktop system on UNIX like operating
systems.  Currently being developed on Debian GNU/Linux 12 with KDE Plasma
Wayland.

Panels provides a C API (application programming interface) library
and utility programs.  It can be used with C++.

The libpanels API lets developers have simple windows that they can draw
raw 32 bit [true color] (https://en.wikipedia.org/wiki/Color_depth) pixels
on; with or without a drawing API.  Sometimes you can draw much faster and
more simply without a drawing API; sometimes antialiasing (and other
effects) are just not needed and slow down the running code.

Mostly "panels" is minimalistic.  We wanted to:

1. divide windows into rectangular sub-sections called widgets,
2. draw and color pixels in the widgets,
3. receive keyboard events associated with these widgets,
4. the libpanels.so can be loaded and unloaded without leaking
   system resources, and
5. keep it small.

We also wanted to be able to create simple windows and draw to
"raw" pixels, without requiring a widget abstraction.


## Why not Qt or GTK?

Mostly because Qt and GTK are bloated and leak system resources.  I wish
to create an on-the-fly programming paradigm that requires that running
programs can load and unload compiled code.  The Qt and GTK libraries
can't be unloaded, and it's by design.  Fixing that is a social problem.
As best as I can tell I could change the codes to fix them, the social
hurdlers seem higher than the coding challenge.

As seen from above, Qt and GTK are not modular.  They are designed to take
over and be central to your development.  They are that way by design.
There may be modular aspects to Qt and GTK, but it is constrained to be
internal to their respective libraries.  Once you use a part of them
you'll likely be stuck with innate incompatibilities when trying to use
them with other code outside these frameworks.  As a very common example
if you wish to code with pthreads (NPTL) and GTK you'll need to understand
the internal workings of the GTK main loop code.  I wrote a hack to do it
(in the same process without changing GTK codes), but if GTK was not
designed to be the main loop controller of your code it would have been
trivial to do; taking hours instead of weeks to code; and it would have
been much much more efficient had they not required using their main loop
code.  gthreads may wrap all of pthreads (and I don't think it does) but
it's not near as standard and robust as pthreads, and it can't be, it's
built on pthreads.


## 3D Graphics and GPUs

I don't understand why the wayland compositors do not automatically use
what GPU resources it can find.  Why not automatically use GPU resources
for both 2D and 3D drawing?  At least make CPU rendering not be the
default if it's slower.


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
pixels.

There are other libraries that these depend on, but they appear to be
robust and stable.


## Reference Examples

 * [hello wayland](https://github.com/emersion/hello-wayland.git)
 * [wleird](https://github.com/emersion/wleird.git)
