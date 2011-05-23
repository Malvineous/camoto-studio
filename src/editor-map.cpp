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

#include <map>
#include <boost/shared_array.hpp>
#include <camoto/gamemaps.hpp>
#include <camoto/gamegraphics.hpp>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/glcanvas.h>
#include "main.hpp"
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

		MapCanvas(wxWindow *parent, Map2DPtr map, VC_TILESET tileset, int *attribList)
			throw () :
				wxGLCanvas(parent, wxID_ANY, wxDefaultPosition,
					wxDefaultSize, 0, wxEmptyString, attribList),
				map(map),
				tileset(tileset),
				zoomFactor(2),
				gridVisible(false),
				activeLayer(0)
		{
			assert(tileset.size() > 0);
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

			PaletteTablePtr palDefault;
			if (tileset[0]->getCaps() & Tileset::HasPalette) {
				palDefault = tileset[0]->getPalette();
				assert(palDefault);
			} else {
				// Load default palette
				palDefault = createDefaultCGAPalette();
			}

			// TODO: Load unknown/default tile the same size (if possible) as the grid
			GLuint unknownTile = 0;

			for (int i = 0; i < layerCount; i++) {
				Map2D::LayerPtr layer = this->map->getLayer(i);
				Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
				TEXTURE_MAP tm;
				for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
					GLuint t;
					glGenTextures(1, &t);
					glBindTexture(GL_TEXTURE_2D, t);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					unsigned int code = (*c)->code;
					if (tm.find(code) == tm.end()) {
						// This tile code doesn't have an associated image yet
						ImagePtr image = layer->imageFromCode(code, tileset);
						if (image) {
							// Got an image for this tile code

							unsigned int width, height;
							image->getDimensions(&width, &height);
							if ((width > 512) || (height > 512)) continue; // image too large

							PaletteTablePtr pal;
							if (image->getCaps() & Image::HasPalette) {
								pal = image->getPalette();
							} else {
								pal = palDefault;
							}
							assert(pal);
							StdImageDataPtr data = image->toStandard();
							StdImageDataPtr mask = image->toStandardMask();
							boost::shared_array<uint32_t> combined(new uint32_t[width * height]);
							uint8_t *d = data.get(), *m = mask.get();
							uint8_t *c = (uint8_t *)combined.get();
							for (int p = 0; p < width * height; p++) {
								*c++ = *m & 0x01 ? 255 : (*pal)[*d].blue;
								*c++ = *m & 0x01 ?   0 : (*pal)[*d].green;
								*c++ = *m & 0x01 ? 255 : (*pal)[*d].red;
								*c++ = *m & 0x01 ?   0 : (*pal)[*d].alpha;
								m++; d++;
							}

							glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, combined.get());
							if (glGetError()) std::cerr << "[editor-tileset] GL error loading texture into id " << t << std::endl;
						} else {
							std::cerr << "[editor-map] Could not get an image for tile "
								"code " << code << std::endl;
							glDeleteTextures(1, &t);
							t = unknownTile;
						}
						tm[code] = t;
					}
				}
				this->textureMap.push_back(tm);
			}

			this->glReset();
		}

		~MapCanvas()
			throw ()
		{
			for (std::vector<TEXTURE_MAP>::iterator l = this->textureMap.begin(); l != this->textureMap.end(); l++) {
				for (TEXTURE_MAP::iterator t = l->begin(); t != l->end(); t++) {
					glDeleteTextures(1, &t->second);
				}
			}
		}

		void setZoomFactor(int f)
			throw ()
		{
			this->zoomFactor = f;
			this->glReset();
			this->redraw();
			return;
		}

		void showGrid(bool visible)
			throw ()
		{
			this->gridVisible = visible;
			this->glReset();
			this->redraw();
			return;
		}

		void setActiveLayer(int layer)
		{
			this->activeLayer = layer;
			this->glReset();
			this->redraw();
			return;
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
			glOrtho(0, s.x / this->zoomFactor, s.y / this->zoomFactor, 0, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			return;
		}

		/// Redraw the document.  Used after toggling layers.
		void redraw()
			throw ()
		{
			this->SetCurrent();
			glClearColor(0.5, 0.5, 0.5, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			glEnable(GL_TEXTURE_2D);

			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			int layerCount = this->map->getLayerCount();
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
					this->getLayerTileSize(layer, &tileWidth, &tileHeight);

					Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
					for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {

						glBindTexture(GL_TEXTURE_2D, this->textureMap[i][(*c)->code]);
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

			if ((this->gridVisible) && (this->activeLayer >= 0)) {
				Map2D::LayerPtr layer = this->map->getLayer(this->activeLayer);
				int tileWidth, tileHeight;
				this->getLayerTileSize(layer, &tileWidth, &tileHeight);
				wxSize s = this->GetClientSize();

				glColor4f(0.2, 0.2, 0.2, 0.35);
				glBegin(GL_LINES);
				for (int x = 0; x < s.x; x += tileWidth) {
					glVertex2i(x, 0);
					glVertex2i(x, s.y);
				}
				for (int y = 0; y < s.y; y += tileHeight) {
					glVertex2i(0,   y);
					glVertex2i(s.x, y);
				}
				glEnd();
			}

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

		void getLayerTileSize(Map2D::LayerPtr layer, int *tileWidth, int *tileHeight)
			throw ()
		{
			if (layer->getCaps() & Map2D::Layer::HasOwnTileSize) {
				layer->getTileSize(tileWidth, tileHeight);
			} else {
				// Use global tile size
				if (this->map->getCaps() & Map2D::HasGlobalTileSize) {
					this->map->getTileSize(tileWidth, tileHeight);
				} else {
					std::cout << "[editor-map] BUG: Layer uses map's tile size, but "
						"map doesn't have a global tile size!\n";
					*tileWidth = *tileHeight = 64; // use some really weird value :-)
					return;
				}
			}
			return;
		}

		void onMouseMove(wxMouseEvent& ev)
		{
			int layerCount = this->map->getLayerCount();
			std::cout << ev.m_x << "," << ev.m_y << " == ";
			for (int i = 0; i < layerCount; i++) {
				if (this->visibleLayers[i]) {
					Map2D::LayerPtr layer = this->map->getLayer(i);

					int tileWidth, tileHeight;
					this->getLayerTileSize(layer, &tileWidth, &tileHeight);

					Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
					for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
						int tileX = (*c)->x * tileWidth;
						int tileY = (*c)->y * tileHeight;
						if (
							(tileX <= ev.m_x) && (tileX + tileWidth > ev.m_x) &&
							(tileY <= ev.m_y) && (tileY + tileHeight > ev.m_y)
						) {
							std::cout << "l" << i << ": 0x" << std::hex << (*c)->code << std::dec << " ";
						}
					}
				}
			}
			std::cout << "\n";
			return;
		}

	protected:
		Map2DPtr map;
		VC_TILESET tileset;

		typedef std::map<unsigned int, GLuint> TEXTURE_MAP;
		std::vector<TEXTURE_MAP> textureMap;

		int zoomFactor;   ///< Zoom level (1 == 1:1, 2 == 2:1/doublesize, etc.)
		bool gridVisible; ///< Draw a grid over the active layer?
		int activeLayer;  ///< Index of layer currently selected in layer palette

		DECLARE_EVENT_TABLE();

};

BEGIN_EVENT_TABLE(MapCanvas, wxGLCanvas)
	EVT_PAINT(MapCanvas::onPaint)
	EVT_ERASE_BACKGROUND(MapCanvas::onEraseBG)
	EVT_SIZE(MapCanvas::onResize)
	EVT_MOTION(MapCanvas::onMouseMove)
END_EVENT_TABLE()

class MapDocument: public IDocument
{
	public:
		MapDocument(wxWindow *parent, Map2DPtr map, VC_TILESET tileset)
			throw () :
				IDocument(parent, _T("map")),
				map(map),
				tileset(tileset)
		{
			int attribList[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 0, 0};
			this->canvas = new MapCanvas(this, map, tileset, attribList);

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

			wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
			s->Add(tb, 0, wxEXPAND);
			s->Add(this->canvas, 1, wxEXPAND);
			this->SetSizer(s);
		}

		void onZoomSmall(wxCommandEvent& ev)
		{
			this->canvas->setZoomFactor(1);
			return;
		}

		void onZoomNormal(wxCommandEvent& ev)
		{
			this->canvas->setZoomFactor(2);
			return;
		}

		void onZoomLarge(wxCommandEvent& ev)
		{
			this->canvas->setZoomFactor(4);
			return;
		}

		void onToggleGrid(wxCommandEvent& ev)
		{
			this->canvas->showGrid(ev.IsChecked());
			return;
		}

	protected:
		MapCanvas *canvas;
		Map2DPtr map;
		VC_TILESET tileset;

		friend class LayerPanel;

		enum {
			IDC_TOGGLEGRID = wxID_HIGHEST + 1,
		};
		DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(MapDocument, IDocument)
	EVT_TOOL(wxID_ZOOM_OUT, MapDocument::onZoomSmall)
	EVT_TOOL(wxID_ZOOM_100, MapDocument::onZoomNormal)
	EVT_TOOL(wxID_ZOOM_IN, MapDocument::onZoomLarge)
	EVT_TOOL(IDC_TOGGLEGRID, MapDocument::onToggleGrid)
END_EVENT_TABLE()


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

	VC_TILESET tilesetVector;
	tilesetVector.push_back(tileset);

	// Open the second (masked tile) tileset if one was given
	s = supp.find(_T("sprites"));
	if (s != supp.end()) {
		strGfxType = "tls-";
		strGfxType.append(s->second.typeMinor.ToUTF8());

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

		tileset = ttp->open(s->second.stream, NULL, suppData);
		if (!tileset) {
			wxString wxtype(strGfxType.c_str(), wxConvUTF8);
			wxString msg = wxString::Format(_T("Tileset was rejected by \"%s\" handler"
					" (wrong format?)"), wxtype.c_str());
			wxMessageDialog dlg(this->parent, msg, _T("Open item"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return NULL;
		}
		tilesetVector.push_back(tileset);
	}


	// Open the map file
	MapPtr pMap(pMapType->open(data, suppData));
	assert(pMap);

	Map2DPtr map2d = boost::dynamic_pointer_cast<Map2D>(pMap);
	if (map2d) {
		return new MapDocument(this->parent, map2d, tilesetVector);
	}

	wxMessageDialog dlg(this->parent, _T("Sorry, this map is in a variant for "
		"which no editor has been written yet!"), _T("Open item"),
		wxOK | wxICON_ERROR);
	dlg.ShowModal();
	return NULL;
}
