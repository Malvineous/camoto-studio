/**
 * @file   MusicWriter_OPLSynth.hpp
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

#ifndef _MUSICWRITER_OPLSYNTH_HPP_
#define _MUSICWRITER_OPLSYNTH_HPP_

#include <camoto/gamemusic/mus-generic-opl.hpp>
#include "audio.hpp"

/// MusicWriter implementation that "writes" direct to an OPL synth.
class MusicWriter_OPLSynth: virtual public camoto::gamemusic::MusicWriter_GenericOPL
{
	public:
		MusicWriter_OPLSynth(Audio::OPLPtr opl)
			throw ();

		virtual ~MusicWriter_OPLSynth()
			throw ();

		virtual void start()
			throw (camoto::stream::error);

		virtual void changeSpeed(uint32_t usPerTick)
			throw ();

		virtual void nextPair(uint32_t delay, uint8_t chipIndex, uint8_t reg,
			uint8_t val)
			throw (camoto::stream::error);

	protected:
		Audio::OPLPtr opl;  ///< OPL chip to write to
		uint32_t usPerTick; ///< Current tempo in microseconds per tick

};

#endif // _MUSICWRITER_OPLSYNTH_HPP_
