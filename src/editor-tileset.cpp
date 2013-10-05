/**
 * @file   editor-tileset.cpp
 * @brief  Tileset editor.
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
#include <boost/bind.hpp>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/glcanvas.h>
#include <png++/png.hpp>
#include "editor-tileset.hpp"
#include "exceptions.hpp"
#include "main.hpp"
#include "editor-tileset-canvas.hpp"

/// Default zoom level
#define CFG_DEFAULT_ZOOM 2

using namespace camoto;
using namespace camoto::gamegraphics;

class TilesetDocument: public IDocument
{
	public:
		TilesetDocument(Studio *studio, TilesetEditor::Settings *settings,
			TilesetPtr tileset, PaletteTablePtr pal)
			:	IDocument(studio, _T("tileset")),
				rootTileset(tileset),
				pal(pal),
				settings(settings),
				offset(0)
		{
			this->canvas = new TilesetCanvas(this, this->studio->getGLContext(),
				this->studio->getGLAttributes(), this->settings->zoomFactor);

			this->allocateTextures(this->rootTileset);
			this->updateTiles(this->rootTileset);
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

			tb->AddTool(IDC_IMPORT, wxEmptyString,
				wxImage(::path.guiIcons + _T("import.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_NORMAL, _T("Import"),
				_T("Replace this image with one loaded from a file"));

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

			this->tilesX = this->rootTileset->getLayoutWidth();
			if (this->tilesX == 0) this->tilesX = 8;
			this->canvas->setTilesX(this->tilesX); // set initial value
		}

		~TilesetDocument()
		{
			// We can't do any OpenGL stuff here (like deleting textures) because the
			// window is no longer visible, so we can't call SetCurrent().
		}

		void onClose(wxCloseEvent& ev)
		{
			// Unload all the textures
			this->canvas->SetCurrent();
			for (TEXTURE_MAP::iterator t = this->tm.begin(); t != this->tm.end(); t++) {
				glDeleteTextures(1, &t->second.glid);
			}
			this->tm.clear(); // hopefully prevent any further attempted redraws
			return;
		}

		unsigned int updateTiles(const TilesetPtr& tileset, unsigned int j = 0)
		{
			// Make the texture operations below apply to this OpenGL surface
			this->canvas->SetCurrent();

			GLushort r[256], g[256], b[256], a[256];
			unsigned int palSize = this->pal->size();
			for (unsigned int i = 0; i < palSize; i++) {
				r[i] = ((*this->pal)[i].red << 8) | (*this->pal)[i].red;
				g[i] = ((*this->pal)[i].green << 8) | (*this->pal)[i].green;
				b[i] = ((*this->pal)[i].blue << 8) | (*this->pal)[i].blue;
				a[i] = ((*this->pal)[i].alpha << 8) | (*this->pal)[i].alpha;
			}
			for (unsigned int i = palSize; i < 256; i++) {
				r[i] = 0;
				g[i] = 0;
				b[i] = 0;
				a[i] = 0xFFFF;
			}
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_R, 256, r);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_G, 256, g);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_B, 256, b);
			glPixelMapusv(GL_PIXEL_MAP_I_TO_A, 256, a);

			// Load all the tiles into OpenGL textures
			const Tileset::VC_ENTRYPTR& tiles = tileset->getItems();
			for (Tileset::VC_ENTRYPTR::const_iterator
				i = tiles.begin(); i != tiles.end(); i++)
			{
				if ((*i)->getAttr() & Tileset::EmptySlot) continue;
				if ((*i)->getAttr() & Tileset::SubTileset) {
					TilesetPtr subTileset = tileset->openTileset(*i);
					j = this->updateTiles(subTileset, j);
					continue;
				}

				Texture& t = this->tm[j];

				ImagePtr image = tileset->openImage(*i);

				image->getDimensions(&t.width, &t.height);
				if ((t.width > 512) || (t.height > 512)) continue; // image too large

				StdImageDataPtr data = image->toStandard();
				// TODO: Handle exceptions

				// Bind each texture in turn to the 2D target
				glBindTexture(GL_TEXTURE_2D, t.glid);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

				// Load an image into the 2D target, which will affect the texture
				// previously bound to it.

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t.width, t.height, 0,
					GL_COLOR_INDEX, GL_UNSIGNED_BYTE, data.get());
				if (glGetError()) {
					std::cerr << "[editor-tileset] GL error loading texture into id " << t.glid << std::endl;
					t.width = 0;
					t.height = 0;
				}
				j++;
			}
			glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
			return j;
		}

		virtual void save()
		{
			// Nothing to save (imports are done directly)
			return;
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

		void onImport(wxCommandEvent& ev)
		{
			assert(this->tilesX > 0);

			wxString path = wxFileSelector(_T("Open image"),
				::path.lastUsed, wxEmptyString, _T(".png"),
#ifdef __WXMSW__
				_T("Supported image files (*.png)|*.png|All files (*.*)|*.*"),
#else
				_T("Supported image files (*.png)|*.png|All files (*)|*"),
#endif
				wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
			if (!path.empty()) {
				// Save path for next time
				wxFileName fn(path, wxPATH_NATIVE);
				::path.lastUsed = fn.GetPath();

				try {
					png::image<png::index_pixel> png;
					try {
						png.read(path.mb_str());
					} catch (const png::error& e) {
						wxMessageDialog dlg(this,
							wxString::Format(_T("Unsupported file.  Only indexed .png images "
								L"are supported.\n\n[%s]"),
								wxString(e.what(), wxConvUTF8).c_str()),
							_T("Import tileset"), wxOK | wxICON_ERROR);
						dlg.ShowModal();
						return;
					}

					IMPORT_DATA params;
					params.curAlloc = 0;
					params.imgData = NULL;
					params.maskData = NULL;

					// Create a palette map in case the .png colours aren't in the same
					// order as the original palette.  This is needed because some image
					// editors (e.g. GIMP) omit colours from the palette if they are
					// unused, messing up the index values.
					memset(params.paletteMap, 0, sizeof(params.paletteMap));
					const png::palette& pngPal = png.get_palette();
					int i_index = 0;
					for (png::palette::const_iterator
						i = pngPal.begin(); i != pngPal.end(); i++, i_index++
					) {
						int j_index = 0;
						for (PaletteTable::iterator
							j = this->pal->begin(); j != this->pal->end(); j++, j_index++
						) {
							if (
								(i->red == j->red) &&
								(i->green == j->green) &&
								(i->blue == j->blue)
							) {
								params.paletteMap[i_index] = j_index;
							}
						}
					}
					png::tRNS transparency = png.get_tRNS();
					for (png::tRNS::const_iterator
						tx = transparency.begin(); tx != transparency.end(); tx++
					) {
						// This colour index is transparent
						params.paletteMap[*tx] = -1;
					}

					params.curTilesX = 0;
					params.srcWidth = png.get_width();
					params.srcHeight = png.get_height();
					params.srcOffX = 0;
					params.srcOffY = 0;
					params.lastMaxHeight = 0;
					params.abort = false;
					params.ppng = &png;
					this->importTileset(this->rootTileset, &params);

					this->rootTileset->flush();

					// This overwrites the image directly, it can't be undone
					//this->isModified = true;
					this->updateTiles(this->rootTileset);
					this->canvas->redraw();

				} catch (const png::error& e) {
					wxMessageDialog dlg(this,
						wxString::Format(_T("Unexpected PNG error importing image!\n\n[%s]"),
						wxString(e.what(), wxConvUTF8).c_str()),
						_T("Import image"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return;
				} catch (const std::exception& e) {
					wxMessageDialog dlg(this,
						wxString::Format(_T("Unexpected error importing image!\n\n[%s]"),
						wxString(e.what(), wxConvUTF8).c_str()),
						_T("Import image"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return;
				}
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

				// Figure out how large the final image will be
				EXPORT_DATA params;
				params.ppng = NULL;
				params.x = 0;
				params.y = 0;
				params.nx = 0;
				params.maxHeight = 0;
				params.totalWidth = 0;
				params.skip = this->offset;
				params.useMask = false; // not important here
				params.dimsOnly = true; // only find the image extents
				this->exportTileset(this->rootTileset, &params);

				// Create an image of the correct dimensions to hold all the tiles
				png::image<png::index_pixel> png(params.totalWidth,
					params.y + params.maxHeight);

				// Populate the palette, leaving room for transparency if needed
				PaletteTablePtr srcPal = this->pal;
				int palSize = srcPal->size();;
				bool useMask = true; // drops colour 255
				assert(srcPal->size() > 0);

				// Add an extra entry for transparency if there's room (otherwise
				// everything gets shifted up one and colour 255 is lost)
				if (useMask && (palSize < 256)) palSize++;

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

				for (PaletteTable::iterator
					i = srcPal->begin(); (i != srcPal->end()) && (j < 256); i++, j++
				) {
					pngPal[j] = png::color(i->red, i->green, i->blue);
				}
				png.set_palette(pngPal);

				// Write the image data to the PNG canvas
				params.ppng = &png;
				params.x = 0;
				params.y = 0;
				params.nx = 0;
				params.maxHeight = 0;
				params.totalWidth = 0;
				params.skip = this->offset;
				params.useMask = useMask;
				params.dimsOnly = false; // do the actual export now
				this->exportTileset(this->rootTileset, &params);

				png.write(path.mb_str());
			}
			return;
		}

		void setZoomFactor(int f)
		{
			this->settings->zoomFactor = f;
			this->canvas->setZoomFactor(f);
			this->canvas->redraw();
			return;
		}

	protected:
		typedef struct {
			png::image<png::index_pixel> *ppng;
			unsigned int x;
			unsigned int y;
			unsigned int nx;
			unsigned int maxHeight;
			unsigned int totalWidth; ///< Total width of the final image, in pixels
			unsigned int skip;  ///< Number of tiles to skip from the start of the export
			bool useMask;
			bool dimsOnly; ///< True to only find total image dimensions, false to write to png
		} EXPORT_DATA;

		typedef struct {
			png::image<png::index_pixel> *ppng;
			signed int paletteMap[256]; ///< -1 means that colour is transparent, 0 == EGA/VGA black, etc.

			// If the tiles are all the same size, images are just imported left
			// to right, wrapping at the edge of the incoming image.  But this
			// won't work if the tiles are different sizes, because one really
			// wide image will make the whole incoming image really wide.  We then
			// can't tell, for narrow images, how many are stored horizontally.
			// So in this case we match the width (in number of tiles) the user
			// has selected in the GUI before doing the import.
			unsigned int curTilesX;

			unsigned int srcWidth;
			unsigned int srcHeight;
			unsigned int srcOffX;
			unsigned int srcOffY;

			unsigned int lastMaxHeight;

			// Cancel the import if this gets set to true
			bool abort;

			unsigned int curAlloc; ///< Amount of memory allocated, in pixels/bytes
			uint8_t *imgData;
			uint8_t *maskData;
			StdImageDataPtr stdimg;
			StdImageDataPtr stdmask;
		} IMPORT_DATA;

		unsigned int allocateTextures(const TilesetPtr& tileset, unsigned int j = 0)
		{
			const Tileset::VC_ENTRYPTR& tiles = tileset->getItems();
			for (Tileset::VC_ENTRYPTR::const_iterator
				i = tiles.begin(); i != tiles.end(); i++)
			{
				if ((*i)->getAttr() & Tileset::EmptySlot) continue;
				if ((*i)->getAttr() & Tileset::SubTileset) {
					TilesetPtr subTileset = tileset->openTileset(*i);
					j = this->allocateTextures(subTileset, j);
					continue;
				}

				Texture t;
				glGenTextures(1, &t.glid);
				this->tm[j] = t;
				j++;
			}
			return j;
		}

		void exportTileset(const TilesetPtr& tileset, EXPORT_DATA *params)
		{
			const Tileset::VC_ENTRYPTR& tiles = tileset->getItems();
			for (Tileset::VC_ENTRYPTR::const_iterator
				i = tiles.begin(); i != tiles.end(); i++)
			{
				if ((*i)->getAttr() & Tileset::EmptySlot) continue;
				if ((*i)->getAttr() & Tileset::SubTileset) {
					TilesetPtr subTileset = tileset->openTileset(*i);
					this->exportTileset(subTileset, params);
					continue;
				}
				if (params->skip) {
					// Skip the first few tiles if requested
					params->skip--;
					continue;
				}

				ImagePtr image = tileset->openImage(*i);
				StdImageDataPtr data = image->toStandard();
				StdImageDataPtr mask = image->toStandardMask();

				unsigned int width, height;
				image->getDimensions(&width, &height);

				if (!params->dimsOnly) {
					// Write the image data
					png::image<png::index_pixel>& png = *params->ppng;
					for (unsigned int py = 0; py < height; py++) {
						for (unsigned int px = 0; px < width; px++) {
							if (params->useMask) {
								if (mask[py*width+px] & 0x01) {
									png[params->y+py][params->x+px] = png::index_pixel(0);
								} else {
									png[params->y+py][params->x+px] =
										// +1 to the colour to skip over transparent (#0)
										png::index_pixel(data[py*width+px] + 1);
								}
							} else {
								png[params->y+py][params->x+px] = png::index_pixel(data[py*width+px]);
							}
						}
					}
				}

				if (height > params->maxHeight) params->maxHeight = height;
				params->x += width;
				params->nx++;
				if (params->nx >= this->tilesX) {
					if (params->x > params->totalWidth) params->totalWidth = params->x;
					params->x = 0;
					params->nx = 0;
					params->y += params->maxHeight;
					params->maxHeight = 0;
				}
			}
			return;
		}

		void importTileset(const TilesetPtr& tileset, IMPORT_DATA *params)
		{
			png::image<png::index_pixel>& png = *params->ppng;
			const Tileset::VC_ENTRYPTR& tiles = tileset->getItems();

			unsigned int defaultTileWidth, defaultTileHeight;
			tileset->getTilesetDimensions(&defaultTileWidth,
				&defaultTileHeight);
			unsigned int thisTileWidth = defaultTileWidth;
			unsigned int thisTileHeight = defaultTileHeight;

			unsigned int targetAlloc = defaultTileWidth * defaultTileHeight;
			if (targetAlloc > params->curAlloc) {
				// Buffer needs to be enlarged
				params->imgData = new uint8_t[targetAlloc];
				params->maskData = new uint8_t[targetAlloc];
				params->stdimg.reset(params->imgData);
				params->stdmask.reset(params->maskData);
				params->curAlloc = targetAlloc;
			} // else previous buffer is big enough, reuse it

			for (Tileset::VC_ENTRYPTR::const_iterator
				i = tiles.begin(); i != tiles.end(); i++
			) {
				// We should really overwrite these with the next tile, but since it's
				// empty we don't know how large it will be, and it wouldn't have been
				// exported anyway.  So for now leave it, and it will have to be overwritten
				// specifically some other way.
				if ((*i)->getAttr() & Tileset::EmptySlot) continue;
				if ((*i)->getAttr() & Tileset::SubTileset) {
					TilesetPtr subTileset = tileset->openTileset(*i);
					this->importTileset(subTileset, params);
					if (params->abort) return;
					continue;
				}

				ImagePtr img = tileset->openImage(*i);

				if ((defaultTileWidth == 0) || (defaultTileHeight == 0)) {
					// All tiles are different sizes
					img->getDimensions(&thisTileWidth, &thisTileHeight);
					unsigned int targetAlloc = thisTileWidth * thisTileHeight;
					if (targetAlloc > params->curAlloc) {
						// Buffer needs to be enlarged
						params->imgData = new uint8_t[targetAlloc];
						params->maskData = new uint8_t[targetAlloc];
						params->stdimg.reset(params->imgData);
						params->stdmask.reset(params->maskData);
						params->curAlloc = targetAlloc;
					} // else previous buffer is big enough, reuse it

					if (params->curTilesX >= this->tilesX) {
						// We've now imported the same number of tiles on this row as
						// the user is viewing, so wrap to the next row.
						params->srcOffX = 0;
						params->srcOffY += params->lastMaxHeight;
						params->lastMaxHeight = thisTileHeight;
						params->curTilesX = 0;
					}
				} else {
					// Figure out the location of this tile in the source image
					if (params->srcOffX + thisTileWidth > params->srcWidth) {
						// This tile would go past the right-hand edge of the source, so
						// wrap around to the next row.
						params->srcOffX = 0;
						params->srcOffY += params->lastMaxHeight;
						params->lastMaxHeight = thisTileHeight;
						params->curTilesX = 0;
					}
				}

				if (params->srcOffX + thisTileWidth > params->srcWidth) {
					// This tile would go past the right edge of the source, so
					// warn the user.
					wxMessageDialog dlg(this, _T("The image being imported is not "
						L"wide enough to contain data for all the tiles being "
						L"imported.  The imported tileset is likely to appear "
						L"corrupted.\n\n"
						L"This can happen if the tileset contains images of varying "
						L"sizes, and it was exported with a different number of "
						L"horizontal tiles displayed.  In this case, you must use the "
						L"toolbar buttons to arrange the tileset such that it is "
						L"showing the same number of tiles horizontally as it was "
						L"when originally exported.  You should then be able to "
						L"re-import the image with no problems.\n\n"
						L"For example, if the tileset was arranged to show five tiles "
						L"across when it was exported, you must now arrange it to show "
						L"five tiles across again, before performing the import."),
						_T("Import tileset"), wxOK | wxICON_WARNING);
					dlg.ShowModal();
					params->abort = true;
					return;
				}
				if (params->srcOffY + thisTileHeight > params->srcHeight) {
					// This tile would go past the bottom edge of the source, so we
					// have to stop.
					wxMessageDialog dlg(this, _T("Not all tiles could be imported, "
						L"as the .png image was smaller than the full tileset.  Only "
						L"complete tiles have been imported.\n\n"
						L"You should not have this problem if you start work on an "
						L"exported image, and do not resize the image in your editor."),
						_T("Import tileset"), wxOK | wxICON_WARNING);
					dlg.ShowModal();
					params->abort = true;
					return;
				}

				// Update the height of the largest tile in this row
				if (thisTileHeight > params->lastMaxHeight) params->lastMaxHeight = thisTileHeight;

				for (unsigned int y = 0; y < thisTileHeight; y++) {
					for (unsigned int x = 0; x < thisTileWidth; x++) {
						uint8_t pixel = png[params->srcOffY + y][params->srcOffX + x];
						int destPixel = params->paletteMap[pixel];
						if (destPixel == -1) {
							params->maskData[y * thisTileWidth + x] = 0x01; // transparent
							params->imgData[y * thisTileWidth + x] = 0x00; // just in case
						} else {
							params->maskData[y * thisTileWidth + x] = 0x00; // opaque
							params->imgData[y * thisTileWidth + x] = destPixel;
						}
					}
				}

				img->fromStandard(params->stdimg, params->stdmask);

				params->srcOffX += thisTileWidth;
				params->curTilesX++;

			} // for (all tiles)
			return;
		}

	protected:
		TilesetCanvas *canvas;  ///< Drawing surface
		TilesetPtr rootTileset; ///< Tileset to display
		PaletteTablePtr pal;    ///< Palette to use
		TilesetEditor::Settings *settings;
		TEXTURE_MAP tm;         ///< Map between tile indices and OpenGL texture info

		unsigned int tilesX;    ///< Number of tiles to draw before wrapping to the next row
		unsigned int offset;    ///< Number of tiles to skip drawing from the start of the tileset

		enum {
			IDC_INC_WIDTH = wxID_HIGHEST + 1,
			IDC_DEC_WIDTH,
			IDC_INC_OFFSET,
			IDC_DEC_OFFSET,
			IDC_IMPORT,
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
	EVT_TOOL(IDC_IMPORT, TilesetDocument::onImport)
	EVT_TOOL(IDC_EXPORT, TilesetDocument::onExport)
END_EVENT_TABLE()


TilesetEditor::TilesetEditor(Studio *studio)
	:	studio(studio)
{
	// Default settings
	this->settings.zoomFactor = CFG_DEFAULT_ZOOM;
}

TilesetEditor::~TilesetEditor()
{
}

std::vector<IToolPanel *> TilesetEditor::createToolPanes() const
{
	std::vector<IToolPanel *> panels;
	return panels;
}

void TilesetEditor::loadSettings(Project *proj)
{
	proj->config.Read(_T("editor-tileset/zoom"), (int *)&this->settings.zoomFactor, CFG_DEFAULT_ZOOM);
	return;
}

void TilesetEditor::saveSettings(Project *proj) const
{
	proj->config.Write(_T("editor-tileset/zoom"), (int)this->settings.zoomFactor);
	return;
}

bool TilesetEditor::isFormatSupported(const wxString& type) const
{
	if (type.IsSameAs(_T(TILESETTYPE_MINOR_FROMIMG))) return true;

	std::string strType("tls-");
	strType.append(type.ToUTF8());
	return this->studio->mgrGraphics->getTilesetTypeByCode(strType);
}

IDocument *TilesetEditor::openObject(const GameObjectPtr& o)
{
	try {
		TilesetPtr tileset = this->studio->openTileset(o);
		if (!tileset) return NULL; // user cancelled

		PaletteTablePtr pal;

		// See if a palette was specified in the XML
		Deps::const_iterator idep = o->dep.find(Palette);
		if (idep != o->dep.end()) {
			// Find this object
			GameObjectMap::iterator io = this->studio->game->objects.find(idep->second);
			if (io == this->studio->game->objects.end()) {
				throw EFailure(wxString::Format(_("Cannot open this item.  It refers "
					"to an entry in the game description XML file which doesn't exist!"
					"\n\n[Item \"%s\" has an invalid ID in the dep:%s attribute]"),
					o->id.c_str(), dep2string(Palette).c_str()));
			}
			pal = this->studio->openPalette(io->second);
		} else {
			if (tileset->getCaps() & Tileset::HasPalette) {
				pal = tileset->getPalette();
				if (pal->size() == 0) {
					std::cout << "[editor-tileset] Palette contains no entries, using default\n";
					// default is loaded below
				}
			}
		}
		if ((!pal) || (pal->size() == 0)) {
			// Need to use the default palette
			switch (tileset->getCaps() & Tileset::ColourDepthMask) {
				case Tileset::ColourDepthVGA:
					pal = createPalette_DefaultVGA();
					break;
				case Tileset::ColourDepthEGA:
					pal = createPalette_DefaultEGA();
					break;
				case Tileset::ColourDepthCGA:
					pal = createPalette_CGA(CGAPal_CyanMagenta);
					break;
				case Tileset::ColourDepthMono:
					pal = createPalette_DefaultMono();
					break;
			}
		}

		return new TilesetDocument(this->studio, &this->settings, tileset, pal);
	} catch (const camoto::stream::error& e) {
		throw EFailure(wxString::Format(_("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}
}
