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

#include "main.hpp"
#include "editor-music.hpp"
#include "editor-music-eventpanel.hpp"
#include "editor-music-instrumentpanel.hpp"
#include "editor-music-instrumentlistpanel.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

MusicEditor::MusicEditor(IMainWindow *parent, AudioPtr audio)
	:	frame(parent),
		audio(audio),
		pManager(camoto::gamemusic::getManager())
{
}

MusicEditor::~MusicEditor()
{
}

std::vector<IToolPanel *> MusicEditor::createToolPanes() const
{
	std::vector<IToolPanel *> panels;
	InstrumentPanel *inst = new InstrumentPanel(this->frame);
	panels.push_back(new InstrumentListPanel(this->frame, inst));
	panels.push_back(inst);
	return panels;
}

void MusicEditor::loadSettings(Project *proj)
{
	proj->config.Read(_T("editor-music/last-export-path"), &this->settings.lastExportPath);
	proj->config.Read(_T("editor-music/last-export-type"), &this->settings.lastExportType);
	proj->config.Read(_T("editor-music/last-export-flags"), (int *)&this->settings.lastExportFlags);
	return;
}

void MusicEditor::saveSettings(Project *proj) const
{
	proj->config.Write(_T("editor-music/last-export-path"), this->settings.lastExportPath);
	proj->config.Write(_T("editor-music/last-export-type"), this->settings.lastExportType);
	proj->config.Write(_T("editor-music/last-export-flags"), (int)this->settings.lastExportFlags);
	return;
}

bool MusicEditor::isFormatSupported(const wxString& type) const
{
	std::string strType(type.ToUTF8());
	return this->pManager->getMusicTypeByCode(strType);
}

IDocument *MusicEditor::openObject(const wxString& typeMinor,
	stream::inout_sptr data, const wxString& filename, SuppMap supp,
	const Game *game)
{
	if (typeMinor.IsEmpty()) {
		throw EFailure(_("No file type was specified for this item!"));
	}

	std::string strType;
	strType.append(typeMinor.ToUTF8());
	MusicTypePtr pMusicType(this->pManager->getMusicTypeByCode(strType));
	if (!pMusicType) {
		wxString wxtype(strType.c_str(), wxConvUTF8);
		throw EFailure(wxString::Format(_("Sorry, it is not possible to edit this "
			"song as the \"%s\" format is unsupported.  (No handler for \"%s\")"),
			typeMinor.c_str(), wxtype.c_str()));
	}
	std::cout << "[editor-music] Using handler for " << pMusicType->getFriendlyName() << "\n";

	// Check to see if the file is actually in this format
	if (pMusicType->isInstance(data) < MusicType::PossiblyYes) {
		std::string friendlyType = pMusicType->getFriendlyName();
		wxString wxtype(friendlyType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_("This file is supposed to be in \"%s\" "
			"format, but it seems this may not be the case.  Would you like to try "
			"opening it anyway?"), wxtype.c_str());
		wxMessageDialog dlg(this->frame, msg, _("Open item"), wxYES_NO | wxICON_ERROR);
		int r = dlg.ShowModal();
		if (r != wxID_YES) return NULL;
	}

	// Collect any supplemental files supplied
	SuppData suppData;
	suppMapToData(supp, suppData);

	// Open the music file
	try {
		return new MusicDocument(this, pMusicType, data, suppData);
	} catch (const camoto::stream::error& e) {
		throw EFailure(wxString::Format(_("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}
}
