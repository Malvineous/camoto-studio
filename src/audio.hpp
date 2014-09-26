/**
 * @file   audio.hpp
 * @brief  Shared audio functionality.
 *
 * Copyright (C) 2010-2014 Adam Nielsen <malvineous@shikadi.net>
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

#include <queue>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <wx/window.h>
#include <portaudio.h>
#include <camoto/gamemusic/playback.hpp>

wxDECLARE_EVENT(MUSICSTREAM_UPDATE, wxCommandEvent);

class Audio;

class AudioStream
{
	public:
		AudioStream();

		virtual void pause(bool paused);
		virtual bool isPaused();
		virtual void mix(void *outputBuffer, unsigned long lenBytes,
			const PaStreamCallbackTimeInfo *timeInfo) = 0;

	protected:
		bool paused;
};
typedef boost::shared_ptr<AudioStream> AudioStreamPtr;

class MusicStream: virtual public AudioStream
{
	public:
		MusicStream(Audio *audio, camoto::gamemusic::ConstMusicPtr music);

		virtual void mix(void *outputBuffer, unsigned long lenBytes,
			const PaStreamCallbackTimeInfo *timeInfo);

		void rewind();
		void notifyWindow(wxWindow *win);

		struct PositionTime {
			PaTime time;
			camoto::gamemusic::Playback::Position pos;
		};

		/// Stream time the main thread is waiting for
		PaTime waitUntil;

		/// List of song positions synthesized and placed in the audio buffer but
		/// not yet played through the speakers.
		std::queue<PositionTime> queuePos;
		boost::mutex mutex_queuePos;

	protected:
		Audio *audio;
		camoto::gamemusic::Playback playback;
		wxWindow *eventTarget;

		PositionTime current;
		camoto::gamemusic::Playback::Position lastPos;

		friend Audio;
};
typedef boost::shared_ptr<MusicStream> MusicStreamPtr;

class Sound: virtual public AudioStream
{
	public:
		Sound();

		virtual void mix(void *outputBuffer, unsigned long lenBytes,
			const PaStreamCallbackTimeInfo *timeInfo);
};
typedef boost::shared_ptr<Sound> SoundPtr;


/// Shared audio handler.
class Audio
{
	public:
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

		PaTime getTime();
		MusicStreamPtr addMusicStream(camoto::gamemusic::ConstMusicPtr music);
		SoundPtr playSound();
		void removeStream(AudioStreamPtr audioStream);

		/// PortAudio callback to put audio data into the output buffer.
		/**
		 * @param outputBuffer
		 *   Pointer to raw audio data in native playback format.
		 *
		 * @param samplesPerBuffer
		 *   Length of buffer in samples.
		 *
		 * @param timeInfo
		 *   Current playback time.
		 */
		int fillAudioBuffer(void *outputBuffer, unsigned long samplesPerBuffer,
			const PaStreamCallbackTimeInfo *timeInfo);

		unsigned int sampleRate;    ///< Sampling rate to use
		PaTime outputLatency;       ///< Cached output latency, in seconds

	protected:
		wxWindow *parent;           ///< Parent window to use for error message popups

		PaStream *stream;
		bool audioGood;             ///< Is the audio device open and streaming?
		bool playing;               ///< Is the stream currently playing audio

		std::vector<AudioStreamPtr> audioStreams;
		boost::mutex mutex_audioStreams;

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
