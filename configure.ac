#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([camoto-studio], [0.1], [camoto@lists.shikadi.net])
AM_INIT_AUTOMAKE([camoto-studio], [0.1])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_LANG(C++)

# Overarching version of all libraries (used for installation dir)
AC_SUBST(camoto_release, camoto-1.0)

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
AC_PROG_LIBTOOL

BOOST_REQUIRE([1.46])
BOOST_FILESYSTEM
BOOST_TEST
BOOST_THREADS

AX_CHECK_GL

PKG_CHECK_MODULES([libgamecommon], [libgamecommon])
PKG_CHECK_MODULES([libgamearchive], [libgamearchive])
PKG_CHECK_MODULES([libgamegraphics], [libgamegraphics])
PKG_CHECK_MODULES([libgamemaps], [libgamemaps])
PKG_CHECK_MODULES([libgamemusic], [libgamemusic])

PKG_CHECK_MODULES([libxml2], [libxml-2.0 >= 2.7.8])
PKG_CHECK_MODULES([sdl], [sdl])
PKG_CHECK_MODULES([libpng], [libpng])

SAVE_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $libpng_CFLAGS"

# Checks for header files.

AC_CACHE_CHECK([for png++],
	[ac_cv_pngpp],
	[AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([#include <png++/png.hpp>], [return 0;])],
		[ac_cv_pngpp=yes],
		[AC_MSG_ERROR([<png++/png.hpp> not found or unusable.

Please download and install png++.  See config.log for failure messages if required.])
		]
	)]
)

CPPFLAGS=$SAVE_CPPFLAGS

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AM_OPTIONS_WXCONFIG
reqwx=2.8.12
AM_PATH_WXCONFIG([$reqwx], [wxWin=1], [wxWin=0], [std,gl])
if test "$wxWin" != 1; then
	AC_MSG_ERROR([
wxWidgets must be installed on your system.

Please check that wx-config is in path, the directory
where wxWidgets libraries are installed (returned by
'wx-config --libs' or 'wx-config --static --libs' command)
is in LD_LIBRARY_PATH or equivalent variable and
wxWidgets version is $reqwx or above.
	])
fi

CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
LIBS="$LIBS $WX_LIBS"

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug],[enable extra debugging output]))

dnl Check for --enable-debug and add appropriate flags for gcc
if test "x$enable_debug" = "xyes";
then
	# Add gdb info (-g), disable any default optimisation
	AC_SUBST(DEBUG_CXXFLAGS, "-O0 -g")
	# Add DEBUG define
	AC_DEFINE([DEBUG], [1], [Define to include extra debugging output])
fi

if test "x$no_gl" = "xyesr";
then
	AC_MSG_ERROR([Could not find OpenGL!])
fi

AM_SILENT_RULES([yes])

AC_OUTPUT(Makefile src/Makefile doc/Makefile)
