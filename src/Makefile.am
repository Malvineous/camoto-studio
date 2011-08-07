bin_PROGRAMS = camoto-studio

camoto_studio_SOURCES = main.cpp
camoto_studio_SOURCES += audio.cpp
camoto_studio_SOURCES += dbopl.cpp
camoto_studio_SOURCES += exceptions.cpp
camoto_studio_SOURCES += MusicWriter_OPLSynth.cpp
camoto_studio_SOURCES += project.cpp
camoto_studio_SOURCES += gamelist.cpp
camoto_studio_SOURCES += mainwindow.cpp
camoto_studio_SOURCES += newproj.cpp
camoto_studio_SOURCES += editor.cpp
camoto_studio_SOURCES += editor-map.cpp
camoto_studio_SOURCES += editor-map-canvas.cpp
camoto_studio_SOURCES += editor-map-document.cpp
camoto_studio_SOURCES += editor-music.cpp
camoto_studio_SOURCES += editor-music-document.cpp
camoto_studio_SOURCES += editor-music-eventpanel.cpp
camoto_studio_SOURCES += editor-tileset.cpp

EXTRA_camoto_studio_SOURCES = main.hpp
EXTRA_camoto_studio_SOURCES += audio.hpp
EXTRA_camoto_studio_SOURCES += dbopl.hpp
EXTRA_camoto_studio_SOURCES += dosbox.hpp
EXTRA_camoto_studio_SOURCES += exceptions.hpp
EXTRA_camoto_studio_SOURCES += MusicWriter_OPLSynth.hpp
EXTRA_camoto_studio_SOURCES += project.hpp
EXTRA_camoto_studio_SOURCES += gamelist.hpp
EXTRA_camoto_studio_SOURCES += mainwindow.hpp
EXTRA_camoto_studio_SOURCES += newproj.hpp
EXTRA_camoto_studio_SOURCES += editor.hpp
EXTRA_camoto_studio_SOURCES += editor-map.hpp
EXTRA_camoto_studio_SOURCES += editor-map-canvas.hpp
EXTRA_camoto_studio_SOURCES += editor-map-document.hpp
EXTRA_camoto_studio_SOURCES += editor-music.hpp
EXTRA_camoto_studio_SOURCES += editor-music-document.hpp
EXTRA_camoto_studio_SOURCES += editor-music-eventpanel.hpp
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
BOOST_LIBS += $(BOOST_THREAD_LIBS)

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(libxml2_CFLAGS) $(CAMOTO_CFLAGS) $(GL_CFLAGS) $(sdl_CFLAGS) -DDATA_PATH=\"$(pkgdatadir)\"
AM_LDFLAGS = $(BOOST_LIBS) $(libxml2_LIBS) $(CAMOTO_LIBS) $(GL_LIBS) $(sdl_LIBS)
