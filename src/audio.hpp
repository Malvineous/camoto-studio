/**
 * @file   audio.hpp
 * @brief  Shared audio functionality.
 *
 * Copyright (C) 2010-2013 Adam Nielsen <malvineous@shikadi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _AUDIO_HPP_
#define _AUDIO_HPP_

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <wx/window.h>
#include <portaudio.h>
#include "dbopl.hpp"

class Audio;

/// Mixer for processing DOSBox OPL data.
class SynthMixer: public MixerChannel {
	public:
		int16_t *buf;

		virtual ~SynthMixer();

		virtual void AddSamples_m32(Bitu samples, Bit32s *buffer);
		virtual void AddSamples_s32(Bitu frames, Bit32s *buffer);
};

/// Wrapper around DOSBox OPL device.
class OPLDevice
{
	public:
		OPLDevice();
		~OPLDevice();

		/// Write the specified value into the given register.
		/**
		 * @param reg
		 *   OPL register.  0x00 to 0xFF.
		 *
		 * @param val
		 *   Value to write.  0x00 to 0xFF.
		 *
		 * @post Register is immediately updated and will be heard if audio is
		 *   currently streaming.
		 */
		void write(int reg, int val);

	protected:
		DBOPL::Handler *chip;             ///< OPL emulator
		SynthMixer *mixer;                ///< Callback for mixing samples into buffer
		bool active;                      ///< Is this OPL chip playing sound?
		boost::mutex audiobuf_full_mutex; ///< Mutex to block when audio buffer is full
		boost::mutex audiobuf_pos_mutex;  ///< Mutex protecting audiobuf_pos
		int16_t *audiobuf_pos;            ///< Current position into shared audio buffer

		friend class Audio;
};

/// Shared audio handler.
class Audio
{
	public:
		typedef boost::shared_ptr<OPLDevice> OPLPtr;

		/// Set up shared audio handling.
		/**
		 * @param parent
		 *   Window to use as parent for popup error messages.
		 *
		 * @param sampleRate
		 *   Sample rate to use globally.
		 */
		Audio(wxWindow *parent, int sampleRate);

		/// Clean up at exit.
		~Audio();

		/// Create a new OPL chip to be mixed into the common output.
		/**
		 * @return An inactive OPL device is returned.  It must be unpaused by
		 *  calling pause() before playback will commence.  This allows allocating
		 *  the OPL device at class instatiation without triggering a grab for the
		 *  audio device.
		 */
		OPLPtr createOPL();

		/// Pause/resume playback.
		/**
		 * It is a good idea to pause playback when the OPL is not in use, as this
		 * reduces the number of streams being mixed and, in the case of the last
		 * stream, halts audio output and releases the audio device.
		 *
		 * @post A modal popup message may have appeared over the UI if the audio
		 *   device could not be opened.
		 */
		void pause(OPLPtr opl, bool pause);

		/// Delete the given OPL chip.
		/**
		 * @param opl
		 *   OPL chip.  Must have been returned by createOPL().
		 *
		 * @post The audio device may be closed if the last chip has been deleted.
		 */
		void releaseOPL(OPLPtr opl);

		/// PortAudio callback to put audio data into the output buffer.
		/**
		 * @param stream
		 *   Pointer to raw audio data in native playback format.
		 *
		 * @param currentTime
		 *   Playback time of this buffer, in seconds.
		 *
		 * @param len
		 *   Length of buffer in bytes.
		 */
		int fillAudioBuffer(void *outputBuffer, PaTime currentTime, unsigned long framesPerBuffer);

		/// Refill the extra buffer.
		/**
		 * This must be called regularly so the extra buffer is full when
		 * fillAudioBuffer() needs to pass it over to PortAudio.
		 */
		void refillBuffer();

		/// Wait until the audio buffer finishes playing.
		/**
		 * This is just a convenient way of waiting for a short period of time
		 * without consuming any CPU.
		 */
		void waitTick();

		/// Return the current playback time in microseconds.
		unsigned long getPlayTime();

		/// Introduce a delay until the next OPL data is processed.
		/**
		 * This will cause currently playing notes on the given OPL chip to continue
		 * playing for the given number of microseconds.  If the audio buffer is
		 * full, this function will block until there is enough room to generate
		 * sufficient samples to provide the delay.  If there is enough room in the
		 * audio buffer to generate the samples, then the function returns
		 * immediately.
		 *
		 * @param opl
		 *   OPL instance to delay.
		 *
		 * @param us
		 *   Number of microseconds to delay for.
		 */
		void oplDelay(OPLPtr opl, unsigned long us);

	protected:
		typedef std::vector<OPLPtr> OPLVector;

		wxWindow *parent;           ///< Parent window to use for error message popups

		PaStream *stream;
		unsigned int sampleRate;    ///< Sampling rate to use
		bool audioGood;             ///< Is the audio device open and streaming?

		OPLVector chips;            ///< All active OPL chips
		boost::mutex chips_mutex;   ///< Mutex protecting items in chips

		boost::mutex sound_buffer_mutex;  ///< Mutex protecting sound_buffer itself
		int16_t *sound_buffer;            ///< Mixing buffer
		int16_t *sound_buffer_end;        ///< First byte past end of buffer

		boost::mutex final_buffer_mutex;  ///< Mutex protecting final_buffer itself
		int16_t *final_buffer;            ///< Final buffer to pass to PortAudio
		int16_t *final_buffer_pos;        ///< PortAudio's position in final_buffer
		int16_t *final_buffer_end;        ///< First byte past end of buffer
		boost::mutex finalbuf_wait_mutex; ///< Mutex for finalbuf_wait_cond
		boost::condition_variable finalbuf_wait_cond;  ///< Wait for final buffer to be read completely

		PaTime lastTime;       ///< Time of last buffer switch

		/// Open the audio hardware and begin streaming sound.
		/**
		 * This is called internally the first time a stream is created.  It may
		 * result in a popup message if the audio device could not be opened.  The
		 * popup will be a child of the window given to the constructor (i.e. the
		 * main window.)
		 *
		 * Whether it succeeds or not, the rest of the class will function the same.
		 */
		void openDevice();

		/// Open/close the audio device according to the number of active streams.
		void adjustDevice();

		/// Stop streaming sound and close the audio device.
		/**
		 * This is called internally when the last stream is paused or removed.
		 */
		void closeDevice();

};

/// Shared pointer to the common audio interface.
typedef boost::shared_ptr<Audio> AudioPtr;

#endif // _AUDIO_HPP_
