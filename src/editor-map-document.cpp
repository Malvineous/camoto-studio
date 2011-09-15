/**
 * @file   editor-map-document.cpp
 * @brief  Map editor document.
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

#include <camoto/util.hpp>
#include "editor-map-document.hpp"

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
END_EVENT_TABLE()

MapDocument::MapDocument(IMainWindow *parent, MapTypePtr mapType,
	SuppData suppData, iostream_sptr mapFile, FN_TRUNCATE fnTrunc,
	VC_TILESET tileset, const MapObjectVector *mapObjectVector)
	throw (std::ios::failure, EFailure) :
		IDocument(parent, _T("map")),
		mapType(mapType),
		suppData(suppData),
		mapFile(mapFile),
		fnTrunc(fnTrunc),
		tileset(tileset)
{
	MapPtr genMap = mapType->open(mapFile, suppData);
	assert(genMap);

	this->map = boost::dynamic_pointer_cast<Map2D>(genMap);
	if (!this->map) {
		throw EFailure(_T("Map is not a 2D grid-based map!"));
	}

	int attribList[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 0, 0};
	this->canvas = new MapCanvas(this, map, tileset, attribList, mapObjectVector);

	wxToolBar *tb = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
	tb->SetToolBitmapSize(wxSize(16, 16));

	tb->AddTool(wxID_ZOOM_OUT, wxEmptyString,
		wxImage(::path.guiIcons + _T("zoom_1-1.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _T("Zoom 1:1 (small)"),
		_T("Set the zoom level so that one screen pixel is used for each tile pixel"));

	tb->AddTool(wxID_ZOOM_100, wxEmptyString,
		wxImage(::path.guiIcons + _T("zoom_2-1.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _T("Zoom 2:1 (normal)"),
		_T("Set the zoom level so that two screen pixels are used for each tile pixel"));
	tb->ToggleTool(wxID_ZOOM_100, true);

	tb->AddTool(wxID_ZOOM_IN, wxEmptyString,
		wxImage(::path.guiIcons + _T("zoom_3-1.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _T("Zoom 3:1 (big)"),
		_T("Set the zoom level so that three screen pixels are used for each tile pixel"));

	tb->AddSeparator();

	tb->AddTool(IDC_TOGGLEGRID, wxEmptyString,
		wxImage(::path.guiIcons + _T("grid.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_CHECK, _T("Show grid?"),
		_T("Draw a grid to show where each tile is?"));

	tb->AddSeparator();

	tb->AddTool(IDC_MODE_TILE, wxEmptyString,
		wxImage(::path.guiIcons + _T("mode-tile.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _T("Tile mode"),
		_T("Edit the map as a grid of tiles"));

	tb->AddTool(IDC_MODE_OBJ, wxEmptyString,
		wxImage(::path.guiIcons + _T("mode-obj.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _T("Object mode"),
		_T("Edit the map as collection of objects"));

	// Object-mode is the default for the canvas
	tb->ToggleTool(IDC_MODE_OBJ, true);

	tb->Realize();

	wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
	s->Add(tb, 0, wxEXPAND);
	s->Add(this->canvas, 1, wxEXPAND);
	this->SetSizer(s);
}

void MapDocument::save()
	throw (std::ios::failure)
{
	try {
		this->mapFile->seekp(0, std::ios::beg);
		unsigned long len = this->mapType->write(this->map, this->mapFile, this->suppData);
		camoto::flush(this->mapFile);

		// Cut anything off the end since we're overwriting an existing file
		this->fnTrunc(len);

	} catch (const std::ios::failure& e) {
		throw e;
	} catch (const std::exception& e) {
		throw std::ios::failure(e.what());
	}
	return;
}

void MapDocument::onZoomSmall(wxCommandEvent& ev)
{
	this->canvas->setZoomFactor(1);
	return;
}

void MapDocument::onZoomNormal(wxCommandEvent& ev)
{
	this->canvas->setZoomFactor(2);
	return;
}

void MapDocument::onZoomLarge(wxCommandEvent& ev)
{
	this->canvas->setZoomFactor(4);
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

void MapDocument::setStatusText(const wxString& text)
{
	this->frame->setStatusText(text);
	return;
}
