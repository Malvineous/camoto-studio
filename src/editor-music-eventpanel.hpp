/**
 * @file   editor-music-eventpanel.hpp
 * @brief  Single channel list of events UI control for the music editor.
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

#ifndef _EDITOR_MUSIC_EVENTPANEL_HPP_
#define _EDITOR_MUSIC_EVENTPANEL_HPP_

#include <wx/panel.h>
#include <camoto/gamemusic.hpp>

class EventPanel;

#include "editor-music-document.hpp"

class EventPanel: public wxPanel, camoto::gamemusic::EventHandler
{
	public:

		EventPanel(MusicDocument *parent, int channel)
			throw ();

		~EventPanel()
			throw ();

		void onPaint(wxPaintEvent& pev);
		void onResize(wxSizeEvent& ev);

		// Event handler functions (used to paint events)

		virtual void handleEvent(const camoto::gamemusic::TempoEvent *ev)
			throw (camoto::stream::error);

		virtual void handleEvent(const camoto::gamemusic::NoteOnEvent *ev)
			throw (camoto::stream::error, camoto::gamemusic::EChannelMismatch,
				camoto::gamemusic::EBadPatchType);

		virtual void handleEvent(const camoto::gamemusic::NoteOffEvent *ev)
			throw (camoto::stream::error);

		virtual void handleEvent(const camoto::gamemusic::PitchbendEvent *ev)
			throw (camoto::stream::error);

		virtual void handleEvent(const camoto::gamemusic::ConfigurationEvent *ev)
			throw (camoto::stream::error);

		void drawNoteOn(int milliHertz, int instrument)
			throw ();

	protected:
		MusicDocument *doc;  ///< Music data to draw
		unsigned int channel; ///< Only show events from this channel (and chan 0)

		wxPaintDC *pdc;      ///< DC used in painting by EventHandler functions
		int paintY;          ///< Y-coord of line to write in EventHandler funcs

		DECLARE_EVENT_TABLE();

};

#endif // _EDITOR_MUSIC_EVENTPANEL_HPP_
