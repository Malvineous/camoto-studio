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

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

BEGIN_EVENT_TABLE(TilePanel, IToolPanel)
	EVT_TOOL(wxID_ZOOM_OUT, TilePanel::onZoomSmall)
	EVT_TOOL(wxID_ZOOM_100, TilePanel::onZoomNormal)
	EVT_TOOL(wxID_ZOOM_IN, TilePanel::onZoomLarge)
	EVT_TOOL(IDC_TOGGLEGRID, TilePanel::onToggleGrid)
	EVT_TOOL(IDC_INC_WIDTH, TilePanel::onIncWidth)
	EVT_TOOL(IDC_DEC_WIDTH, TilePanel::onDecWidth)
	EVT_TOOL(IDC_INC_OFFSET, TilePanel::onIncOffset)
	EVT_TOOL(IDC_DEC_OFFSET, TilePanel::onDecOffset)
END_EVENT_TABLE()

TilePanel::TilePanel(Studio *parent)
	:	IToolPanel(parent),
		doc(NULL),
		tilesX(0),
		offset(0)
{
	this->canvas = new TilePanelCanvas(this, parent->getGLContext(),
		parent->getGLAttributes());

	this->tb = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
	this->tb->SetToolBitmapSize(wxSize(16, 16));

	this->tb->AddTool(wxID_ZOOM_OUT, wxEmptyString,
		wxImage(::path.guiIcons + _T("zoom_1-1.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _("Zoom 1:1 (small)"),
		_("Set the zoom level so that one screen pixel is used for each tile pixel"));

	this->tb->AddTool(wxID_ZOOM_100, wxEmptyString,
		wxImage(::path.guiIcons + _T("zoom_2-1.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _("Zoom 2:1 (normal)"),
		_("Set the zoom level so that two screen pixels are used for each tile pixel"));
	this->tb->ToggleTool(wxID_ZOOM_100, true);

	this->tb->AddTool(wxID_ZOOM_IN, wxEmptyString,
		wxImage(::path.guiIcons + _T("zoom_3-1.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_RADIO, _("Zoom 3:1 (big)"),
		_("Set the zoom level so that three screen pixels are used for each tile pixel"));

	this->tb->AddSeparator();

	this->tb->AddTool(IDC_TOGGLEGRID, wxEmptyString,
		wxImage(::path.guiIcons + _T("grid.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_CHECK, _("Show grid?"),
		_("Draw a grid to show where each tile is?"));

	this->tb->AddSeparator();

	this->tb->AddTool(IDC_INC_WIDTH, wxEmptyString,
		wxImage(::path.guiIcons + _T("inc-width.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Increase width"),
		_T("Draw more tiles horizontally before wrapping to the next row"));

	this->tb->AddTool(IDC_DEC_WIDTH, wxEmptyString,
		wxImage(::path.guiIcons + _T("dec-width.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Decrease width"),
		_T("Draw fewer tiles horizontally before wrapping to the next row"));

	this->tb->AddTool(IDC_INC_OFFSET, wxEmptyString,
		wxImage(::path.guiIcons + _T("inc-offset.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Increase offset"),
		_T("Skip the first tile"));

	this->tb->AddTool(IDC_DEC_OFFSET, wxEmptyString,
		wxImage(::path.guiIcons + _T("dec-offset.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Decrease offset"),
		_T("Return the first tile if it has been skipped"));

	this->tb->Realize();

	wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
	s->Add(tb, 0, wxEXPAND);
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

	// Switch to the new document's textures
	this->canvas->notifyNewDoc();
	return;
}

void TilePanel::loadSettings(Project *proj)
{
	proj->config.Read(_T("editor-map-tilepanel/zoom"),
		&this->settings.zoomFactor, 1);

	// Update the UI
	switch (this->settings.zoomFactor) {
		case 1: this->tb->ToggleTool(wxID_ZOOM_OUT, true); break;
		case 2: this->tb->ToggleTool(wxID_ZOOM_100, true); break;
		case 4: this->tb->ToggleTool(wxID_ZOOM_IN, true); break;
			// default: don't select anything, just in case!
	}
	this->setZoomFactor(this->settings.zoomFactor);
	return;
}

void TilePanel::saveSettings(Project *proj) const
{
	proj->config.Write(_T("editor-map-tilepanel/zoom"),
		this->settings.zoomFactor);
	return;
}

void TilePanel::onZoomSmall(wxCommandEvent& ev)
{
	this->setZoomFactor(1);
	return;
}

void TilePanel::onZoomNormal(wxCommandEvent& ev)
{
	this->setZoomFactor(2);
	return;
}

void TilePanel::onZoomLarge(wxCommandEvent& ev)
{
	this->setZoomFactor(4);
	return;
}

void TilePanel::onToggleGrid(wxCommandEvent& ev)
{
	this->canvas->setGridVisible(ev.IsChecked());
	return;
}

void TilePanel::onIncWidth(wxCommandEvent& ev)
{
	this->tilesX++;
	this->canvas->setTilesX(this->tilesX);
	return;
}

void TilePanel::onDecWidth(wxCommandEvent& ev)
{
	if (this->tilesX > 0) {
		this->tilesX--;
		this->canvas->setTilesX(this->tilesX);
	}
	return;
}

void TilePanel::onIncOffset(wxCommandEvent& ev)
{
	this->offset++;
	this->canvas->setOffset(this->offset);
	return;
}

void TilePanel::onDecOffset(wxCommandEvent& ev)
{
	if (this->offset > 0) {
		this->offset--;
		this->canvas->setOffset(this->offset);
	}
	return;
}

void TilePanel::setZoomFactor(int f)
{
	this->settings.zoomFactor = f;
	this->canvas->setZoomFactor(f);
	return;
}

void TilePanel::notifyLayerChanged()
{
	this->canvas->calcCurrentExtents();
	this->canvas->redraw();
	return;
}
