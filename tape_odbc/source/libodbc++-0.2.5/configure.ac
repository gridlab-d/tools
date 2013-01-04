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

AC_PREREQ(2.53)
AC_INIT(libodbcxx,0.2.5,libodbcxx-devel@lists.sourceforge.net,libodbc++)
AC_CONFIG_SRCDIR(src/connection.cpp)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

# version is current:revision:age
# versioning works like this:
# on implementation change:     revision++
# on interface change:          revision=0, current++
# on interface addition:        age++
# on interface removal:         age=0
 
lib_version=4:0:0
AC_SUBST(lib_version)

AM_ENABLE_SHARED(yes)
AM_ENABLE_STATIC(no)

# Prevents the Makefile rebuild rules runnning automatically. Use
# autogen.sh instead.
# AM_MAINTAINER_MODE

AC_CANONICAL_HOST

AC_PATH_PROG(sed,sed)

if test "x$sed" = "x"
then
	AC_MSG_ERROR([No sed found. Please install one and try again.
For example, GNU sed can be obtained at ftp://prep.ai.mit.edu/pub/gnu/sed/.])
fi

AC_PROG_CXX
AC_PROG_CXXCPP
AC_LANG_C
AM_PROG_LIBTOOL
AC_C_LONG_LONG
AC_LANG_CPLUSPLUS

AC_PROG_INSTALL

if test "x$GXX" = "xyes"
then
CXXFLAGS="-D_GNU_SOURCE $CXXFLAGS"
fi

docdir=$datadir/doc

AC_ARG_WITH(docdir,
[  --with-docdir=DIR       Where to install documentation [DATADIR/doc]],
[
	if test "x$withval" != xyes && test "x$withval" != xno
	then
		docdir="$withval"
	fi
])

AC_SUBST(docdir)


AC_CHECK_HEADERS(exception string,,
AC_MSG_ERROR([Header $ac_hdr not found.]))
AC_CHECK_HEADERS(set set.h)
AC_CHECK_HEADERS(vector vector.h)
AC_CHECK_HEADERS(iostream istream iostream.h)
AC_CHECK_HEADERS(sstream strstream strstream.h)
AC_CHECK_HEADERS(ctime cstring cstdlib cstdio)

AC_CHECK_THREADS

if test "x$enable_threads" = "xyes"
then

#perform some checks for reentrant functions

AC_CHECK_FUNCS(localtime_r)

#make sure pkgconfig has proper library
ODBCPP_LIBRARY="odbc++-mt"
else
ODBCPP_LIBRARY="odbc++"

fi
AC_SUBST(ODBCPP_LIBRARY)

AM_CONDITIONAL(THREADS, test x$enable_threads = xyes)

AC_CHECK_FUNCS(strtoll strtoq snprintf)

AC_CHECK_ODBC

if test "x$odbc_ok" != "xyes"
then
	AC_CHECK_IODBC
fi


if test "x$iodbc_ok" != "xyes" &&
   test "x$odbc_ok" != "xyes" 
then
	AC_MSG_ERROR([No ODBC driver manager configured. Pperhaps you need to give configure --with-odbc=/some/location?])
fi

AC_ARG_WITH(odbc-version,
[  --with-odbc-version=VER Force ODBCVER to VER],
[
if test "x$withval" != "xyes"
then
	AC_DEFINE_UNQUOTED(ODBCVER, $withval, ODBC version)
fi
])

AC_CHECK_HEADERS(inttypes.h)

AC_CHECK_SIZEOF(int,0)
AC_CHECK_SIZEOF(long,0)
AC_CHECK_SIZEOF(long long,0)

LIBREADLINE=""

AC_MSG_CHECKING(whether to build isql++)

AC_ARG_WITH(isqlxx,
[  --with-isqlxx           Build isql++ [default=no]],
with_isqlxx=yes,
with_isqlxx=no)

AC_MSG_RESULT($with_isqlxx)

if test "x$with_isqlxx" = xyes
then

# check for readline
AC_CHECK_HEADERS(readline/readline.h readline/history.h,
[readline_ok=yes],[readline_ok=no; break])

# check for libreadline and libtermcap
if test "x$readline_ok" = "xyes"
then
	AC_CHECK_LIB(readline,readline,readline_ok=yes,readline_ok=no,-ltermcap)
	if test "x$readline_ok" = "xyes"
	then
		LIBREADLINE="-lreadline -ltermcap"
	else
		readline_ok=no
	fi
fi

if test "x$readline_ok" != "xyes"
then
	with_isqlxx=no
fi

fi

AC_SUBST(LIBREADLINE)

# set up our include paths
INCLUDES="-I\$(top_srcdir)/include -I\$(top_builddir)/include $INCLUDES"
AC_SUBST(INCLUDES)

AC_PATH_PROG(doxygen,doxygen)
AC_PATH_PROG(perl,perl)

if test "x$perl" = x
then
	doxygen=""
fi

AC_PATH_PROG(db2html,db2html)
AC_PATH_PROG(zip,zip)
AC_PATH_PROG(bzip2,bzip2)


AC_SUBST(db2html)
AC_SUBST(zip)
AC_SUBST(bzip2)

# Unicode flag
AC_ARG_WITH(unicode,
[  --with-unicode          Use wide strings globally],
[
if test "x$withval" != "xno"
then
        enable_unicode=yes
fi
],enable_unicode=no)

# qt specific stuff
enable_qt=no
qt_prefix=/usr/local/qt

AC_ARG_WITH(qt,
[  --with-qt[=PREFIX]        Use QT installed in DIR],
[
if test "x$withval" != "xno"
then
enable_qt=yes
if test "x$withval" = "xyes"
then
	if test "x$QTDIR" != "x"
	then
		qt_prefix="$QTDIR"
	fi
else
	qt_prefix="$withval"
fi
fi
])

if test "x$enable_qt" = "xyes"
then

qt_includes="$qt_prefix/include"
AC_ARG_WITH(qt-includes,
[  --with-qt-includes=DIR  Use QT headers from DIR],
[
if test "x$withval" != "xyes"
then
	qt_includes="$withval"
fi
])

qt_libs="$qt_prefix/lib"
AC_ARG_WITH(qt-libs,
[  --with-qt-libs=DIR      Use QT libraries from DIR],
[
if test "x$withval" != "xyes"
then
	qt_libs="$withval"
fi
])

qt_moc="$qt_prefix/bin/moc"
AC_ARG_WITH(moc,
[  --with-moc=PATH         Use this moc (for QT support)],
[
if test "x$withval" != "xyes"
then
	qt_moc="$withval"
fi
])
fi

AC_ARG_WITH(qtsqlxx,
[  --with-qtsqlxx          Build the qtsql++ application],
[
if test "x$withval" != "xno"
then
	with_qtsqlxx=yes
fi
],with_qtsqlxx=no)


AC_MSG_CHECKING([whether Unicode support should be enabled])
if test "x$enable_unicode" = "xyes"
then
	AC_MSG_RESULT(yes)
	enable_qt=no
        with_qtsqlxx=no
        UNICODE_DEFS="-DODBCXX_UNICODE"
else
	AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING([whether QT support should be enabled])
if test "x$enable_qt" = "xyes"
then
	AC_MSG_RESULT(yes)
	QT_INCLUDES="-I$qt_includes"
	
	if test "x$enable_threads" = "xyes"
	then
		QT_LIBS="-lqt-mt"
		QT_DEFS="-DODBCXX_QT -DQT_THREAD_SUPPORT"
	else
		QT_LIBS="-lqt"
		QT_DEFS="-DODBCXX_QT"
	fi

	QT_MOC="$qt_moc"
	if test "$qt_libs" != "/usr/lib"
	then
		QT_LIBS="-L$qt_libs $QT_LIBS"
	fi

	AC_MSG_CHECKING([whehter qtsql++ should be built])
	AC_MSG_RESULT($with_qtsqlxx)
else
	with_qtsqlxx=no
	AC_MSG_RESULT(no)
fi

AC_SUBST(UNICODE_DEFS)

AM_CONDITIONAL(BUILD_QTSQLXX, test x$with_qtsqlxx = xyes)

AC_SUBST(QT_INCLUDES)
AC_SUBST(QT_LIBS)
AC_SUBST(QT_DEFS)
AC_SUBST(QT_MOC)

AM_CONDITIONAL(QT, test x$enable_qt = xyes)

# isql++ can't be built in qt mode
if test x$enable_qt = xyes
then
	with_isqlxx=no
fi

AM_CONDITIONAL(BUILD_ISQLXX, test x$with_isqlxx = xyes)

AC_ARG_WITH(tests,
[  --without-tests         Don't build the test programs],
[
if test "x$withval" = "xno"
then
	with_tests=no
else
	with_tests=yes
fi
],with_tests=yes)

AC_MSG_CHECKING([whether to build test programs])
AC_MSG_RESULT($with_tests)

AM_CONDITIONAL(BUILD_TESTS, test x$with_tests = xyes)

# finally, add our we-are-in-the-library flag
CXXFLAGS="-DIN_ODBCXX $CXXFLAGS"


AC_OUTPUT(\
Makefile \
include/Makefile \
include/odbc++/Makefile \
src/Makefile \
tests/Makefile \
isql++/Makefile \
qtsql++/Makefile \
doc/Makefile \
doc/progref/Makefile \
win32/Makefile \
win32/tests/Makefile \
libodbc++.spec \
libodbc++.pc \
)

if test "x$USE_MAINTAINER_MODE" = "xyes"
then

if test "x$doxygen" = x
then
	echo "WARNING: doxygen (and/or perl) not found, progref will NOT be regenerated"
fi

if test "x$db2html" = x
then
	echo "WARNING: db2html not found, doc/INSTALL/ will not be built"
fi

if test "x$zip" = x
then
	echo "WARNING: zip not found, 'make dist' will NOT generate a win32 distribution"
fi

if test "x$bzip2" = x
then
	echo "WARNING: bzip2 not found, 'make dist' will NOT generate a .tar.bz2 distribution"
fi

fi
