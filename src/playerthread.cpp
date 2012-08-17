/**
 * @file  playerthread.cpp
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

#include <camoto/gamemusic/patch-midi.hpp>
#include "playerthread.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

PlayerThread::PlayerThread(AudioPtr audio, MusicPtr music, PlayerCallback *cb)
	:	audio(audio),
		music(music),
		cb(cb),
		playpause_lock(playpause_mutex), // initial state is locked (paused)
		doquit(false),
		dorewind(false),
		usPerTickOPL(1000), // sensible defaults, just in case
		usPerTickMIDI(MIDI_DEF_uS_PER_TICK)
{
	this->opl = this->audio->createOPL();
	this->midiData.reserve(16);
	try {
		this->midiOut.reset(new RtMidiOut());
		if (::config.midiDevice == -1) {
			this->midiOut->openVirtualPort("Camoto Studio");
		} else {
			this->midiOut->openPort(::config.midiDevice);
		}

		this->midiData.resize(3);
		for (int c = 0; c < 16; c++) {
			// Set volume to 100%
			this->midiData[0] = 0xB0 | c;
			this->midiData[1] = 0x07;
			this->midiData[2] = 0x7F;
			this->midiOut->sendMessage(&midiData);
		}

	} catch (const RtError& e) {
		std::cerr << "Error initialising MIDI device: " << e.getMessage()
			<< std::endl;;
		// this->midiOut will be NULL
		this->midiOut.reset();
	}
}

PlayerThread::~PlayerThread()
{
	if (this->midiOut) {
		// Turn all MIDI notes off
		this->midiData.resize(3);
		this->midiData[0] = 0xB0;
		this->midiData[1] = 0x7B;
		this->midiData[2] = 0x7F;
		this->midiOut->sendMessage(&midiData);
	}

	this->audio->releaseOPL(this->opl);
}

void PlayerThread::operator()()
{
	// Prepare Event -> OPL converter
	this->oplConv.reset(new EventConverter_OPL(this, this->music->patches,
		OPL_FNUM_DEFAULT, Default));

	// Prepare Event -> MIDI converter
	EventConverter_MIDI midiConv(this, this->music->patches,
		MIDIFlags::MIDIPatchesOnly, MIDI_DEF_TICKS_PER_QUARTER_NOTE);

	this->opl->write(0x01, 0x20); // Enable WaveSel
	this->opl->write(0x05, 0x01); // Enable OPL3

	EventVector::iterator posOPL;
	EventVector::iterator posMIDI = posOPL = this->music->events->begin();
	unsigned long playTime = 0;

	struct timespec tm;
	tm.tv_sec = 0;
	for (;;) {
		// Block if we've been paused.  This has to be in a control block as we
		// don't want to hold the mutex while we're blocking in the delay()
		// call, which is in processEvent().
		{
			boost::mutex::scoped_lock loop_lock(this->playpause_mutex);
		}

		// If we've just been resumed, check to see if it's because we're exiting
		if (this->doquit) break;

		try {
			// Update the playback time so the highlight gets moved
			this->cb->notifyPosition(playTime);

			while (
				(posMIDI != this->music->events->end())
				&& ((*posMIDI)->absTime * this->usPerTickMIDI <= playTime)
			) {
				if (this->midiOut) {
					(*posMIDI)->processEvent(&midiConv);
				}
				posMIDI++;
			}

			if (this->oplConv) {
				while (
					(posOPL != this->music->events->end())
					&& ((*posOPL)->absTime * this->usPerTickOPL + ::config.pcmDelay <= playTime)
				) {
					(*posOPL)->processEvent(this->oplConv.get());
					posOPL++;
				}
			}

			// Wait the minimum amount of time until the next event of any type
			unsigned long nextTimeOPL, nextTimeMIDI;
			if (posOPL != this->music->events->end()) {
				nextTimeOPL = (*posOPL)->absTime * usPerTickOPL + ::config.pcmDelay;
			} else {
				// No next event, wait 100ms after current pos
				nextTimeOPL = playTime + 100 * usPerTickOPL;
			}
			if (posMIDI != this->music->events->end()) {
				nextTimeMIDI = (*posMIDI)->absTime * usPerTickMIDI;
			} else {
				// No next event, wait 100ms after current pos
				nextTimeMIDI = playTime + 100 * usPerTickMIDI;
			}
			unsigned long nextTime = std::min(nextTimeMIDI, nextTimeOPL);
			tm.tv_nsec = (nextTime - playTime) * 1000; // microseconds -> nanoseconds
			nanosleep(&tm, NULL);
			playTime = nextTime;

			// Loop once we reach the end of the song.  We don't check to see if the
			// MIDI data has finished because it will always finish earlier or the
			// same time as the OPL data, and it will cope sitting at the end for
			// a while.
			if ((this->dorewind) || (posOPL == this->music->events->end())) {
				posOPL = posMIDI = this->music->events->begin();
				if (this->oplConv) this->oplConv->rewind();
				midiConv.rewind();
				playTime = 0;
				this->dorewind = false;
			}
		} catch (...) {
			std::cerr << "Error converting event into OPL data!  Ignoring.\n";
		}
	}
	return;
}

void PlayerThread::resume()
{
	this->audio->pause(this->opl, false); // this makes delays start blocking
	this->playpause_lock.unlock();
	return;
}

void PlayerThread::pause()
{
	this->playpause_lock.lock();
	this->audio->pause(this->opl, true); // this makes delays expire immediately
	return;
}

void PlayerThread::quit()
{
	this->doquit = true;
	// Wake up the thread if need be
	if (this->playpause_lock.owns_lock()) this->resume();
	return;
}

void PlayerThread::rewind()
{
	this->dorewind = true;
	return;
}

void PlayerThread::writeNextPair(const OPLEvent *oplEvent)
{
	this->opl->write(oplEvent->reg, oplEvent->val);
	return;
}

void PlayerThread::writeTempoChange(tempo_t usPerTick)
{
	this->usPerTickOPL = usPerTick;
	return;
}

void PlayerThread::midiNoteOff(uint32_t delay, uint8_t channel, uint8_t note,
	uint8_t velocity)
{
	assert(this->midiOut);
	this->midiData.resize(3);
	this->midiData[0] = 0x80 | channel;
	this->midiData[1] = note;
	this->midiData[2] = velocity;
	this->midiOut->sendMessage(&midiData);
	return;
}

void PlayerThread::midiNoteOn(uint32_t delay, uint8_t channel, uint8_t note,
	uint8_t velocity)
{
	assert(this->midiOut);
	this->midiData.resize(3);
	this->midiData[0] = 0x90 | channel;
	this->midiData[1] = note;
	this->midiData[2] = velocity;
	this->midiOut->sendMessage(&midiData);
	return;
}

void PlayerThread::midiPatchChange(uint32_t delay, uint8_t channel,
	uint8_t instrument)
{
	assert(this->midiOut);
	this->midiData.resize(2);
	this->midiData[0] = 0xC0 | channel;
	this->midiData[1] = instrument;
	this->midiOut->sendMessage(&midiData);
	return;
}

void PlayerThread::midiController(uint32_t delay, uint8_t channel,
	uint8_t controller, uint8_t value)
{
	assert(this->midiOut);
	this->midiData.resize(3);
	this->midiData[0] = 0xB0 | channel;
	this->midiData[1] = controller;
	this->midiData[2] = value;
	this->midiOut->sendMessage(&midiData);
	return;
}

void PlayerThread::midiPitchbend(uint32_t delay, uint8_t channel, uint16_t bend)
{
	assert(this->midiOut);
	// TODO
	return;
}

void PlayerThread::midiSetTempo(uint32_t delay, tempo_t usPerTick)
{
	assert(this->midiOut);
	this->usPerTickMIDI = usPerTick;
	return;
}
