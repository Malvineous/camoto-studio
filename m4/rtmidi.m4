# Check for MIDI support
AC_DEFUN([REQUIRE_RTMIDI],
[

AC_CANONICAL_HOST
AC_MSG_CHECKING(for MIDI API)

AC_SUBST([RTMIDI_CPPFLAGS], [""])
AC_SUBST([RTMIDI_LIBS], [""])

case $host in
	*-*-linux*)
	# Use Jack only if --with-jack was specified
	AC_ARG_WITH(jack, AC_HELP_STRING([--with-jack], [choose JACK server support (Linux only)]), [
	AC_SUBST([RTMIDI_CPPFLAGS], [-D__LINUX_JACK__])
	AC_MSG_RESULT(using JACK)
	AC_CHECK_LIB(jack, jack_client_open, [RTMIDI_LIBS="-ljack"], AC_MSG_ERROR(JACK support requires the jack library!))], )

	# Otherwise try ALSA
	if [test "$RTMIDI_CPPFLAGS" == "";] then
		AC_SUBST([RTMIDI_CPPFLAGS], [-D__LINUX_ALSASEQ__])
		AC_MSG_RESULT(using ALSA)
		AC_CHECK_LIB(asound, snd_seq_open, [:], AC_MSG_ERROR(Linux ALSA MIDI support requires the asound library!))
		AC_CHECK_LIB(pthread, pthread_create, [:], AC_MSG_ERROR(Linux ALSA MIDI support requires the pthread library!))
		AC_SUBST([RTMIDI_LIBS], ["-lasound -lpthread"])
	fi
	;;

	*-sgi*)
	AC_SUBST(RTMIDI_CPPFLAGS, ["-D__IRIX_MD__ -LANG:std -w"])
	AC_MSG_RESULT(using IRIX MD)
	AC_CHECK_LIB(md, mdInit, [:], AC_MSG_ERROR(IRIX MIDI support requires the md library!) )
	AC_CHECK_LIB(pthread, pthread_create, [:], AC_MSG_ERROR(RtMidi requires the pthread library!))
	AC_SUBST([RTMIDI_LIBS], ["-lmd -lpthread"])
	;;

	*-apple*)
	# Check for CoreAudio framework
	AC_CHECK_HEADER(CoreMIDI/CoreMIDI.h, [AC_SUBST(RTMIDI_CPPFLAGS, [-D__MACOSX_CORE__] )], [AC_MSG_ERROR(CoreMIDI header files not found!)])
	AC_SUBST(RTMIDI_LIBS, ["-framework CoreMIDI -framework CoreFoundation -framework CoreAudio"])
	AC_MSG_RESULT(using CoreMIDI)
	;;

	*-mingw32*)
	# Use WinMM library
	AC_SUBST(RTMIDI_CPPFLAGS, [-D__WINDOWS_MM__])
	# I can't get the following check to work so just manually add the library
	# AC_CHECK_LIB(winmm, midiInGetNumDevs, [:], AC_MSG_ERROR(Windows MIDI support requires the winmm library!) )
	AC_SUBST(RTMIDI_LIBS, ["-lwinmm"])
	AC_MSG_RESULT(using WinMM)
	;;

	*)
	# Default case for unknown realtime systems.
	AC_MSG_ERROR(Unknown system type $host for MIDI support!)
	;;
esac
])
