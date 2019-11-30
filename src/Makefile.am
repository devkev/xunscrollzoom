# xunscrollzoom: ignore when ctrl is held down while scrolling the mouse wheel
# Copyright (C) 2019 Kevin Pulo
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# 
# Contact:
# Kevin Pulo
# kev@pulo.com.au

AUTOMAKE_OPTIONS = 1.4 foreign

CFLAGS += -Wall
#CFLAGS += -Werror


lib_LTLIBRARIES = libxunscrollzoom.la
libxunscrollzoom_la_SOURCES = libxunscrollzoom.c
libxunscrollzoom_la_CFLAGS = -O1
libxunscrollzoom_la_LIBADD = -ldl -lXi

lib_LTLIBRARIES += libxunscrollzoom-debug.la
libxunscrollzoom_debug_la_SOURCES = libxunscrollzoom.c
libxunscrollzoom_debug_la_CFLAGS = -O1 -DDEBUG
libxunscrollzoom_debug_la_LIBADD = -ldl -lXi


#dist_man_MANS = libxunscrollzoom.1 xunscrollzoom.1 xunscrollzoom-debug.1

nodist_bin_SCRIPTS  = xunscrollzoom
nodist_bin_SCRIPTS += xunscrollzoom-debug

dist_noinst_DATA  = xunscrollzoom.in
dist_noinst_DATA += xunscrollzoom-debug.in

edit = $(SED) \
	-e 's,@bindir\@,$(bindir),g' \
	-e 's,@mandir\@,$(mandir),g' \
	-e 's,@libdir\@,$(libdir),g' \
	-e 's,@datadir\@,$(datadir),g' \
	-e 's,@localstatedir\@,$(localstatedir),g' \
	-e 's,@version\@,$(PACKAGE_VERSION),g'

DISTCLEANFILES  = xunscrollzoom
DISTCLEANFILES += xunscrollzoom-debug

xunscrollzoom:: Makefile $(srcdir)/xunscrollzoom.in
	rm -f xunscrollzoom xunscrollzoom.tmp && \
	$(edit) $(srcdir)/xunscrollzoom.in >xunscrollzoom.tmp && \
	mv xunscrollzoom.tmp xunscrollzoom

xunscrollzoom-debug:: Makefile $(srcdir)/xunscrollzoom-debug.in
	rm -f xunscrollzoom-debug xunscrollzoom-debug.tmp && \
	$(edit) $(srcdir)/xunscrollzoom-debug.in >xunscrollzoom-debug.tmp && \
	mv xunscrollzoom-debug.tmp xunscrollzoom-debug


