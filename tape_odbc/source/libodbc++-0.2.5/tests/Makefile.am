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

if BUILD_TESTS

noinst_PROGRAMS=\
streams \
dbmetadata \
scroll \
oracle \
mysql \
dmtest

endif


INCLUDES=@INCLUDES@ @QT_INCLUDES@
DEFS=@DEFS@ @QT_DEFS@ @UNICODE_DEFS@


if THREADS

if QT

testlibs=@THREAD_LIBS@ ../src/libodbc++_qt-mt.la

else

testlibs=@THREAD_LIBS@ ../src/libodbc++-mt.la

endif

else

if QT

testlibs=../src/libodbc++_qt.la

else

testlibs=../src/libodbc++.la

endif

endif


scroll_SOURCES=scroll.cpp
scroll_LDADD=$(testlibs)

dbmetadata_SOURCES=dbmetadata.cpp
dbmetadata_LDADD=$(testlibs)

mysql_SOURCES=mysql.cpp
mysql_LDADD=$(testlibs)

dmtest_SOURCES=dmtest.cpp
dmtest_LDADD=$(testlibs)

streams_SOURCES=streams.cpp
streams_LDADD=$(testlibs)

oracle_SOURCES=oracle.cpp
oracle_LDADD=$(testlibs)

EXTRA_DIST=README
