/**
 * @file  tab-map2d.cpp
 * @brief Tab for editing 2D tile-based maps.
 *
 * Copyright (C) 2013-2015 Adam Nielsen <malvineous@shikadi.net>
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

#include <cassert>
#include <iostream>
#include <gtkmm.h>
#include <glibmm/i18n.h>
#include <camoto/gamemaps/util.hpp>
#include "main.hpp"
#include "util-gfx.hpp"
#include "ct-map2d-canvas.hpp"

using namespace camoto::gamegraphics;
using namespace camoto::gamemaps;


#include <cairomm/context.h>

DrawingArea_Map2D::DrawingArea_Map2D(BaseObjectType *obj,
	const Glib::RefPtr<Gtk::Builder>& refBuilder)
	:	Gtk::DrawingArea(obj)
{
	auto imgDigits = createCairoSurface(UtilImage::HexDigits);
	this->hexDigitDims.x = imgDigits->get_width() / 16;
	this->hexDigitDims.y = imgDigits->get_height();
	this->patDigits = Cairo::SurfacePattern::create(imgDigits);
}

DrawingArea_Map2D::~DrawingArea_Map2D()
{
}

void DrawingArea_Map2D::content(std::shared_ptr<camoto::gamemaps::Map2D> obj,
	TilesetCollection& allTilesets)
{
	this->obj = obj;
	this->allTilesets = allTilesets;

	// Set the canvas size to match the map size
	Point mapSize = {0, 0};
	for (auto& layer : obj->layers()) {
		Point layerSize, tileSize;
		getLayerDims(*this->obj, *layer, &layerSize, &tileSize);
		mapSize.x = std::max(mapSize.x, layerSize.x * tileSize.x);
		mapSize.y = std::max(mapSize.y, layerSize.y * tileSize.y);
	}
	this->set_size_request(mapSize.x, mapSize.y);
	return;
}

bool DrawingArea_Map2D::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	if (!this->obj) return false; // map2d instance not set yet

	for (auto& layer : this->obj->layers()) {
		// Figure out the layer size (in tiles) and the tile size
		Point layerSize, tileSize;
		getLayerDims(*this->obj, *layer, &layerSize, &tileSize);

		// Run through all items in the layer and render them one by one
		for (auto& t : layer->items()) {
			TileImage& thisTile = this->imgCache[t.code];
			if (!thisTile.loaded) {
				// Tile hasn't been cached yet, load it from the tileset
				Map2D::Layer::ImageFromCodeInfo imgType;
				try {
					imgType = layer->imageFromCode(t, this->allTilesets);
				} catch (const std::exception& e) {
					std::cerr << "Error loading image: " << e.what() << std::endl;
					imgType.type = Map2D::Layer::ImageFromCodeInfo::ImageType::Unknown;
				}
				// Display nothing by default, but could be changed to a question mark
				thisTile.dims = {0, 0};

				switch (imgType.type) {
					case Map2D::Layer::ImageFromCodeInfo::ImageType::Supplied: {
						assert(imgType.img);
						auto surface = createCairoSurface(imgType.img.get(), nullptr);
						thisTile.surfacePattern = Cairo::SurfacePattern::create(surface);
						thisTile.dims = imgType.img->dimensions();
						break;
					}
					case Map2D::Layer::ImageFromCodeInfo::ImageType::Blank:
						thisTile.dims = {0, 0};
						break;
//					case Map2D::Layer::ImageFromCodeInfo::ImageType::Unknown:
					case Map2D::Layer::ImageFromCodeInfo::ImageType::HexDigit: {
						int numDigits = 4;
						if (imgType.digit <= 0x1F) numDigits = 1;
						else if (imgType.digit <= 0x1FF) numDigits = 2;
						thisTile.dims = tileSize;//{this->hexDigitDims.x * numDigits, this->hexDigitDims.y};
						auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
							thisTile.dims.x, thisTile.dims.y);
						// Draw the digits
						auto crDigits = Cairo::Context::create(surface);

						unsigned int numberWidth = (this->hexDigitDims.x - 1) * numDigits + 1;
						Point origin = {
							(tileSize.x - numberWidth) / 2,
							(tileSize.y - this->hexDigitDims.y) / 2
						};
						// Start at the end of the number and draw digits from right-to-left
						// from the last (least significant) digit back to the first.
						crDigits->translate(origin.x + numberWidth, origin.y);
						for (int i = 0; i < numDigits; i++) {
							int digit = imgType.digit & (0xF << (i * 4));
							auto patMatrix = Cairo::identity_matrix();
							patMatrix.translate(this->hexDigitDims.x * digit, 0);

							crDigits->translate(-this->hexDigitDims.x, 0);
							crDigits->rectangle(0, 0, this->hexDigitDims.x, this->hexDigitDims.y);

							this->patDigits->set_matrix(patMatrix);
							crDigits->set_source(this->patDigits);
							crDigits->fill();
							// Overwrite the padding pixel for the next digit
							crDigits->translate(1, 0);
						}
						thisTile.surfacePattern = Cairo::SurfacePattern::create(surface);
						break;
					}
//					case Map2D::Layer::ImageFromCodeInfo::ImageType::Interactive:
					case Map2D::Layer::ImageFromCodeInfo::ImageType::NumImageTypes: // Avoid compiler warning about unhandled enum
						assert(false);
						break;
				}
				thisTile.loaded = true;
			}

			if ((thisTile.dims.x == 0) || (thisTile.dims.y == 0)) continue; // no image
			cr->save();
			cr->translate(t.pos.x * tileSize.x, t.pos.y * tileSize.y);
			cr->set_source(thisTile.surfacePattern);//, 0, 0);
			cr->paint();
			cr->restore();
		}
	}
	return true;
}
