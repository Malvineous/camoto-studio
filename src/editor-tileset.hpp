/**
 * @file   editor-tileset.hpp
 * @brief  Tileset editor.
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

#ifndef _EDITOR_TILESET_HPP_
#define _EDITOR_TILESET_HPP_

#include <camoto/gamegraphics.hpp>
#include "editor.hpp"

class TilesetEditor: public IEditor
{
	public:
		/// View settings for this editor which are saved with the project.
		struct Settings {
			unsigned int zoomFactor; ///< Amount of zoom (1,2,4)
		};

		TilesetEditor(Studio *studio);
		virtual ~TilesetEditor();

		virtual IToolPanelVector createToolPanes() const;
		virtual void loadSettings(Project *proj);
		virtual void saveSettings(Project *proj) const;
		virtual bool isFormatSupported(const wxString& type) const;
		virtual IDocument *openObject(const GameObjectPtr& o);

	protected:
		Studio *studio;
		Settings settings;
};

#endif // _EDITOR_TILESET_HPP_
