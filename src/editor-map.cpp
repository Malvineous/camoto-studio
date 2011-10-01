/**
 * @file   editor-map.cpp
 * @brief  Map editor.
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

#include <camoto/gamegraphics.hpp>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include "efailure.hpp"
#include "editor-map.hpp"
#include "editor-map-document.hpp"
#include "editor-map-canvas.hpp"

/// Default zoom level
#define CFG_DEFAULT_ZOOM 2

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

bool tryLoadTileset(wxWindow *parent, SuppData& suppData, SuppMap& suppMap,
	const wxString& id, VC_TILESET *tilesetVector)
	throw (EFailure);

// Must not overlap with normal layer IDs (0+)
#define LAYER_ID_VIEWPORT -1

class LayerPanel: public IToolPanel
{
	public:
		LayerPanel(IMainWindow *parent)
			throw () :
				IToolPanel(parent)
		{
			this->list = new wxListCtrl(this, IDC_LAYER, wxDefaultPosition,
				wxDefaultSize, wxLC_REPORT | wxBORDER_NONE | wxLC_NO_HEADER |
				wxLC_SINGLE_SEL);

			wxImageList *il = new wxImageList(16, 16, true, 3);
			il->Add(wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_OTHER, wxSize(16, 16)));
			il->Add(wxArtProvider::GetBitmap(wxART_CROSS_MARK, wxART_OTHER, wxSize(16, 16)));
			this->list->AssignImageList(il, wxIMAGE_LIST_SMALL);

			wxListItem info;
			info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE | wxLIST_MASK_FORMAT;
			info.m_image = 0;
			info.m_format = 0;
			info.m_text = _T("Layer");
			this->list->InsertColumn(0, info);

			info.m_format = wxLIST_FORMAT_CENTRE;
			info.m_text = _T("Size");
			this->list->InsertColumn(1, info);

			wxSizer *s = new wxBoxSizer(wxVERTICAL);
			s->Add(this->list, 1, wxEXPAND);
			this->SetSizer(s);

			// Defaults, unless overridden by loadSettings()
			for (int i = 0; i < MapCanvas::ElementCount; i++) {
				this->visibleElements[i] = true;
			}
		}

		virtual void getPanelInfo(wxString *id, wxString *label) const
			throw ()
		{
			*id = _T("map.layer");
			*label = _T("Layers");
			return;
		}

		virtual void switchDocument(IDocument *doc)
			throw ()
		{
			this->list->DeleteAllItems();

			this->doc = static_cast<MapDocument *>(doc);
			if (!this->doc) return; // NULL was passed in

			// Populate the list
			int layerCount = this->doc->map->getLayerCount();
			for (int i = 0; i < layerCount; i++) {
				Map2D::LayerPtr layer = this->doc->map->getLayer(i);

				// Calculate layer size
				int layerWidth, layerHeight, tileWidth, tileHeight;
				getLayerDims(this->doc->map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);

				long id = this->list->InsertItem(i,
					wxString(layer->getTitle().c_str(), wxConvUTF8),
					this->doc->canvas->visibleLayers[i] ? 0 : 1);
				this->list->SetItem(id, 1, wxString::Format(_T("%d x %d"), layerWidth, layerHeight));
				this->list->SetItemData(id, MapCanvas::ElementCount + i);
			}

			int mapCaps = this->doc->map->getCaps();
			// Add an element (fake) layer for any paths to be drawn in
			if (mapCaps & Map2D::HasPaths) {
				// Use the saved state
				this->doc->canvas->visibleElements[MapCanvas::ElPaths] =
					this->visibleElements[MapCanvas::ElPaths];

				long id = this->list->InsertItem(layerCount, _T("Paths"),
					this->doc->canvas->visibleElements[MapCanvas::ElPaths] ? 0 : 1);
				this->list->SetItem(id, 1, _T("-"));
				this->list->SetItemData(id, MapCanvas::ElPaths);
			}

			// Add an element (fake) layer for the viewport box
			if (mapCaps & Map2D::HasViewport) {
				// Use the saved state
				this->doc->canvas->visibleElements[MapCanvas::ElViewport] =
					this->visibleElements[MapCanvas::ElViewport];

				long id = this->list->InsertItem(layerCount, _T("Viewport"),
					this->doc->canvas->visibleElements[MapCanvas::ElViewport] ? 0 : 1);
				int vpWidth, vpHeight;
				this->doc->map->getViewport(&vpWidth, &vpHeight);
				this->list->SetItem(id, 1, wxString::Format(_T("%d x %d (px)"), vpWidth, vpHeight));
				this->list->SetItemData(id, MapCanvas::ElViewport);
			}

			this->list->SetColumnWidth(0, wxLIST_AUTOSIZE);
			this->list->SetColumnWidth(1, wxLIST_AUTOSIZE);

			this->list->Connect(wxID_ANY, wxEVT_SET_FOCUS,
				wxFocusEventHandler(LayerPanel::onFocus), NULL, this);
			return;
		}

		virtual void loadSettings(Project *proj)
			throw ()
		{
			for (int i = 0; i < MapCanvas::ElementCount; i++) {
				proj->config.Read(wxString::Format(_T("editor-map/show-elementlayer%d"), i),
					&this->visibleElements[i], true);
			}
			return;
		}

		virtual void saveSettings(Project *proj) const
			throw ()
		{
			for (int i = 0; i < MapCanvas::ElementCount; i++) {
				proj->config.Write(wxString::Format(_T("editor-map/show-elementlayer%d"), i),
					this->visibleElements[i]);
			}
			return;
		}

		void onItemClick(wxListEvent& ev)
		{
			if (!this->doc) return;
			this->doc->canvas->setActiveLayer(ev.GetData());
			return;
		}

		void onItemRightClick(wxListEvent& ev)
		{
			if (!this->doc) return;

			int layerIndex = ev.GetData();
			bool newState;
			if (layerIndex < MapCanvas::ElementCount) {
				newState = this->doc->canvas->visibleElements[layerIndex] ^= 1;
				this->visibleElements[layerIndex] = newState;
			} else {
				layerIndex -= MapCanvas::ElementCount;
				newState = !this->doc->canvas->visibleLayers[layerIndex]; // ^=1 doesn't work :-(
				this->doc->canvas->visibleLayers[layerIndex] = newState;
			}
			this->list->SetItemImage(ev.GetIndex(), newState ? 0 : 1);
			this->doc->canvas->redraw();
			return;
		}

		void onFocus(wxFocusEvent& ev)
		{
			// Push the focus back to the map canvas
			this->doc->canvas->SetFocus();
			return;
		}

	protected:
		wxListCtrl *list;
		MapDocument *doc;

		bool visibleElements[MapCanvas::ElementCount];  ///< Virtual layers (e.g. viewport)

		enum {
			IDC_LAYER = wxID_HIGHEST + 1,
		};

		DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(LayerPanel, IToolPanel)
	EVT_LIST_ITEM_SELECTED(IDC_LAYER, LayerPanel::onItemClick)
	EVT_LIST_ITEM_RIGHT_CLICK(IDC_LAYER, LayerPanel::onItemRightClick)
END_EVENT_TABLE()


MapEditor::MapEditor(IMainWindow *parent)
	throw () :
		frame(parent),
		pManager(camoto::gamemaps::getManager())
{
	// Default settings
	this->settings.zoomFactor = CFG_DEFAULT_ZOOM;
}

std::vector<IToolPanel *> MapEditor::createToolPanes() const
	throw ()
{
	std::vector<IToolPanel *> panels;
	panels.push_back(new LayerPanel(this->frame));
	return panels;
}

void MapEditor::loadSettings(Project *proj)
	throw ()
{
	proj->config.Read(_T("editor-map/zoom"), (int *)&this->settings.zoomFactor, CFG_DEFAULT_ZOOM);
	return;
}

void MapEditor::saveSettings(Project *proj) const
	throw ()
{
	proj->config.Write(_T("editor-map/zoom"), (int)this->settings.zoomFactor);
	return;
}

bool MapEditor::isFormatSupported(const wxString& type) const
	throw ()
{
	std::string strType("map-");
	strType.append(type.ToUTF8());
	return this->pManager->getMapTypeByCode(strType);
}

IDocument *MapEditor::openObject(const wxString& typeMinor,
	camoto::iostream_sptr data, FN_TRUNCATE fnTrunc, const wxString& filename,
	SuppMap supp, const Game *game)
	throw (EFailure)
{
	assert(fnTrunc);

	MapTypePtr pMapType;
	if (typeMinor.IsEmpty()) {
		throw EFailure(_T("No file type was specified for this item!"));
	} else {
		std::string strType("map-");
		strType.append(typeMinor.ToUTF8());
		MapTypePtr pTestType(this->pManager->getMapTypeByCode(strType));
		if (!pTestType) {
			wxString wxtype(strType.c_str(), wxConvUTF8);
			throw EFailure(wxString::Format(_T("Sorry, it is not possible to edit this "
				"map as the \"%s\" format is unsupported.  (No handler for \"%s\")"),
				typeMinor.c_str(), wxtype.c_str()));
		}
		pMapType = pTestType;
	}

	assert(pMapType);
	std::cout << "[editor-map] Using handler for " << pMapType->getFriendlyName() << "\n";

	// Check to see if the file is actually in this format
	if (pMapType->isInstance(data) < MapType::PossiblyYes) {
		std::string friendlyType = pMapType->getFriendlyName();
		wxString wxtype(friendlyType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_T("This file is supposed to be in \"%s\" "
			"format, but it seems this may not be the case.  Would you like to try "
			"opening it anyway?"), wxtype.c_str());
		wxMessageDialog dlg(this->frame, msg, _T("Open item"), wxYES_NO | wxICON_ERROR);
		int r = dlg.ShowModal();
		if (r != wxID_YES) return NULL;
	}

	// Collect any supplemental files supplied
	SuppData suppData;
	suppMapToData(supp, suppData);

	VC_TILESET tilesetVector;
	try {
		if (!tryLoadTileset(this->frame, suppData, supp, _T("tiles"), &tilesetVector)) return NULL;
		if (!tryLoadTileset(this->frame, suppData, supp, _T("tiles2"), &tilesetVector)) return NULL;
		if (!tryLoadTileset(this->frame, suppData, supp, _T("tiles3"), &tilesetVector)) return NULL;
		if (!tryLoadTileset(this->frame, suppData, supp, _T("sprites"), &tilesetVector)) return NULL;
	} catch (const EFailure& e) {
		wxString msg = _T("Error opening the map tileset:\n\n");
		msg += e.getMessage();
		throw EFailure(msg);
	}

	if (tilesetVector.empty()) {
		throw EFailure(_T("Map files require a tileset to be "
			"specified in the game description .xml file, however none has been "
			"given for this file."));
	}

	// Open the map file
	try {
		return new MapDocument(this->frame, &this->settings, pMapType, suppData, data, fnTrunc,
			tilesetVector, &game->mapObjects);
	} catch (const std::ios::failure& e) {
		throw EFailure(wxString::Format(_T("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}

	throw EFailure(_T("Sorry, this map is in a variant for which no editor has "
		"been written yet!"));
}

bool tryLoadTileset(wxWindow *parent, SuppData& suppData, SuppMap& supp,
	const wxString& id, VC_TILESET *tilesetVector)
	throw (EFailure)
{
	SuppMap::iterator s;
	s = supp.find(id);
	if (s == supp.end()) return true;

	std::string strGfxType("tls-");
	strGfxType.append(s->second.typeMinor.ToUTF8());

	camoto::gamegraphics::ManagerPtr gfxMgr = camoto::gamegraphics::getManager();
	TilesetTypePtr ttp = gfxMgr->getTilesetTypeByCode(strGfxType);
	if (!ttp) {
		wxString wxtype(strGfxType.c_str(), wxConvUTF8);
		throw EFailure(wxString::Format(_T("Sorry, it is not possible to edit this "
			"map as the \"%s\" tileset format is unsupported.  (No handler for \"%s\")"),
			s->second.typeMinor.c_str(), wxtype.c_str()));
	}

	try {
		TilesetPtr tileset = ttp->open(s->second.stream, NULL, suppData);
		if (!tileset) {
			wxString wxtype(strGfxType.c_str(), wxConvUTF8);
			throw EFailure(wxString::Format(_T("Tileset was rejected by \"%s\" handler"
				" (wrong format?)"), wxtype.c_str()));
		}
		tilesetVector->push_back(tileset);
	} catch (const std::ios::failure& e) {
		throw EFailure(wxString::Format(_T("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}

	return true;
}
