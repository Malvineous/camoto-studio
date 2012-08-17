/**
 * @file  playerthread.hpp
 * @brief Music player.
 *
 * Copyright (C) 2010-2012 Adam Nielsen <malvineous@shikadi.net>
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

#ifndef _PLAYERTHREAD_HPP_
#define _PLAYERTHREAD_HPP_

#include <camoto/gamemusic/eventconverter-opl.hpp>
#include <camoto/gamemusic/eventconverter-midi.hpp>
#include "main.hpp"
#include "audio.hpp"
#include "RtMidi.h"

class PlayerCallback
{
	public:
		/// Notify current playback time
		virtual void notifyPosition(unsigned long absTime) = 0;
};

class PlayerThread: virtual public camoto::gamemusic::OPLWriterCallback,
                    virtual public camoto::gamemusic::MIDIEventCallback
{
	public:
		/// Prepare the player thread.
		/**
		 * @param audio
		 *   PCM audio device.
		 *
		 * @param music
		 *   The song to play.
		 *
		 * @param cb
		 *   The notification callback.
		 */
		PlayerThread(AudioPtr audio, camoto::gamemusic::MusicPtr music,
			PlayerCallback *cb);

		virtual ~PlayerThread();

		/// Start the playback thread.
		void operator()();

		/// Resume playback after pause.
		void resume();

		/// Pause playback.  The audio device is left open.
		void pause();

		/// Tell the thread to terminate
		void quit();

		/// Return to the beginning of the song.
		void rewind();

		// OPLWriterCallback

		void writeNextPair(const camoto::gamemusic::OPLEvent *oplEvent);

		void writeTempoChange(camoto::gamemusic::tempo_t usPerTick);

		// MIDIEventCallback

		virtual void midiNoteOff(uint32_t delay, uint8_t channel, uint8_t note,
			uint8_t velocity);

		virtual void midiNoteOn(uint32_t delay, uint8_t channel, uint8_t note,
			uint8_t velocity);

		virtual void midiPatchChange(uint32_t delay, uint8_t channel,
			uint8_t instrument);

		virtual void midiController(uint32_t delay, uint8_t channel,
			uint8_t controller, uint8_t value);

		virtual void midiPitchbend(uint32_t delay, uint8_t channel, uint16_t bend);

		virtual void midiSetTempo(uint32_t delay,
			camoto::gamemusic::tempo_t usPerTick);

	protected:
		AudioPtr audio;               ///< Audio device and OPL
		camoto::gamemusic::MusicPtr music;  ///< Song to play
		PlayerCallback *cb;           ///< Class to notify about progress
		boost::mutex playpause_mutex; ///< Mutex to pause playback
		boost::unique_lock<boost::mutex> playpause_lock; ///< Main play/pause lock
		bool doquit;                  ///< Set to true to make thread terminate
		bool dorewind;                ///< Set to true to go back to start of song
		camoto::gamemusic::tempo_t usPerTickOPL;  ///< Tempo for OPL events
		camoto::gamemusic::tempo_t usPerTickMIDI; ///< Tempo for MIDI events

		Audio::OPLPtr opl;
		boost::shared_ptr<camoto::gamemusic::EventConverter_OPL> oplConv;
		std::vector<unsigned char> midiData;
		boost::shared_ptr<RtMidiOut> midiOut;
};

#endif // _PLAYERTHREAD_HPP_
