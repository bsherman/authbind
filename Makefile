# Makefile for authbind
# 
# authbind is Copyright (C) 1998 Ian Jackson
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
#
# $Id$

prefix=/usr/local
share_dir=$(prefix)/share
etc_dir=/etc/authbind
lib_dir=$(prefix)/lib/authbind
man_dir=$(share_dir)/man
man1_dir=$(man_dir)/man1
man8_dir=$(man_dir)/man8

OPTIMISE=	-O2
CFLAGS=		-g $(OPTIMISE) \
		-Wall -Wwrite-strings -Wpointer-arith -Wimplicit \
		-Wnested-externs -Wmissing-prototypes -Wstrict-prototypes
CPPFLAGS=	-DMAJOR_VER='"$(MAJOR)"' -DMINOR_VER='"$(MINOR)"' \
		-DLIBAUTHBIND='"$(lib_dir)/$(LIBCANON)"' \
		-DHELPER='"$(prefix)/helper"' -DCONFIGDIR='"$(etc_dir)"'

MAJOR=1
MINOR=0
LIBCANON=	libauthbind.so.$(MAJOR)
LIBTARGET=	$(LIBCANON).$(MINOR)

TARGETS=		authbind helper $(LIBTARGET)
MANPAGES_1=		authbind
MANPAGES_8=		authbind-helper

all:			$(TARGETS)

install:		$(TARGETS)
		install -o root -g root -m 755 -d $(lib_dir) $(man_dir)
		install -o root -g root -m 755 authbind $(bin_dir)/.
		install -o root -g root -m 755 $(LIBTARGET) $(lib_dir)/.
		strip --strip-unneeded $(lib_dir)/$(LIBTARGET)
		ln -s $(LIBTARGET) $(lib_dir)/$(LIBCANON)
		install -o root -g root -m 4755 helper $(lib_dir)/.
		install -o root -g root -m 755 -d $(etc_dir) \
			$(etc_dir)/byport $(etc_dir)/byaddr $(etc_dir)/byuid

install_man:		$(MANPAGES_1) $(MANPAGES_8)
		install -o root -g root -m 644 $(MANPAGES_1) $(man1_dir)/.
		install -o root -g root -m 644 $(MANPAGES_8) $(man8_dir)/.

libauthbind.o:		libauthbind.c
		$(CC) -D_REENTRANT $(CFLAGS) $(CPPFLAGS) -c -o $@ -fPIC $<

$(LIBTARGET):		libauthbind.o
		gcc -g -shared -Wl,-soname,$(LIBCANON) -o $@ $< -ldl -lc

clean distclean:
		rm -f $(TARGETS) *.o *~ ./#*# *.bak *.new core libauthbind.so*
