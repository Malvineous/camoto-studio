/**
 * @file   editor-map-document.cpp
 * @brief  Map editor document.
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
#include <camoto/util.hpp>
#include "editor-map-document.hpp"
#include "dlg-map-attr.hpp"

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

BEGIN_EVENT_TABLE(MapDocument, IDocument)
	EVT_TOOL(wxID_ZOOM_OUT, MapDocument::onZoomSmall)
	EVT_TOOL(wxID_ZOOM_100, MapDocument::onZoomNormal)
	EVT_TOOL(wxID_ZOOM_IN, MapDocument::onZoomLarge)
	EVT_TOOL(IDC_TOGGLEGRID, MapDocument::onToggleGrid)
	EVT_TOOL(IDC_MODE_TILE, MapDocument::onTileMode)
	EVT_TOOL(IDC_MODE_OBJ, MapDocument::onObjMode)
	EVT_TOOL(wxID_PROPERTIES, MapDocument::onSetAttributes)
END_EVENT_TABLE()

MapDocument::MapDocument(Studio *parent, MapEditor::Settings *settings,
	Map2DPtr map, fn_write fnWriteMap,
	camoto::gamemaps::TilesetCollectionPtr tilesets,
	const MapObjectVector *mapObjectVector)
	:	IDocument(parent, _T("map")),
		settings(settings),
		map(map),
		fnWriteMap(fnWriteMap),
		tilesets(tilesets)
{
	this->canvas = new MapCanvas(this, parent->getGLContext(), map, tilesets,
		parent->getGLAttributes(), mapObjectVector);

	wxToolBar *tb = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
	tb->SetToolBitmapSize(wxSize(16, 16));

	tb->AddTool(wxID_ZOOM_OUT, wxEmptyString,
		wxImage(::path.guiIcons + _T("zoom_1-1.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _("Zoom 1:1 (small)"),
		_("Set the zoom level so that one screen pixel is used for each tile pixel"));

	tb->AddTool(wxID_ZOOM_100, wxEmptyString,
		wxImage(::path.guiIcons + _T("zoom_2-1.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _("Zoom 2:1 (normal)"),
		_("Set the zoom level so that two screen pixels are used for each tile pixel"));
	tb->ToggleTool(wxID_ZOOM_100, true);

	tb->AddTool(wxID_ZOOM_IN, wxEmptyString,
		wxImage(::path.guiIcons + _T("zoom_3-1.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _("Zoom 3:1 (big)"),
		_("Set the zoom level so that three screen pixels are used for each tile pixel"));

	tb->AddSeparator();

	tb->AddTool(IDC_TOGGLEGRID, wxEmptyString,
		wxImage(::path.guiIcons + _T("grid.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_CHECK, _("Show grid?"),
		_("Draw a grid to show where each tile is?"));

	tb->AddSeparator();

	tb->AddTool(IDC_MODE_TILE, wxEmptyString,
		wxImage(::path.guiIcons + _T("mode-tile.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _("Tile mode"),
		_("Edit the map as a grid of tiles"));

	tb->AddTool(IDC_MODE_OBJ, wxEmptyString,
		wxImage(::path.guiIcons + _T("mode-obj.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _("Object mode"),
		_("Edit the map as collection of objects"));

	tb->AddSeparator();

	tb->AddTool(wxID_PROPERTIES, wxEmptyString,
		wxImage(::path.guiIcons + _T("properties.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _("Attributes"),
		_("View and change global map attributes"));

	// Tile-mode is the default for the canvas
	tb->ToggleTool(IDC_MODE_TILE, true);
	this->canvas->setTileMode();

	// Update the UI
	switch (this->settings->zoomFactor) {
		case 1: tb->ToggleTool(wxID_ZOOM_OUT, true); break;
		case 2: tb->ToggleTool(wxID_ZOOM_100, true); break;
		case 4: tb->ToggleTool(wxID_ZOOM_IN, true); break;
			// default: don't select anything, just in case!
	}
	this->canvas->setZoomFactor(this->settings->zoomFactor);

	tb->Realize();

	wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
	s->Add(tb, 0, wxEXPAND);
	s->Add(this->canvas, 1, wxEXPAND);
	this->SetSizer(s);
}

void MapDocument::save()
{
	try {
		this->fnWriteMap();
		this->isModified = false;
	} catch (const camoto::stream::error& e) {
		throw e;
	} catch (const std::exception& e) {
		throw camoto::stream::error(e.what());
	}
	return;
}

void MapDocument::onZoomSmall(wxCommandEvent& ev)
{
	this->setZoomFactor(1);
	return;
}

void MapDocument::onZoomNormal(wxCommandEvent& ev)
{
	this->setZoomFactor(2);
	return;
}

void MapDocument::onZoomLarge(wxCommandEvent& ev)
{
	this->setZoomFactor(4);
	return;
}

void MapDocument::onToggleGrid(wxCommandEvent& ev)
{
	this->canvas->showGrid(ev.IsChecked());
	return;
}

void MapDocument::onTileMode(wxCommandEvent& ev)
{
	this->canvas->setTileMode();
	return;
}

void MapDocument::onObjMode(wxCommandEvent& ev)
{
	this->canvas->setObjMode();
	return;
}

void MapDocument::onSetAttributes(wxCommandEvent& ev)
{
	DlgMapAttr dlg(this->studio, this->map);
	if (dlg.ShowModal() == wxID_OK) this->isModified = true;
	return;
}

void MapDocument::setZoomFactor(int f)
{
	this->settings->zoomFactor = f;
	this->canvas->setZoomFactor(f);
	return;
}
