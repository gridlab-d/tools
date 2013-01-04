dnl  This file is part of libodbc++.
dnl  
dnl  Copyright (C) 1999-2000 Manush Dodunekov <manush@stendahls.net>
dnl   
dnl  This library is free software; you can redistribute it and/or
dnl  modify it under the terms of the GNU Library General Public
dnl  License as published by the Free Software Foundation; either
dnl  version 2 of the License, or (at your option) any later version.
dnl   
dnl  This library is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl  Library General Public License for more details.
dnl  
dnl  You should have received a copy of the GNU Library General Public License
dnl  along with this library; see the file COPYING.  If not, write to
dnl  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
dnl  Boston, MA 02111-1307, USA.



dnl Local macros for libodbc++


dnl Macro: AC_CHECK_CXX_STL
dnl Sets $ac_cv_cxx_stl to yes or no
dnl defines HAVE_CXX_STL if ok

AC_DEFUN(AC_CHECK_CXX_STL,
[
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_MSG_CHECKING([whether STL is available])
AC_CACHE_VAL(ac_cv_cxx_stl,
[
AC_TRY_COMPILE([
#include <set>
],[
std::set<int> t;
t.insert(t.begin(),1);
std::set<int>::iterator i=t.find(1);
],
ac_cv_cxx_stl=yes,
ac_cv_cxx_stl=no)
])
AC_MSG_RESULT($ac_cv_cxx_stl)
if test "x$ac_cv_cxx_stl" = "xyes"
then
	AC_DEFINE(HAVE_CXX_STL, 1,Defined if you have stl)
fi
AC_LANG_RESTORE
])



dnl Macro: AC_CHECK_CXX_EH
dnl Sets $ac_cv_cxx_eh to yes or no

AC_DEFUN(AC_CHECK_CXX_EH,
[
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_MSG_CHECKING([whether the C++ compiler ($CXX $CXXFLAGS) has correct exception handling])
AC_CACHE_VAL(ac_cv_cxx_eh,
[
AC_TRY_RUN(
[
#include <exception>
#include <string.h>

using namespace std;

struct test : public exception {
	virtual const char* what() const throw() { return "test"; }
};

static void func() { throw test(); }
int main(void)
{
	try {
		func();
	} catch(exception& e) {
		return (strcmp(e.what(),"test")!=0);
	} catch(...) { return 1; }
	return 1;
}
],
ac_cv_cxx_eh=yes,
ac_cv_cxx_eh=no)
])
AC_MSG_RESULT([$ac_cv_cxx_eh])
if test "x$ac_cv_cxx_eh" = "xyes"
then
	AC_DEFINE(HAVE_CXX_EH, 1,Defined if we have exception handling)
fi
AC_LANG_RESTORE
])


dnl Macro: AC_CHECK_CXX_NS
dnl Test if the c++ compiler supports namespaces
dnl Set $ac_cv_cxx_ns to either yes or no
dnl Define HAVE_CXX_NS if yes

AC_DEFUN(AC_CHECK_CXX_NS,
[
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_MSG_CHECKING([whether the C++ compiler ($CXX $CXXFLAGS) supports namespaces])
AC_CACHE_VAL(ac_cv_cxx_ns,
[
AC_TRY_COMPILE([
namespace A {
	namespace B {
		struct X {};
	};
};
],[
	A::B::X x;
],
ac_cv_cxx_ns=yes,
ac_cv_cxx_ns=no)
])

AC_MSG_RESULT([$ac_cv_cxx_ns])

if test "x$ac_cv_cxx_ns" = "xyes"
then
	AC_DEFINE(HAVE_CXX_NS, 1,Defined if we have namespaces)
fi
AC_LANG_RESTORE
])


dnl Macro: AC_CHECK_THREADS
dnl Test if we should compile with thread support
AC_DEFUN(AC_CHECK_THREADS,
[
AC_MSG_CHECKING([whether to enable threads])
AC_ARG_ENABLE(threads,
[  --enable-threads        Enable threads],
	enable_threads=yes
)

if test "x$enable_threads" = "xyes" 
then
	AC_MSG_RESULT(yes)

# ok, now check for pthreads
AC_CHECK_HEADER(pthread.h,[
	AC_DEFINE(HAVE_PTHREAD_H, 1,Defined if pthreads are available )
	],[AC_MSG_ERROR([pthread.h not found. Consider not using --enable-threads])])

# check if pthreads are in our default library environment
AC_CHECK_FUNCS(pthread_create,pthreads_ok=yes,pthreads_ok=no)
THREAD_LIBS=""

if test "x$pthreads_ok" != xyes
then

AC_CHECK_LIB(pthread,pthread_create,
	pthreads_ok=yes
	THREAD_LIBS="-lpthread",pthreads_ok=no)
fi

if test "x$pthreads_ok" != xyes
then
# hpux 11 uses macros for pthread_create so test another function
AC_CHECK_LIB(pthread,pthread_join,
	pthreads_ok=yes
	THREAD_LIBS="-lpthread",pthreads_ok=no)
fi

if test "x$pthreads_ok" != xyes
then

# try libc_r (*BSD)
	AC_CHECK_LIB(c_r,pthread_create,
	pthreads_ok=yes
	THREAD_LIBS="-lc_r",pthreads_ok=no)
fi

if test "x$pthreads_ok" = xyes
then
# now we know we can use pthreads
	AC_DEFINE(ENABLE_THREADS,1,Defined if threads enabled)
	CXXFLAGS="-D_REENTRANT $CXXFLAGS"
	CFLAGS="-D_REENTRANT $CFLAGS"
else
	AC_MSG_ERROR([Unable to find a POSIX threads environment.])
fi
else
	AC_MSG_RESULT(no)
fi
AC_SUBST(THREAD_LIBS)
])



dnl Macro: AC_CHECK_IODBC
dnl Checks for iodbc optionally in provided directories
dnl sets shell variable iodbc_ok to yes or no.
dnl Defines HAVE_LIBIODBC, HAVE_ISQL_H, HAVE_ISQLEXT_H if they are found
AC_DEFUN(AC_CHECK_IODBC,
[
AC_LANG_SAVE
AC_LANG_C
AC_ARG_WITH(iodbc,
[  --with-iodbc[=DIR]      Use iodbc, optionally installed in DIR],
[
if test "x$withval" != "xyes"
then
	iodbc_dir=$withval
else
	iodbc_dir="/usr/local"
fi
])

if test "x$iodbc_dir" = "x"
then
	iodbc_dir="/usr/local"
fi

AC_ARG_WITH(iodbc-includes,
[  --with-iodbc-includes=DIR Find iodbc headers in DIR],
[iodbc_includes_dir=$withval],
[iodbc_includes_dir="$iodbc_dir/include"]
)

AC_ARG_WITH(iodbc-libraries,
[  --with-iodbc-libraries=DIR Find iodbc libraries in DIR],
[iodbc_libraries_dir=$withval],
[iodbc_libraries_dir="$iodbc_dir/lib"]
)

save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"

CPPFLAGS="$CPPFLAGS -I$iodbc_includes_dir"
LIBS="$LIBS -L$iodbc_libraries_dir"

AC_CHECK_HEADERS([isql.h isqlext.h],
[iodbc_ok=yes; odbc_headers="$odbc_headers $ac_hdr"],
[iodbc_ok=no; break])

if test "x$iodbc_ok" = "xyes"
then
	AC_CHECK_LIB(iodbc,SQLConnect,[iodbc_ok=yes],[iodbc_ok=no])
fi

if test "x$iodbc_ok" = "xyes"
then
	LIBS="$LIBS -liodbc"
	AC_DEFINE(HAVE_LIBIODBC,1,Defined if using iodbc)
	AC_DEFINE(HAVE_ISQL_H,1,Defined if using iodbc)
	AC_DEFINE(HAVE_ISQLEXT_H,1,Defined if using iodbc)
else
	CPPFLAGS="$save_CPPFLAGS"
	LIBS="$save_LIBS"
fi
AC_LANG_RESTORE
])

dnl Macro: AC_CHECK_ODBC_TYPE
dnl Checks if $1 is a valid type in the ODBC environment,
dnl and #defines it to $2 if not

AC_DEFUN(AC_CHECK_ODBC_TYPE,
[

AC_MSG_CHECKING([for $1])
AC_CACHE_VAL(ac_cv_odbc_$1,
[
AC_LANG_SAVE
AC_LANG_C
echo > conftest.c

for i in $odbc_headers
do
	echo "#include <$i>" >> conftest.c
done

echo "int main(void) { $1 x; return 0; }" >> conftest.c

if $CC -c $CFLAGS $CPPFLAGS conftest.c > /dev/null 2> /dev/null
then
	eval ac_cv_odbc_$1=yes
else
	eval ac_cv_odbc_$2=no
fi
rm -f conftest*
AC_LANG_RESTORE

])

eval ac_odbc_check_res=$ac_cv_odbc_$1
if test "x$ac_odbc_check_res" = "xyes"
then
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT([no ($2)])
	AC_DEFINE($1,$2, No description)
fi
])
