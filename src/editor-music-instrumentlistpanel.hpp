/**
 * @file   editor-music-instrumentlistpanel.hpp
 * @brief  Panel showing list of instruments for music editor.
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

#ifndef _EDITOR_MUSIC_INSTRUMENTLISTPANEL_HPP_
#define _EDITOR_MUSIC_INSTRUMENTLISTPANEL_HPP_

class InstrumentListPanel;

#include <camoto/gamemusic/patchbank.hpp>
#include "editor.hpp"
#include "editor-music.hpp"
#include "editor-music-instrumentpanel.hpp"

class InstrumentListPanel: public IToolPanel
{
	public:
		InstrumentListPanel(IMainWindow *parent, InstrumentPanel *instPanel)
			throw ();

		virtual void getPanelInfo(wxString *id, wxString *label) const
			throw ();

		virtual void switchDocument(IDocument *doc)
			throw ();

		virtual void loadSettings(Project *proj)
			throw ();

		virtual void saveSettings(Project *proj) const
			throw ();

		/// Put a different instrument over the top of the given one in the PatchBank
		void replaceInstrument(unsigned int index,
			camoto::gamemusic::PatchPtr newInstrument)
			throw ();

		/// Update the UI to reflect changes in the given instrument
		void updateInstrument(unsigned int index)
			throw ();

		void onItemSelected(wxListEvent& ev);
		void onItemRightClick(wxListEvent& ev);

	protected:
		InstrumentPanel *instPanel;
		wxListCtrl *list;
		MusicDocument *doc;
		unsigned int lastInstrument; ///< Last instrument edited

		void updateInstrumentView(unsigned int index)
			throw ();

		DECLARE_EVENT_TABLE();
};

#endif // _EDITOR_MUSIC_INSTRUMENTLISTPANEL_HPP_
