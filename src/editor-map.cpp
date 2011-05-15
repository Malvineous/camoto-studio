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
#include <wx/glcanvas.h>
#include "editor-map.hpp"

#ifdef HAVE_GL_GL_H
#include <GL/gl.h>
#else
#ifdef HAVE_OPENGL_GL_H
#include <OpenGL/gl.h>
#endif
#endif

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

enum {
	IDC_LAYER = wxID_HIGHEST + 1,
};

// Must not overlap with normal layer IDs (0+)
#define LAYER_ID_VIEWPORT -1

class LayerPanel;

class MapCanvas: public wxGLCanvas
{
	public:
		std::vector<bool> visibleLayers;
		bool viewportVisible;

		MapCanvas(wxWindow *parent, Map2DPtr map, TilesetPtr tileset, int *attribList)
			throw () :
				wxGLCanvas(parent, wxID_ANY, wxDefaultPosition,
					wxDefaultSize, 0, wxEmptyString, attribList),
				map(map),
				tileset(tileset)
		{
			// Initial state is all layers visible
			int layerCount = map->getLayerCount();
			for (int i = 0; i < layerCount; i++) {
				this->visibleLayers.push_back(true);
			}
			this->viewportVisible = this->map->getCaps() & Map2D::HasViewport;

			this->SetCurrent();
			glClearColor(0.5, 0.5, 0.5, 1);
			glShadeModel(GL_FLAT);
			glEnable(GL_TEXTURE_2D);
			glDisable(GL_DEPTH_TEST);

			const Tileset::VC_ENTRYPTR& tiles = tileset->getItems();
			this->textureCount = tiles.size();
			this->texture = new GLuint[this->textureCount];
			glGenTextures(this->textureCount, this->texture);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			GLushort r[256], g[256], b[256], a[256];;
			if (tileset->getCaps() & Tileset::HasPalette) {
				PaletteTablePtr pal = tileset->getPalette();
				for (int i = 0; i < 256; i++) {
					r[i] = ((*pal)[i].red << 8) | (*pal)[i].red;
					g[i] = ((*pal)[i].green << 8) | (*pal)[i].green;
					b[i] = ((*pal)[i].blue << 8) | (*pal)[i].blue;
					a[i] = (GLushort)-1;
				}
			} else {
				// Default palette
				for (int i = 0; i < 256; i++) {
					r[i] = (i & 4) ? ((i & 8) ? 0xFFFF : 0xAAAA) : ((i & 8) ? 0x5555 : 0x0000);
					g[i] = (i & 2) ? ((i & 8) ? 0xFFFF : 0xAAAA) : ((i & 8) ? 0x5555 : 0x0000);
					b[i] = (i & 1) ? ((i & 8) ? 0xFFFF : 0xAAAA) : ((i & 8) ? 0x5555 : 0x0000);
					a[i] = (GLushort)-1;
				}
			}
			glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_R, 256, r);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_G, 256, g);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_B, 256, b);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_A, 256, a);

			GLuint *t = this->texture;
			for (Tileset::VC_ENTRYPTR::const_iterator i = tiles.begin();
				i != tiles.end(); i++, t++)
			{
				// Bind each texture in turn to the 2D target
				glBindTexture(GL_TEXTURE_2D, *t);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

				// Load an image into the 2D target, which will affect the texture
				// previously bound to it.
				ImagePtr image = tileset->openImage(*i);
				StdImageDataPtr data = image->toStandard();
				unsigned int width, height;
				image->getDimensions(&width, &height);
				if ((width > 512) || (height > 512)) continue; // image too large
				// TODO: If this image has a palette, load it
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, data.get());
				if (glGetError()) std::cerr << "[editor-tileset] GL error loading texture into id " << *t << std::endl;
			}
			glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

			this->glReset();
		}

		~MapCanvas()
			throw ()
		{
			glDeleteTextures(this->textureCount, this->texture);
			delete[] this->texture;
		}


		void onEraseBG(wxEraseEvent& ev)
		{
			return;
		}

		void onPaint(wxPaintEvent& ev)
		{
			wxPaintDC dummy(this);
			this->redraw();
			return;
		}

		void onResize(wxSizeEvent& ev)
		{
			this->Layout();
			this->glReset();
			return;
		}

		void glReset()
		{
			this->SetCurrent();
			wxSize s = this->GetClientSize();
			glViewport(0, 0, s.x, s.y);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, s.x / 2, s.y / 2, 0, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			return;
		}

		/// Redraw the document.  Used after toggling layers.
		void redraw()
			throw ()
		{
			wxSize s = this->GetClientSize();
			this->SetCurrent();
			glClearColor(0.5, 0.5, 0.5, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			glEnable(GL_TEXTURE_2D);

			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

			int layerCount = map->getLayerCount();
			for (int i = 0; i < layerCount; i++) {
				if (this->visibleLayers[i]) {
					Map2D::LayerPtr layer = this->map->getLayer(i);

					int layerWidth, layerHeight;
					if (layer->getCaps() & Map2D::Layer::HasOwnSize) {
						layer->getLayerSize(&layerWidth, &layerHeight);
					} else {
						// Use global map size
						if (this->map->getCaps() & Map2D::HasGlobalSize) {
							this->map->getMapSize(&layerWidth, &layerHeight);
						} else {
							std::cout << "[editor-map] BUG: Layer uses map size, but map "
								"doesn't have a global size!\n";
							continue;
						}
					}

					int tileWidth, tileHeight;
					if (layer->getCaps() & Map2D::Layer::HasOwnTileSize) {
						layer->getTileSize(&tileWidth, &tileHeight);
					} else {
						// Use global tile size
						if (this->map->getCaps() & Map2D::HasGlobalTileSize) {
							this->map->getTileSize(&tileWidth, &tileHeight);
						} else {
							std::cout << "[editor-map] BUG: Layer uses map's tile size, but "
								"map doesn't have a global tile size!\n";
							continue;
						}
					}

					Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
					for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {

						glBindTexture(GL_TEXTURE_2D, this->texture[(*c)->code]);
						glBegin(GL_QUADS);
						glTexCoord2d(0.0, 0.0);
						glVertex2i( (*c)->x      * tileWidth,  (*c)->y      * tileHeight);
						glTexCoord2d(0.0, 1.0);
						glVertex2i( (*c)->x      * tileWidth, ((*c)->y + 1) * tileHeight);
						glTexCoord2d(1.0, 1.0);
						glVertex2i(((*c)->x + 1) * tileWidth, ((*c)->y + 1) * tileHeight);
						glTexCoord2d(1.0, 0.0);
						glVertex2i(((*c)->x + 1) * tileWidth,  (*c)->y      * tileHeight);
						glEnd();
					}
				}
			}

			glDisable(GL_TEXTURE_2D);

			if (this->viewportVisible) {
				int vpX, vpY;
				this->map->getViewport(&vpX, &vpY);
				glColor3f(0.0, 1.0, 0.0);
				glBegin(GL_LINE_LOOP);
				glVertex2i(0, 0);
				glVertex2i(0, vpY);
				glVertex2i(vpX, vpY);
				glVertex2i(vpX, 0);
				glEnd();
			}

			this->SwapBuffers();
			return;
		}

	protected:
		Map2DPtr map;
		TilesetPtr tileset;

		int textureCount;
		GLuint *texture;

		DECLARE_EVENT_TABLE();

};

BEGIN_EVENT_TABLE(MapCanvas, wxGLCanvas)
	EVT_PAINT(MapCanvas::onPaint)
	EVT_ERASE_BACKGROUND(MapCanvas::onEraseBG)
	EVT_SIZE(MapCanvas::onResize)
END_EVENT_TABLE()

class MapDocument: public IDocument
{
	public:
		MapDocument(wxWindow *parent, Map2DPtr map, TilesetPtr tileset)
			throw () :
				IDocument(parent, _T("map")),
				map(map),
				tileset(tileset)
		{
			int attribList[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 0, 0};
			this->canvas = new MapCanvas(this, map, tileset, attribList);

			wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
			s->Add(this->canvas, 1, wxEXPAND);
			this->SetSizer(s);
		}

	protected:
		MapCanvas *canvas;
		Map2DPtr map;
		TilesetPtr tileset;

		friend class LayerPanel;

};


class LayerPanel: public IToolPanel
{
	public:
		LayerPanel(wxWindow *parent)
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

		DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(LayerPanel, IToolPanel)
	EVT_LIST_ITEM_SELECTED(IDC_LAYER, LayerPanel::onItemClick)
	EVT_LIST_ITEM_RIGHT_CLICK(IDC_LAYER, LayerPanel::onItemRightClick)
END_EVENT_TABLE()


MapEditor::MapEditor(wxWindow *parent)
	throw () :
		parent(parent)
{
}

std::vector<IToolPanel *> MapEditor::createToolPanes() const
	throw ()
{
	std::vector<IToolPanel *> panels;
	panels.push_back(new LayerPanel(this->parent));
	return panels;
}

IDocument *MapEditor::openObject(const wxString& typeMinor,
	camoto::iostream_sptr data, const wxString& filename, SuppMap supp) const
	throw ()
{
	camoto::gamemaps::ManagerPtr pManager = camoto::gamemaps::getManager();
	MapTypePtr pMapType;
	if (typeMinor.IsEmpty()) {
		wxMessageDialog dlg(this->parent,
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
			wxMessageDialog dlg(this->parent, msg, _T("Open item"), wxOK | wxICON_ERROR);
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
		wxMessageDialog dlg(this->parent, msg, _T("Open item"), wxYES_NO | wxICON_ERROR);
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

	s = supp.find(_T("tiles"));
	if (s == supp.end()) {
		wxMessageDialog dlg(this->parent, _T("Map files require a tileset to be "
			"specified in the game description .xml file, however none has been "
			"given for this file."), _T("Open item"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return NULL;
	}
	std::string strGfxType("tls-");
	strGfxType.append(s->second.typeMinor.ToUTF8());

	camoto::gamegraphics::ManagerPtr gfxMgr = camoto::gamegraphics::getManager();
	TilesetTypePtr ttp = gfxMgr->getTilesetTypeByCode(strGfxType);
	if (!ttp) {
		wxString wxtype(strGfxType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_T("Sorry, it is not possible to edit this "
			"map as the \"%s\" tileset format is unsupported.  (No handler for \"%s\")"),
			s->second.typeMinor.c_str(), wxtype.c_str());
		wxMessageDialog dlg(this->parent, msg, _T("Open item"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return NULL;
	}

	TilesetPtr tileset = ttp->open(s->second.stream, NULL, suppData);
	if (!tileset) {
		wxString wxtype(strGfxType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_T("Tileset was rejected by \"%s\" handler"
			" (wrong format?)"), wxtype.c_str());
		wxMessageDialog dlg(this->parent, msg, _T("Open item"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return NULL;
	}

	// Open the map file
	MapPtr pMap(pMapType->open(data, suppData));
	assert(pMap);

	Map2DPtr map2d = boost::dynamic_pointer_cast<Map2D>(pMap);
	if (map2d) {
		return new MapDocument(this->parent, map2d, tileset);
	}

	wxMessageDialog dlg(this->parent, _T("Sorry, this map is in a variant for "
		"which no editor has been written yet!"), _T("Open item"),
		wxOK | wxICON_ERROR);
	dlg.ShowModal();
	return NULL;
}
