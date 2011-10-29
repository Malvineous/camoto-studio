/**
 * @file   MusicWriter_OPLSynth.cpp
 * @brief  MusicWriter interface to an OPL synth.
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

#include <iostream>
#include "MusicWriter_OPLSynth.hpp"

using namespace camoto::gamemusic;

MusicWriter_OPLSynth::MusicWriter_OPLSynth(Audio::OPLPtr opl)
	throw () :
		MusicWriter_GenericOPL(DelayIsPostData),
		opl(opl)
{
}

MusicWriter_OPLSynth::~MusicWriter_OPLSynth()
	throw ()
{
}

void MusicWriter_OPLSynth::start()
	throw (camoto::stream::error)
{
	this->opl->write(0x01, 0x20); // Enable WaveSel
	this->opl->write(0x05, 0x01); // Enable OPL3
	return;
}

void MusicWriter_OPLSynth::changeSpeed(uint32_t usPerTick)
	throw ()
{
	this->usPerTick = usPerTick;
	return;
}

void MusicWriter_OPLSynth::nextPair(uint32_t delay, uint8_t chipIndex, uint8_t reg,
	uint8_t val)
	throw (camoto::stream::error)
{
	if (chipIndex > 0) {
		std::cerr << "[MusicWriter_OPLSynth] Multiple OPL chips not yet implemented!\n";
		return;
	}
	this->opl->write(reg, val);
	this->opl->delay(delay * this->usPerTick / 1000);
	return;
}
