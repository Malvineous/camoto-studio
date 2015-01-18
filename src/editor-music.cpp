/**
 * @file   editor-music.cpp
 * @brief  Music editor.
 *
 * Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>
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

MusicEditor::MusicEditor(Studio *studio, AudioPtr audio)
	:	studio(studio),
		audio(audio)
{
}

MusicEditor::~MusicEditor()
{
}

std::vector<IToolPanel *> MusicEditor::createToolPanes() const
{
	std::vector<IToolPanel *> panels;
	InstrumentPanel *inst = new InstrumentPanel(this->studio);
	panels.push_back(new InstrumentListPanel(this->studio, inst));
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
	return !!this->studio->mgrMusic->getMusicTypeByCode(strType);
}

IDocument *MusicEditor::openObject(const GameObjectPtr& o)
{
	MusicPtr music;
	fn_write fnWriteMusic;
	try {
		music = this->studio->openMusic(o, &fnWriteMusic);
		if (!music) return NULL; // user cancelled
	} catch (const camoto::stream::error& e) {
		throw EFailure(wxString::Format(_("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}

	return new MusicDocument(this, music, fnWriteMusic);
}
