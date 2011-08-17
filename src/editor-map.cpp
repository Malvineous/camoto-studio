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

#include <camoto/gamemaps.hpp>
#include <camoto/gamegraphics.hpp>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include "editor-map.hpp"
#include "editor-map-document.hpp"
#include "editor-map-canvas.hpp"

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

bool tryLoadTileset(wxWindow *parent, SuppData& suppData, SuppMap& suppMap,
	const wxString& id, VC_TILESET *tilesetVector);

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
				// Calculate layer size
				int layerWidth, layerHeight;
				Map2D::LayerPtr layer = this->doc->map->getLayer(i);
				if (layer->getCaps() & Map2D::Layer::HasOwnSize) {
					layer->getLayerSize(&layerWidth, &layerHeight);
				} else {
					// Use global map size
					if (this->doc->map->getCaps() & Map2D::HasGlobalSize) {
						this->doc->map->getMapSize(&layerWidth, &layerHeight);
					} else {
						std::cout << "[editor-map] BUG: Layer uses map size, but map "
							"doesn't have a global size!\n";
						layerWidth = layerHeight = 0;
					}
				}
				long id = this->list->InsertItem(i,
					wxString(layer->getTitle().c_str(), wxConvUTF8),
					this->doc->canvas->visibleLayers[i] ? 0 : 1);
				this->list->SetItem(id, 1, wxString::Format(_T("%d x %d"), layerWidth, layerHeight));
				this->list->SetItemData(id, i);
			}

			// Add a fake layer for the viewport box
			if (this->doc->map->getCaps() & Map2D::HasViewport) {
				long id = this->list->InsertItem(layerCount, _T("Viewport"),
					this->doc->canvas->viewportVisible ? 0 : 1);
				int vpWidth, vpHeight;
				this->doc->map->getViewport(&vpWidth, &vpHeight);
				this->list->SetItem(id, 1, wxString::Format(_T("%d x %d (px)"), vpWidth, vpHeight));
				this->list->SetItemData(id, LAYER_ID_VIEWPORT);
			}

			this->list->SetColumnWidth(0, wxLIST_AUTOSIZE);
			this->list->SetColumnWidth(1, wxLIST_AUTOSIZE);
			return;
		}

		virtual void loadSettings(Project *proj)
			throw ()
		{
			return;
		}

		virtual void saveSettings(Project *proj) const
			throw ()
		{
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
			if (layerIndex == LAYER_ID_VIEWPORT) {
				newState = this->doc->canvas->viewportVisible ^= 1;
			} else {
				newState = !this->doc->canvas->visibleLayers[layerIndex]; // ^=1 doesn't work :-(
				this->doc->canvas->visibleLayers[layerIndex] = newState;
			}
			this->list->SetItemImage(ev.GetIndex(), newState ? 0 : 1);
			this->doc->canvas->redraw();
			return;
		}

	protected:
		wxListCtrl *list;
		MapDocument *doc;

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
		frame(parent)
{
}

std::vector<IToolPanel *> MapEditor::createToolPanes() const
	throw ()
{
	std::vector<IToolPanel *> panels;
	panels.push_back(new LayerPanel(this->frame));
	return panels;
}

IDocument *MapEditor::openObject(const wxString& typeMinor,
	camoto::iostream_sptr data, const wxString& filename, SuppMap supp,
	const Game *game) const
	throw ()
{
	camoto::gamemaps::ManagerPtr pManager = camoto::gamemaps::getManager();
	MapTypePtr pMapType;
	if (typeMinor.IsEmpty()) {
		wxMessageDialog dlg(this->frame,
			_T("No file type was specified for this item!"), _T("Open item"),
			wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return NULL;
	} else {
		std::string strType("map-");
		strType.append(typeMinor.ToUTF8());
		MapTypePtr pTestType(pManager->getMapTypeByCode(strType));
		if (!pTestType) {
			wxString wxtype(strType.c_str(), wxConvUTF8);
			wxString msg = wxString::Format(_T("Sorry, it is not possible to edit this "
				"map as the \"%s\" format is unsupported.  (No handler for \"%s\")"),
				typeMinor.c_str(), wxtype.c_str());
			wxMessageDialog dlg(this->frame, msg, _T("Open item"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return NULL;
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
	SuppMap::iterator s;

	s = supp.find(_T("dict"));
	if (s != supp.end()) suppData[SuppItem::Dictionary].stream = s->second.stream;

	s = supp.find(_T("fat"));
	if (s != supp.end()) suppData[SuppItem::FAT].stream = s->second.stream;

	s = supp.find(_T("pal"));
	if (s != supp.end()) suppData[SuppItem::Palette].stream = s->second.stream;

	s = supp.find(_T("layer1"));
	if (s != supp.end()) suppData[SuppItem::Layer1].stream = s->second.stream;

	s = supp.find(_T("layer2"));
	if (s != supp.end()) suppData[SuppItem::Layer2].stream = s->second.stream;

	VC_TILESET tilesetVector;
	if (!tryLoadTileset(this->frame, suppData, supp, _T("tiles"), &tilesetVector)) return NULL;
	if (!tryLoadTileset(this->frame, suppData, supp, _T("tiles2"), &tilesetVector)) return NULL;
	if (!tryLoadTileset(this->frame, suppData, supp, _T("tiles3"), &tilesetVector)) return NULL;
	if (!tryLoadTileset(this->frame, suppData, supp, _T("sprites"), &tilesetVector)) return NULL;

	if (tilesetVector.empty()) {
		wxMessageDialog dlg(this->frame, _T("Map files require a tileset to be "
			"specified in the game description .xml file, however none has been "
			"given for this file."), _T("Open item"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return NULL;
	}

	// Open the map file
	MapPtr pMap(pMapType->open(data, suppData));
	assert(pMap);

	Map2DPtr map2d = boost::dynamic_pointer_cast<Map2D>(pMap);
	if (map2d) {
		return new MapDocument(this->frame, map2d, tilesetVector, &game->mapObjects);
	}

	wxMessageDialog dlg(this->frame, _T("Sorry, this map is in a variant for "
		"which no editor has been written yet!"), _T("Open item"),
		wxOK | wxICON_ERROR);
	dlg.ShowModal();
	return NULL;
}

bool tryLoadTileset(wxWindow *parent, SuppData& suppData, SuppMap& supp, const wxString& id, VC_TILESET *tilesetVector)
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
		wxString msg = wxString::Format(_T("Sorry, it is not possible to edit this "
			"map as the \"%s\" tileset format is unsupported.  (No handler for \"%s\")"),
			s->second.typeMinor.c_str(), wxtype.c_str());
		wxMessageDialog dlg(parent, msg, _T("Open item"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return false;
	}

	TilesetPtr tileset = ttp->open(s->second.stream, NULL, suppData);
	if (!tileset) {
		wxString wxtype(strGfxType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_T("Tileset was rejected by \"%s\" handler"
			" (wrong format?)"), wxtype.c_str());
		wxMessageDialog dlg(parent, msg, _T("Open item"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return false;
	}

	tilesetVector->push_back(tileset);
	return true;
}
