/**
 * @file   editor-music-instrumentpanel.hpp
 * @brief  Instrument adjustment panel for music editor.
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

#ifndef _EDITOR_MUSIC_INSTRUMENTPANEL_HPP_
#define _EDITOR_MUSIC_INSTRUMENTPANEL_HPP_

class InstrumentPanel;

#include <wx/choicebk.h>
#include <wx/gbsizer.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <camoto/gamemusic/patchbank.hpp>
#include <camoto/gamemusic/patch-opl.hpp>
#include <camoto/gamemusic/patch-midi.hpp>
#include "editor.hpp"
#include "editor-music-instrumentlistpanel.hpp"

class InstrumentPanel: public IToolPanel
{
	public:
		InstrumentPanel(IMainWindow *parent)
			throw ();

		void addNumberControl(wxWindow *parent, wxGridBagSizer *sizer, int *row, unsigned int id,
			const wxString& title, unsigned int min, unsigned int max)
			throw ();

		virtual void getPanelInfo(wxString *id, wxString *label) const
			throw ();

		virtual void switchDocument(IDocument *doc)
			throw ();

		virtual void loadSettings(Project *proj)
			throw ();

		virtual void saveSettings(Project *proj) const
			throw ();

		void setInstrumentList(InstrumentListPanel *instList, camoto::gamemusic::PatchBankPtr patchBank)
			throw ();

		void setInstrument(unsigned int newIndex)
			throw ();

		void onTypeChanged(wxChoicebookEvent& ev);
		void onValueChanged(wxSpinEvent& ev);
		void onRenamed(wxCommandEvent& ev);

	protected:
		wxStaticText *txtInstNum;
		wxTextCtrl *txtInstName;
		wxChoicebook *tabs;
		InstrumentListPanel *instList;
		unsigned int instIndex;  ///< Index into instList of currently edited instrument
		camoto::gamemusic::PatchBankPtr patchBank;

		int instType; ///< Type of instrument currently in use
		camoto::gamemusic::PatchPtr inst; ///< Instrument being edited
		camoto::gamemusic::OPLPatchPtr oplInst; ///< inst cast to OPLPatch, or NULL
		camoto::gamemusic::MIDIPatchPtr midiInst; ///< inst cast to MIDIPatch, or NULL
		//camoto::gamemusic::PCMPatchPtr pcmInst; ///< inst cast to PCMPatch, or NULL

		DECLARE_EVENT_TABLE();
};

#endif // _EDITOR_MUSIC_INSTRUMENTPANEL_HPP_
