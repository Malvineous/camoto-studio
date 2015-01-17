/**
 * @file   editor-map.hpp
 * @brief  Map editor.
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

#ifndef _EDITOR_MAP_HPP_
#define _EDITOR_MAP_HPP_

#include <camoto/gamemaps.hpp>
#include "editor.hpp"

#define HT_DEFAULT   _("Ready")
#define HT_PASTE     _("L-click=paste")
#define HT_APPLY     _("L-click=apply")
#define HT_COPY      _("R-drag=copy")
#define HT_SELECT    _("R-drag=select")
#define HT_SCROLL    _("M-drag=scroll")
#define HT_CLOSEPATH _("Space=close path")
#define HT_INS       _("Ins=insert point")
#define HT_DEL       _("Del=delete")
#define HT_ESC       _("Esc=cancel")
#define HT_LAYER_ACT _("L-dclick=(de)activate layer")
#define HT_LAYER_VIS _("R-click=show/hide layer")
#define __           _(" | ")

class MapEditor: public IEditor
{
	public:
		/// View settings for this editor which are saved with the project.
		struct Settings {
			unsigned int zoomFactor; ///< Amount of zoom (1,2,4)
		};

		MapEditor(Studio *studio);
		virtual ~MapEditor();

		virtual IToolPanelVector createToolPanes() const;
		virtual void loadSettings(Project *proj);
		virtual void saveSettings(Project *proj) const;
		virtual bool isFormatSupported(const wxString& type) const;
		virtual IDocument *openObject(const GameObjectPtr& o);

	protected:
		Studio *studio;
		Settings settings;
};

#endif // _EDITOR_MAP_HPP_
