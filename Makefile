# nwm - a programmable window manager
# Copyright (C) 2010  Nathan Sullivan
#
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation; either version 2 
# of the License, or (at your option) any later version. 
#
# This program is distributed in the hope that it will be useful, 
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details. 
#
# You should have received a copy of the GNU General Public License 
# along with this program; if not, write to the Free Software 
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
# 02110-1301, USA 
#

CFLAGS = -Wall `pkg-config --cflags xcb xcb-aux xcb-event xcb-keysyms xcb-xinerama glib-2.0` `guile-config compile` -g
LIBS = `pkg-config --libs xcb xcb-aux xcb-event xcb-keysyms xcb-xinerama glib-2.0` `guile-config link` -lreadline

.PHONY: clean build

build: nwm nwm-repl

clean:
	rm -f nwm nwm-repl nwm.o nwm-repl.o repl-server.o scheme.o event.o

nwm: nwm.o repl-server.o scheme.o event.o
	gcc nwm.o repl-server.o scheme.o event.o -o $@ $(LIBS)

nwm.o repl-server.o scheme.o event.o: nwm.c repl-server.c nwm.h repl-server.h scheme.h scheme.c event.c
	gcc -c nwm.c repl-server.c scheme.c event.c $(CFLAGS)

nwm-repl: nwm-repl.o
	gcc $< -o $@ $(LIBS)

nwm-repl.o: nwm-repl.c
	gcc -c $< -o $@ $(CFLAGS)
