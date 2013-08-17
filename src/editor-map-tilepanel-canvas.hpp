/**
 * @file   editor-map-tilepanel-canvas.hpp
 * @brief  Graphical display for tile selection panel in map editor.
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

#ifndef _EDITOR_MAP_TILEPANEL_CANVAS_HPP_
#define _EDITOR_MAP_TILEPANEL_CANVAS_HPP_

class TilePanelCanvas;

#include <GL/glew.h>
#include "editor-map-tilepanel.hpp"
#include <wx/glcanvas.h>
#include "editor-map-basecanvas.hpp"

/// Specialisation of tileset viewer for map tile selector panel.
class TilePanelCanvas: public MapBaseCanvas
{
	public:
		TilePanelCanvas(TilePanel *parent, wxGLContext *glcx, int *attribList);
		~TilePanelCanvas();

		/// Alert that the document has changed, and trigger a redraw of the tiles.
		void notifyNewDoc();

		void onEraseBG(wxEraseEvent& ev);
		void onPaint(wxPaintEvent& ev);
		void onResize(wxSizeEvent& ev);
		void glReset();

		/// Redraw the document.
		virtual void redraw();

		void setTilesX(int t);
		void setOffset(unsigned int o);

		void onRightClick(wxMouseEvent& ev);
		virtual void onMouseMove(wxMouseEvent& ev);
		void clampScroll();
		virtual void calcCurrentExtents();

	protected:
		typedef boost::function<bool (int pixelX, int pixelY, const
			camoto::gamemaps::Map2D::Layer::ItemPtr item, const Texture *texture)>
			fn_foralltiles;

		void drawCanvas();
		void forAllTiles(fn_foralltiles fnCallback, bool inclHidden);

		bool updateExtent(int pixelX, int pixelY,
			const camoto::gamemaps::Map2D::Layer::ItemPtr item,
			const Texture *texture);

		bool testSelectMapItem(MapCanvas *mapCanvas,
			camoto::gamemaps::Map2D::Layer::ItemPtrVector *selItems, int pointerX,
			int pointerY, int pixelX, int pixelY,
			const camoto::gamemaps::Map2D::Layer::ItemPtr item,
			const Texture *texture);

		wxGLContext *glcx;
		TilePanel *tilePanel;

		unsigned int tilesX;  ///< Number of tiles to draw before wrapping to the next row
		unsigned int offset;  ///< Number of tiles to skip drawing from the start of the tileset

		unsigned int fullWidth;  ///< Width of all tiles in current arrangement, in pixels
		unsigned int fullHeight; ///< Height of all tiles in current arrangement, in pixels
		unsigned int maxScrollX; ///< Maximum distance permitted to scroll horizontally
		unsigned int maxScrollY; ///< Maximum distance permitted to scroll vertically

		friend TilePanel; // access for loading/saving settings

		DECLARE_EVENT_TABLE();
};

#endif // _EDITOR_MAP_TILEPANEL_CANVAS_HPP_
