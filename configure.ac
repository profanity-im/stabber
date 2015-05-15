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

PKG_CHECK_MODULES([glib], [glib-2.0 >= 2.26], [],
    [AC_MSG_ERROR([glib 2.26 or higher is required])])

PKG_CHECK_MODULES([expat], [expat >= 2.0.0], [],
    [AC_MSG_ERROR([expat 2.0.0 or higher is required])])

AM_CFLAGS="-Wall -Wno-deprecated-declarations"
AM_CFLAGS="$AM_CFLAGS -Wunused -Werror"
AM_CPPFLAGS="$AM_CPPFLAGS $glib_CFLAGS $expat_CFLAGS"
LIBS="$glib_LIBS $expat_LIBS $LIBS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)

### Checks for library functions.
AC_CHECK_FUNCS([atexit memset strdup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
