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
camoto_studio_SOURCES += prefsdlg.cpp
camoto_studio_SOURCES += editor.cpp
camoto_studio_SOURCES += editor-image.cpp
camoto_studio_SOURCES += editor-map.cpp
camoto_studio_SOURCES += editor-map-canvas.cpp
camoto_studio_SOURCES += editor-map-document.cpp
camoto_studio_SOURCES += editor-music.cpp
camoto_studio_SOURCES += editor-music-document.cpp
camoto_studio_SOURCES += editor-music-eventpanel.cpp
camoto_studio_SOURCES += editor-tileset.cpp
camoto_studio_SOURCES += efailure.cpp

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
EXTRA_camoto_studio_SOURCES += prefsdlg.hpp
EXTRA_camoto_studio_SOURCES += editor.hpp
EXTRA_camoto_studio_SOURCES += editor-image.hpp
EXTRA_camoto_studio_SOURCES += editor-map.hpp
EXTRA_camoto_studio_SOURCES += editor-map-canvas.hpp
EXTRA_camoto_studio_SOURCES += editor-map-document.hpp
EXTRA_camoto_studio_SOURCES += editor-music.hpp
EXTRA_camoto_studio_SOURCES += editor-music-document.hpp
EXTRA_camoto_studio_SOURCES += editor-music-eventpanel.hpp
EXTRA_camoto_studio_SOURCES += editor-tileset.hpp
EXTRA_camoto_studio_SOURCES += efailure.hpp

pngpp_sources = png++/color.hpp
pngpp_sources += png++/config.hpp
pngpp_sources += png++/consumer.hpp
pngpp_sources += png++/convert_color_space.hpp
pngpp_sources += png++/end_info.hpp
pngpp_sources += png++/error.hpp
pngpp_sources += png++/ga_pixel.hpp
pngpp_sources += png++/generator.hpp
pngpp_sources += png++/gray_pixel.hpp
pngpp_sources += png++/image.hpp
pngpp_sources += png++/image_info.hpp
pngpp_sources += png++/index_pixel.hpp
pngpp_sources += png++/info.hpp
pngpp_sources += png++/info_base.hpp
pngpp_sources += png++/io_base.hpp
pngpp_sources += png++/packed_pixel.hpp
pngpp_sources += png++/palette.hpp
pngpp_sources += png++/pixel_buffer.hpp
pngpp_sources += png++/pixel_traits.hpp
pngpp_sources += png++/png.hpp
pngpp_sources += png++/reader.hpp
pngpp_sources += png++/require_color_space.hpp
pngpp_sources += png++/rgb_pixel.hpp
pngpp_sources += png++/rgba_pixel.hpp
pngpp_sources += png++/streaming_base.hpp
pngpp_sources += png++/tRNS.hpp
pngpp_sources += png++/types.hpp
pngpp_sources += png++/writer.hpp

# Include the png++ files in the tarball
EXTRA_camoto_studio_SOURCES += $(pngpp_sources)

CAMOTO_CFLAGS = $(libgamecommon_CFLAGS)
CAMOTO_CFLAGS += $(libgamearchive_CFLAGS)
CAMOTO_CFLAGS += $(libgamegraphics_CFLAGS)
CAMOTO_CFLAGS += $(libgamemaps_CFLAGS)
CAMOTO_CFLAGS += $(libgamemusic_CFLAGS)

CAMOTO_LIBS = $(libgamecommon_LIBS)
CAMOTO_LIBS += $(libgamearchive_LIBS)
CAMOTO_LIBS += $(libgamegraphics_LIBS)
CAMOTO_LIBS += $(libgamemaps_LIBS)
CAMOTO_LIBS += $(libgamemusic_LIBS)

BOOST_LIBS = $(BOOST_SYSTEM_LIBS)
BOOST_LIBS += $(BOOST_FILESYSTEM_LIBS)
BOOST_LIBS += $(BOOST_PROGRAM_OPTIONS_LIBS)
BOOST_LIBS += $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)
BOOST_LIBS += $(BOOST_THREAD_LIBS)

WARNINGS = -Wall -Wextra -Wno-unused-parameter

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(libxml2_CFLAGS) $(CAMOTO_CFLAGS) $(GL_CFLAGS) $(sdl_CFLAGS) -DDATA_PATH=\"$(pkgdatadir)\" $(WARNINGS)
AM_LDFLAGS = $(BOOST_LIBS)
AM_LDFLAGS += $(libxml2_LIBS)
AM_LDFLAGS += $(CAMOTO_LIBS)
AM_LDFLAGS += $(GL_LIBS)
AM_LDFLAGS += $(sdl_LIBS)
AM_LDFLAGS += $(libpng_LIBS)
