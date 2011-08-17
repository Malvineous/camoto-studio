/**
 * @file   editor-map-canvas.cpp
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

#include "editor-map-canvas.hpp"

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

// How many *pixels* (i.e. irrespective of zoom) the mouse pointer can be moved
// out of the focus box without losing focus.  This is also the number of pixels
// either side of the border that the mouse will perform a resize action.
#define FOCUS_BOX_PADDING 2

int matchTileRun(int *tile, const MapObject::TileRun *run)
{
	int matched = 0;
	for (MapObject::TileRun::const_iterator t = run->begin(); t != run->end(); t++) {
		if ((*t != -1) && (*t != *tile)) break; // no more matches
		tile++; matched++;
	}
	return matched;
}

int matchAnyTileRun(int *tile, const MapObject::TileRun *run)
{
	int matched = 0;
	bool found;
	do {
		found = false;
		for (MapObject::TileRun::const_iterator t = run->begin(); t != run->end(); t++) {
			if ((*t == -1) || (*t == *tile)) {
				found = true;
				matched++;
				tile++;
				break;
			}
		}
	} while (found);
	return matched;
}

BEGIN_EVENT_TABLE(MapCanvas, wxGLCanvas)
	EVT_PAINT(MapCanvas::onPaint)
	EVT_ERASE_BACKGROUND(MapCanvas::onEraseBG)
	EVT_SIZE(MapCanvas::onResize)
	EVT_MOTION(MapCanvas::onMouseMove)
	EVT_LEFT_DOWN(MapCanvas::onMouseDownLeft)
	EVT_LEFT_UP(MapCanvas::onMouseUpLeft)
	EVT_RIGHT_DOWN(MapCanvas::onMouseDownRight)
	EVT_RIGHT_UP(MapCanvas::onMouseUpRight)
	EVT_MIDDLE_DOWN(MapCanvas::onMouseDownMiddle)
	EVT_MIDDLE_UP(MapCanvas::onMouseUpMiddle)
	EVT_MOUSE_CAPTURE_LOST(MapCanvas::onMouseCaptureLost)
	EVT_KEY_DOWN(MapCanvas::onKeyDown)
END_EVENT_TABLE()

MapCanvas::MapCanvas(MapDocument *parent, Map2DPtr map, VC_TILESET tileset,
	int *attribList, const MapObjectVector *mapObjects)
	throw () :
		wxGLCanvas(parent, wxID_ANY, wxDefaultPosition,
			wxDefaultSize, wxTAB_TRAVERSAL | wxWANTS_CHARS, wxEmptyString, attribList),
		doc(parent),
		map(map),
		tileset(tileset),
		mapObjects(mapObjects),
		zoomFactor(2),
		gridVisible(false),
		activeLayer(0),
		offX(0),
		offY(0),
		scrollFromX(-1),
		// scrollFromY doesn't matter when X is -1
		selectFromX(-1),
		// selectFromY doesn't matter when X is -1
		actionFromX(-1),
		// actionFromY doesn't matter when X is -1
		pointerX(0),
		pointerY(0)
{
	assert(tileset.size() > 0);
	// Initial state is all layers visible
	int layerCount = map->getLayerCount();
	for (int i = 0; i < layerCount; i++) {
		this->visibleLayers.push_back(true);
	}
	this->viewportVisible = this->map->getCaps() & Map2D::HasViewport;

	this->SetCurrent();
	glClearColor(0.5, 0.5, 0.5, 1);
	glShadeModel(GL_FLAT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	PaletteTablePtr palDefault;
	if (tileset[0]->getCaps() & Tileset::HasPalette) {
		palDefault = tileset[0]->getPalette();
		assert(palDefault);
	} else {
		// Load default palette
		palDefault = createDefaultCGAPalette();
	}

	// TODO: Load unknown/default tile the same size (if possible) as the grid
	GLuint unknownTile = 0;

	for (int i = 0; i < layerCount; i++) {
		Map2D::LayerPtr layer = this->map->getLayer(i);
		Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
		TEXTURE_MAP tm;
		for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
			GLuint t;
			glGenTextures(1, &t);
			glBindTexture(GL_TEXTURE_2D, t);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			unsigned int code = (*c)->code;
			if (tm.find(code) == tm.end()) {
				// This tile code doesn't have an associated image yet
				ImagePtr image = layer->imageFromCode(code, tileset);
				if (image) {
					// Got an image for this tile code

					unsigned int width, height;
					image->getDimensions(&width, &height);
					if ((width > 512) || (height > 512)) continue; // image too large

					PaletteTablePtr pal;
					if (image->getCaps() & Image::HasPalette) {
						pal = image->getPalette();
					} else {
						pal = palDefault;
					}
					assert(pal);
					StdImageDataPtr data = image->toStandard();
					StdImageDataPtr mask = image->toStandardMask();
					boost::shared_array<uint32_t> combined(new uint32_t[width * height]);
					uint8_t *d = data.get(), *m = mask.get();
					uint8_t *c = (uint8_t *)combined.get();
					for (int p = 0; p < width * height; p++) {
						*c++ = *m & 0x01 ? 255 : (*pal)[*d].blue;
						*c++ = *m & 0x01 ?   0 : (*pal)[*d].green;
						*c++ = *m & 0x01 ? 255 : (*pal)[*d].red;
						*c++ = *m & 0x01 ?   0 : (*pal)[*d].alpha;
						m++; d++;
					}

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, combined.get());
					if (glGetError()) std::cerr << "[editor-tileset] GL error loading texture into id " << t << std::endl;
				} else {
					std::cerr << "[editor-map] Could not get an image for tile "
						"code " << code << std::endl;
					glDeleteTextures(1, &t);
					t = unknownTile;
				}
				tm[code] = t;
			}
		}
		this->textureMap.push_back(tm);
	}

	this->glReset();

	// TEMP: this does layer 0 only!
	Map2D::LayerPtr layer = this->map->getLayer(0); // TEMP

	int layerWidth, layerHeight;
	if (layer->getCaps() & Map2D::Layer::HasOwnSize) {
		layer->getLayerSize(&layerWidth, &layerHeight);
	} else {
		// Use global map size
		if (this->map->getCaps() & Map2D::HasGlobalSize) {
			this->map->getMapSize(&layerWidth, &layerHeight);
		} else {
			std::cout << "[editor-map] BUG: Layer uses map size, but map "
				"doesn't have a global size!\n";
		}
		//continue;
	}
	int tileWidth, tileHeight;
	this->getLayerTileSize(layer, &tileWidth, &tileHeight);

	// Load the layer into an array
	int *tile = new int[layerWidth * layerHeight];
	for (int i = 0; i < layerWidth * layerHeight; i++) tile[i] = -1;
	Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
	for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
		tile[(*c)->y * layerWidth + (*c)->x] = (*c)->code;
	}
	// tile[] now uses -1 for no-tile-present and the actual tile code otherwise.

	for (int y = 0; y < layerHeight; y++) {
		for (int x = 0; x < layerWidth; x++) {
			int startCode = tile[y * layerWidth + x];
			if (startCode == 0) continue; // skip empty tiles
			for (MapObjectVector::const_iterator i = this->mapObjects->begin();
				i != this->mapObjects->end();
				i++
			) {
				int y2 = y;
				int x2 = x;

				// Start tracking this in case it turns out to be an actual object
				Object newObject;
				newObject.obj = &(*i);
				newObject.x = x;
				newObject.y = y;
				newObject.height = 0;
				newObject.width = 0;

				bool done = false;

				for (int s = 0; s < MapObject::SectionCount; s++) {
					if ((!done) && (i->section[s].size() > 0)) {
						// There are top/mid/bot rows, see if they match the map
						for (MapObject::RowVector::const_iterator oY = i->section[s].begin(); oY != i->section[s].end(); oY++) {
							int leftMatched = matchTileRun(&tile[y2 * layerWidth + x2], &oY->segment[MapObject::Row::L]);

							// If no tiles were matched, then the current tile is not a starting
							// point for one of these objects.
							if (leftMatched == 0) {
								done = true; // don't search any more rows

								// But in the case of the mid section, the rows can match in any
								// order so try the next mid section's row.
								if (s == MapObject::MidSection) continue;
								else break; // but not for top/bot, one mismatch and that's it
							}
							x2 += leftMatched;

							// Try to match zero or more middle tiles, as many as possible, in
							// any order.
							int midMatched = matchAnyTileRun(&tile[y2 * layerWidth + x2], &oY->segment[MapObject::Row::M]);
							x2 += midMatched;

							// Now match the right-size tiles, in order.
							int rightMatched = matchTileRun(&tile[y2 * layerWidth + x2], &oY->segment[MapObject::Row::R]);
							x2 += rightMatched;

							// Otherwise this row's left-side matched, so consider it an object
							newObject.height++;

							// Now we should have as many tiles as are valid on the top row
							int width = x2 - x;
							if (newObject.width < width) newObject.width = width;

							x2 = x;
							y2++;
							done = false; // this row matched
						}

						// Repeat the mid section if need be
						if (s == MapObject::MidSection) {
							if (!done) {
								// Matched mid, try again
								done = false;
								s--;
							} // else no more matches on mid, continue on with end section
							done = false;
						}
					}

				} // for (top/mid/bot sections)

				// If this did turn out to be an object, add it to the list.
				if (newObject.height > 0) {
					this->objects.push_back(newObject);
				}
			}
		}
	}

	// Start with no object selected
	this->focusedObject = this->objects.end();
}

MapCanvas::~MapCanvas()
	throw ()
{
	for (std::vector<TEXTURE_MAP>::iterator l = this->textureMap.begin(); l != this->textureMap.end(); l++) {
		for (TEXTURE_MAP::iterator t = l->begin(); t != l->end(); t++) {
			glDeleteTextures(1, &t->second);
		}
	}
}

void MapCanvas::setZoomFactor(int f)
	throw ()
{
	// Keep the viewport centred after the zoom
	wxSize s = this->GetClientSize();
	int centreX = s.x / 2, centreY = s.y / 2;
	int absX = this->offX + centreX / this->zoomFactor;
	int absY = this->offY + centreY / this->zoomFactor;
	this->zoomFactor = f;
	this->offX = absX - centreX / this->zoomFactor;
	this->offY = absY - centreY / this->zoomFactor;
	this->glReset();
	this->redraw();
	return;
}

void MapCanvas::showGrid(bool visible)
	throw ()
{
	this->gridVisible = visible;
	this->glReset();
	this->redraw();
	return;
}

void MapCanvas::setActiveLayer(int layer)
{
	this->activeLayer = layer;
	this->glReset();
	this->redraw();
	return;
}

void MapCanvas::onEraseBG(wxEraseEvent& ev)
{
	return;
}

void MapCanvas::onPaint(wxPaintEvent& ev)
{
	wxPaintDC dummy(this);
	this->redraw();
	return;
}

void MapCanvas::onResize(wxSizeEvent& ev)
{
	this->Layout();
	this->glReset();
	return;
}

void MapCanvas::glReset()
{
	this->SetCurrent();
	wxSize s = this->GetClientSize();
	glViewport(0, 0, s.x, s.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, s.x / this->zoomFactor, s.y / this->zoomFactor, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	return;
}

void MapCanvas::redraw()
	throw ()
{
	wxSize s = this->GetClientSize();
	s.x /= this->zoomFactor;
	s.y /= this->zoomFactor;

	// Limit scroll range (this is here for efficiency as we already have the
	// window size, but it means you can scroll past the bounds of the map
	// during the scroll operation and the limits don't kick in until the
	// the scroll is complete.)  It does mean you won't get limited while
	// scrolling though, so I'm calling it a feature.
	if (this->offX < -s.x) this->offX = -s.x;
	if (this->offY < -s.y) this->offY = -s.y;
	// TODO: Prevent scrolling past the end of the map too

	this->SetCurrent();
	glClearColor(0.5, 0.5, 0.5, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_LINE_STIPPLE);
	glEnable(GL_TEXTURE_2D);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	int layerCount = this->map->getLayerCount();

	for (int i = 0; i < layerCount; i++) {
		if (this->visibleLayers[i]) {
			Map2D::LayerPtr layer = this->map->getLayer(i);

			int layerWidth, layerHeight;
			if (layer->getCaps() & Map2D::Layer::HasOwnSize) {
				layer->getLayerSize(&layerWidth, &layerHeight);
			} else {
				// Use global map size
				if (this->map->getCaps() & Map2D::HasGlobalSize) {
					this->map->getMapSize(&layerWidth, &layerHeight);
				} else {
					std::cout << "[editor-map] BUG: Layer uses map size, but map "
						"doesn't have a global size!\n";
					continue;
				}
			}

			int tileWidth, tileHeight;
			this->getLayerTileSize(layer, &tileWidth, &tileHeight);

			Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
			for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {

				int tileX = (*c)->x * tileWidth;
				int tileY = (*c)->y * tileHeight;

				if (tileX > offX + s.x) continue; // tile is off viewport to the right
				if (tileX + tileWidth < offX) continue; // tile is off viewport to the left
				if (tileY > offY + s.y) continue; // tile is off viewport to the bottom
				if (tileY + tileHeight < offY) continue; // tile is off viewport to the top

				glBindTexture(GL_TEXTURE_2D, this->textureMap[i][(*c)->code]);
				glBegin(GL_QUADS);
				glTexCoord2d(0.0, 0.0);
				glVertex2i(tileX - offX, tileY - offY);
				glTexCoord2d(0.0, 1.0);
				glVertex2i(tileX - offX, tileY + tileHeight - offY);
				glTexCoord2d(1.0, 1.0);
				glVertex2i(tileX + tileWidth - offX, tileY + tileHeight - offY);
				glTexCoord2d(1.0, 0.0);
				glVertex2i(tileX + tileWidth - offX, tileY - offY);
				glEnd();
			}
		}
	}

	glDisable(GL_TEXTURE_2D);

	// Draw the viewport overlay
	if (this->viewportVisible) {
		int vpX, vpY;
		this->map->getViewport(&vpX, &vpY);
		int vpOffX = (s.x - vpX) / 2;
		int vpOffY = (s.y - vpY) / 2;

		// Draw a line around the viewport
		glColor4f(1.0, 1.0, 1.0, 0.3);
		glBegin(GL_LINE_LOOP);
		glVertex2i(vpOffX, vpOffY + 0);
		glVertex2i(vpOffX, vpOffY + vpY);
		glVertex2i(vpOffX + vpX, vpOffY + vpY);
		glVertex2i(vpOffX + vpX, vpOffY + 0);
		glEnd();

		// Darken the area outside the viewport
		glColor4f(0.0, 0.0, 0.0, 0.3);
		glBegin(GL_QUAD_STRIP);
		glVertex2i(0, 0);
		glVertex2i(vpOffX, vpOffY);
		glVertex2i(0, s.y);
		glVertex2i(vpOffX, vpOffY + vpY);
		glVertex2i(s.x, s.y);
		glVertex2i(vpOffX + vpX, vpOffY + vpY);
		glVertex2i(s.x, 0);
		glVertex2i(vpOffX + vpX, vpOffY);
		glVertex2i(0, 0);
		glVertex2i(vpOffX, vpOffY);
		glEnd();
	}

	// Do some operations which require a real map layer to be selected (as
	// opposed to the viewport or other virtual layers.)
	int tileWidth, tileHeight;
	if (this->activeLayer >= 0) {
		Map2D::LayerPtr layer = this->map->getLayer(this->activeLayer);
		this->getLayerTileSize(layer, &tileWidth, &tileHeight);

		// If the grid is visible, draw it using the tile size of the active layer
		if (this->gridVisible) {
			wxSize s = this->GetClientSize();

			glEnable(GL_COLOR_LOGIC_OP);
			glLogicOp(GL_AND_REVERSE);
			glColor4f(0.3, 0.3, 0.3, 0.5);
			glBegin(GL_LINES);
			for (int x = -offX % tileWidth; x < s.x; x += tileWidth) {
				glVertex2i(x, 0);
				glVertex2i(x, s.y);
			}
			for (int y = -offY % tileHeight; y < s.y; y += tileHeight) {
				glVertex2i(0,   y);
				glVertex2i(s.x, y);
			}
			glEnd();
			glDisable(GL_COLOR_LOGIC_OP);
		}

		// If an object is currently focused, draw a box around it
		if (this->focusedObject != this->objects.end())  {
			glColor4f(1.0, 1.0, 1.0, 0.75);
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(3, 0xAAAA);
			glBegin(GL_LINE_LOOP);
			int x1 = this->focusedObject->x * tileWidth - this->offX;
			int y1 = this->focusedObject->y * tileHeight - this->offY;
			int x2 = x1 + this->focusedObject->width * tileWidth;
			int y2 = y1 + this->focusedObject->height * tileHeight;
			glVertex2i(x1, y1);
			glVertex2i(x1, y2);
			glVertex2i(x2, y2);
			glVertex2i(x2, y1);
			glEnd();

			glColor4f(0.0, 0.0, 0.0, 0.75);
			glLineStipple(3, 0x5555);
			glBegin(GL_LINE_LOOP);
			glVertex2i(x1, y1);
			glVertex2i(x1, y2);
			glVertex2i(x2, y2);
			glVertex2i(x2, y1);
			glEnd();
		}

	} // if (there is an active layer)

	// Draw the selection rectangle
	if (this->selectFromX >= 0) {
		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp(GL_XOR);
		glColor4f(0.5, 0.5, 0.5, 1.0);
		if (this->selectFromX > this->pointerX) {
			// Negative rectangle, use a dashed line
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(3, 0xAAAA);
		} else {
			glDisable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINE_LOOP);
		int x1 = this->selectFromX / this->zoomFactor;
		int y1 = this->selectFromY / this->zoomFactor;
		int x2 = this->pointerX / this->zoomFactor;
		int y2 = this->pointerY / this->zoomFactor;
		glVertex2i(x1, y1);
		glVertex2i(x1, y2);
		glVertex2i(x2, y2);
		glVertex2i(x2, y1);
		glEnd();
		glDisable(GL_COLOR_LOGIC_OP);
	}

	this->SwapBuffers();
	return;
}

void MapCanvas::getLayerTileSize(Map2D::LayerPtr layer, int *tileWidth, int *tileHeight)
	throw ()
{
	if (layer->getCaps() & Map2D::Layer::HasOwnTileSize) {
		layer->getTileSize(tileWidth, tileHeight);
	} else {
		// Use global tile size
		if (this->map->getCaps() & Map2D::HasGlobalTileSize) {
			this->map->getTileSize(tileWidth, tileHeight);
		} else {
			std::cout << "[editor-map] BUG: Layer uses map's tile size, but "
				"map doesn't have a global tile size!\n";
			*tileWidth = *tileHeight = 64; // use some really weird value :-)
			return;
		}
	}
	return;
}

bool MapCanvas::focusObject(ObjectVector::iterator start)
{
	bool needRedraw = false;

	// See if the mouse is over an object
	int mapPointerX = this->pointerX / this->zoomFactor + this->offX;
	int mapPointerY = this->pointerY / this->zoomFactor + this->offY;

	ObjectVector::iterator oldFocusedObject = this->focusedObject;
	ObjectVector::iterator end = this->objects.end();
	this->focusedObject = this->objects.end(); // default to nothing focused

	// No active layer.  Only return true (redraw) if this caused
	// the selected object to be deselected.
	if (this->activeLayer < 0) return (this->focusedObject != oldFocusedObject);

	Map2D::LayerPtr layer = this->map->getLayer(this->activeLayer);
	int tileWidth, tileHeight;
	this->getLayerTileSize(layer, &tileWidth, &tileHeight);

	for (int n = 0; n < 2; n++) {
		for (ObjectVector::iterator i = start; i != end; i++) {
			if (
				(mapPointerX >= i->x * tileWidth - FOCUS_BOX_PADDING) &&
				(mapPointerX < (i->x + i->width) * tileWidth + FOCUS_BOX_PADDING) &&
				(mapPointerY >= i->y * tileHeight - FOCUS_BOX_PADDING) &&
				(mapPointerY < (i->y + i->height) * tileHeight + FOCUS_BOX_PADDING)
			) {
				// The pointer is over this object
				this->focusedObject = i;
				break;
			}
		}
		if (this->focusedObject != this->objects.end()) break;

		// If we didn't begin at the start of the object list, go back there and
		// keep searching up until we reach the point we actually started at.
		if (start == this->objects.begin()) break;
		end = start;
		start = this->objects.begin();
	}
	if (this->focusedObject != oldFocusedObject) needRedraw = true;

	// See if the cursor is over the border of the focus box
	if (this->focusedObject != this->objects.end()) {
		this->resizeX = 0;
		this->resizeY = 0;
		if (
			(mapPointerX >= this->focusedObject->x * tileWidth - FOCUS_BOX_PADDING) &&
			(mapPointerX < this->focusedObject->x * tileWidth + FOCUS_BOX_PADDING)
		) {
			this->resizeX = -1;
		} else if (
			(mapPointerX >= (this->focusedObject->x + this->focusedObject->width) * tileWidth - FOCUS_BOX_PADDING) &&
			(mapPointerX < (this->focusedObject->x + this->focusedObject->width) * tileWidth + FOCUS_BOX_PADDING)
		) {
			this->resizeX = 1;
		}

		if (
			(mapPointerY >= this->focusedObject->y * tileHeight - FOCUS_BOX_PADDING) &&
			(mapPointerY < this->focusedObject->y * tileHeight + FOCUS_BOX_PADDING)
		) {
			this->resizeY = -1;
		} else if (
			(mapPointerY >= (this->focusedObject->y + this->focusedObject->height) * tileHeight - FOCUS_BOX_PADDING) &&
			(mapPointerY < (this->focusedObject->y + this->focusedObject->height) * tileHeight + FOCUS_BOX_PADDING)
		) {
			this->resizeY = 1;
		}

		// Prevent the resize in one or more axes if the object's dimensions in that
		// axis are fixed.  This is just UI sugar as the object couldn't be resized
		// anyway.
		if (
			(this->focusedObject->obj->maxWidth == this->focusedObject->obj->minWidth) &&
			(this->focusedObject->obj->maxWidth == this->focusedObject->width)
		) {
			this->resizeX = 0;
		}
		if (
			(this->focusedObject->obj->maxHeight == this->focusedObject->obj->minHeight) &&
			(this->focusedObject->obj->maxHeight == this->focusedObject->height)
		) {
			this->resizeY = 0;
		}

		/*
		  resizeX resizeY cursor
		  -1      -1      \
		  -1       0      -
		  -1       1      /
		   0      -1      |
		   0       0      N/A
		   0       1      |
		   1      -1      /
		   1       0      -
		   1       1      \
		*/
		switch (this->resizeX) {
			case -1:
				switch (this->resizeY) {
					case -1: this->SetCursor(wxCURSOR_SIZENWSE); break;
					case  0: this->SetCursor(wxCURSOR_SIZEWE); break;
					case  1: this->SetCursor(wxCURSOR_SIZENESW); break;
				}
				break;
			case 0:
				switch (this->resizeY) {
					case -1: this->SetCursor(wxCURSOR_SIZENS); break;
					case  0: this->SetCursor(wxNullCursor); break;
					case  1: this->SetCursor(wxCURSOR_SIZENS); break;
				}
				break;
			case 1:
				switch (this->resizeY) {
					case -1: this->SetCursor(wxCURSOR_SIZENESW); break;
					case  0: this->SetCursor(wxCURSOR_SIZEWE); break;
					case  1: this->SetCursor(wxCURSOR_SIZENWSE); break;
				}
				break;
		}
	} else {
		// No object is selected, but only reset the mouse cursor if this is
		// something new (don't want to set the cursor on every single mouse
		// move event)
		if (this->focusedObject != oldFocusedObject) {
			this->SetCursor(wxNullCursor);
		}
	}
	return needRedraw;
}

void MapCanvas::onMouseMove(wxMouseEvent& ev)
{
	this->pointerX = ev.m_x;
	this->pointerY = ev.m_y;

	bool needRedraw = false;

	int mapPointerX = this->pointerX / this->zoomFactor + this->offX;
	int mapPointerY = this->pointerY / this->zoomFactor + this->offY;

	// Perform actions that require an active layer
	if (this->activeLayer >= 0) {
		Map2D::LayerPtr layer = this->map->getLayer(this->activeLayer);
		int tileWidth, tileHeight;
		this->getLayerTileSize(layer, &tileWidth, &tileHeight);

		int pointerTileX = mapPointerX / tileWidth;
		int pointerTileY = mapPointerY / tileHeight;

		// If the shift key is held down while the mouse is being moved, also
		// display the tile code under the cursor.  This isn't done all the time
		// to avoid wasting CPU as it's only useful when implementing support
		// for new games.
		if (ev.m_shiftDown) {
			Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
			bool found = false;
			for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
				int tileX = (*c)->x * tileWidth;
				int tileY = (*c)->y * tileHeight;
				if (
					(tileX <= mapPointerX) && (tileX + tileWidth > mapPointerX) &&
					(tileY <= mapPointerY) && (tileY + tileHeight > mapPointerY)
				) {
					this->doc->setStatusText(wxString::Format(_T("%d,%d [0x%04X]"),
						pointerTileX, pointerTileY, (*c)->code));
					found = true;
					break;
				}
			}
			if (!found) {
				this->doc->setStatusText(wxString::Format(_T("%d,%d [-]"),
						pointerTileX, pointerTileY));
			}
		} else { // shift key not pressed
			this->doc->setStatusText(wxString::Format(_T("%d,%d"), pointerTileX, pointerTileY));
		}

		// Perform the primary action (left mouse drag) if it is active
		if (this->actionFromX >= 0) {
			if (this->resizeX || this->resizeY) {
				int deltaX = (ev.m_x - this->actionFromX) * this->resizeX / this->zoomFactor;
				int deltaY = (ev.m_y - this->actionFromY) * this->resizeY / this->zoomFactor;

				// Make sure the delta values are even multiple of the tile size
				if (deltaX) deltaX -= deltaX % tileWidth;
				if (deltaY) deltaY -= deltaY % tileHeight;

				// Make sure the delta values are within range so we can apply them
				// with no further checks.
				int finalX = this->focusedObject->width * tileWidth + deltaX;
				if ((this->focusedObject->obj->maxWidth > 0) && (finalX > this->focusedObject->obj->maxWidth)) {
					int overflowX = finalX - this->focusedObject->obj->maxWidth;
					if (overflowX < deltaX) deltaX -= overflowX;
					else deltaX = 0;
				} else if ((this->focusedObject->obj->minWidth > 0) && (finalX < this->focusedObject->obj->minWidth)) {
					int overflowX = finalX - this->focusedObject->obj->minWidth;
					if (overflowX > deltaX) deltaX -= overflowX;
					else deltaX = 0;
				}
				int finalY = this->focusedObject->height * tileHeight + deltaY;
				if ((this->focusedObject->obj->maxHeight > 0) && (finalY > this->focusedObject->obj->maxHeight)) {
					int overflowY = finalY - this->focusedObject->obj->maxHeight;
					if (overflowY < deltaY) deltaY -= overflowY;
					else deltaY = 0;
				} else if ((this->focusedObject->obj->minHeight > 0) && (finalY < this->focusedObject->obj->minHeight)) {
					int overflowY = finalY - this->focusedObject->obj->minHeight;
					if (overflowY > deltaY) deltaY -= overflowY;
					else deltaY = 0;
				}
				switch (this->resizeX) {
					case -1:
						this->focusedObject->x -= deltaX / tileWidth;
						// fall through
					case 1:
						this->focusedObject->width += deltaX / tileWidth;
						break;
					case 0:
						break;
				}
				switch (this->resizeY) {
					case -1:
						this->focusedObject->y -= deltaY / tileHeight;
						// fall through
					case 1:
						this->focusedObject->height += deltaY / tileHeight;
						break;
					case 0:
						break;
				}
				this->actionFromX += deltaX * this->resizeX * this->zoomFactor;
				this->actionFromY += deltaY * this->resizeY * this->zoomFactor;

				needRedraw = true;
			}
		} else {
			// Not performing the primary action

			// Redraw the selection area if the user is right-dragging
			if (this->selectFromX >= 0) {
				needRedraw = true;

			} else {
				// Not selecting, so highlight objects as they are hovered over
				ObjectVector::iterator start;
				if (this->focusedObject != this->objects.end()) {
					start = this->focusedObject;
				} else {
					start = this->objects.begin();
				}
				if (this->focusObject(start)) needRedraw = true;
			}

		} // if (not performing primary action)
	} else { // no active layer
		this->doc->setStatusText(wxString());
	}

	// Scroll the map if the user is middle-dragging
	if (this->scrollFromX >= 0) {
		this->offX = this->scrollFromX - ev.m_x;
		this->offY = this->scrollFromY - ev.m_y;
		needRedraw = true;
	}

	if (needRedraw) this->redraw();
	return;
}

void MapCanvas::onMouseDownLeft(wxMouseEvent& ev)
{
	if (this->resizeX || this->resizeY) {
		// Mouse was over focus border, start a resize
		this->actionFromX = ev.m_x;
		this->actionFromY = ev.m_y;
		this->CaptureMouse();
	} else {
		this->actionFromX = -1; // no action
	}
	ev.Skip();
	return;
}

void MapCanvas::onMouseUpLeft(wxMouseEvent& ev)
{
	if (this->actionFromX >= 0) {
		// There was an action
		this->ReleaseMouse();
	}

	this->actionFromX = -1;
	this->redraw();
	return;
}

void MapCanvas::onMouseDownRight(wxMouseEvent& ev)
{
	// We add one to each coordinate so that a single right-click behaves like
	// a small negative rectangle, immediately selecting the current tile.
	this->selectFromX = ev.m_x + 1;
	this->selectFromY = ev.m_y + 1;
	this->CaptureMouse();
	return;
}

void MapCanvas::onMouseUpRight(wxMouseEvent& ev)
{
	this->ReleaseMouse();

	// TODO: Select the tiles contained within the rectangle

	this->selectFromX = -1;
	this->redraw();
	return;
}

void MapCanvas::onMouseDownMiddle(wxMouseEvent& ev)
{
	this->scrollFromX = this->offX + ev.m_x;
	this->scrollFromY = this->offY + ev.m_y;
	this->CaptureMouse();
	return;
}

void MapCanvas::onMouseUpMiddle(wxMouseEvent& ev)
{
	this->scrollFromX = -1;
	this->ReleaseMouse();
	return;
}

void MapCanvas::onMouseCaptureLost(wxMouseCaptureLostEvent& ev)
{
	this->scrollFromX = -1;
	this->selectFromX = -1;
	return;
}

void MapCanvas::onKeyDown(wxKeyEvent& ev)
{
	bool needRedraw = false;
	switch (ev.GetKeyCode()) {
		case WXK_TAB:
			// Toggle between overlapping objects under the cursor
			if (this->focusedObject != this->objects.end()) {
				if (this->focusObject(this->focusedObject + 1)) needRedraw = true;
			}
			break;
	}
	if (needRedraw) this->redraw();
	return;
}
