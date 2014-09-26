/**
 * @file   editor-music-eventpanel.hpp
 * @brief  Single channel list of events UI control for the music editor.
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

#ifndef _EDITOR_MUSIC_EVENTPANEL_HPP_
#define _EDITOR_MUSIC_EVENTPANEL_HPP_

#include <wx/panel.h>
#include <camoto/gamemusic.hpp>

class EventPanel;

#include "editor-music-document.hpp"

class EventPanel: public wxPanel, camoto::gamemusic::EventHandler
{
	public:
		EventPanel(MusicDocument *parent, unsigned int trackIndex);
		~EventPanel();

		void onPaint(wxPaintEvent& pev);
		void onResize(wxSizeEvent& ev);

		// Event handler functions (used to paint events)
		virtual void handleEvent(unsigned long delay, unsigned int trackIndex,
			unsigned int patternIndex, const camoto::gamemusic::TempoEvent *ev);
		virtual void handleEvent(unsigned long delay, unsigned int trackIndex,
			unsigned int patternIndex, const camoto::gamemusic::NoteOnEvent *ev);
		virtual void handleEvent(unsigned long delay, unsigned int trackIndex,
			unsigned int patternIndex, const camoto::gamemusic::NoteOffEvent *ev);
		virtual void handleEvent(unsigned long delay, unsigned int trackIndex,
			unsigned int patternIndex, const camoto::gamemusic::EffectEvent *ev);
		virtual void handleEvent(unsigned long delay, unsigned int trackIndex,
			unsigned int patternIndex, const camoto::gamemusic::ConfigurationEvent *ev);
virtual void endOfTrack(unsigned long delay);
virtual void endOfPattern(unsigned long delay);
		void drawNoteOn(int milliHertz, int instrument, int velocity);

	protected:
		MusicDocument *doc;        ///< Music data to draw
		unsigned int trackIndex;   ///< Track from which to draw events

		wxPaintDC *pdc;      ///< DC used in painting by EventHandler functions
		int paintY;          ///< Y-coord of line to write in EventHandler funcs

		DECLARE_EVENT_TABLE();
};

#endif // _EDITOR_MUSIC_EVENTPANEL_HPP_
