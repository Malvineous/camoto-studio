/**
 * @file   editor-map-tilepanel.cpp
 * @brief  List of available tiles for the map editor.
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

#include <GL/glew.h>
#include <camoto/gamegraphics.hpp>
#include "editor-map-tilepanel.hpp"
#include "editor-map-tilepanel-canvas.hpp"

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

BEGIN_EVENT_TABLE(TilePanel, IToolPanel)
END_EVENT_TABLE()

TilePanel::TilePanel(Studio *parent)
	:	IToolPanel(parent)
{
	this->canvas = new TilePanelCanvas(this, parent->getGLContext(),
		parent->getGLAttributes(), 1);

	wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
	s->Add(this->canvas, 1, wxEXPAND);
	this->SetSizer(s);
}

void TilePanel::getPanelInfo(wxString *id, wxString *label) const
{
	*id = _T("map.tiles");
	*label = _("Tiles");
	return;
}

void TilePanel::switchDocument(IDocument *doc)
{
	this->doc = static_cast<MapDocument *>(doc);
	if (!this->doc) return; // NULL was passed in

	// Switch to the new document's textures
	TEXTURE_MAP *tm = &this->doc->canvas->textureMap[0]; // TEMP: layer 0 only
	this->canvas->setTextures(*tm);

	// Reflect the map's zoom level
	this->canvas->setZoomFactor(this->doc->settings->zoomFactor);

	return;
}

void TilePanel::loadSettings(Project *proj)
{
	return;
}

void TilePanel::saveSettings(Project *proj) const
{
	return;
}
