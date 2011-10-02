/**
 * @file   editor-image.cpp
 * @brief  Image editor.
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
#include <wx/filedlg.h>
#include "png++/png.hpp"
#include "main.hpp"
#include "editor-image.hpp"

/// Default zoom level
#define CFG_DEFAULT_ZOOM 2

using namespace camoto;
using namespace camoto::gamegraphics;

class ImageCanvas: public wxPanel
{
	public:

		ImageCanvas(wxWindow *parent, ImagePtr image, int zoomFactor)
			throw () :
				wxPanel(parent, wxID_ANY),
				image(image),
				wximg(),
				zoomFactor(zoomFactor)
		{
			this->imageChanged();
		}

		void imageChanged()
		{
			StdImageDataPtr data = image->toStandard();
			this->image->getDimensions(&this->width, &this->height);

			// Load the palette
			PaletteTablePtr pal;
			if (image->getCaps() & Image::HasPalette) {
				pal = image->getPalette();
			} else {
				pal = createDefaultCGAPalette();
			}
			if (pal->size() == 0) {
				std::cout << "[editor-image] Palette contains no entries, using default\n";
				pal = createDefaultCGAPalette();
			}

			this->wximg = wxImage(width, height, false);
			uint8_t *pixels = this->wximg.GetData();
			if (pixels) {
				int size = width * height;
				for (int i = 0, p = 0; i < size; i++) {
					uint8_t clr = data[i];
					if (clr >= pal->size()) clr = 0;
					pixels[p++] = pal->at(clr).red;
					pixels[p++] = pal->at(clr).green;
					pixels[p++] = pal->at(clr).blue;
				}
			} else {
				std::cout << "[editor-image] Error getting pixels out of wxImage\n";
			}

			// Initialise the image
			this->setZoomFactor(this->zoomFactor);
		}

		~ImageCanvas()
			throw ()
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
			throw ()
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
		ImageDocument(IMainWindow *parent, ImageEditor::Settings *settings, ImagePtr image)
			throw () :
				IDocument(parent, _T("image")),
				settings(settings),
				image(image)
		{
			this->canvas = new ImageCanvas(this, image, this->settings->zoomFactor);

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
		}

		virtual void save()
			throw (std::ios::failure)
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
			wxString path = wxFileSelector(_T("Open image"),
				::path.lastUsed, wxEmptyString, _T(".png"),
#ifdef __WXMSW__
				_T("Supported image files (*.png)|*.png|All files (*.*)|*.*"),
#else
				_T("Supported image files (*.png)|*.png|All files (*)|*"),
#endif
				wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
			if (!path.empty()) {
				try {
					unsigned int width, height;
					this->image->getDimensions(&width, &height);

					png::image<png::index_pixel> png;
					try {
						png.read(path.mb_str());
					} catch (const png::error& e) {
						wxString depth;
						switch (this->image->getCaps() & Image::ColourDepthMask) {
							case Image::ColourDepthVGA: depth = _T("256-colour"); break;
							case Image::ColourDepthEGA: depth = _T("16-colour"); break;
							case Image::ColourDepthCGA: depth = _T("4-colour"); break;
							case Image::ColourDepthMono: depth = _T("monochrome"); break;
							default: depth = _T("<unknown depth>"); break;
						}
						wxMessageDialog dlg(this,
							wxString::Format(_T("Unsupported file.  Only indexed .png images "
								"are supported, and the colour depth must match the image "
								"being replaced (%s).\n\n[%s]"), depth.c_str(),
								wxString(e.what(), wxConvUTF8).c_str()),
							_T("Import image"), wxOK | wxICON_ERROR);
						dlg.ShowModal();
						return;
					}

					if ((png.get_width() != width) || (png.get_height() != height)) {
						wxMessageDialog dlg(this,
							_T("Bad image format.  The image to import must be the same "
								"dimensions as the image being replaced."),
							_T("Import image"), wxOK | wxICON_ERROR);
						dlg.ShowModal();
						return;
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
							case Image::ColourDepthVGA: depth = _T("256-colour"); break;
							case Image::ColourDepthEGA: depth = _T("16-colour"); break;
							case Image::ColourDepthCGA: depth = _T("4-colour"); break;
							case Image::ColourDepthMono: depth = _T("monochrome"); break;
							default: depth = _T("<unknown depth>"); break;
						}
						wxMessageDialog dlg(this,
							wxString::Format(_T("There are too many colours in the image "
								"being imported.  Only indexed .png images are supported, and "
								"the colour depth must match the image being replaced (in "
								"this case the image must be %s)."), depth.c_str()),
							_T("Import image"), wxOK | wxICON_ERROR);
						dlg.ShowModal();
						return;
					}

					bool hasTransparency = true;
					int pixelOffset = -1; // to account for palette #0 being inserted for use as transparency
					png::tRNS transparency = png.get_tRNS();
					if ((transparency.size() > 0) && (transparency[0] != 0)) {
						wxMessageDialog dlg(this,
							_T("Bad image format.  The image to import must have palette "
								"entry #0 and only this entry set to transparent, or no "
								"transparency at all."),
							_T("Import image"), wxOK | wxICON_ERROR);
						dlg.ShowModal();
						return;
					} else {
						hasTransparency = false;
						pixelOffset = 0;
					}

					int imgSizeBytes = width * height;
					uint8_t *imgData = new uint8_t[imgSizeBytes];
					uint8_t *maskData = new uint8_t[imgSizeBytes];
					StdImageDataPtr stdimg(imgData);
					StdImageDataPtr stdmask(maskData);

					for (int y = 0; y < height; y++) {
						for (int x = 0; x < width; x++) {
							uint8_t pixel = png[y][x];
							if (hasTransparency && (pixel == 0)) { // Palette #0 must be transparent
								maskData[y * width + x] = 0x01; // transparent
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
							PaletteEntry p(i->red, i->green, i->blue, 255);
							newPal->push_back(p);
						}
						this->image->setPalette(newPal);
					}
					this->image->fromStandard(stdimg, stdmask);

					// This overwrites the image directly, it can't be undone
					//this->isModified = true;
					this->canvas->imageChanged();
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

				// Export the image
				unsigned int width, height;
				this->image->getDimensions(&width, &height);

				StdImageDataPtr data = this->image->toStandard();
				StdImageDataPtr mask = this->image->toStandardMask();

				png::image<png::index_pixel> png(width, height);

				PaletteTablePtr srcPal;
				if (this->image->getCaps() & Image::HasPalette) {
					srcPal = this->image->getPalette();
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

				// Copy the pixel data across
				for (int y = 0; y < height; y++) {
					for (int x = 0; x < width; x++) {
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
			throw ()
		{
			this->settings->zoomFactor = f;
			this->canvas->setZoomFactor(f);
			return;
		}

	protected:
		ImageCanvas *canvas;
		ImagePtr image;
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


ImageEditor::ImageEditor(IMainWindow *parent)
	throw () :
		frame(parent),
		pManager(camoto::gamegraphics::getManager())
{
	// Default settings
	this->settings.zoomFactor = CFG_DEFAULT_ZOOM;
}

std::vector<IToolPanel *> ImageEditor::createToolPanes() const
	throw ()
{
	std::vector<IToolPanel *> panels;
	return panels;
}

void ImageEditor::loadSettings(Project *proj)
	throw ()
{
	proj->config.Read(_T("editor-image/zoom"), (int *)&this->settings.zoomFactor, CFG_DEFAULT_ZOOM);
	return;
}

void ImageEditor::saveSettings(Project *proj) const
	throw ()
{
	proj->config.Write(_T("editor-image/zoom"), (int)this->settings.zoomFactor);
	return;
}

bool ImageEditor::isFormatSupported(const wxString& type) const
	throw ()
{
	std::string strType("img-");
	strType.append(type.ToUTF8());
	return this->pManager->getImageTypeByCode(strType);
}

IDocument *ImageEditor::openObject(const wxString& typeMinor,
	iostream_sptr data, FN_TRUNCATE fnTrunc, const wxString& filename,
	SuppMap supp, const Game *game)
	throw (EFailure)
{
	ImageTypePtr pImageType;
	if (typeMinor.IsEmpty()) {
		throw EFailure(_T("No file type was specified for this item!"));
	} else {
		std::string strType("img-");
		strType.append(typeMinor.ToUTF8());
		ImageTypePtr pTestType(this->pManager->getImageTypeByCode(strType));
		if (!pTestType) {
			wxString wxtype(strType.c_str(), wxConvUTF8);
			throw EFailure(wxString::Format(_T("Sorry, it is not possible to edit this "
				"image as the \"%s\" format is unsupported.  (No handler for \"%s\")"),
				typeMinor.c_str(), wxtype.c_str()));
		}
		pImageType = pTestType;
	}

	assert(pImageType);
	std::cout << "[editor-image] Using handler for " << pImageType->getFriendlyName() << "\n";

	// Check to see if the file is actually in this format
	if (pImageType->isInstance(data) < ImageType::PossiblyYes) {
		std::string friendlyType = pImageType->getFriendlyName();
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

	// Open the image file
	try {
		FN_TRUNCATE fnTruncate = boost::bind<void>(truncate, filename.fn_str(), _1);
		ImagePtr pImage(pImageType->open(data, fnTruncate, suppData));
		assert(pImage);

		return new ImageDocument(this->frame, &this->settings, pImage);
	} catch (const std::ios::failure& e) {
		throw EFailure(wxString::Format(_T("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}
}
