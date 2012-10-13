# `nwm`: A programmable window manager for X

`nwm` is a "tiling" window manager for X.  It is written in C, but exposes a 
Scheme API that can be used to change it's behavior at run-time.  For example, 
the algorithm for automatically laying out windows is implemented in Scheme 
and can be changed at run-time.  There is a simple REPL (read, evaluate, print 
loop) program included which can be used to "talk" to the window manager in 
Scheme while it is running.  This project is definitely a work in progress and 
I doubt it would be usable for anybody in it's current state.

## Why?

I like to use tiling window managers, (especially on small screens) I enjoy 
programming in C and learning how X works.  I also like Lisp and thought it 
would be fun to try using Scheme as an extension language.  So yeah... 
basically just for fun! :)

## Building

If you want to try building this project, you will need to first install the 
following dependencies:

 * XCB libraries:
   * xcb
   * xcb-aux
   * xcb-event
   * xcb-keysyms
   * xcb-xinerama
 * libguile
 * libreadline

Once you have all the dependencies, run `make` in the project directory to 
build the `nwm` and `nwm-repl` programs.  I intend to improve the build 
system at some point, but that's currently not a high priority.

## Running

Before running the first time, you should set up the default configuration 
file in your home directory, like this:

 1. `mkdir ~/.nwm`
 2. `cp scheme/init.scm ~/.nwm`

Now set the `DISPLAY` environment variable (if needed) and then run the 
`nwm` program.

You can run `nwm-repl` to get a REPL you can use to execute Scheme code 
inside the running window manager.
