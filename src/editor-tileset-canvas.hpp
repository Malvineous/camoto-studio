/**
 * @file   editor-tileset-canvas.hpp
 * @brief  Graphical display for tileset editor.
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

#ifndef _EDITOR_TILESET_CANVAS_HPP_
#define _EDITOR_TILESET_CANVAS_HPP_

#include <GL/glew.h>

#include <camoto/gamegraphics.hpp>
#include "editor-tileset.hpp"
#include <wx/glcanvas.h> // must be after editor-tileset.hpp?!

/// Information about a single texture/tile.
struct Texture
{
	GLuint glid;         ///< OpenGL texture ID
	unsigned int width;  ///< Image width
	unsigned int height; ///< Image height
};

/// Map some internal image code to an OpenGL texture ID
typedef std::map<unsigned int, Texture> TEXTURE_MAP;

/// OpenGL surface for drawing tiles.
class TilesetCanvas: public wxGLCanvas
{

	public:
		TilesetCanvas(wxWindow *parent, wxGLContext *glcx, int *attribList,
			int zoomFactor);

		~TilesetCanvas();

		/// Supply the OpenGL texture IDs to draw with.
		void setTextures(TEXTURE_MAP& tm);

		void onEraseBG(wxEraseEvent& ev);
		void onPaint(wxPaintEvent& ev);
		void onResize(wxSizeEvent& ev);
		void glReset();

		/// Redraw the document.
		void redraw();

		void setZoomFactor(int f);

		void setTilesX(int t);

		void setOffset(unsigned int o);

	protected:
		TEXTURE_MAP tm;       ///< Tiles to display
		int zoomFactor;       ///< Zoom level, 1 == 1:1, 2 == doublesize, etc.
		unsigned int tilesX;  ///< Number of tiles to draw before wrapping to the next row
		unsigned int offset;  ///< Number of tiles to skip drawing from the start of the tileset

		DECLARE_EVENT_TABLE();

};

#endif // _EDITOR_TILESET_CANVAS_HPP_
