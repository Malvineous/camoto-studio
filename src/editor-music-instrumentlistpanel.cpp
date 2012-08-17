/**
 * @file   editor-music-instrumentlistpanel.cpp
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

#include <wx/listctrl.h>
#include "editor-music-instrumentlistpanel.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

enum {
	IDC_INSTLIST = wxID_HIGHEST + 1,
};

BEGIN_EVENT_TABLE(InstrumentListPanel, IToolPanel)
	EVT_LIST_ITEM_SELECTED(IDC_INSTLIST, InstrumentListPanel::onItemSelected)
	EVT_LIST_ITEM_RIGHT_CLICK(IDC_INSTLIST, InstrumentListPanel::onItemRightClick)
END_EVENT_TABLE()

InstrumentListPanel::InstrumentListPanel(IMainWindow *parent, InstrumentPanel *instPanel)
	:	IToolPanel(parent),
		instPanel(instPanel),
		lastInstrument(0)
{
	this->list = new wxListCtrl(this, IDC_INSTLIST, wxDefaultPosition,
		wxDefaultSize, wxLC_REPORT | wxBORDER_NONE | wxLC_NO_HEADER |
		wxLC_SINGLE_SEL);

	this->list->SetImageList(parent->smallImages, wxIMAGE_LIST_SMALL);

	wxListItem info;
	info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE | wxLIST_MASK_FORMAT;
	info.m_image = 0;
	info.m_format = 0;
	info.m_text = _("Instrument");
	this->list->InsertColumn(0, info);

	wxSizer *s = new wxBoxSizer(wxVERTICAL);
	s->Add(this->list, 1, wxEXPAND);
	this->SetSizer(s);

	instPanel->setInstrumentList(this, PatchBankPtr());
}

void InstrumentListPanel::getPanelInfo(wxString *id, wxString *label) const
{
	*id = _T("music.instrumentlist");
	*label = _("Instrument list");
	return;
}

void InstrumentListPanel::switchDocument(IDocument *doc)
{
	this->list->DeleteAllItems();

	this->doc = static_cast<MusicDocument *>(doc);
	if (!this->doc) return; // NULL was passed in

	// Populate the list
	PatchBankPtr instruments = this->doc->music->patches;
	if (instruments) {
		unsigned int numInst = instruments->size();
		for (unsigned int i = 0; i < numInst; i++) {
			PatchPtr patch = instruments->at(i);

			wxString title;
			title.Printf(_T("%02X: "), i);
			if (patch->name.empty()) {
				title.Append(_("[no name]"));
			} else {
				title.Append(wxString(patch->name.c_str(), wxConvUTF8));
			}

			int image;
			if (dynamic_cast<OPLPatch *>(patch.get())) image = ImageListIndex::InstOPL;
			else if (dynamic_cast<MIDIPatch *>(patch.get())) image = ImageListIndex::InstMIDI;
			//else if (dynamic_cast<PCMPatch *>(patch.get())) image = ImageListIndex::InstPCM;
			else image = 0;
			long id = this->list->InsertItem(i, title, image);
			this->list->SetItemData(id, i);
		}
	}

	this->list->SetColumnWidth(0, wxLIST_AUTOSIZE);

	this->instPanel->setInstrumentList(this, instruments);

	// Use the previous instrument in the editing panel
	this->instPanel->setInstrument(this->lastInstrument);
	return;
}

void InstrumentListPanel::loadSettings(Project *proj)
{
	return;
}

void InstrumentListPanel::saveSettings(Project *proj) const
{
	return;
}

void InstrumentListPanel::replaceInstrument(unsigned int index,
	PatchPtr newInstrument)
{
	this->doc->music->patches->at(index) = newInstrument;
	this->updateInstrumentView(index);
	return;
}

void InstrumentListPanel::updateInstrument(unsigned int index)
{
	this->updateInstrumentView(index);
	return;
}

void InstrumentListPanel::onItemSelected(wxListEvent& ev)
{
	if (!this->doc) return;

	// Open the instrument in the instrument editing panel
	this->lastInstrument = ev.GetData();
	this->instPanel->setInstrument(this->lastInstrument);
	return;
}

void InstrumentListPanel::onItemRightClick(wxListEvent& ev)
{
	if (!this->doc) return;

	int instIndex = ev.GetData();
	// TODO: Mute instrument

	this->updateInstrumentView(instIndex);
	return;
}

void InstrumentListPanel::updateInstrumentView(unsigned int index)
{
	PatchPtr patch = this->doc->music->patches->at(index);
	if (!patch) return;

	int image;
	bool muted = false; // temp
	if (muted) {
		image = ImageListIndex::InstMute;
	} else {
		if (dynamic_cast<OPLPatch *>(patch.get())) image = ImageListIndex::InstOPL;
		else if (dynamic_cast<MIDIPatch *>(patch.get())) image = ImageListIndex::InstMIDI;
		//else if (dynamic_cast<PCMPatch *>(patch.get())) image = ImageListIndex::InstPCM;
		else image = ImageListIndex::File; // "unknown"
	}
	this->list->SetItemImage(index, image);

	wxString title;
	title.Printf(_T("%02X: "), index);
	if (patch->name.empty()) {
		title.Append(_("[no name]"));
	} else {
		title.Append(wxString(patch->name.c_str(), wxConvUTF8));
	}
	this->list->SetItemText(index, title);
	return;
}
