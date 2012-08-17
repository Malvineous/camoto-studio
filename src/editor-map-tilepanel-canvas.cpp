/**
 * @file   editor-map-tilepanel-canvas.cpp
 * @brief  Graphical display for tile selection panel in map editor.
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

#include "editor-map-tilepanel-canvas.hpp"

using namespace camoto;
using namespace camoto::gamegraphics;

BEGIN_EVENT_TABLE(TilePanelCanvas, TilesetCanvas)
	EVT_RIGHT_DOWN(TilePanelCanvas::onRightClick)
END_EVENT_TABLE()

TilePanelCanvas::TilePanelCanvas(TilePanel *parent, wxGLContext *glcx,
	int *attribList, int zoomFactor)
	:	TilesetCanvas(parent, glcx, attribList, zoomFactor),
		tilePanel(parent)
{
}

TilePanelCanvas::~TilePanelCanvas()
{
}

void TilePanelCanvas::onRightClick(wxMouseEvent& ev)
{
	return;
}
