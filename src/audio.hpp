/**
 * @file   audio.hpp
 * @brief  Shared audio functionality.
 *
 * Copyright (C) 2010-2011 Adam Nielsen <malvineous@shikadi.net>
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
#include <SDL_mutex.h>
#include "dbopl.hpp"

class Audio;

void fillAudioBuffer(void *udata, Uint8 *stream, int len);

/// Mixer for processing DOSBox OPL data.
class SynthMixer: public MixerChannel {
	public:
		uint8_t *buf;

		virtual void AddSamples_m32(Bitu samples, Bit32s *buffer);
		virtual void AddSamples_s32(Bitu samples, Bit32s *buffer);
};

/// Thread-safe wrapper around DOSBox OPL device.
class OPLDevice
{
	public:
		OPLDevice()
			throw ();

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
		void write(int reg, int val)
			throw ();

		/// Block for the specified number of ticks.
		/**
		 * @param ms
		 *   Delay time, in milliseconds.
		 *
		 * @post The elapsed time has passed.
		 */
		void delay(int ms)
			throw ();

	protected:
		DBOPL::Handler *chip;
		bool active;
		int delayMS;                 ///< Number of milliseconds to delay for
		boost::mutex delay_mutex;    ///< Mutex for delay_lock
		boost::condition delay_cond; ///< To notify when the delay has reached zero

		friend class Audio;
};

class Audio
{
	public:
		typedef boost::shared_ptr<OPLDevice> OPLPtr;

		Audio(wxWindow *parent, int sampleRate)
			throw ();

		~Audio()
			throw ();

		/// Create a new OPL chip to be mixed into the common output.
		/**
		 * @return An inactive OPL device is returned.  It must be unpaused by
		 *  calling pause() before playback will commence.  This allows allocating
		 *  the OPL device at class instatiation without triggering a grab for the
		 *  audio device.
		 */
		OPLPtr createOPL()
			throw ();

		/// Pause/resume playback.
		/**
		 * It is a good idea to pause playback when the OPL is not in use, as this
		 * reduces the number of streams being mixed and, in the case of the last
		 * stream, halts audio output and releases the audio device.
		 *
		 * @post A modal popup message may have appeared over the UI if the audio
		 *   device could not be opened.
		 */
		void pause(OPLPtr opl, bool pause)
			throw ();

		/// Delete the given OPL chip.
		/**
		 * @param opl
		 *   OPL chip.  Must have been returned by createOPL().
		 *
		 * @post The audio device may be closed if the last chip has been deleted.
		 */
		void releaseOPL(OPLPtr opl)
			throw ();

		/// SDL callback to put audio data into SDL buffer.
		/**
		 * @param stream
		 *   Pointer to raw audio data in native playback format.
		 *
		 * @param len
		 *   Length of buffer in bytes.
		 */
		void fillAudioBuffer(uint8_t *stream, unsigned int len);

	protected:
		typedef std::vector<OPLPtr> OPLVector;

		wxWindow *parent;           ///< Parent window to use for error message popups
		int sampleRate;             ///< Sampling rate to use
		bool audioGood;             ///< Is the audio device open and streaming?
		OPLVector chips;            ///< All active OPL chips
		boost::mutex chips_mutex;   ///< Mutex protecting items in chips
		SynthMixer *mixer;          ///< Mixer for OPL chips
		int16_t sound_buffer[2048]; ///< Mixing buffer

		/// Open the audio hardware and begin streaming sound.
		/**
		 * This is called internally the first time a stream is created.  It may
		 * result in a popup message if the audio device could not be opened.  The
		 * popup will be a child of the window given to the constructor (i.e. the
		 * main window.)
		 *
		 * Whether it succeeds or not, the rest of the class will function the same.
		 */
		void openDevice()
			throw ();

		/// Open/close the audio device according to the number of active streams.
		void adjustDevice()
			throw ();

		/// Stop streaming sound and close the audio device.
		/**
		 * This is called internally when the last stream is paused or removed.
		 */
		void closeDevice()
			throw ();

};

/// Shared pointer to the common audio interface.
typedef boost::shared_ptr<Audio> AudioPtr;

#endif // _AUDIO_HPP_
