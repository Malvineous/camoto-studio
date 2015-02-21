#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([Camoto Studio], [1.2],
	[https://github.com/Malvineous/camoto-studio/issues],
	[], [http://www.shikadi.net/camoto])
AM_INIT_AUTOMAKE([foreign dist-bzip2 no-dist-gzip])
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

AX_CHECK_GL

PKG_CHECK_MODULES([libgamecommon], [libgamecommon])
PKG_CHECK_MODULES([libgamearchive], [libgamearchive])
PKG_CHECK_MODULES([libgamegraphics], [libgamegraphics])
PKG_CHECK_MODULES([libgamemaps], [libgamemaps])
PKG_CHECK_MODULES([libgamemusic], [libgamemusic])

PKG_CHECK_MODULES([libxml2], [libxml-2.0 >= 2.7.6])
PKG_CHECK_MODULES([portaudio], [portaudio-2.0])
PKG_CHECK_MODULES([libpng], [libpng])
PKG_CHECK_MODULES([glew], [glew])

# Check for MIDI support
REQUIRE_RTMIDI

# Check for wxWidgets
AM_OPTIONS_WXCONFIG
reqwx=2.9.4
AM_PATH_WXCONFIG([$reqwx], [wxWin=1], [wxWin=0], [core,base,gl,aui])
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

# This has to go before the WX tests because they break CXXFLAGS
BOOST_REQUIRE([1.46])
BOOST_FILESYSTEM
BOOST_THREADS

# Save the global CFLAGS etc. while we run some more tests
SAVE_CPPFLAGS="$CPPFLAGS"
SAVE_CXXFLAGS="$CXXFLAGS"
SAVE_CFLAGS="$CFLAGS"
SAVE_LIBS="$LIBS"

# Set up flags for libpng
CPPFLAGS="$SAVE_CPPFLAGS $libpng_CFLAGS"

# Run png++ test
AC_CACHE_CHECK([for png++],
	[ac_cv_pngpp],
	[AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([#include <png++/png.hpp>], [return 0;])],
		[ac_cv_pngpp=yes],
		[AC_MSG_ERROR([<png++/png.hpp> not found or unusable.

Please download and install png++ <http://download.savannah.gnu.org/releases/pngpp/>.
See config.log for failure messages if required.])
		]
	)]
)

AC_CACHE_CHECK([if png++ is new enough],
	[ac_cv_pngpp_ver],
	[AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([#include <png++/png.hpp>], [std::string fn; png::image<png::index_pixel> png(fn); return 0;])],
		[ac_cv_pngpp_ver=yes],
		[AC_MSG_ERROR([png++ is too old.

Please download and install png++ 0.2.7 or newer <http://download.savannah.gnu.org/releases/pngpp/>])
		]
	)]
)

# Reset flags for wxWidgets
CPPFLAGS="$SAVE_CPPFLAGS $WX_CPPFLAGS"
CXXFLAGS="$SAVE_CXXFLAGS $WX_CXXFLAGS_ONLY"
CFLAGS="$SAVE_CFLAGS $WX_CFLAGS_ONLY"
LIBS="$SAVE_LIBS $WX_LIBS"

# Test for wxWidgets GL support
AC_CACHE_CHECK([for wxWidgets OpenGL support],
	[ac_cv_wxgl],
	[AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([#include <wx/glcanvas.h>], [wxGLContext *dummy; return 0;])],
		[ac_cv_wxgl=yes],
		[AC_MSG_ERROR([wxWidgets was found, but it was not compiled with OpenGL support.

You will need a version of wxWidgets with OpenGL enabled to continue.
See config.log for failure messages if required.])
		]
	)]
)

# Reset compiler flags back to what they were originally
CPPFLAGS="$SAVE_CPPFLAGS"
CXXFLAGS="$SAVE_CXXFLAGS"
CFLAGS="$SAVE_CFLAGS"
LIBS="$SAVE_LIBS"

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
