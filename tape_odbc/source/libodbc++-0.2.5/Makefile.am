#  This file is part of libodbc++.
#  
#  Copyright (C) 1999-2000 Manush Dodunekov <manush@stendahls.net>
#   
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#   
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#  
#  You should have received a copy of the GNU Library General Public License
#  along with this library; see the file COPYING.  If not, write to
#  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#  Boston, MA 02111-1307, USA.

SUBDIRS = \
	include \
	src \
	tests \
	qtsql++ \
	isql++ \
	doc \
	win32

EXTRA_DIST = \
	BUGS \
	README.jdbc \
	README.threads \
	m4/ac_c_long_long.m4 \
	m4/ac_check_odbc.m4

ACLOCAL_AMFLAGS = -I m4

dist-hook:
	if test "x$(zip)" != "x"; then \
		rm -f $(distdir).zip; \
		$(zip) -q -r $(distdir).zip $(distdir); \
	fi
	if test "x$(bzip2)" != "x"; then \
		rm -f $(distdir).tar.bz2; \
		tar chof - $(distdir) | $(bzip2) > $(distdir).tar.bz2; \
	fi


pkgconfdir=$(libdir)/pkgconfig
pkgconf_DATA = libodbc++.pc
