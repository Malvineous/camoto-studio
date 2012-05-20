/**
 * @file   editor-music.cpp
 * @brief  Music editor.
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

#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/listctrl.h>
#include "main.hpp"
#include "editor-music.hpp"
#include "editor-music-eventpanel.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

enum {
	IDC_LAYER = wxID_HIGHEST + 1,
};

class InstrumentPanel: public IToolPanel
{
	public:
		InstrumentPanel(IMainWindow *parent)
			throw () :
				IToolPanel(parent)
		{
			this->list = new wxListCtrl(this, IDC_LAYER, wxDefaultPosition,
				wxDefaultSize, wxLC_REPORT | wxBORDER_NONE | wxLC_NO_HEADER |
				wxLC_SINGLE_SEL);

			this->list->SetImageList(parent->smallImages, wxIMAGE_LIST_SMALL);

			wxListItem info;
			info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE | wxLIST_MASK_FORMAT;
			info.m_image = 0;
			info.m_format = 0;
			info.m_text = _T("Instrument");
			this->list->InsertColumn(0, info);

			wxSizer *s = new wxBoxSizer(wxVERTICAL);
			s->Add(this->list, 1, wxEXPAND);
			this->SetSizer(s);
		}

		virtual void getPanelInfo(wxString *id, wxString *label) const
			throw ()
		{
			*id = _T("music.instruments");
			*label = _T("Instruments");
			return;
		}

		virtual void switchDocument(IDocument *doc)
			throw ()
		{
			this->list->DeleteAllItems();

			this->doc = static_cast<MusicDocument *>(doc);
			if (!this->doc) return; // NULL was passed in

			// Populate the list
			PatchBankPtr instruments = this->doc->music->patches;
			if (instruments) {
				for (unsigned int i = 0; i < instruments->getPatchCount(); i++) {
					PatchPtr patch = instruments->getPatch(i);
					std::string name = patch->name;
					if (name.empty()) name = "[no name]";
					int image;
					if (dynamic_cast<OPLPatch *>(patch.get())) image = ImageListIndex::InstOPL;
					else if (dynamic_cast<MIDIPatch *>(patch.get())) image = ImageListIndex::InstMIDI;
					//else if (dynamic_cast<PCMPatch *>(patch.get())) image = ImageListIndex::InstPCM;
					else image = 0;
					long id = this->list->InsertItem(i,
						wxString(name.c_str(), wxConvUTF8), image);
					this->list->SetItemData(id, i);
				}
			}

			this->list->SetColumnWidth(0, wxLIST_AUTOSIZE);
			return;
		}

		virtual void loadSettings(Project *proj)
			throw ()
		{
			return;
		}

		virtual void saveSettings(Project *proj) const
			throw ()
		{
			return;
		}

		/// Open the selected instrument in the instrument editor
		void onItemOpen(wxListEvent& ev)
		{
			if (!this->doc) return;
			// TODO: Open instrument in instrument editor
			return;
		}

		void onItemRightClick(wxListEvent& ev)
		{
			if (!this->doc) return;

			// TODO: Mute instrument
			int instIndex = ev.GetData();

			int image;
			bool muted = true; // temp
			if (muted) {
				image = 1;
			} else {
				PatchBankPtr instruments = this->doc->music->patches;
				PatchPtr patch = instruments->getPatch(instIndex);
				if (dynamic_cast<OPLPatch *>(patch.get())) image = 2;
				else if (dynamic_cast<MIDIPatch *>(patch.get())) image = 3;
				//else if (dynamic_cast<PCMPatch *>(patch.get())) image = 4;
				else image = 0;
			}
			this->list->SetItemImage(ev.GetIndex(), image);
			return;
		}

	protected:
		wxListCtrl *list;
		MusicDocument *doc;

		DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(InstrumentPanel, IToolPanel)
	EVT_LIST_ITEM_ACTIVATED(IDC_LAYER, InstrumentPanel::onItemOpen)
	EVT_LIST_ITEM_RIGHT_CLICK(IDC_LAYER, InstrumentPanel::onItemRightClick)
END_EVENT_TABLE()


MusicEditor::MusicEditor(IMainWindow *parent, AudioPtr audio)
	throw () :
		frame(parent),
		audio(audio),
		pManager(camoto::gamemusic::getManager())
{
}

MusicEditor::~MusicEditor()
	throw ()
{
}

std::vector<IToolPanel *> MusicEditor::createToolPanes() const
	throw ()
{
	std::vector<IToolPanel *> panels;
	panels.push_back(new InstrumentPanel(this->frame));
	return panels;
}

void MusicEditor::loadSettings(Project *proj)
	throw ()
{
	proj->config.Read(_T("editor-music/last-export-path"), &this->settings.lastExportPath);
	proj->config.Read(_T("editor-music/last-export-type"), &this->settings.lastExportType);
	proj->config.Read(_T("editor-music/last-export-flags"), (int *)&this->settings.lastExportFlags);
	return;
}

void MusicEditor::saveSettings(Project *proj) const
	throw ()
{
	proj->config.Write(_T("editor-music/last-export-path"), this->settings.lastExportPath);
	proj->config.Write(_T("editor-music/last-export-type"), this->settings.lastExportType);
	proj->config.Write(_T("editor-music/last-export-flags"), (int)this->settings.lastExportFlags);
	return;
}

bool MusicEditor::isFormatSupported(const wxString& type) const
	throw ()
{
	std::string strType(type.ToUTF8());
	return this->pManager->getMusicTypeByCode(strType);
}

IDocument *MusicEditor::openObject(const wxString& typeMinor,
	stream::inout_sptr data, const wxString& filename, SuppMap supp,
	const Game *game)
	throw (EFailure)
{
	if (typeMinor.IsEmpty()) {
		throw EFailure(_T("No file type was specified for this item!"));
	}

	std::string strType;
	strType.append(typeMinor.ToUTF8());
	MusicTypePtr pMusicType(this->pManager->getMusicTypeByCode(strType));
	if (!pMusicType) {
		wxString wxtype(strType.c_str(), wxConvUTF8);
		throw EFailure(wxString::Format(_T("Sorry, it is not possible to edit this "
			"song as the \"%s\" format is unsupported.  (No handler for \"%s\")"),
			typeMinor.c_str(), wxtype.c_str()));
	}
	std::cout << "[editor-music] Using handler for " << pMusicType->getFriendlyName() << "\n";

	// Check to see if the file is actually in this format
	if (pMusicType->isInstance(data) < MusicType::PossiblyYes) {
		std::string friendlyType = pMusicType->getFriendlyName();
		wxString wxtype(friendlyType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_T("This file is supposed to be in \"%s\" "
			"format, but it seems this may not be the case.  Would you like to try "
			"opening it anyway?"), wxtype.c_str());
		wxMessageDialog dlg(this->frame, msg, _T("Open item"), wxYES_NO | wxICON_ERROR);
		int r = dlg.ShowModal();
		if (r != wxID_YES) return NULL;
	}

	// Collect any supplemental files supplied
	SuppData suppData;
	suppMapToData(supp, suppData);

	// Open the music file
	try {
		MusicPtr pMusic = pMusicType->read(data, suppData);
		assert(pMusic);

		return new MusicDocument(this->frame, pMusic, this->audio, this->pManager,
			&this->settings);
	} catch (const camoto::stream::error& e) {
		throw EFailure(wxString::Format(_T("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}
}
