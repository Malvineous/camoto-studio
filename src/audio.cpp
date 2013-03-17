/**
 * @file   audio.cpp
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

#include <algorithm>
#include <boost/thread/thread.hpp>
#include <wx/msgdlg.h>
#include "audio.hpp"
#include "exceptions.hpp"

#include <iostream>

#define min(a, b) (((a) < (b)) ? (a) : (b))

// Clipping function to prevent integer wraparound after amplification
#define SAMPLE_SIZE 2
#define SAMP_BITS (SAMPLE_SIZE << 3)
#define SAMP_MAX ((1 << (SAMP_BITS-1)) - 1)
#define SAMP_MIN -((1 << (SAMP_BITS-1)))
#define CLIP(v) (((v) > SAMP_MAX) ? SAMP_MAX : (((v) < SAMP_MIN) ? SAMP_MIN : (v)))

#define NUM_CHANNELS 2  ///< Stereo

/// Number of frames in our own audio buffer.  The PCM data is buffered and
/// later passed to PortAudio as the OPL needs a bit more time to render.
#define FRAMES_TO_BUFFER 2048

/// Maximum number of OPL frames to generate in one block (512 in dbopl.cpp)
#define MAX_OPL_FRAMES 512

SynthMixer::~SynthMixer()
{
}

void SynthMixer::AddSamples_m32(Bitu samples, Bit32s *buffer)
{
	while (samples) {
		int32_t a = 32768 + *this->buf;
		int32_t b = 32768 + (*buffer << 1);
		*this->buf++ = CLIP(-32768 + 2 * (a + b) - (a * b) / 32768 - 65536);
		buffer++;
		samples--;
	}
	return;
}

void SynthMixer::AddSamples_s32(Bitu frames, Bit32s *buffer)
{
	this->AddSamples_m32(frames * 2, buffer);
	return;
}


static int paCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo,
	PaStreamCallbackFlags statusFlags, void *userData)
{
	Audio *audio = (Audio *)userData;
	return audio->fillAudioBuffer(outputBuffer, timeInfo->outputBufferDacTime, framesPerBuffer);
}


OPLDevice::OPLDevice()
	:	chip(new DBOPL::Handler()),
		mixer(new SynthMixer()),
		active(false)
{
}

OPLDevice::~OPLDevice()
{
	delete this->chip;
	delete this->mixer;
}

void OPLDevice::write(int reg, int val)
{
	this->chip->WriteReg(reg, val);
	return;
}


Audio::Audio(wxWindow *parent, int sampleRate)
	:	parent(parent),
		sampleRate(sampleRate),
		audioGood(false),
		sound_buffer(NULL),
		lastTime(0)
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

Audio::OPLPtr Audio::createOPL()
{
	boost::mutex::scoped_lock chips_lock(this->chips_mutex);
	Audio::OPLPtr n(new OPLDevice());
	n->audiobuf_pos = this->sound_buffer;
	n->chip->Init(this->sampleRate);
	this->chips.push_back(n);
	return n;
}

void Audio::pause(OPLPtr opl, bool pause)
{
	opl->active = !pause;

	if (!pause) {
		// We're resuming, continue at the start of the buffer
		opl->audiobuf_pos = this->sound_buffer;
	}
	// Wake up the thread so it can flush its buffer
	opl->audiobuf_full_mutex.unlock();

	this->adjustDevice();
	return;
}

void Audio::releaseOPL(OPLPtr opl)
{
	{
		// Don't want this to disappear while being used by fillAudioBuffer
		boost::mutex::scoped_lock chips_lock(this->chips_mutex);

		OPLVector::iterator x = std::find(this->chips.begin(), this->chips.end(), opl);
		assert(x != this->chips.end());
		(*x)->active = false; // in case anyone else is watching, like the playback thread
		this->chips.erase(x);
	}

	this->adjustDevice();
	return;
}

int Audio::fillAudioBuffer(void *outputBuffer, PaTime currentTime, unsigned long framesPerBuffer)
{
	this->lastTime = currentTime;

	// Signal waitTick() to return
	this->finalbuf_wait_cond.notify_all();

	// Copy our audio buffer into PortAudio's
	//unsigned long numFrames = framesPerBuffer * 2 * NUM_CHANNELS;
	unsigned long numFrames = framesPerBuffer;// * NUM_CHANNELS;
	unsigned long samplesLeft = this->final_buffer_end - this->final_buffer_pos;
	unsigned long framesLeft = samplesLeft / NUM_CHANNELS;
	if (numFrames > framesLeft) {
		// We need to read more data than is left in the buffer, so we have to
		// loop around to the start.
		memcpy(outputBuffer, this->final_buffer_pos, framesLeft * 2 * NUM_CHANNELS);
		numFrames -= framesLeft;

		// Really this should be in another thread...
		this->refillBuffer();

		this->final_buffer_pos = this->final_buffer;
	}

	// If this fails, Pa is trying to read more data in one hit than we have in
	// the entire buffer!  We can't loop as there's no more data to read.  Fix
	// this if it ever comes up.
	assert(numFrames <= (unsigned)(this->final_buffer_end - this->final_buffer_pos));

	memcpy(outputBuffer, this->final_buffer_pos, numFrames * 2 * NUM_CHANNELS);
	this->final_buffer_pos += numFrames * NUM_CHANNELS;

	return paContinue;
}

void Audio::refillBuffer()
{
	// See if all the OPL chips have finished rendering to their buffers
	bool bufComplete = true;
	{
		boost::mutex::scoped_lock chips_lock(this->chips_mutex);
		for (OPLVector::iterator i = this->chips.begin(); i != this->chips.end(); i++) {
			OPLPtr& opl = *i; // This is thread-safe as we're protected by chips_mutex
			if (!opl->active) continue;
			boost::mutex::scoped_lock audiobuf_pos_lock(opl->audiobuf_pos_mutex);
			if (opl->audiobuf_pos < this->sound_buffer_end) {
				bufComplete = false;
				break;
			}
		}
	}

	if (!bufComplete) {
		// The next buffer isn't ready yet, play silence
		memset(this->final_buffer, 0, FRAMES_TO_BUFFER * 2 * NUM_CHANNELS);

	} else {
		// Buffer full, wait until it has been copied into the final buffer

		memcpy(this->final_buffer, this->sound_buffer, FRAMES_TO_BUFFER * 2 * NUM_CHANNELS);
		memset(this->sound_buffer, 0, FRAMES_TO_BUFFER * 2 * NUM_CHANNELS);

		// Set pointer back to start of buffer
		boost::mutex::scoped_lock chips_lock(this->chips_mutex);
		for (OPLVector::iterator i = this->chips.begin(); i != this->chips.end(); i++) {
			OPLPtr& opl = *i;
			boost::mutex::scoped_lock audiobuf_pos_lock(opl->audiobuf_pos_mutex);
			opl->audiobuf_pos = this->sound_buffer;

			// Unlock the 'buffer full' mutex now the buffer is empty again
			opl->audiobuf_full_mutex.unlock();
		}

	}
	return;
}

void Audio::waitTick()
{
	boost::unique_lock<boost::mutex> lock(this->finalbuf_wait_mutex);
	this->finalbuf_wait_cond.wait(lock);
	return;
}

unsigned long Audio::getPlayTime()
{
	return this->lastTime * 1000000;
}

void Audio::oplDelay(OPLPtr opl, unsigned long us)
{
	// A frame represents one sample in mono, and two samples in stereo
	unsigned long frames = (us / 1000) * this->sampleRate / 1000;

	while (frames > 0) {

		// Make sure there is still space left in the buffer, otherwise block until
		// more space becomes available.
		{
			boost::mutex::scoped_lock audiobuf_pos_lock(opl->audiobuf_pos_mutex);
			if (opl->audiobuf_pos >= this->sound_buffer_end) {
				opl->audiobuf_full_mutex.lock();
			}
		}
		// Block when the next audio buffer is full
		opl->audiobuf_full_mutex.lock();
		opl->audiobuf_full_mutex.unlock();
		if (!opl->active) break; // playback has been paused

		unsigned long maxFrames;
		{
			boost::mutex::scoped_lock audiobuf_pos_lock(opl->audiobuf_pos_mutex);
			if (opl->audiobuf_pos >= this->sound_buffer_end) {
				// Something else has filled up the buffer, or playback has been stopped
				break;
			}

			// This shouldn't happen unless it's possible for the buffer to be played
			// by the other thread while we're here.
			assert(opl->audiobuf_pos < this->sound_buffer_end);

			opl->mixer->buf = opl->audiobuf_pos;

			unsigned long maxSamples = this->sound_buffer_end - opl->audiobuf_pos;
			maxFrames = maxSamples / NUM_CHANNELS;
		}

		unsigned long actualFrames = min(frames, maxFrames);
		{
			boost::mutex::scoped_lock sound_buffer_lock(this->sound_buffer_mutex);
			if (!this->sound_buffer) return; // audio device closed

			unsigned long neededFrames = actualFrames;
			do {
				unsigned long generatedFrames = min(neededFrames, MAX_OPL_FRAMES);

				// This will write generatedFrames * NUM_CHANNELS to the buffer!
				opl->chip->Generate(opl->mixer, generatedFrames);

				assert(opl->mixer->buf <= this->sound_buffer_end);
				neededFrames -= generatedFrames;
			} while (neededFrames > 0);
		}

		opl->audiobuf_pos_mutex.lock();
		opl->audiobuf_pos += actualFrames * NUM_CHANNELS;
		opl->audiobuf_pos_mutex.unlock();

		frames -= actualFrames;
	}
	return;
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
	op.suggestedLatency = Pa_GetDeviceInfo(op.device)->defaultLowOutputLatency;
	op.hostApiSpecificStreamInfo = NULL;

	err = Pa_OpenStream(&this->stream, NULL, &op, this->sampleRate,
		paFramesPerBufferUnspecified, paClipOff, paCallback, this);
	if (err != paNoError) goto error;

	this->sound_buffer = new int16_t[FRAMES_TO_BUFFER * NUM_CHANNELS];
	this->sound_buffer_end = this->sound_buffer + FRAMES_TO_BUFFER * NUM_CHANNELS;
	memset(this->sound_buffer, 0, FRAMES_TO_BUFFER * 2 * NUM_CHANNELS);

	this->final_buffer = new int16_t[FRAMES_TO_BUFFER * NUM_CHANNELS];
	this->final_buffer_end = this->final_buffer + FRAMES_TO_BUFFER * NUM_CHANNELS;
	this->final_buffer_pos = this->final_buffer;
	memset(this->final_buffer, 0, FRAMES_TO_BUFFER * 2 * NUM_CHANNELS);

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
	bool start = false, stop = false;
	{
		boost::mutex::scoped_lock chips_lock(this->chips_mutex);
		int totalActive = 0;
		for (OPLVector::iterator i = this->chips.begin(); i != this->chips.end(); i++) {
			if ((*i)->active) totalActive++;
		}
		if ((totalActive == 0) && (this->audioGood)) {
			this->audioGood = false;
			stop = true;
		} else if ((totalActive > 0) && (!this->audioGood)) {
			start = true;
		}
	}

	if (start) {
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
			this->audioGood = true;
			this->lastTime = Pa_GetStreamTime(this->stream);
		}
	} else if (stop) {
		// This function will call the callback so we need to be outside the mutex
		Pa_StopStream(this->stream);
	}
	return;
}

void Audio::closeDevice()
{
	// Sanity check: Make sure all OPL devices have been released already
	assert(this->chips.size() == 0);

	this->audioGood = false;

	{
		boost::mutex::scoped_lock sound_buffer_lock(this->sound_buffer_mutex);
		if (this->sound_buffer) delete[] this->sound_buffer;
		this->sound_buffer = NULL;
		this->sound_buffer_end = NULL;
	}

	Pa_CloseStream(this->stream);
	return;
}
