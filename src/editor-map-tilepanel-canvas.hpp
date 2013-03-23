/**
 * @file   editor-map-tilepanel-canvas.hpp
 * @brief  Graphical display for tile selection panel in map editor.
 *
 * Copyright (C) 2010-2013 Adam Nielsen <malvineous@shikadi.net>
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

#ifndef _EDITOR_MAP_TILEPANEL_CANVAS_HPP_
#define _EDITOR_MAP_TILEPANEL_CANVAS_HPP_

#include "editor-tileset-canvas.hpp"
#include "editor-map-tilepanel.hpp"

/// Specialisation of tileset viewer for map tile selector panel.
class TilePanelCanvas: public TilesetCanvas
{
	public:
		TilePanelCanvas(TilePanel *parent, wxGLContext *glcx, int *attribList,
			int zoomFactor);

		~TilePanelCanvas();

		void onRightClick(wxMouseEvent& ev);

	protected:
		TilePanel *tilePanel;

		DECLARE_EVENT_TABLE();
};

#endif // _EDITOR_MAP_TILEPANEL_CANVAS_HPP_
