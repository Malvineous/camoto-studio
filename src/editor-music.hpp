/**
 * @file   editor-music.hpp
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

#ifndef _EDITOR_MUSIC_HPP_
#define _EDITOR_MUSIC_HPP_

#include <camoto/gamemusic.hpp>
#include "audio.hpp"
#include "editor.hpp"

class MusicEditor;

#include "editor-music-document.hpp"

class MusicEditor: public IEditor
{
	public:
		/// Settings for this editor which are saved with the project.
		struct Settings {
			wxString lastExportPath;      ///< Filename and path of last exported file
			wxString lastExportType;      ///< File format used for last export
			unsigned int lastExportFlags; ///< Flags used for last export
		};

		MusicEditor(Studio *studio, AudioPtr audio);
		virtual ~MusicEditor();

		virtual IToolPanelVector createToolPanes() const;
		virtual void loadSettings(Project *proj);
		virtual void saveSettings(Project *proj) const;
		virtual bool isFormatSupported(const wxString& type) const;
		virtual IDocument *openObject(const GameObjectPtr& o);

	protected:
		Studio *studio;
		AudioPtr audio;
		Settings settings;

		friend class MusicDocument;
};

#endif // _EDITOR_MUSIC_HPP_
