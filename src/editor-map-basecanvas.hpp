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
#include "editor-tileset-canvas.hpp" // for Texture

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
		MapBaseCanvas(wxWindow *parent, wxGLContext *glcx, int *attribList);
		~MapBaseCanvas();

		/// Set the zoom factor.
		/**
		 * @param f
		 *  Zoom multiplier.  1 == pixel perfect, 2 == double size.
		 */
		virtual void setZoomFactor(int f);

		/// Show/hide the tile grid.
		/**
		 * @param visible
		 *   true to show the grid, false to hide it.
		 *
		 * @note Grid change is immediately visible as GL surface is redrawn before
		 *   returning.
		 */
		void setGridVisible(bool visible);

		/// Draw the tile grid onto the OpenGL canvas.
		/**
		 * @param divWidth
		 *   Horizontal distance between vertical gridlines, in pixels, ignoring zoom level.
		 *
		 * @param divHeight
		 *   Vertical distance between horizontal gridlines, in pixels, ignoring zoom level.
		 */
		void drawGrid(unsigned int divWidth, unsigned int divHeight);

		/// Handle drag scroll
		virtual void onMouseMove(wxMouseEvent& ev);

		/// Begin drag scroll
		void onMouseDownMiddle(wxMouseEvent& ev);

		/// End drag scroll
		void onMouseUpMiddle(wxMouseEvent& ev);

		/// Abort drag scroll
		void onMouseCaptureLost(wxMouseCaptureLostEvent& ev);

		/// Recalculate the current scrolling extents.  Default is a no-op.
		virtual void calcCurrentExtents();

		/// Redraw the document.  Used after toggling layers.
		virtual void redraw() = 0;

	protected:
		bool drawMapItem(int pixelX, int pixelY, unsigned int tileWidth,
			unsigned int tileHeight,
			const camoto::gamemaps::Map2D::Layer::ItemPtr item,
			const Texture *texture);

		/// Load a texture from a .png image.
		/**
		 * @param name
		 *   Base filename of .png, not including path or extension.  Images are
		 *   loaded from path given by paths::mapIndicators.
		 */
		Texture loadTileFromFile(const char *name);

		wxGLContext *glcx;
		int zoomFactor;   ///< Zoom level (1 == 1:1, 2 == 2:1/doublesize, etc.)
		int offX;         ///< Current X position (in pixels) to draw at (0,0)
		int offY;         ///< Current Y position (in pixels) to draw at (0,0)
		bool gridVisible; ///< Draw a grid when requested?
		bool needRedraw;  ///< Do we need to redraw at the end of processing?

		Texture indicators[camoto::gamemaps::Map2D::Layer::NumImageTypes]; ///< Images for each ImageType

	private:
		bool scrolling;   ///< True if currently scrolling with middle-drag
		int scrollFromX;  ///< Mouse X pos when scroll/drag started, or -1 if not dragging
		int scrollFromY;  ///< Mouse Y pos of scrolling origin

		DECLARE_EVENT_TABLE();
};

#endif // _EDITOR_MAP_BASECANVAS_HPP_
