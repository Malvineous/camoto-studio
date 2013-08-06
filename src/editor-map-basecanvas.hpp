/**
 * @file   editor-map-basecanvas.hpp
 * @brief  Shared functions for an OpenGL surface where map tiles are drawn.
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

#ifndef _EDITOR_MAP_BASECANVAS_HPP_
#define _EDITOR_MAP_BASECANVAS_HPP_

#include <wx/glcanvas.h>
#include <camoto/gamemaps.hpp>

// Some colours (R, G, B, A)
#define CLR_ATTR_BG           1.0, 0.0, 1.0, 1.0  ///< Attribute indicator background
#define CLR_ATTR_FG           1.0, 1.0, 0.0, 1.0  ///< Attribute indicator foreground

// Convert the given coordinate from client (window) coordinates
// into map coordinates.
#define CLIENT_TO_MAP_X(x) ((x) / this->zoomFactor + this->offX)
#define CLIENT_TO_MAP_Y(y) ((y) / this->zoomFactor + this->offY)

// Is the given point contained within the rectangle?
#define POINT_IN_RECT(px, py, x1, y1, x2, y2) \
	((px >= x1) && (px < x2) && \
	 (py >= y1) && (py < y2))

class MapBaseCanvas: public wxGLCanvas
{
	public:
		MapBaseCanvas(wxWindow *parent, int *attribList);
		~MapBaseCanvas();

		/// Set the zoom factor.
		/**
		 * @param f
		 *  Zoom multiplier.  1 == pixel perfect, 2 == double size.
		 */
		void setZoomFactor(int f);

		/// Redraw the document.  Used after toggling layers.
		virtual void redraw() = 0;

	protected:
		void drawMapItem(int pixelX, int pixelY, unsigned int tileWidth,
			unsigned int tileHeight, const camoto::gamemaps::Map2D::Layer::ItemPtr& item,
			GLuint textureId);

		int zoomFactor;   ///< Zoom level (1 == 1:1, 2 == 2:1/doublesize, etc.)
		int offX;         ///< Current X position (in pixels) to draw at (0,0)
		int offY;         ///< Current Y position (in pixels) to draw at (0,0)
};

#endif // _EDITOR_MAP_BASECANVAS_HPP_
