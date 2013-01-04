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

if THREADS
thelibs=@THREAD_LIBS@ ../src/libodbc++-mt.la
else
thelibs=../src/libodbc++.la
endif

if BUILD_ISQLXX
bin_PROGRAMS = isql++
else
EXTRA_PROGRAMS = isql++
endif

INCLUDES=@INCLUDES@ -I$(top_srcdir)/src

isql___SOURCES=\
isql++.cpp \
isql++.h 

isql___LDADD=$(thelibs) @LIBREADLINE@

EXTRA_DIST=README
