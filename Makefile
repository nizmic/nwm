# nwm - a programmable window manager
# Copyright (C) 2010-2013  Nathan Sullivan
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

DESTDIR =
prefix = /usr/local
bindir = $(prefix)/bin
datadir = $(prefix)/share

CC = gcc
PKG_CONFIG = pkg-config
GUILE_CONFIG = guile-config
INSTALL = install -D
INSTALL_BIN = $(INSTALL) -m755
INSTALL_DATA = $(INSTALL) -m644
MKDIR_P = mkdir -p

XCB_CFLAGS = $(shell $(PKG_CONFIG) --cflags xcb xcb-aux xcb-event \
		xcb-keysyms xcb-xinerama)
XCB_LIBS = $(shell $(PKG_CONFIG) --libs xcb xcb-aux xcb-event \
		xcb-keysyms xcb-xinerama)
GUILE_CFLAGS = $(shell $(GUILE_CONFIG) compile)
GUILE_LIBS = $(shell $(GUILE_CONFIG) link)

CFLAGS = -Wall -g $(XCB_CFLAGS) $(GUILE_CFLAGS)
LIBS = $(XCB_LIBS) $(GUILE_LIBS) -lreadline
LDFLAGS = -Wl,-O1,--sort-common,--as-needed,-z,relro,--hash-style=gnu \
	$(LIBS)

objects = nwm.o repl-server.o scheme.o event.o nwm-repl.o
bins = nwm nwm-repl

.PHONY: all build clean install uninstall

all: build

build: $(bins)

clean:
	rm -vf $(bins) $(objects) 

install: build
	$(MKDIR_P) $(bindir)
	$(MKDIR_P) $(datadir)/nwm/scheme
	$(INSTALL_BIN) nwm $(bindir)/nwm
	$(INSTALL_BIN) nwm-repl $(bindir)/nwm-repl
	$(INSTALL_DATA) scheme/init.scm $(datadir)/nwm/scheme/init.scm

uninstall:
	-rm -rvf $(datadir)/nwm
	-rm -vf $(bindir)/nwm
	-rm -vf $(bindir)/nwm-repl

nwm: nwm.o repl-server.o scheme.o event.o
	gcc $^ -o $@ $(LDFLAGS)

nwm-repl: nwm-repl.o
	gcc $^ -o $@ $(LDFLAGS)

