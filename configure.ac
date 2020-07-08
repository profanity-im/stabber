#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([stabber], [0.1.0], [boothj5web@gmail.com])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects])
LT_INIT

### Checks for programs.
AC_PROG_CC

### Get canonical host
AC_CANONICAL_HOST
PLATFORM="unknown"
AS_CASE([$host_os],
    [freebsd*], [PLATFORM="freebsd" AC_DEFINE([PLATFORM_FREEBSD], [1], [FreeBSD])],
    [darwin*], [PLATFORM="osx" AC_DEFINE([PLATFORM_OSX], [1], [OSx])],
    [cygwin], [PLATFORM="cygwin" AC_DEFINE([PLATFORM_CYGWIN], [1], [Cygwin])],
    [PLATFORM="nix" AC_DEFINE([PLATFORM_NIX], [1], [Nix])])

PKG_CHECK_MODULES([glib], [glib-2.0 >= 2.26], [],
    [AC_MSG_ERROR([glib 2.26 or higher is required])])

PKG_CHECK_MODULES([expat], [expat >= 2.0.0], [],
    [AC_CHECK_HEADER([expat.h],
        [expat_LIBS="-lexpat"],
        [AC_MSG_ERROR([expat 2.0.0 or higher is required])])])

PKG_CHECK_MODULES([libmicrohttpd], [libmicrohttpd >= 0.9.71],
    [AC_CHECK_HEADER([microhttpd.h],
        [microhttpd_LIBS="-lmicrohttpd"],
        [AC_MSG_ERROR([libmicrohttpd 0.9.71 or higher is required])])], [])

AC_CHECK_LIB([pthread], [main], [],
    [AC_MSG_ERROR([pthread is required])])

AM_CFLAGS="-Wall -Wno-deprecated-declarations"
AM_CFLAGS="$AM_CFLAGS -Wunused -Werror"
AM_CPPFLAGS="$AM_CPPFLAGS $glib_CFLAGS $expat_CFLAGS $microhttpd_CFLAGS"
LIBS="$glib_LIBS $expat_LIBS $microhttpd_LIBS $LIBS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)

### Checks for library functions.
AC_CHECK_FUNCS([atexit memset strdup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
