bin_PROGRAMS = camoto-studio

camoto_studio_SOURCES = main.cpp
camoto_studio_SOURCES += exceptions.cpp
camoto_studio_SOURCES += project.cpp
camoto_studio_SOURCES += gamelist.cpp
camoto_studio_SOURCES += newproj.cpp
camoto_studio_SOURCES += editor.cpp
camoto_studio_SOURCES += editor-map.cpp
camoto_studio_SOURCES += editor-tileset.cpp

EXTRA_camoto_studio_SOURCES = main.hpp
EXTRA_camoto_studio_SOURCES += exceptions.hpp
EXTRA_camoto_studio_SOURCES += project.hpp
EXTRA_camoto_studio_SOURCES += gamelist.hpp
EXTRA_camoto_studio_SOURCES += newproj.hpp
EXTRA_camoto_studio_SOURCES += editor.hpp
EXTRA_camoto_studio_SOURCES += editor-map.hpp
EXTRA_camoto_studio_SOURCES += editor-tileset.hpp

CAMOTO_CFLAGS = $(libgamecommon_CFLAGS)
CAMOTO_CFLAGS += $(libgamegraphics_CFLAGS)
CAMOTO_CFLAGS += $(libgamemaps_CFLAGS)
CAMOTO_CFLAGS += $(libgamemusic_CFLAGS)

CAMOTO_LIBS = $(libgamecommon_LIBS)
CAMOTO_LIBS += $(libgamegraphics_LIBS)
CAMOTO_LIBS += $(libgamemaps_LIBS)
CAMOTO_LIBS += $(libgamemusic_LIBS)

BOOST_LIBS = $(BOOST_SYSTEM_LIBS)
BOOST_LIBS += $(BOOST_FILESYSTEM_LIBS)
BOOST_LIBS += $(BOOST_PROGRAM_OPTIONS_LIBS)
BOOST_LIBS += $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(libxml2_CFLAGS) $(CAMOTO_CFLAGS) $(GL_CFLAGS) -DDATA_PATH=\"$(pkgdatadir)\"
AM_LDFLAGS = $(BOOST_LIBS) $(libxml2_LIBS) $(CAMOTO_LIBS) $(GL_LIBS)
