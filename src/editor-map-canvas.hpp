/**
 * @file   editor-map-canvas.hpp
 * @brief  OpenGL surface where map is drawn.
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

#ifndef _EDITOR_MAP_CANVAS_HPP_
#define _EDITOR_MAP_CANVAS_HPP_

#ifdef HAVE_GL_GL_H
#include <GL/gl.h>
#else
#ifdef HAVE_OPENGL_GL_H
#include <OpenGL/gl.h>
#endif
#endif

#include <vector>
#include <camoto/gamemaps.hpp>
#include <camoto/gamegraphics.hpp>
#include <wx/glcanvas.h>
#include "main.hpp"

class MapCanvas;

#include "editor-map-document.hpp"

// How many *pixels* (i.e. irrespective of zoom) the mouse pointer can be moved
// out of the focus box without losing focus.  This is also the number of pixels
// either side of the border that the mouse will perform a resize action.
#define FOCUS_BOX_PADDING 2

class MapCanvas: public wxGLCanvas
{
	protected:
		struct MapObject
		{
			/*const*/ int minWidth, minHeight;
			/*const*/ int maxWidth, maxHeight;
			/*const*/ int tileWidth, tileHeight;
			int x, y, width, height;
				MapObject(int minWidth, int minHeight, int maxWidth, int maxHeight,
					int tileWidth, int tileHeight)
					throw () :
					minWidth(minWidth),
					minHeight(minHeight),
					maxWidth(maxWidth),
					maxHeight(maxHeight),
					tileWidth(tileWidth),
					tileHeight(tileHeight)
			{
			}
		};

		typedef std::vector<MapObject> MapObjectVector;

	public:
		std::vector<bool> visibleLayers;
		bool viewportVisible;

		MapCanvas(MapDocument *parent, camoto::gamemaps::Map2DPtr map,
			camoto::gamegraphics::VC_TILESET tileset, int *attribList)
			throw ();

		~MapCanvas()
			throw ();

		void setZoomFactor(int f)
			throw ();
		void showGrid(bool visible)
			throw ();
		void setActiveLayer(int layer);
		void onEraseBG(wxEraseEvent& ev);
		void onPaint(wxPaintEvent& ev);
		void onResize(wxSizeEvent& ev);
		void glReset();

		/// Redraw the document.  Used after toggling layers.
		void redraw()
			throw ();

		void getLayerTileSize(camoto::gamemaps::Map2D::LayerPtr layer,
			int *tileWidth, int *tileHeight)
			throw ();

		/// Focus the object (if any) under the mouse cursor.
		/**
		 * @param start
		 *   Begin the search for an object at the given location.  This allows
		 *   the search to begin at the first object, or also after the currently
		 *   selected object to allow cycling between all objects under the cursor.
		 *
		 * @return true if the object changed and a redraw is required, false if
		 *   no redraw needed.
		 */
		bool focusObject(MapObjectVector::iterator start);

		void onMouseMove(wxMouseEvent& ev);

		/// Perform primary action
		void onMouseDownLeft(wxMouseEvent& ev);

		/// Finish primary action
		void onMouseUpLeft(wxMouseEvent& ev);

		/// Begin selection
		void onMouseDownRight(wxMouseEvent& ev);

		/// End selection
		void onMouseUpRight(wxMouseEvent& ev);

		/// Begin drag scroll
		void onMouseDownMiddle(wxMouseEvent& ev);

		/// End drag scroll
		void onMouseUpMiddle(wxMouseEvent& ev);

		void onMouseCaptureLost(wxMouseCaptureLostEvent& ev);

		void onKeyDown(wxKeyEvent& ev);

	protected:
		MapDocument *doc;
		camoto::gamemaps::Map2DPtr map;
		camoto::gamegraphics::VC_TILESET tileset;

		typedef std::map<unsigned int, GLuint> TEXTURE_MAP;
		std::vector<TEXTURE_MAP> textureMap;

		int zoomFactor;   ///< Zoom level (1 == 1:1, 2 == 2:1/doublesize, etc.)
		bool gridVisible; ///< Draw a grid over the active layer?
		int activeLayer;  ///< Index of layer currently selected in layer palette

		int offX;         ///< Current X position (in pixels) to draw at (0,0)
		int offY;         ///< Current Y position (in pixels) to draw at (0,0)

		int scrollFromX;  ///< Mouse X pos when scroll/drag started, or -1 if not dragging
		int scrollFromY;  ///< Mouse Y pos of scrolling origin

		int selectFromX;  ///< Mouse X pos when selecting started, or -1 if not selecting
		int selectFromY;  ///< Mouse Y pos of selection origin

		int actionFromX;  ///< Mouse X pos when primary action started, or -1 if none
		int actionFromY;  ///< Mouse Y pos of primary action (left click/drag)

		int pointerX;     ///< Current mouse X co-ordinate
		int pointerY;     ///< Current mouse Y co-ordinate

		int resizeX;      ///< Is vert focus border under pointer? -1 == left, 0 == none, 1 == right
		int resizeY;      ///< Is horiz focus border under pointer? -1 == top, 0 == none, 1 == bottom

		MapObjectVector objects; ///< Currently known objects found in the map
		MapObjectVector::iterator focusedObject; ///< Object currently under mouse pointer

		DECLARE_EVENT_TABLE();

};

#endif // _EDITOR_MAP_CANVAS_HPP_
