/**
 * @file  util-gfx.hpp
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

#ifndef _UTIL_GFX_HPP_
#define _UTIL_GFX_HPP_

#include <cairomm/surface.h>
#include <camoto/gamegraphics/image.hpp>
#include <camoto/gamegraphics/tileset.hpp>

/// Copy a libgamegraphics Image instance into a new Cairo surface.
/**
 * @param ggimg
 *   Source image.
 *
 * @param ggtileset
 *   Optional tileset the image came from.  This can be nullptr it the image
 *   did not come from a tileset.  If specified, it is used to obtain the
 *   palette if the image does not contain its own and shares the same palette
 *   as the other tiles in the tileset.
 *
 * @return A Cairo ImageSurface matching the source image.
 */
Cairo::RefPtr<Cairo::ImageSurface> createCairoSurface(
	const camoto::gamegraphics::Image *ggimg,
	const camoto::gamegraphics::Tileset *ggtileset);

#endif // _UTIL_GFX_HPP_
