/**
 * @file   editor-tileset.cpp
 * @brief  Tileset editor.
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

#include <boost/bind.hpp>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/glcanvas.h>
#include <png++/png.hpp>
#include "editor-tileset.hpp"
#include "efailure.hpp"
#include "main.hpp"
#include "editor-tileset-canvas.hpp"

#ifdef HAVE_GL_GL_H
#include <GL/gl.h>
#else
#ifdef HAVE_OPENGL_GL_H
#include <OpenGL/gl.h>
#endif
#endif

/// Default zoom level
#define CFG_DEFAULT_ZOOM 2

using namespace camoto;
using namespace camoto::gamegraphics;

class TilesetDocument: public IDocument
{
	public:
		TilesetDocument(IMainWindow *parent, TilesetEditor::Settings *settings, TilesetPtr tileset)
			throw () :
				IDocument(parent, _T("tileset")),
				tileset(tileset),
				settings(settings),
				offset(0)
		{
			// Load the palette
			PaletteTablePtr pal;
			if (tileset->getCaps() & Tileset::HasPalette) {
				pal = tileset->getPalette();
			} else {
				pal = createDefaultCGAPalette();
			}

			this->canvas = new TilesetCanvas(this, this->frame->getGLContext(),
				this->frame->getGLAttributes(), this->settings->zoomFactor);

			// Make the texture operations below apply to this OpenGL surface
			this->canvas->SetCurrent();

			GLushort r[256], g[256], b[256], a[256];
			for (int i = 0; i < 256; i++) {
				r[i] = ((*pal)[i].red << 8) | (*pal)[i].red;
				g[i] = ((*pal)[i].green << 8) | (*pal)[i].green;
				b[i] = ((*pal)[i].blue << 8) | (*pal)[i].blue;
				a[i] = (GLushort)-1;
			}
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_R, 256, r);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_G, 256, g);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_B, 256, b);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_A, 256, a);

			// Load all the tiles into OpenGL textures
			const Tileset::VC_ENTRYPTR& tiles = tileset->getItems();
			int j = 0;
			for (Tileset::VC_ENTRYPTR::const_iterator i = tiles.begin();
				i != tiles.end(); i++, j++)
			{
				if ((*i)->attr & Tileset::EmptySlot) continue;
				if ((*i)->attr & Tileset::SubTileset) continue;

				Texture t;
				glGenTextures(1, &t.glid);

				// Bind each texture in turn to the 2D target
				glBindTexture(GL_TEXTURE_2D, t.glid);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

				// Load an image into the 2D target, which will affect the texture
				// previously bound to it.
				ImagePtr image = tileset->openImage(*i);
				StdImageDataPtr data = image->toStandard();
				image->getDimensions(&t.width, &t.height);
				if ((t.width > 512) || (t.height > 512)) continue; // image too large

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t.width, t.height, 0,
					GL_COLOR_INDEX, GL_UNSIGNED_BYTE, data.get());
				if (glGetError()) {
					std::cerr << "[editor-tileset] GL error loading texture into id " << t.glid << std::endl;
				} else {
					// Store this tile in the map, by its index
					this->tm[j] = t;
				}
			}
			glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
			this->canvas->setTextures(this->tm);

			wxToolBar *tb = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
				wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
			tb->SetToolBitmapSize(wxSize(16, 16));

			tb->AddTool(wxID_ZOOM_OUT, wxEmptyString,
				wxImage(::path.guiIcons + _T("zoom_1-1.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_RADIO, _T("Zoom 1:1 (small)"),
				_T("Set the zoom level so that one screen pixel is used for each tile pixel"));

			tb->AddTool(wxID_ZOOM_100, wxEmptyString,
				wxImage(::path.guiIcons + _T("zoom_2-1.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_RADIO, _T("Zoom 2:1 (normal)"),
				_T("Set the zoom level so that two screen pixels are used for each tile pixel"));

			tb->AddTool(wxID_ZOOM_IN, wxEmptyString,
				wxImage(::path.guiIcons + _T("zoom_3-1.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_RADIO, _T("Zoom 3:1 (big)"),
				_T("Set the zoom level so that three screen pixels are used for each tile pixel"));

			tb->AddSeparator();

			tb->AddTool(IDC_INC_WIDTH, wxEmptyString,
				wxImage(::path.guiIcons + _T("inc-width.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_NORMAL, _T("Increase width"),
				_T("Draw more tiles horizontally before wrapping to the next row"));

			tb->AddTool(IDC_DEC_WIDTH, wxEmptyString,
				wxImage(::path.guiIcons + _T("dec-width.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_NORMAL, _T("Decrease width"),
				_T("Draw fewer tiles horizontally before wrapping to the next row"));

			tb->AddTool(IDC_INC_OFFSET, wxEmptyString,
				wxImage(::path.guiIcons + _T("inc-offset.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_NORMAL, _T("Increase offset"),
				_T("Skip the first tile"));

			tb->AddTool(IDC_DEC_OFFSET, wxEmptyString,
				wxImage(::path.guiIcons + _T("dec-offset.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_NORMAL, _T("Decrease offset"),
				_T("Return the first tile if it has been skipped"));

			tb->AddSeparator();

			tb->AddTool(IDC_EXPORT, wxEmptyString,
				wxImage(::path.guiIcons + _T("export.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_NORMAL, _T("Export"),
				_T("Save this image to a file"));

			// Update the UI
			switch (this->settings->zoomFactor) {
				case 1: tb->ToggleTool(wxID_ZOOM_OUT, true); break;
				case 2: tb->ToggleTool(wxID_ZOOM_100, true); break;
				case 4: tb->ToggleTool(wxID_ZOOM_IN, true); break;
					// default: don't select anything, just in case!
			}

			tb->Realize();

			wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
			s->Add(tb, 0, wxEXPAND);
			s->Add(this->canvas, 1, wxEXPAND);
			this->SetSizer(s);

			this->tilesX = this->tileset->getLayoutWidth();
			if (this->tilesX == 0) this->tilesX = 8;
			this->canvas->setTilesX(this->tilesX); // set initial value
		}

		~TilesetDocument()
			throw ()
		{
			// Unload all the textures
			this->canvas->SetCurrent();
			for (TEXTURE_MAP::iterator t = this->tm.begin(); t != this->tm.end(); t++) {
				glDeleteTextures(1, &t->second.glid);
			}
		}

		virtual void save()
			throw (camoto::stream::error)
		{
			throw camoto::stream::error("Saving has not been implemented yet!");
		}

		void onZoomSmall(wxCommandEvent& ev)
		{
			this->setZoomFactor(1);
			return;
		}

		void onZoomNormal(wxCommandEvent& ev)
		{
			this->setZoomFactor(2);
			return;
		}

		void onZoomLarge(wxCommandEvent& ev)
		{
			this->setZoomFactor(4);
			return;
		}

		void onIncWidth(wxCommandEvent& ev)
		{
			this->tilesX++;
			this->canvas->setTilesX(this->tilesX);
			return;
		}

		void onDecWidth(wxCommandEvent& ev)
		{
			if (this->tilesX > 1) {
				this->tilesX--;
				this->canvas->setTilesX(this->tilesX);
			}
			return;
		}

		void onIncOffset(wxCommandEvent& ev)
		{
			this->offset++;
			this->canvas->setOffset(this->offset);
			return;
		}

		void onDecOffset(wxCommandEvent& ev)
		{
			if (this->offset > 0) {
				this->offset--;
				this->canvas->setOffset(this->offset);
			}
			return;
		}

		void onExport(wxCommandEvent& ev)
		{
			assert(this->tilesX > 0);

			wxString path = wxFileSelector(_T("Save image"),
				::path.lastUsed, wxEmptyString, _T(".png"),
#ifdef __WXMSW__
				_T("Supported image files (*.png)|*.png|All files (*.*)|*.*"),
#else
				_T("Supported image files (*.png)|*.png|All files (*)|*"),
#endif
				wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
			if (!path.empty()) {
				// Save path for next time
				wxFileName fn(path, wxPATH_NATIVE);
				::path.lastUsed = fn.GetPath();

				const Tileset::VC_ENTRYPTR& tiles = tileset->getItems();
				unsigned int numTiles = tiles.size();
				if (this->tilesX > numTiles) this->tilesX = numTiles;

				// Find the width and height of the output image
				unsigned int x = 0, y = 0, nx = 0, maxHeight = 0, maxWidth = 0;
				for (unsigned int i = 0, tileIndex = this->offset; tileIndex < numTiles; i++, tileIndex++) {
					ImagePtr image = this->tileset->openImage(tiles[tileIndex]);
					unsigned int width, height;
					image->getDimensions(&width, &height);

					if (height > maxHeight) maxHeight = height;
					x += width;
					nx++;
					if (nx >= this->tilesX) {
						if (x > maxWidth) maxWidth = x;
						x = 0;
						nx = 0;
						y += maxHeight;
						maxHeight = 0;
					}
				}

				png::image<png::index_pixel> png(maxWidth, y + maxHeight);

				PaletteTablePtr srcPal;
				if (this->tileset->getCaps() & Tileset::HasPalette) {
					srcPal = this->tileset->getPalette();
				} else {
					srcPal = createDefaultCGAPalette();
				}
				if (srcPal->size() == 0) {
					std::cout << "[editor-image] Palette contains no entries, using default\n";
					srcPal = createDefaultCGAPalette();
				}

				int palSize = srcPal->size();
				bool useMask = palSize < 255;
				if (useMask) palSize++;

				png::palette pngPal(palSize);
				int j = 0;
				if (useMask) {
					// Assign first colour as transparent
					png::tRNS transparency;
					transparency.push_back(j);
					png.set_tRNS(transparency);

					pngPal[j] = png::color(0xFF, 0x00, 0xFF);
					j++;
				}

				for (PaletteTable::iterator i = srcPal->begin();
					i != srcPal->end();
					i++, j++
				) {
					pngPal[j] = png::color(i->red, i->green, i->blue);
				}

				png.set_palette(pngPal);

				x = 0; y = 0; nx = 0; maxHeight = 0;
				for (unsigned int i = 0, tileIndex = this->offset; tileIndex < numTiles; i++, tileIndex++) {
					if (tiles[tileIndex]->attr & Tileset::SubTileset) continue; // aah! tileset! bad!

					ImagePtr image = tileset->openImage(tiles[tileIndex]);
					StdImageDataPtr data = image->toStandard();
					StdImageDataPtr mask = image->toStandardMask();

					unsigned int width, height;
					image->getDimensions(&width, &height);

					for (unsigned int py = 0; py < height; py++) {
						for (unsigned int px = 0; px < width; px++) {
							if (useMask) {
								if (mask[py*width+x] & 0x01) {
									png[y+py][x+px] = png::index_pixel(0);
								} else {
									png[y+py][x+px] =
										// +1 to the colour to skip over transparent (#0)
										png::index_pixel(data[py*width+px] + 1);
								}
							} else {
								png[y+py][x+px] = png::index_pixel(data[py*width+px]);
							}
						}
					}

					if (height > maxHeight) maxHeight = height;
					x += width;
					nx++;
					if (nx >= this->tilesX) {
						x = 0;
						nx = 0;
						y += maxHeight;
						maxHeight = 0;
					}
				}

				png.write(path.mb_str());
			}
			return;
		}

		void setZoomFactor(int f)
			throw ()
		{
			this->settings->zoomFactor = f;
			this->canvas->setZoomFactor(f);
			this->canvas->redraw();
			return;
		}

	protected:
		TilesetCanvas *canvas; ///< Drawing surface
		TilesetPtr tileset;    ///< Tileset to display
		TilesetEditor::Settings *settings;
		TEXTURE_MAP tm;        ///< Map between tile indices and OpenGL texture info

		unsigned int tilesX;   ///< Number of tiles to draw before wrapping to the next row
		unsigned int offset;   ///< Number of tiles to skip drawing from the start of the tileset

		enum {
			IDC_INC_WIDTH = wxID_HIGHEST + 1,
			IDC_DEC_WIDTH,
			IDC_INC_OFFSET,
			IDC_DEC_OFFSET,
			IDC_EXPORT,
		};
		DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(TilesetDocument, IDocument)
	EVT_TOOL(wxID_ZOOM_OUT, TilesetDocument::onZoomSmall)
	EVT_TOOL(wxID_ZOOM_100, TilesetDocument::onZoomNormal)
	EVT_TOOL(wxID_ZOOM_IN, TilesetDocument::onZoomLarge)
	EVT_TOOL(IDC_INC_WIDTH, TilesetDocument::onIncWidth)
	EVT_TOOL(IDC_DEC_WIDTH, TilesetDocument::onDecWidth)
	EVT_TOOL(IDC_INC_OFFSET, TilesetDocument::onIncOffset)
	EVT_TOOL(IDC_DEC_OFFSET, TilesetDocument::onDecOffset)
	EVT_TOOL(IDC_EXPORT, TilesetDocument::onExport)
END_EVENT_TABLE()


TilesetEditor::TilesetEditor(IMainWindow *parent)
	throw () :
		frame(parent),
		pManager(camoto::gamegraphics::getManager())
{
	// Default settings
	this->settings.zoomFactor = CFG_DEFAULT_ZOOM;
}

TilesetEditor::~TilesetEditor()
	throw ()
{
}

std::vector<IToolPanel *> TilesetEditor::createToolPanes() const
	throw ()
{
	std::vector<IToolPanel *> panels;
	return panels;
}

void TilesetEditor::loadSettings(Project *proj)
	throw ()
{
	proj->config.Read(_T("editor-tileset/zoom"), (int *)&this->settings.zoomFactor, CFG_DEFAULT_ZOOM);
	return;
}

void TilesetEditor::saveSettings(Project *proj) const
	throw ()
{
	proj->config.Write(_T("editor-tileset/zoom"), (int)this->settings.zoomFactor);
	return;
}

bool TilesetEditor::isFormatSupported(const wxString& type) const
	throw ()
{
	std::string strType("tls-");
	strType.append(type.ToUTF8());
	return this->pManager->getTilesetTypeByCode(strType);
}

IDocument *TilesetEditor::openObject(const wxString& typeMinor,
	stream::inout_sptr data, const wxString& filename, SuppMap supp, const Game *game)
	throw (EFailure)
{
	if (typeMinor.IsEmpty()) {
		throw EFailure(_T("No file type was specified for this item!"));
	}

	std::string strType("tls-");
	strType.append(typeMinor.ToUTF8());
	TilesetTypePtr pTilesetType(this->pManager->getTilesetTypeByCode(strType));
	if (!pTilesetType) {
		wxString wxtype(strType.c_str(), wxConvUTF8);
		throw EFailure(wxString::Format(_T("Sorry, it is not possible to edit this "
			"tileset as the \"%s\" format is unsupported.  (No handler for \"%s\")"),
			typeMinor.c_str(), wxtype.c_str()));
	}
	std::cout << "[editor-tileset] Using handler for "
		<< pTilesetType->getFriendlyName() << "\n";

	// Check to see if the file is actually in this format
	if (pTilesetType->isInstance(data) < TilesetType::PossiblyYes) {
		std::string friendlyType = pTilesetType->getFriendlyName();
		wxString wxtype(friendlyType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_T("This file is supposed to be in \"%s\" "
			"format, but it seems this may not be the case.  Would you like to try "
			"opening it anyway?"), wxtype.c_str());

		wxMessageDialog dlg(this->frame, msg, _T("Open item"),
			wxYES_NO | wxICON_ERROR);
		int r = dlg.ShowModal();
		if (r != wxID_YES) return NULL;
	}

	// Collect any supplemental files supplied
	SuppData suppData;
	suppMapToData(supp, suppData);

	try {
		// Open the tileset file
		TilesetPtr pTileset(pTilesetType->open(data, suppData));
		assert(pTileset);

		return new TilesetDocument(this->frame, &this->settings, pTileset);
	} catch (const camoto::stream::error& e) {
		throw EFailure(wxString::Format(_T("Library exception: %s"),
				wxString(e.what(), wxConvUTF8).c_str()));
	}
}
