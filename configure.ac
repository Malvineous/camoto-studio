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
BOOST_PROGRAM_OPTIONS
BOOST_TEST

PKG_CHECK_MODULES([libgamecommon], [libgamecommon])
PKG_CHECK_MODULES([libgamegraphics], [libgamegraphics])
PKG_CHECK_MODULES([libgamemaps], [libgamemaps])
PKG_CHECK_MODULES([libgamemusic], [libgamemusic])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug],[enable extra debugging output]))

dnl Check for --enable-debug and add appropriate flags for gcc
if test "x$enable_debug" = "xyes";
then
	# Remove any optimisation flags
	CPPFLAGS=`echo "$CPPFLAGS" | sed -e 's/-O[0-9]*//g'`
	CXXFLAGS=`echo "$CXXFLAGS" | sed -e 's/-O[0-9]*//g'`
	CFLAGS=`echo "$CFLAGS" | sed -e 's/-O[0-9]*//g'`
	# Add gdb info (-g), disable any default optimisation
	DEBUG_CXXFLAGS="-O0 -g"
	#AM_CXXFLAGS="-O0 -g"
	# Add DEBUG define
	AC_DEFINE([DEBUG], [1], [Define to include extra debugging output])
fi

AM_SILENT_RULES([yes])

AC_OUTPUT(Makefile src/Makefile doc/Makefile)
