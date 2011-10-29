/**
 * @file   audio.cpp
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

#include <algorithm>
#include <boost/thread/thread.hpp>
#include <wx/msgdlg.h>
#include <SDL.h>
#include "audio.hpp"

#define min(a, b) (((a) < (b)) ? (a) : (b))

void SynthMixer::AddSamples_m32(Bitu samples, Bit32s *buffer)
{
	// Convert samples from mono s32 to s16
	int16_t *out = (int16_t *)this->buf;
	for (unsigned int i = 0; i < samples; i++) {
		int32_t a = 32768 + *out;
		int32_t b = 32768 + buffer[i];
		*out++ = -32768 + 2 * (a + b) - (a * b) / 32768 - 65536;
	}
	return;
}

void SynthMixer::AddSamples_s32(Bitu samples, Bit32s *buffer)
{
	// Convert samples from stereo s32 to s16
	std::cerr << "TODO: Stereo samples are not yet implemented!\n";
	return;
}


void fillAudioBuffer(void *udata, Uint8 *stream, int len)
{
	Audio *audio = (Audio *)udata;
	audio->fillAudioBuffer(stream, len);
	return;
}


OPLDevice::OPLDevice()
	throw () :
		chip(new DBOPL::Handler()),
		active(false),
		delayMS(0)
{
}

void OPLDevice::write(int reg, int val)
	throw ()
{
	boost::mutex::scoped_lock(this->delay_mutex);
	this->chip->WriteReg(reg, val);
	return;
}

void OPLDevice::delay(int ms)
	throw ()
{
	try {
		boost::mutex::scoped_lock opl_lock(this->delay_mutex);
		this->delayMS += ms;
		while ((this->delayMS > 0) && (this->active)) {
			// Notify the audio routine that there is now a delay ready to be processed
			this->delay_cond.notify_one();

			// Now wait until the delay has reached zero again
			this->delay_cond.wait(opl_lock);
		}
	} catch (boost::thread_interrupted) {
		// Just finish the delay if we were interrupted.  This can happen when the
		// song is paused (i.e. we're blocking) and the user quits.
	}
	return;
}


Audio::Audio(wxWindow *parent, int sampleRate)
	throw () :
		parent(parent),
		sampleRate(sampleRate)
{
	this->mixer = new SynthMixer();
}

Audio::~Audio()
	throw ()
{
	delete this->mixer;
}

Audio::OPLPtr Audio::createOPL()
	throw ()
{
	boost::mutex::scoped_lock chips_lock(this->chips_mutex);
	Audio::OPLPtr n(new OPLDevice());
	n->chip->Init(this->sampleRate);
	this->chips.push_back(n);
	return n;
}

void Audio::pause(OPLPtr opl, bool pause)
	throw ()
{
	opl->active = !pause;
	this->adjustDevice();
	return;
}

void Audio::releaseOPL(OPLPtr opl)
	throw ()
{
	OPLVector::iterator x = std::find(this->chips.begin(), this->chips.end(), opl);
	assert(x != this->chips.end());
	(*x)->active = false; // in case anyone else is watching, like the playback thread

	// Don't want this to disappear while being used by fillAudioBuffer
	{
		// Wake up the thread if it's waiting for more delays.  It will realise in
		// the next loop the chip is now inactive and won't keep waiting.
		(*x)->delay_cond.notify_one();

		boost::mutex::scoped_lock chips_lock(this->chips_mutex);
		this->chips.erase(x);
	}

	this->adjustDevice();
	return;
}

void Audio::fillAudioBuffer(uint8_t *stream, unsigned int len)
{
	boost::mutex::scoped_lock chips_lock(this->chips_mutex);
	unsigned int bufvalid_bytes = min(len, sizeof(this->sound_buffer));
	unsigned int bufvalid_samples = bufvalid_bytes / sizeof(int16_t);

	memset(this->sound_buffer, 0, bufvalid_bytes);

	for (OPLVector::iterator i = this->chips.begin(); i != this->chips.end(); i++) {
		OPLPtr& opl = *i; // This is thread-safe as we're protected by chips_mutex
		if (!opl->active) continue;

		boost::mutex::scoped_lock opl_lock(opl->delay_mutex);

		this->mixer->buf = (uint8_t *)this->sound_buffer;
		int remaining_samples = bufvalid_samples;
		do {
			while ((opl->delayMS <= 0) && (opl->active)) {
				// No more delay, get more song notes
				opl->delay_cond.notify_one();

				// Wait until we're notified that there's another delay
				opl->delay_cond.wait(opl_lock);
			}
			if (opl->delayMS == 0) break; // end of playback

			int chunk_samples = min(remaining_samples, 48 * opl->delayMS);
			int generated_samples = min(48*10, chunk_samples);
			opl->chip->Generate(this->mixer, generated_samples);
			remaining_samples -= generated_samples;
			this->mixer->buf += generated_samples * sizeof(int16_t);
			opl->delayMS -= generated_samples / 48;
			//assert(opl->delayMS > 0);
		} while (remaining_samples > 0);
	}

	SDL_MixAudio(stream, (Uint8 *)this->sound_buffer, bufvalid_bytes, SDL_MIX_MAXVOLUME);

	return;
}

void Audio::openDevice()
	throw ()
{
	SDL_AudioSpec wanted;
	wanted.freq = this->sampleRate;
	wanted.format = AUDIO_S16;
	wanted.channels = 1;
	wanted.samples = 2048;
	wanted.callback = ::fillAudioBuffer;
	wanted.userdata = this;
	if (SDL_OpenAudio(&wanted, NULL) < 0) {
		this->audioGood = false;
		wxMessageDialog dlg(this->parent, _T("Unable to open audio device!  "
			"Another program may be using it."),
			_T("Audio error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	this->audioGood = true;

	SDL_PauseAudio(0);

	return;
}

void Audio::adjustDevice()
	throw ()
{
	int totalActive = 0;
	for (OPLVector::iterator i = this->chips.begin(); i != this->chips.end(); i++) {
		if ((*i)->active) totalActive++;
	}
	if ((totalActive == 0) && (this->audioGood)) {
		this->closeDevice();
	} else if ((totalActive > 0) && (!this->audioGood)) {
		this->openDevice();
	}
	return;
}

void Audio::closeDevice()
	throw ()
{
	SDL_CloseAudio();
	this->audioGood = false;
	return;
}
