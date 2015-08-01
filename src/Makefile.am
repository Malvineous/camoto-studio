bin_PROGRAMS = camoto-studio

camoto_studio_SOURCES = main.cpp
camoto_studio_SOURCES += audio.cpp
camoto_studio_SOURCES += exceptions.cpp
camoto_studio_SOURCES += gamelist.cpp
camoto_studio_SOURCES += project.cpp
camoto_studio_SOURCES += tab-graphics.cpp
camoto_studio_SOURCES += tab-newproject.cpp
camoto_studio_SOURCES += tab-openfile.cpp
camoto_studio_SOURCES += tab-project.cpp
camoto_studio_SOURCES += util-gfx.cpp

EXTRA_camoto_studio_SOURCES = main.hpp
EXTRA_camoto_studio_SOURCES += audio.hpp
EXTRA_camoto_studio_SOURCES += exceptions.hpp
EXTRA_camoto_studio_SOURCES += gamelist.hpp
EXTRA_camoto_studio_SOURCES += project.hpp
EXTRA_camoto_studio_SOURCES += tab-graphics.hpp
EXTRA_camoto_studio_SOURCES += tab-newproject.hpp
EXTRA_camoto_studio_SOURCES += tab-openfile.hpp
EXTRA_camoto_studio_SOURCES += tab-project.hpp
EXTRA_camoto_studio_SOURCES += util-gfx.hpp

WARNINGS = -Wall -Wextra -Wno-unused-parameter

AM_CPPFLAGS  = -I $(top_srcdir)/include
AM_CPPFLAGS += $(libgamecommon_CPPFLAGS)
AM_CPPFLAGS += $(libgamearchive_CPPFLAGS)
AM_CPPFLAGS += $(libgamegraphics_CPPFLAGS)
AM_CPPFLAGS += $(libgamemaps_CPPFLAGS)
AM_CPPFLAGS += $(libgamemusic_CPPFLAGS)
AM_CPPFLAGS += $(gtk_CPPFLAGS)
AM_CPPFLAGS += $(WARNINGS)

AM_CXXFLAGS  = $(DEBUG_CXXFLAGS)
AM_CXXFLAGS += $(GL_CFLAGS)
AM_CXXFLAGS += $(glew_CFLAGS)
AM_CXXFLAGS += $(libxml2_CFLAGS)
AM_CXXFLAGS += $(libgamecommon_CFLAGS)
AM_CXXFLAGS += $(libgamearchive_CFLAGS)
AM_CXXFLAGS += $(libgamegraphics_CFLAGS)
AM_CXXFLAGS += $(libgamemaps_CFLAGS)
AM_CXXFLAGS += $(libgamemusic_CFLAGS)
AM_CXXFLAGS += $(portaudio_CFLAGS)
AM_CXXFLAGS += $(gtk_CFLAGS)
AM_CXXFLAGS += -DDATA_PATH=\"$(pkgdatadir)\"

AM_LDFLAGS  = $(libxml2_LIBS)
AM_LDFLAGS += $(GL_LIBS)
AM_LDFLAGS += $(glew_LIBS)
AM_LDFLAGS += $(portaudio_LIBS)
AM_LDFLAGS += $(libpng_LIBS)
AM_LDFLAGS += $(libgamecommon_LIBS)
AM_LDFLAGS += $(libgamearchive_LIBS)
AM_LDFLAGS += $(libgamegraphics_LIBS)
AM_LDFLAGS += $(libgamemaps_LIBS)
AM_LDFLAGS += $(libgamemusic_LIBS)
AM_LDFLAGS += $(gtk_LIBS)
