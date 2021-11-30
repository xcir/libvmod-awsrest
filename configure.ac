AC_PREREQ([2.68])
AC_INIT([libvmod-awsrest], [0.1], [], [vmod-awsrest])
AC_COPYRIGHT([Copyright (c) 2012-2017 Shohei Tanaka(@xcir)])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR(src/vmod_awsrest.vcc)
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([1.12 -Wall -Werror foreign parallel-tests])
AM_SILENT_RULES([yes])
AM_PROG_AR

LT_PREREQ([2.2.6])
LT_INIT([dlopen disable-static])

AX_PTHREAD(,[AC_MSG_ERROR([Could not configure pthreads support])])

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"


AC_CHECK_LIB(mhash, mhash_count, [AC_DEFINE([HAVE_MHASH],[1],[Define we have mhash])],
       [AC_MSG_ERROR([libvmod-awsrest requires libmhash.])])

AC_ARG_WITH([rst2man],
	[AS_HELP_STRING(
		[--with-rst2man=PATH],
		[Location of rst2man (auto)])],
	[RST2MAN="$withval"],
	[AC_CHECK_PROGS(RST2MAN, [rst2man rst2man.py], [])])


m4_ifndef([VARNISH_PREREQ], AC_MSG_ERROR([Need varnish.m4 -- see README.rst]))

VARNISH_PREREQ([5.0])
VARNISH_VMODS([awsrest])

VMOD_TESTS="$(cd $srcdir/src && echo tests/*.vtc)"
AC_SUBST(VMOD_TESTS)

PKG_CHECK_VAR([LIBVARNISHAPI_LIBDIR], [varnishapi], [libdir])
AC_SUBST([VARNISH_LIBRARY_PATH],
	[$LIBVARNISHAPI_LIBDIR:$LIBVARNISHAPI_LIBDIR/varnish])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT