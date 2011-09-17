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
#include "gamelist.hpp" // MapObject

// How many *pixels* (i.e. irrespective of zoom) the mouse pointer can be moved
// out of the focus box without losing focus.  This is also the number of pixels
// either side of the border that the mouse will perform a resize action.
#define FOCUS_BOX_PADDING 2

class MapCanvas: public wxGLCanvas
{
	protected:
		/// Instance of a MapObject in this map
		struct Object
		{
			/// The object to draw here.
			const MapObject *obj;

			/// Location of the object, in layer grid coordinates.
			int x;

			/// Location of the object, in layer grid coordinates.
			int y;

			/// Width of the object, in layer grid coordinates.
			int width;

			/// Height of the object, in layer grid coordinates.
			int height;

			/// True to use all 'left' tiles before any right.
			/**
			 * If the available width is too small to fit all the 'left' tiles as well
			 * as all the 'right' tiles, set this to true to give priority to the left
			 * tiles, drawing as many of them as possible at the expense of losing
			 * some right tiles.  Set to false for the opposite, drawing all right
			 * tiles on narrow objects even if it means losing some left tiles.
			 *
			 * If the object's minWidth is sufficient to hold both the left and right
			 * tiles, this option will have no effect.
			 */
			bool leftPriority;

			/// True to use all 'top' rows before any bottom.
			/**
			 * If the available height is too small to fit all the top rows as well as
			 * all the bottom rows, then set this to true to give priority to the top
			 * rows, drawing as many of them as possible at the expense of losing some
			 * bottom rows.  Set to false for the opposite, drawing all bottom rows on
			 * short objects even if it means losing some top rows.
			 *
			 * If the object's minHeight is sufficient to hold both the top and bottom
			 * rows, this option will have no effect.
			 */
			bool topPriority;

			/// Copy of the tiles currently being used to draw the object.
			/**
			 * This is used so that objects can be picked out of maps that don't
			 * exactly match what we would use to draw the object ourselves, without
			 * overwriting that object with our idea of what it should look like.
			 *
			 * Once an object is identified, its tiles are copied here and it is
			 * removed from the map layer (the tiles are set back to the default
			 * background tile.)  This way when the object is moved, it doesn't leave
			 * behind a copy of itself.
			 */
			int *tiles;
		};

		typedef std::vector<Object> ObjectVector;

	public:
		std::vector<bool> visibleLayers;     ///< Map layers
		enum Elements {
			ElViewport,
			ElPaths,
			ElementCount
		};
		bool visibleElements[ElementCount];  ///< Virtual layers (e.g. viewport)

		MapCanvas(MapDocument *parent, camoto::gamemaps::Map2DPtr map,
			camoto::gamegraphics::VC_TILESET tileset, int *attribList,
			const MapObjectVector *mapObjects)
			throw ();

		~MapCanvas()
			throw ();

		/// Set the zoom factor.
		/**
		 * @param f
		 *  Zoom multiplier.  1 == pixel perfect, 2 == double size.
		 */
		void setZoomFactor(int f)
			throw ();

		/// Show/hide the tile grid.
		/**
		 * @param visible
		 *   true to show the grid, false to hide it.
		 *
		 * @note Grid change is immediately visible as GL surface is redrawn before
		 *   returning.
		 */
		void showGrid(bool visible)
			throw ();

		/// Switch to tile editing mode.
		void setTileMode()
			throw ();

		/// Switch to object editing mode.
		void setObjMode()
			throw ();

		void setActiveLayer(int layer);
		void onEraseBG(wxEraseEvent& ev);
		void onPaint(wxPaintEvent& ev);
		void onResize(wxSizeEvent& ev);
		void glReset();

		/// Redraw the document.  Used after toggling layers.
		void redraw()
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
		bool focusObject(ObjectVector::iterator start);

		/// Paint the current selection (if any) at the given coordinates.
		/**
		 * @param x
		 *   X coordinate in pixels relative to top-left of canvas.
		 *
		 * @param y
		 *   Y coordinate in pixels relative to top-left of canvas.
		 */
		void paintSelection(int x, int y)
			throw ();

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

		/// Make sure the keyboard help reflects the current editor state.
		void updateHelpText()
			throw ();

	protected:
		MapDocument *doc;
		camoto::gamemaps::Map2DPtr map;
		camoto::gamegraphics::VC_TILESET tileset;
		const MapObjectVector *mapObjects;

		typedef std::map<unsigned int, GLuint> TEXTURE_MAP;
		std::vector<TEXTURE_MAP> textureMap;

		int zoomFactor;   ///< Zoom level (1 == 1:1, 2 == 2:1/doublesize, etc.)
		bool gridVisible; ///< Draw a grid over the active layer?
		enum {TileMode, ObjectMode} editingMode; ///< Current editing mode
		int activeLayer;  ///< Index of layer currently selected in layer palette

		int offX;         ///< Current X position (in pixels) to draw at (0,0)
		int offY;         ///< Current Y position (in pixels) to draw at (0,0)

		int scrollFromX;  ///< Mouse X pos when scroll/drag started, or -1 if not dragging
		int scrollFromY;  ///< Mouse Y pos of scrolling origin

		int selectFromX;  ///< Mouse X pos when selecting started, or -1 if not selecting
		int selectFromY;  ///< Mouse Y pos of selection origin

		int selHotX;      ///< X pos of selection hotspot
		int selHotY;      ///< Y pos of selection hotspot

		int actionFromX;  ///< Mouse X pos when primary action started, or -1 if none
		int actionFromY;  ///< Mouse Y pos of primary action (left click/drag)

		int pointerX;     ///< Current mouse X co-ordinate
		int pointerY;     ///< Current mouse Y co-ordinate

		int resizeX;      ///< Is vert focus border under pointer? -1 == left, 0 == none, 1 == right
		int resizeY;      ///< Is horiz focus border under pointer? -1 == top, 0 == none, 1 == bottom

		ObjectVector objects; ///< Currently known objects found in the map
		ObjectVector::iterator focusedObject; ///< Object currently under mouse pointer

		/// Details about the current selection in tile-mode.
		struct {
			int x;      ///< X-coordinate of original selection's top-right corner
			int y;      ///< Y-coordinate of original selection's top-right corner
			int width;  ///< Width of selection, in tiles
			int height; ///< Height of selection, in tiles
			int *tiles; ///< Array of tiles selected
		} selection;

		/// Details about the current selection when editing paths.
		struct path_point {
			camoto::gamemaps::Map2D::PathPtr path; ///< Selected path
			unsigned int start; ///< Which instance of the path?  Index into start point vector
			unsigned int point; ///< Which point along the path is selected? 0 means start point, 1 means index 0 in point vector
		};
		std::vector<path_point> pathSelection;

		DECLARE_EVENT_TABLE();

};

#endif // _EDITOR_MAP_CANVAS_HPP_
