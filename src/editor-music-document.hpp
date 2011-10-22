/**
 * @file   editor-music-document.hpp
 * @brief  Music IDocument interface.
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

#ifndef _EDITOR_MUSIC_DOCUMENT_HPP_
#define _EDITOR_MUSIC_DOCUMENT_HPP_

#include <boost/thread/thread.hpp>
#include <camoto/gamemusic.hpp>
#include "editor-music.hpp"

class MusicDocument;
class PlayerThread;

#include "editor-music-eventpanel.hpp"

typedef std::vector<camoto::gamemusic::EventPtr> EventVector;

class MusicDocument: public IDocument
{
	public:
		MusicDocument(IMainWindow *parent, camoto::gamemusic::MusicReaderPtr music,
			AudioPtr audio)
			throw ();

		~MusicDocument();

		virtual void save()
			throw (camoto::stream::error);

		void onSeekPrev(wxCommandEvent& ev);
		void onPlay(wxCommandEvent& ev);
		void onPause(wxCommandEvent& ev);
		void onSeekNext(wxCommandEvent& ev);
		void onZoomIn(wxCommandEvent& ev);
		void onZoomNormal(wxCommandEvent& ev);
		void onZoomOut(wxCommandEvent& ev);
		void onMouseWheel(wxMouseEvent& ev);
		void onResize(wxSizeEvent& ev);

		/// Set the position of the playback time highlight row.
		void setHighlight(int ticks)
			throw ();

		/// Push the current scroll and zoom settings to each channel and redraw
		void pushViewSettings();

	protected:
		camoto::gamemusic::MusicReaderPtr music;
		AudioPtr audio;
		Audio::OPLPtr opl;
		bool playing;

		int optimalTicksPerRow; ///< Cache best value for ticksPerRow (for zoom reset)
		int ticksPerRow;        ///< Current zoom level for all channels
		int absTimeStart;       ///< Current scroll pos for all channels
		int playPos;            ///< Current absTime of playback (for row highlight)

		PlayerThread *player;
		boost::thread thread;
		typedef std::vector<EventPanel *> EventPanelVector;
		EventPanelVector channelPanels;

		EventVector events;

		wxFont font;         ///< Font to use for event text
		int fontWidth;       ///< Width of each char in pixels
		int fontHeight;      ///< Height of each char/line in pixels
		int halfHeight;      ///< Number of char rows in half a screen (for positioning highlight row)

		friend class PlayerThread;
		friend class InstrumentPanel;
		friend class EventPanel;

		enum {
			IDC_SEEK_PREV = wxID_HIGHEST + 1,
			IDC_PLAY,
			IDC_PAUSE,
			IDC_SEEK_NEXT,
		};
		DECLARE_EVENT_TABLE();
};

#endif // _EDITOR_MUSIC_DOCUMENT_HPP_
