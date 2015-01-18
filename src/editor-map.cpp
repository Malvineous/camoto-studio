/**
 * @file   editor-map.cpp
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

#include <GL/glew.h>
#include <camoto/gamegraphics.hpp>
#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/listctrl.h>
#include "exceptions.hpp"
#include "editor-map.hpp"
#include "editor-map-document.hpp"
#include "editor-map-canvas.hpp"
#include "editor-map-tilepanel.hpp"

/// Default zoom level
#define CFG_DEFAULT_ZOOM 2

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

class LayerPanel: public IToolPanel
{
	public:
		LayerPanel(Studio *parent, TilePanel *tilePanel)
			:	IToolPanel(parent),
				tilePanel(tilePanel)
		{
			this->list = new wxListCtrl(this, IDC_LAYER, wxDefaultPosition,
				wxDefaultSize, wxLC_REPORT | wxBORDER_NONE |
				wxLC_SINGLE_SEL);
			this->list->Connect(wxID_ANY, wxEVT_SET_FOCUS,
				wxFocusEventHandler(LayerPanel::onFocus), NULL, this);

			wxImageList *il = new wxImageList(16, 16, true, 3);
			il->Add(wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_OTHER, wxSize(16, 16)));
			il->Add(wxArtProvider::GetBitmap(wxART_CROSS_MARK, wxART_OTHER, wxSize(16, 16)));
			this->list->AssignImageList(il, wxIMAGE_LIST_SMALL);

			wxListItem info;
			info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT;
			info.m_format = 0;
			info.m_text = _("A");
			this->list->InsertColumn(0, info);

			// inherit: info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT;
			// inherit: info.m_format = 0;
			info.m_text = _("V");
			this->list->InsertColumn(1, info);

			// inherit: info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT;
			// inherit: info.m_format = 0;
			info.m_text = _("Layer");
			this->list->InsertColumn(2, info);

			// inherit: info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT;
			info.m_format = wxLIST_FORMAT_CENTRE;
			info.m_text = _("Size");
			this->list->InsertColumn(3, info);

			wxSizer *s = new wxBoxSizer(wxVERTICAL);
			s->Add(this->list, 1, wxEXPAND);
			this->SetSizer(s);

			// Defaults, unless overridden by loadSettings()
			for (int i = 0; i < MapCanvas::ElementCount; i++) {
				this->visibleElements[i] = true;
			}
		}

		virtual void getPanelInfo(wxString *id, wxString *label) const
		{
			*id = _T("map.layer");
			*label = _("Layers");
			return;
		}

		virtual void switchDocument(IDocument *doc)
		{
			this->list->DeleteAllItems();

			this->doc = static_cast<MapDocument *>(doc);
			if (!this->doc) return; // NULL was passed in

			// Populate the list
			unsigned int layerCount = this->doc->map->getLayerCount();
			wxListItem item;
			for (unsigned int i = 0; i < layerCount; i++) {
				Map2D::LayerPtr layer = this->doc->map->getLayer(i);

				long id = this->list->InsertItem(i,
					this->doc->canvas->activeLayers[i] ? 0 : 1);
				this->list->SetItemColumnImage(id, 1, this->doc->canvas->visibleLayers[i] ? 0 : 1);
				this->list->SetItem(id, 2, wxString(layer->getTitle().c_str(), wxConvUTF8));

				// Calculate layer size
				unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
				getLayerDims(this->doc->map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);
				this->list->SetItem(id, 3, wxString::Format(_T("%d x %d"), layerWidth, layerHeight));

				this->list->SetItemData(id, MapCanvas::ElementCount + i);
			}

			int mapCaps = this->doc->map->getCaps();
			// Add an element (fake) layer for any paths to be drawn in
			if (mapCaps & Map2D::HasPaths) {
				// Use the saved state
				this->doc->canvas->visibleElements[MapCanvas::ElPaths] =
					this->visibleElements[MapCanvas::ElPaths];

				long id = this->list->InsertItem(layerCount,
					(this->doc->canvas->activeElement == MapCanvas::ElPaths) ? 0 : 1);

				this->list->SetItemColumnImage(id, 1,
					this->doc->canvas->visibleElements[MapCanvas::ElPaths] ? 0 : 1);

				this->list->SetItem(id, 2, _("Paths"));
				this->list->SetItem(id, 3, _T("-"));
				this->list->SetItemData(id, MapCanvas::ElPaths);
			}

			// Add an element (fake) layer for the viewport box
			if (mapCaps & Map2D::HasViewport) {
				// Use the saved state
				this->doc->canvas->visibleElements[MapCanvas::ElViewport] =
					this->visibleElements[MapCanvas::ElViewport];

				long id = this->list->InsertItem(layerCount, _T("-"));
				this->list->SetItemColumnImage(id, 1,
					this->doc->canvas->visibleElements[MapCanvas::ElViewport] ? 0 : 1);
				this->list->SetItem(id, 2, _("Viewport"));
				unsigned int vpWidth, vpHeight;
				this->doc->map->getViewport(&vpWidth, &vpHeight);
				this->list->SetItem(id, 3, wxString::Format(_T("%d x %d (px)"), vpWidth, vpHeight));
				this->list->SetItemData(id, MapCanvas::ElViewport);
			}

			// Resize to fit new content
			this->list->SetColumnWidth(0, wxLIST_AUTOSIZE);
			this->list->SetColumnWidth(1, wxLIST_AUTOSIZE);
			this->list->SetColumnWidth(2, wxLIST_AUTOSIZE);
			this->list->SetColumnWidth(3, wxLIST_AUTOSIZE);

			return;
		}

		virtual void loadSettings(Project *proj)
		{
			for (int i = 0; i < MapCanvas::ElementCount; i++) {
				proj->config.Read(wxString::Format(_T("editor-map/show-elementlayer%d"), i),
					&this->visibleElements[i], true);
			}
			return;
		}

		virtual void saveSettings(Project *proj) const
		{
			for (int i = 0; i < MapCanvas::ElementCount; i++) {
				proj->config.Write(wxString::Format(_T("editor-map/show-elementlayer%d"), i),
					this->visibleElements[i]);
			}
			return;
		}

		void onItemSelected(wxListEvent& ev)
		{
			if (!this->doc) return;

			int layerIndex = ev.GetData();
			this->doc->canvas->setPrimaryLayer(layerIndex);
			this->tilePanel->notifyLayerChanged();
			return;
		}

		void onItemActivated(wxListEvent& ev)
		{
			if (!this->doc) return;

			int layerIndex = ev.GetData();
			this->doc->canvas->toggleActiveLayer(layerIndex);

			// Update all the icons
			unsigned int layerCount = this->doc->map->getLayerCount();
			for (unsigned int i = 0; i < layerCount; i++) {
				long id = this->list->FindItem(-1, MapCanvas::ElementCount + i);
				if (id < 0) continue;
				this->list->SetItemColumnImage(id, 0,
					this->doc->canvas->activeLayers[i] ? 0 : 1);
			}
			for (unsigned int i = 0; i < MapCanvas::ElementCount; i++) {
				if (i == MapCanvas::ElViewport) continue;
				long id = this->list->FindItem(-1, i);
				if (id < 0) continue;
				this->list->SetItemColumnImage(id, 0, (this->doc->canvas->activeElement == i + 1) ? 0 : 1);
			}

			return;
		}

		void onItemRightClick(wxListEvent& ev)
		{
			if (!this->doc) return;

			long id = ev.GetIndex();
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
			this->list->SetItemColumnImage(id, 1, newState ? 0 : 1);
			this->doc->canvas->redraw();
			return;
		}

		void onFocus(wxFocusEvent& ev)
		{
			// Push the focus back to the map canvas
			this->doc->canvas->SetFocus();
			return;
		}

		void onMouseOver(wxMouseEvent& ev)
		{
			this->doc->setHelpText(wxString(HT_LAYER_ACT) + __ + HT_LAYER_VIS);
			return;
		}

	protected:
		TilePanel *tilePanel;
		wxListCtrl *list;
		MapDocument *doc;

		bool visibleElements[MapCanvas::ElementCount];  ///< Virtual layers (e.g. viewport)

		enum {
			IDC_LAYER = wxID_HIGHEST + 1,
		};

		DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(LayerPanel, IToolPanel)
	EVT_LIST_ITEM_SELECTED(IDC_LAYER, LayerPanel::onItemSelected)
	EVT_LIST_ITEM_ACTIVATED(IDC_LAYER, LayerPanel::onItemActivated)
	EVT_LIST_ITEM_RIGHT_CLICK(IDC_LAYER, LayerPanel::onItemRightClick)
	EVT_ENTER_WINDOW(LayerPanel::onMouseOver)
END_EVENT_TABLE()


MapEditor::MapEditor(Studio *studio)
	:	studio(studio)
{
	// Default settings
	this->settings.zoomFactor = CFG_DEFAULT_ZOOM;
}

MapEditor::~MapEditor()
{
}

std::vector<IToolPanel *> MapEditor::createToolPanes() const
{
	std::vector<IToolPanel *> panels;
	TilePanel *tilePanel = new TilePanel(this->studio);
	panels.push_back(new LayerPanel(this->studio, tilePanel));
	panels.push_back(tilePanel);
	return panels;
}

void MapEditor::loadSettings(Project *proj)
{
	proj->config.Read(_T("editor-map/zoom"), (int *)&this->settings.zoomFactor, CFG_DEFAULT_ZOOM);
	return;
}

void MapEditor::saveSettings(Project *proj) const
{
	proj->config.Write(_T("editor-map/zoom"), (int)this->settings.zoomFactor);
	return;
}

bool MapEditor::isFormatSupported(const wxString& type) const
{
	std::string strType("map-");
	strType.append(type.ToUTF8());
	return !!this->studio->mgrMaps->getMapTypeByCode(strType);
}

IDocument *MapEditor::openObject(const GameObjectPtr& o)
{
	MapPtr map;
	fn_write fnWriteMap;
	try {
		map = this->studio->openMap(o, &fnWriteMap);
		if (!map) return NULL; // user cancelled
	} catch (const camoto::stream::error& e) {
		throw EFailure(wxString::Format(_("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}

	TilesetCollectionPtr tilesets(new TilesetCollection);

	// See if the map file can supply us with any filenames.
	Map::GraphicsFilenamesPtr gfxFiles = map->getGraphicsFilenames();
	if (gfxFiles) {
		for (Map::GraphicsFilenames::const_iterator
			i = gfxFiles->begin(); i != gfxFiles->end(); i++
		) {
			wxString targetFilename(i->second.filename.c_str(), wxConvUTF8);
			if (IMAGEPURPOSE_IS_TILESET(i->first)) {
				GameObjectPtr ot = this->studio->game->findObjectByFilename(
					targetFilename, _T("tileset"));
				if (!ot) {
					throw EFailure(wxString::Format(_("Cannot open this map.  It needs "
						"a tileset that is missing from the game description XML file."
						"\n\n[There is no <file/> element with the filename \"%s\" and a "
						"typeMajor of \"tileset\", as needed by \"%s\"]"),
						targetFilename.c_str(), o->id.c_str()));
				}
				(*tilesets)[i->first] = this->studio->openTileset(ot);
			}
		}
	}

	// Then, see if any tilesets have been specified in the XML.  This is
	// second so that it's possible to override some or all of the map-supplied
	// filenames.
	try {
		for (unsigned int i = 0; i < DepTypeCount; i++) { // load them in order
			Deps::const_iterator idep = o->dep.find((DepType)i);
			if (idep == o->dep.end()) continue; // no attribute for this dep in the XML

			// Find this object
			GameObjectMap::iterator io = this->studio->game->objects.find(idep->second);
			if (io == this->studio->game->objects.end()) {
				throw EFailure(wxString::Format(_("Cannot open this item.  It refers "
					"to an entry in the game description XML file which doesn't exist!"
					"\n\n[Item \"%s\" has an invalid ID in the dep:%s attribute]"),
					o->id.c_str(), dep2string((DepType)i).c_str()));
			}
			(*tilesets)[(ImagePurpose)i] = this->studio->openTileset(io->second);
		}
	} catch (const EFailure& e) {
		wxString msg = _("Error opening the map tileset:\n\n");
		msg += e.getMessage();
		throw EFailure(msg);
	}

	if (tilesets->empty()) {
		throw EFailure(_("This map format requires at least one tileset to be "
			"specified in the game description XML file, however none have been "
			"given for this item."));
	}

	Map2DPtr map2d = boost::dynamic_pointer_cast<Map2D>(map);
	if (map2d) {
		return new MapDocument(this->studio, &this->settings, map2d, fnWriteMap,
			tilesets, &this->studio->game->mapObjects);
		/// @todo The MapObjects might be different for each map!  Use the right
		/// ones here, since we know the map ID and other identifying info.
		/// Put <for map="map-abc"/> inside the XML to allow multiple maps to share
		/// the same object list?
	}
	throw EFailure(_("Sorry, this map is in a variant for which no editor has "
		"been written yet!"));
}
