/**
 * @file   audio.cpp
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

#include <algorithm>
#include <boost/thread/thread.hpp>
#include <wx/msgdlg.h>
#include "audio.hpp"
#include "exceptions.hpp"

#define NUM_CHANNELS 2  ///< Stereo

/// Number of frames in our own audio buffer.  The PCM data is buffered and
/// later passed to PortAudio as the OPL needs a bit more time to render.
#define FRAMES_TO_BUFFER 2048

/// Maximum number of OPL frames to generate in one block (512 in dbopl.cpp)
#define MAX_OPL_FRAMES 512

wxDEFINE_EVENT(MUSICSTREAM_UPDATE, wxCommandEvent);

using namespace camoto::gamemusic;

AudioStream::AudioStream()
	:	paused(true)
{
}

void AudioStream::pause(bool paused)
{
	this->paused = paused;
	return;
}

bool AudioStream::isPaused()
{
	return this->paused;
}


MusicStream::MusicStream(Audio *audio, ConstMusicPtr music)
	:	waitUntil(0),
		audio(audio),
		playback(audio->sampleRate, NUM_CHANNELS, 16),
		eventTarget(NULL)
{
	this->lastPos.row = -1;
	playback.setSong(music);
	playback.setLoopCount(0); // loop forever
}

void MusicStream::mix(void *outputBuffer, unsigned long lenBytes,
	const PaStreamCallbackTimeInfo *timeInfo)
{
	this->playback.mix((int16_t *)outputBuffer, lenBytes, &this->current.pos);

	bool posChanged = this->current.pos != this->lastPos;
	bool sendUpdate;
	{
		// This mutex protects both queuePos and waitUntil
		boost::lock_guard<boost::mutex> lock(this->mutex_queuePos);
		if (posChanged) {
			// The playback position has changed
			this->current.time = timeInfo->outputBufferDacTime + audio->outputLatency;
			this->queuePos.push(this->current);
		}
		this->lastPos = this->current.pos;
		sendUpdate =
			((this->waitUntil == 0) && posChanged)
			|| (
				(this->waitUntil > 0)
				&& (this->waitUntil <= timeInfo->currentTime)
			);
	}

	if (sendUpdate) {
		// We've just passed the wait point
		// Notify anyone waiting that the position has updated
		if (this->eventTarget) {
			wxCommandEvent *notify = new wxCommandEvent(MUSICSTREAM_UPDATE, wxID_ANY);
			this->eventTarget->GetEventHandler()->QueueEvent(notify);
		}
	}
	return;
}

void MusicStream::rewind()
{
	this->playback.seekByOrder(0);
	return;
}

void MusicStream::notifyWindow(wxWindow *win)
{
	this->eventTarget = win;
	return;
}


Sound::Sound()
{
}

void Sound::mix(void *outputBuffer, unsigned long lenBytes,
	const PaStreamCallbackTimeInfo *timeInfo)
{
}

static int paCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo,
	PaStreamCallbackFlags statusFlags, void *userData)
{
	Audio *audio = (Audio *)userData;
	return audio->fillAudioBuffer(outputBuffer, framesPerBuffer, timeInfo);
}


Audio::Audio(wxWindow *parent, int sampleRate)
	:	sampleRate(sampleRate),
		parent(parent),
		audioGood(false),
		playing(false)
{
	PaError err = Pa_Initialize();
	if (err != paNoError) {
		wxString msg = wxString::Format(_("Unable to initialise PortAudio.\n\n"
			"[Pa_Initialize() failed: %s]"),
			wxString((const char *)Pa_GetErrorText(err), wxConvUTF8).c_str());
		throw EFailure(msg);
	}

	this->openDevice();
}

Audio::~Audio()
{
	this->closeDevice();
	Pa_Terminate();
}

PaTime Audio::getTime()
{
	if (!this->audioGood) return 0;
	return Pa_GetStreamTime(this->stream);
}

MusicStreamPtr Audio::addMusicStream(ConstMusicPtr music)
{
	MusicStreamPtr musicStream(new MusicStream(this, music));
	{
		boost::lock_guard<boost::mutex> lock(this->mutex_audioStreams);
		this->audioStreams.push_back(musicStream);
	}

	this->adjustDevice();
	return musicStream;
}

SoundPtr Audio::playSound()
{
	SoundPtr sound(new Sound());
	{
		boost::lock_guard<boost::mutex> lock(this->mutex_audioStreams);
		this->audioStreams.push_back(sound);
	}
	this->adjustDevice();
	return sound;
}

void Audio::removeStream(AudioStreamPtr audioStream)
{
	{
		boost::lock_guard<boost::mutex> lock(this->mutex_audioStreams);

		std::vector<AudioStreamPtr>::iterator x = std::find(
			this->audioStreams.begin(), this->audioStreams.end(), audioStream);
		assert(x != this->audioStreams.end());
		this->audioStreams.erase(x);
	}
	this->adjustDevice();
	return;
}

int Audio::fillAudioBuffer(void *outputBuffer, unsigned long samplesPerBuffer,
	const PaStreamCallbackTimeInfo *timeInfo)
{
	memset(outputBuffer, 0, samplesPerBuffer * sizeof(int16_t) * NUM_CHANNELS);
	{
		boost::lock_guard<boost::mutex> lock(this->mutex_audioStreams);
		for (std::vector<AudioStreamPtr>::iterator
			i = this->audioStreams.begin(); i != this->audioStreams.end(); i++
		) {
			AudioStreamPtr& aud = *i;
			if (!aud->isPaused()) {
				aud->mix(outputBuffer, samplesPerBuffer * NUM_CHANNELS, timeInfo);
			}
		}
	}
	return paContinue;
}

void Audio::openDevice()
{
	int err;

	PaStreamParameters op;
	op.device = Pa_GetDefaultOutputDevice();
	if (op.device == paNoDevice) {
		err = paDeviceUnavailable;
		goto error;
	}
	op.channelCount = NUM_CHANNELS;
	op.sampleFormat = paInt16; // paFloat32
	op.suggestedLatency = 1;
	op.hostApiSpecificStreamInfo = NULL;

	err = Pa_OpenStream(&this->stream, NULL, &op, this->sampleRate,
		paFramesPerBufferUnspecified, paClipOff, paCallback, this);
	if (err != paNoError) goto error;

	this->outputLatency = 0;

	this->audioGood = true;
	return;

error:
	this->audioGood = false;
	wxString msg = wxString::Format(_("Unable to initialise audio.\n\n[%s]"),
		wxString((const char *)Pa_GetErrorText(err), wxConvUTF8).c_str());
	Pa_Terminate();
	wxMessageDialog dlg(NULL, msg, _("Audio failure"),
		wxOK | wxICON_ERROR);
	dlg.ShowModal();
	return;

}

void Audio::adjustDevice()
{
	if (!this->audioGood) return;
	if (this->audioStreams.size()) {
		if (!this->playing) {
			PaError err = Pa_StartStream(this->stream);
			if (err != paNoError) {
				this->audioGood = false;
				wxString msg = wxString::Format(_("Unable to resume audio.\n\n[%s]"),
					wxString((const char *)Pa_GetErrorText(err), wxConvUTF8).c_str());
				Pa_Terminate();
				wxMessageDialog dlg(NULL, msg, _("Audio failure"),
					wxOK | wxICON_ERROR);
				dlg.ShowModal();
			} else {
				this->playing = true;
			}
			const PaStreamInfo *info = Pa_GetStreamInfo(this->stream);
			if (info) {
				this->outputLatency = info->outputLatency;
			}
		}
	} else {
		if (this->playing) {
			// This function will call the callback so we need to be outside the mutex
			Pa_StopStream(this->stream);
			this->playing = false;
		}
	}
	return;
}

void Audio::closeDevice()
{
	this->audioGood = false;
	Pa_CloseStream(this->stream);
	return;
}
