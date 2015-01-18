/**
 * @file   editor-image.cpp
 * @brief  Image editor.
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

#include <boost/bind.hpp>
#include <wx/filedlg.h>
#include <png++/png.hpp>
#include "main.hpp"
#include "editor-image.hpp"

/// Default zoom level
#define CFG_DEFAULT_ZOOM 2

using namespace camoto;
using namespace camoto::gamegraphics;

class ImageCanvas: public wxPanel
{
	public:

		ImageCanvas(wxWindow *parent, ImagePtr image, PaletteTablePtr pal,
			int zoomFactor)
			:	wxPanel(parent, wxID_ANY),
				image(image),
				pal(pal),
				wximg(),
				zoomFactor(zoomFactor)
		{
			this->imageChanged();
		}

		void imageChanged()
		{
			StdImageDataPtr data, mask;
			try {
				data = image->toStandard();
				mask = image->toStandardMask();
			} catch (const stream::error& e) {
				wxMessageDialog dlg(this,
					wxString::Format(_("Error reading image from native format:\n\n%s"),
						wxString(e.what(), wxConvUTF8).c_str()),
					_("Conversion error"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
			}
			this->image->getDimensions(&this->width, &this->height);

			this->wximg = wxImage(width, height, false);
			uint8_t *pixels = this->wximg.GetData();
			if (pixels) {
				// Convert the image from palettised to RGB
				int size = width * height;
				if (data) {
					int palSize = this->pal->size();
					for (int i = 0, p = 0; i < size; i++) {
						if ((mask[i] & 1) == 1) {
							// Transparent
							bool check = ((i >> 2) & 1) ^ (((i / width) >> 2) & 1);
							uint8_t clr = check ? 0x90 : 0xD0;
							pixels[p++] = clr;
							pixels[p++] = clr;
							pixels[p++] = clr;
						} else {
							// Opaque
							uint8_t clr = data[i];
							if (clr >= palSize) clr = 0;
							pixels[p++] = this->pal->at(clr).red;
							pixels[p++] = this->pal->at(clr).green;
							pixels[p++] = this->pal->at(clr).blue;
						}
					}
				} else {
					// no data, corrupted image
					memset(pixels, 0, size * 3);
				}
			} else {
				std::cout << "[editor-image] Error getting pixels out of wxImage\n";
			}

			// Initialise the image
			this->setZoomFactor(this->zoomFactor);
		}

		~ImageCanvas()
		{
		}

		void onEraseBG(wxEraseEvent& ev)
		{
			return;
		}

		void onPaint(wxPaintEvent& ev)
		{
			wxPaintDC dc(this);

			int canvasWidth, canvasHeight;
			this->GetClientSize(&canvasWidth, &canvasHeight);

			unsigned int imgWidth = this->width * this->zoomFactor;
			unsigned int imgHeight = this->height * this->zoomFactor;

			// Draw a dark grey background behind the image
			dc.SetBrush(*wxMEDIUM_GREY_BRUSH);
			dc.SetPen(*wxMEDIUM_GREY_PEN);
			dc.DrawRectangle(imgWidth, 0, canvasWidth - imgWidth, canvasHeight);
			dc.DrawRectangle(0, imgHeight, imgWidth, canvasHeight - imgHeight);

			// Draw the image
			dc.DrawBitmap(this->wxbmp, 0, 0, false);
			return;
		}

		void setZoomFactor(int f)
		{
			this->zoomFactor = f;
			if (this->zoomFactor == 1) {
				this->wxbmp = wxBitmap(this->wximg, -1);
			} else {
				this->wxbmp = wxBitmap(this->wximg.Scale(this->width * f, this->height * f), -1);
			}
			this->Refresh();
			return;
		}

	protected:
		ImagePtr image;             ///< Camoto libgamegraphics image instance
		PaletteTablePtr pal;        ///< Camoto colour palette for this image
		wxImage wximg;              ///< Camoto image converted to wxImage
		wxBitmap wxbmp;             ///< wxImage converted to wxBitmap ready to be drawn
		unsigned int width, height; ///< Dimensions of underlying image
		int zoomFactor;             ///< Zoom level, 1 == 1:1, 2 == doublesize, etc.

		DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(ImageCanvas, wxPanel)
	EVT_PAINT(ImageCanvas::onPaint)
	EVT_ERASE_BACKGROUND(ImageCanvas::onEraseBG)
END_EVENT_TABLE()

class ImageDocument: public IDocument
{
	public:
		ImageDocument(Studio *studio, ImageEditor::Settings *settings,
			ImagePtr image, PaletteTablePtr pal)
			:	IDocument(studio, _T("image")),
				image(image),
				pal(pal),
				settings(settings)
		{
			this->canvas = new ImageCanvas(this, image, pal, this->settings->zoomFactor);

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

			tb->AddTool(wxID_ZOOM_IN, wxEmptyString,
				wxImage(::path.guiIcons + _T("zoom_3-1.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_RADIO, _("Zoom 3:1 (big)"),
				_("Set the zoom level so that three screen pixels are used for each tile pixel"));

			tb->AddSeparator();

			tb->AddTool(IDC_IMPORT, wxEmptyString,
				wxImage(::path.guiIcons + _T("import.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_NORMAL, _("Import"),
				_("Replace this image with one loaded from a file"));

			tb->AddTool(IDC_EXPORT, wxEmptyString,
				wxImage(::path.guiIcons + _T("export.png"), wxBITMAP_TYPE_PNG),
				wxNullBitmap, wxITEM_NORMAL, _("Export"),
				_("Save this image to a file"));

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
		}

		virtual void save()
		{
			// Nothing to do - can't modify anything yet that isn't immediately saved
			this->isModified = false;
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

		void onImport(wxCommandEvent& ev)
		{
			wxString path = wxFileSelector(_("Open image"),
				::path.lastUsed, wxEmptyString, _T(".png"),
#ifdef __WXMSW__
				_("Supported image files (*.png)|*.png|All files (*.*)|*.*"),
#else
				_("Supported image files (*.png)|*.png|All files (*)|*"),
#endif
				wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
			if (!path.empty()) {
				// Save path for next time
				wxFileName fn(path, wxPATH_NATIVE);
				::path.lastUsed = fn.GetPath();

				try {
					unsigned int width, height;
					this->image->getDimensions(&width, &height);

					png::image<png::index_pixel> png;
					try {
						png.read(path.mb_str());
					} catch (const png::error& e) {
						wxString depth;
						switch (this->image->getCaps() & Image::ColourDepthMask) {
							case Image::ColourDepthVGA: depth = _("256-colour"); break;
							case Image::ColourDepthEGA: depth = _("16-colour"); break;
							case Image::ColourDepthCGA: depth = _("4-colour"); break;
							case Image::ColourDepthMono: depth = _("monochrome"); break;
							default: depth = _("<unknown depth>"); break;
						}
						wxMessageDialog dlg(this,
							wxString::Format(_("Unsupported file.  Only indexed .png images "
								"are supported, and the colour depth must match the image "
								"being replaced (%s).\n\n[%s]"), depth.c_str(),
								wxString(e.what(), wxConvUTF8).c_str()),
							_("Import image"), wxOK | wxICON_ERROR);
						dlg.ShowModal();
						return;
					}

					if ((png.get_width() != width) || (png.get_height() != height)) {
						// The image is a different size, see if we can resize it.
						if (this->image->getCaps() & Image::CanSetDimensions) {
							wxMessageDialog dlg(this,
								_("The image to be imported is a different size to the image "
									"being replaced.  The target image will be resized to match "
									"the image being imported.  This can have unpredictable "
									"consequences if the game cannot cope with the new image "
									"size.\n\nDo you want to continue?"),
								_("Import image"), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
							int r = dlg.ShowModal();
							if (r == wxID_YES) {
								width = png.get_width();
								height = png.get_height();
								this->image->setDimensions(width, height);
							} else {
								return;
							}
						} else {
							wxMessageDialog dlg(this,
								_("Bad image format.  The image to import must be the same "
									"dimensions as the image being replaced (this particular "
									"image has a fixed size.)"),
								_("Import image"), wxOK | wxICON_ERROR);
							dlg.ShowModal();
							return;
						}
					}

					const png::palette& pngPal = png.get_palette();
					int palSize = pngPal.size();
					int targetDepth = this->image->getCaps() & Image::ColourDepthMask;
					if (
						((targetDepth == Image::ColourDepthEGA)  && (palSize > 16+1)) ||
						((targetDepth == Image::ColourDepthCGA)  && (palSize > 4+1)) ||
						((targetDepth == Image::ColourDepthMono) && (palSize > 1+1))
					) {
						wxString depth;
						switch (this->image->getCaps() & Image::ColourDepthMask) {
							case Image::ColourDepthVGA: depth = _("256-colour"); break;
							case Image::ColourDepthEGA: depth = _("16-colour"); break;
							case Image::ColourDepthCGA: depth = _("4-colour"); break;
							case Image::ColourDepthMono: depth = _("monochrome"); break;
							default: depth = _("<unknown depth>"); break;
						}
						wxMessageDialog dlg(this,
							wxString::Format(_("There are too many colours in the image "
								"being imported.  Only indexed .png images are supported, and "
								"the colour depth must match the image being replaced (in "
								"this case the image must be %s)."), depth.c_str()),
							_("Import image"), wxOK | wxICON_ERROR);
						dlg.ShowModal();
						return;
					}

					bool hasTransparency = false;
					int pixelOffset = 0;
					png::tRNS transparency = png.get_tRNS();
					if (transparency.size() > 0) {
						if (transparency[0] != 0) {
							wxMessageDialog dlg(this,
								_("Bad image format.  The image to import must have palette "
									"entry #0 and only this entry set to transparent, or no "
									"transparency at all."),
								_("Import image"), wxOK | wxICON_ERROR);
							dlg.ShowModal();
							return;
						}
						hasTransparency = true;
						pixelOffset = -1; // to account for palette #0 being inserted for use as transparency
					}

					int imgSizeBytes = width * height;
					uint8_t *imgData = new uint8_t[imgSizeBytes];
					uint8_t *maskData = new uint8_t[imgSizeBytes];
					StdImageDataPtr stdimg(imgData);
					StdImageDataPtr stdmask(maskData);

					for (unsigned int y = 0; y < height; y++) {
						for (unsigned int x = 0; x < width; x++) {
							uint8_t pixel = png[y][x];
							if (hasTransparency && (pixel == 0)) { // Palette #0 must be transparent
								maskData[y * width + x] = 0x01; // transparent
								imgData[y * width + x] = 0;
							} else {
								maskData[y * width + x] = 0x00; // opaque
								imgData[y * width + x] = pixel + pixelOffset;
							}
						}
					}

					if (this->image->getCaps() & Image::HasPalette) {
						// This format supports custom palettes, so update it from the
						// PNG image.
						PaletteTablePtr newPal(new PaletteTable());
						newPal->reserve(pngPal.size());
						for (png::palette::const_iterator i = pngPal.begin(); i != pngPal.end(); i++) {
							PaletteEntry p;
							p.red = p.green = p.blue = p.alpha = 255;
							newPal->push_back(p);
						}
						this->image->setPalette(newPal);
						this->pal = newPal;
					}
					this->image->fromStandard(stdimg, stdmask);

					// This overwrites the image directly, it can't be undone
					//this->isModified = true;
					this->canvas->imageChanged();
				} catch (const png::error& e) {
					wxMessageDialog dlg(this,
						wxString::Format(_("Unexpected PNG error importing image!\n\n[%s]"),
							wxString(e.what(), wxConvUTF8).c_str()),
						_("Import image"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return;
				} catch (const std::exception& e) {
					wxMessageDialog dlg(this,
						wxString::Format(_("Unexpected error importing image!\n\n[%s]"),
							wxString(e.what(), wxConvUTF8).c_str()),
						_("Import image"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return;
				}
			}
			return;
		}

		void onExport(wxCommandEvent& ev)
		{
			wxString path = wxFileSelector(_("Save image"),
				::path.lastUsed, wxEmptyString, _T(".png"),
#ifdef __WXMSW__
				_("Supported image files (*.png)|*.png|All files (*.*)|*.*"),
#else
				_("Supported image files (*.png)|*.png|All files (*)|*"),
#endif
				wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
			if (!path.empty()) {
				// Save path for next time
				wxFileName fn(path, wxPATH_NATIVE);
				::path.lastUsed = fn.GetPath();

				// Export the image
				unsigned int width, height;
				this->image->getDimensions(&width, &height);

				StdImageDataPtr data = this->image->toStandard();
				StdImageDataPtr mask = this->image->toStandardMask();

				png::image<png::index_pixel> png(width, height);

				PaletteTablePtr& srcPal = this->pal;

				int palSize = srcPal->size();
				bool useMask = palSize < 255;
				if (useMask) palSize++;

				png::tRNS transparency;
				png::palette pngPal(palSize);
				int j = 0;
				if (useMask) {
					// Assign first colour as transparent
					transparency.push_back(j);

					pngPal[j] = png::color(0xFF, 0x00, 0xFF);
					j++;
				}

				for (PaletteTable::iterator i = srcPal->begin();
					i != srcPal->end();
					i++, j++
				) {
					pngPal[j] = png::color(i->red, i->green, i->blue);
					if (i->alpha == 0x00) transparency.push_back(j);
				}

				png.set_palette(pngPal);

				if (transparency.size()) png.set_tRNS(transparency);

				// Copy the pixel data across
				for (unsigned int y = 0; y < height; y++) {
					for (unsigned int x = 0; x < width; x++) {
						if (useMask) {
							if (mask[y*width+x] & 0x01) {
								png[y][x] = png::index_pixel(0);
							} else {
								png[y][x] = png::index_pixel(data[y*width+x] + 1);
							}
						} else {
							png[y][x] = png::index_pixel(data[y*width+x]);
						}
					}
				}

				png.write(path.mb_str());
			}
			return;
		}

		void setZoomFactor(int f)
		{
			this->settings->zoomFactor = f;
			this->canvas->setZoomFactor(f);
			return;
		}

	protected:
		ImageCanvas *canvas;
		ImagePtr image;
		PaletteTablePtr pal;        ///< Camoto colour palette for this image
		ImageEditor::Settings *settings;

		friend class LayerPanel;

		enum {
			IDC_IMPORT = wxID_HIGHEST + 1,
			IDC_EXPORT,
		};
		DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(ImageDocument, IDocument)
	EVT_TOOL(wxID_ZOOM_OUT, ImageDocument::onZoomSmall)
	EVT_TOOL(wxID_ZOOM_100, ImageDocument::onZoomNormal)
	EVT_TOOL(wxID_ZOOM_IN, ImageDocument::onZoomLarge)
	EVT_TOOL(IDC_IMPORT, ImageDocument::onImport)
	EVT_TOOL(IDC_EXPORT, ImageDocument::onExport)
END_EVENT_TABLE()


ImageEditor::ImageEditor(Studio *studio)
	:	studio(studio)
{
	// Default settings
	this->settings.zoomFactor = CFG_DEFAULT_ZOOM;
}

ImageEditor::~ImageEditor()
{
}

std::vector<IToolPanel *> ImageEditor::createToolPanes() const
{
	std::vector<IToolPanel *> panels;
	return panels;
}

void ImageEditor::loadSettings(Project *proj)
{
	proj->config.Read(_T("editor-image/zoom"), (int *)&this->settings.zoomFactor, CFG_DEFAULT_ZOOM);
	return;
}

void ImageEditor::saveSettings(Project *proj) const
{
	proj->config.Write(_T("editor-image/zoom"), (int)this->settings.zoomFactor);
	return;
}

bool ImageEditor::isFormatSupported(const wxString& type) const
{
	std::string strType("img-");
	strType.append(type.ToUTF8());
	return !!this->studio->mgrGraphics->getImageTypeByCode(strType);
}

IDocument *ImageEditor::openObject(const GameObjectPtr& o)
{
	try {
		ImagePtr image = this->studio->openImage(o);
		if (!image) return NULL; // user cancelled

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
			if (image->getCaps() & Image::HasPalette) {
				pal = image->getPalette();
				if (pal->size() == 0) {
					std::cout << "[editor-image] Palette contains no entries, using default\n";
					// default is loaded below
				}
			}
		}
		if ((!pal) || (pal->size() == 0)) {
			// Need to use the default palette
			switch (image->getCaps() & Image::ColourDepthMask) {
				case Image::ColourDepthVGA:
					pal = createPalette_DefaultVGA();
					break;
				case Image::ColourDepthEGA:
					pal = createPalette_DefaultEGA();
					break;
				case Image::ColourDepthCGA:
					pal = createPalette_CGA(CGAPal_CyanMagenta);
					break;
				case Image::ColourDepthMono:
					pal = createPalette_DefaultMono();
					break;
			}
		}

		return new ImageDocument(this->studio, &this->settings, image, pal);
	} catch (const camoto::stream::error& e) {
		throw EFailure(wxString::Format(_("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}
}
