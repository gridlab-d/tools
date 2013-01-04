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

sources= \
threads.cpp \
datetime.cpp \
driverinfo.cpp \
drivermanager.cpp \
connection.cpp \
databasemetadata.cpp \
statement.cpp \
preparedstatement.cpp \
callablestatement.cpp \
resultset.cpp \
resultsetmetadata.cpp \
errorhandler.cpp \
datahandler.cpp \
datastream.cpp \
datastream.h \
dtconv.h \
datahandler.h \
driverinfo.h

ldflags=-version-info @lib_version@

# confusing conditionals follow

DEFS=@DEFS@ @QT_DEFS@ @UNICODE_DEFS@
INCLUDES=@INCLUDES@ @QT_INCLUDES@

if THREADS

if QT

lib_LTLIBRARIES=libodbc++_qt-mt.la

else

lib_LTLIBRARIES=libodbc++-mt.la

endif

else

if QT

lib_LTLIBRARIES=libodbc++_qt.la

else

lib_LTLIBRARIES=libodbc++.la

endif

endif


libodbc___mt_la_SOURCES=$(sources)
libodbc___mt_la_LDFLAGS=$(ldflags)
libodbc___mt_la_LIBADD=@THREAD_LIBS@

libodbc___la_SOURCES=$(sources)
libodbc___la_LDFLAGS=$(ldflags)

libodbc___qt_la_SOURCES=$(sources)
libodbc___qt_la_LDFLAGS=$(ldflags)
libodbc___qt_la_LIBADD=@QT_LIBS@

libodbc___qt_mt_la_SOURCES=$(sources)
libodbc___qt_mt_la_LDFLAGS=$(ldflags)
libodbc___qt_mt_la_LIBADD=@THREAD_LIBS@ @QT_LIBS@
