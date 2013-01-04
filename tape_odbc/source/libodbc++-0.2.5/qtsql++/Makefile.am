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

if BUILD_QTSQLXX
bin_PROGRAMS = qtsql++
else
EXTRA_PROGRAMS = qtsql++
endif

if THREADS
thelib=../src/libodbc++_qt-mt.la
else
thelib=../src/libodbc++_qt.la
endif

INCLUDES += $(QT_INCLUDES)
DEFS=@DEFS@ @QT_DEFS@ @UNICODE_DEFS@

MOCS = \
mainwindow.moc.cpp \
connectwindow.moc.cpp \
resultwindow.moc.cpp

dist_qtsql___SOURCES = \
main.cpp \
mainwindow.cpp \
mainwindow.h \
connectwindow.cpp \
connectwindow.h \
resultwindow.cpp \
resultwindow.h

nodist_qtsql___SOURCES = \
$(MOCS)

qtsql___LDADD = $(thelib)

%.moc.cpp : %.h
	$(QT_MOC) -o $@ $<

CLEANFILES = $(MOCS)
