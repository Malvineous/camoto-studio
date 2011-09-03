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
#include "editor-image.hpp"

using namespace camoto;
using namespace camoto::gamegraphics;

class ImageCanvas: public wxPanel
{
	public:

		ImageCanvas(wxWindow *parent, ImagePtr image)
			throw () :
				wxPanel(parent, wxID_ANY),
				image(image),
				wximg()
		{
			StdImageDataPtr data = image->toStandard();
			unsigned int width, height;
			this->image->getDimensions(&width, &height);

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
			this->wxbmp = wxBitmap(this->wximg, -1);
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

			unsigned int imgWidth, imgHeight;
			this->image->getDimensions(&imgWidth, &imgHeight);

			int canvasWidth, canvasHeight;
			this->GetClientSize(&canvasWidth, &canvasHeight);

			// Draw a dark grey background behind the image
			dc.SetBrush(*wxMEDIUM_GREY_BRUSH);
			dc.SetPen(*wxMEDIUM_GREY_PEN);
			dc.DrawRectangle(imgWidth, 0, canvasWidth - imgWidth, canvasHeight);
			dc.DrawRectangle(0, imgHeight, imgWidth, canvasHeight - imgHeight);

			// Draw the image
			dc.DrawBitmap(this->wxbmp, 0, 0, false);
			return;
		}

	protected:
		ImagePtr image;
		wxImage wximg;
		wxBitmap wxbmp;

		DECLARE_EVENT_TABLE();

};

BEGIN_EVENT_TABLE(ImageCanvas, wxPanel)
	EVT_PAINT(ImageCanvas::onPaint)
	EVT_ERASE_BACKGROUND(ImageCanvas::onEraseBG)
END_EVENT_TABLE()

class ImageDocument: public IDocument
{
	public:
		ImageDocument(IMainWindow *parent, ImagePtr image)
			throw () :
				IDocument(parent, _T("image")),
				image(image)
		{
			this->canvas = new ImageCanvas(this, image);

			wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
			s->Add(this->canvas, 1, wxEXPAND);
			this->SetSizer(s);
		}

		virtual void save()
			throw (std::ios::failure)
		{
			throw std::ios::failure("Saving has not been implemented yet!");
		}


	protected:
		ImageCanvas *canvas;
		ImagePtr image;

		friend class LayerPanel;

};


ImageEditor::ImageEditor(IMainWindow *parent)
	throw () :
		frame(parent),
		pManager(camoto::gamegraphics::getManager())
{
}

std::vector<IToolPanel *> ImageEditor::createToolPanes() const
	throw ()
{
	std::vector<IToolPanel *> panels;
	return panels;
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
	SuppMap supp, const Game *game) const
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

		return new ImageDocument(this->frame, pImage);
	} catch (const std::ios::failure& e) {
		throw EFailure(wxString::Format(_T("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}
}
