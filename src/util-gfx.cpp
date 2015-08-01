/**
 * @file  util-gfx.cpp
 * @brief Graphics-related utility functions.
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

#include "util-gfx.hpp"

using namespace camoto::gamegraphics;

Cairo::RefPtr<Cairo::ImageSurface> createCairoSurface(const Image *ggimg,
	const Tileset *ggtileset)
{
	auto rawimg = ggimg->convert();
	auto rawmask = ggimg->convert_mask();
	std::shared_ptr<const Palette> ggpal;
	if (ggimg->caps() & Image::Caps::HasPalette) {
		ggpal = ggimg->palette();
	} else {
		if (ggtileset) {
			// We are within a tileset, see if that has a palette
			if (ggtileset->caps() & Tileset::Caps::HasPalette) {
				ggpal = ggtileset->palette();
			}
		}
	}
	if (!ggpal) {
		// No image or tileset palette, use default for image colour depth
		switch (ggimg->colourDepth()) {
			case ColourDepth::Mono: ggpal = createPalette_DefaultMono(); break;
			case ColourDepth::CGA: ggpal = createPalette_CGA(CGAPaletteType::CyanMagenta); break;
			case ColourDepth::EGA: ggpal = createPalette_DefaultEGA(); break;
			case ColourDepth::VGA: ggpal = createPalette_DefaultVGA(); break;
		}
	}

	auto dims = ggimg->dimensions();
	auto cimg = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, dims.x, dims.y);
	auto cdata = cimg->get_data();
	auto in = &rawimg[0];
	auto in_mask = &rawmask[0];
	auto stride = Cairo::ImageSurface::format_stride_for_width(Cairo::FORMAT_ARGB32, dims.x);
	for (unsigned int y = 0; y < dims.y; y++) {
		uint32_t *out = (uint32_t*)&cdata[y * stride];
		for (unsigned int x = 0; x < dims.x; x++) {
			auto& pix = ggpal->at(*in);
			*out = (pix.red << 16)
				| (pix.green  <<  8)
				| (pix.blue   <<  0);

			if (!(((Image::Mask)*in_mask) & Image::Mask::Transparent)) {
				// Pixel is NOT transparent, use palette transparency
				*out |= pix.alpha << 24;
			}
			out++;

			in++;
			in_mask++;
		}
	}
	return cimg;
}
