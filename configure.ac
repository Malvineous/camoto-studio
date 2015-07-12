#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([Camoto Studio], [2.0],
	[https://github.com/Malvineous/camoto-studio/issues],
	[], [http://www.shikadi.net/camoto])
AM_INIT_AUTOMAKE([foreign dist-bzip2 no-dist-gzip])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_LANG(C++)

# Overarching version of all libraries (used for installation dir)
AC_SUBST(camoto_release, camoto-2.0)

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
AC_PROG_LIBTOOL

AX_CXX_COMPILE_STDCXX_11(noext, mandatory)

PKG_CHECK_MODULES([libgamecommon], [libgamecommon >= 2.0])
PKG_CHECK_MODULES([libgamearchive], [libgamearchive >= 2.0])
PKG_CHECK_MODULES([libgamegraphics], [libgamegraphics >= 2.0])
PKG_CHECK_MODULES([libgamemaps], [libgamemaps >= 2.0])
PKG_CHECK_MODULES([libgamemusic], [libgamemusic >= 2.0])

PKG_CHECK_MODULES([libxml2], [libxml-2.0 >= 2.7.6])
PKG_CHECK_MODULES([gtk], [gtk+-3.0 gtkmm-3.0 gdlmm-3.0])
PKG_CHECK_MODULES([portaudio], [portaudio-2.0])
PKG_CHECK_MODULES([libpng], [libpng])

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
