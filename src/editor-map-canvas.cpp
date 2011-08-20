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
		editingMode(ObjectMode),
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
	this->selection.tiles = NULL;
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

void MapCanvas::setTileMode()
	throw ()
{
	this->editingMode = TileMode;

	// Deselect anything that was selected
	this->focusedObject = this->objects.end();

	// Remove any selection markers
	this->redraw();
	return;
}

void MapCanvas::setObjMode()
	throw ()
{
	this->editingMode = ObjectMode;

	// Deselect anything that was selected
	if (this->selection.tiles) {
		delete[] this->selection.tiles;
		this->selection.tiles = NULL;
	}

	// Remove any selection markers
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

	// Enable on/off transparency
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0);

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

			int oX, oY;
			bool drawSelection = false;
			if (
				(this->activeLayer == i) &&  // if this is the active layer
				(this->selection.tiles) &&   // and there are selected tiles
				(this->selectFromX < 0)      // and we're not selecting new tiles
			) {
				// Draw the current selection
				drawSelection = true;
				oX = (this->pointerX / this->zoomFactor + this->offX) / tileWidth - this->selection.width / 2;
				oY = (this->pointerY / this->zoomFactor + this->offY) / tileHeight - this->selection.height / 2;
			}

			// Set the selection colour to use as blending is turned on and off
			glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR);
			glBlendColor(0.75, 0.0, 0.0, 0.65);

			Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
			for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {

				int tileX = (*c)->x * tileWidth;
				int tileY = (*c)->y * tileHeight;

				if (tileX > this->offX + s.x) continue; // tile is off viewport to the right
				if (tileX + tileWidth < this->offX) continue; // tile is off viewport to the left
				if (tileY > this->offY + s.y) continue; // tile is off viewport to the bottom
				if (tileY + tileHeight < this->offY) continue; // tile is off viewport to the top

				// If there's a selection, see whether the current tile is where one of
				// the selected tiles should be drawn.  If so we skip it, so there's an
				// empty space ready to draw the selection later.  This way any
				// selection will temporarily replace the actual map tiles as the mouse
				// moves around.  This is best because it will make it obvious what
				// will happen when the mouse is clicked.  Otherwise with a nice alpha
				// blend over the top of the map tiles, the user may get a surprise when
				// the pasted selection overwrites tiles which were previously blended.
				if (
					(drawSelection) &&
					((signed)(*c)->x >= oX) &&
					((signed)(*c)->x < oX + this->selection.width) &&
					((signed)(*c)->y >= oY) &&
					((signed)(*c)->y < oY + this->selection.height)
				) {
					// This item is contained within the selection rendering area, so
					// don't draw the map tile - unless the selection has no tile for
					// that bit.
					if (this->selection.tiles[((*c)->y - oY) * this->selection.width + ((*c)->x - oX)] >= 0) {
						continue;
					}
				}

				glDisable(GL_BLEND);
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				if (
					(drawSelection) &&
					((*c)->x >= this->selection.x) &&
					((*c)->x < this->selection.x + this->selection.width) &&
					((*c)->y >= this->selection.y) &&
					((*c)->y < this->selection.y + this->selection.height)
				) {
					// This tile is contained within the original selection, i.e. the area
					// that will be removed if the delete key is pressed.
					int relX = (*c)->x - this->selection.x;
					int relY = (*c)->y - this->selection.y;
					if (this->selection.tiles[relY * this->selection.width + relX] >= 0) {
						// There is a selected tile at this point, so highlight it
						glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
						glColor4f(1.0, 0.2, 0.2, 1.0);
					}
				}

				glBindTexture(GL_TEXTURE_2D, this->textureMap[i][(*c)->code]);
				glBegin(GL_QUADS);
				glTexCoord2d(0.0, 0.0);
				glVertex2i(tileX - this->offX, tileY - this->offY);
				glTexCoord2d(0.0, 1.0);
				glVertex2i(tileX - this->offX, tileY + tileHeight - this->offY);
				glTexCoord2d(1.0, 1.0);
				glVertex2i(tileX + tileWidth - this->offX, tileY + tileHeight - this->offY);
				glTexCoord2d(1.0, 0.0);
				glVertex2i(tileX + tileWidth - this->offX, tileY - this->offY);
				glEnd();
			}

			// If space was left for the selection, draw it now
			if (drawSelection) {
				// Enable semitransparency
				glEnable(GL_BLEND);
				glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
				glBlendColor(0.0, 0.0, 0.0, 0.65);

				for (int y = 0; y < this->selection.height; y++) {
					for (int x = 0; x < this->selection.width; x++) {
						int tileX = (x + oX) * tileWidth;
						int tileY = (y + oY) * tileHeight;
						int code = this->selection.tiles[y * this->selection.width + x];
						if (code < 0) continue; // omitted tile

						if (tileX > this->offX + s.x) continue; // tile is off viewport to the right
						if (tileX + tileWidth < this->offX) continue; // tile is off viewport to the left
						if (tileY > this->offY + s.y) continue; // tile is off viewport to the bottom
						if (tileY + tileHeight < this->offY) continue; // tile is off viewport to the top

						glBindTexture(GL_TEXTURE_2D, this->textureMap[this->activeLayer][code]);
						glBegin(GL_QUADS);
						glTexCoord2d(0.0, 0.0);
						glVertex2i(tileX - this->offX, tileY - this->offY);
						glTexCoord2d(0.0, 1.0);
						glVertex2i(tileX - this->offX, tileY + tileHeight - this->offY);
						glTexCoord2d(1.0, 1.0);
						glVertex2i(tileX + tileWidth - this->offX, tileY + tileHeight - this->offY);
						glTexCoord2d(1.0, 0.0);
						glVertex2i(tileX + tileWidth - this->offX, tileY - this->offY);
						glEnd();
					}
				}
			}

		}
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

		switch (this->editingMode) {
			case TileMode:
				break;
			case ObjectMode:
				// If an object is currently focused, draw a box around it
				if (this->focusedObject != this->objects.end()) {
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
				break;
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
			switch (this->editingMode) {
				case TileMode: {
					int mouseTileX = ev.m_x / this->zoomFactor;
					int mouseTileY = ev.m_y / this->zoomFactor;
					// TODO: break if tile coords are out of range

					needRedraw = true;
					break;
				}
				case ObjectMode:
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
					break;
			}
		} else {
			// Not performing the primary action

			// Redraw the selection area if the user is right-dragging
			if (this->selectFromX >= 0) {
				needRedraw = true;

			} else {
				// Not selecting
				switch (this->editingMode) {
					case TileMode:
						// If there's a current selection, overlay it
						if (this->selection.tiles) {
							needRedraw = true;
						}
						break;
					case ObjectMode:
						// Highlight objects as they are hovered over
						ObjectVector::iterator start;
						if (this->focusedObject != this->objects.end()) {
							start = this->focusedObject;
						} else {
							start = this->objects.begin();
						}
						if (this->focusObject(start)) needRedraw = true;
						break;
				}
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

	if (this->activeLayer < 0) return; // must have active layer

	switch (this->editingMode) {
		case TileMode: {
			// Select the tiles contained within the rectangle

			Map2D::LayerPtr layer = this->map->getLayer(this->activeLayer);

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
					return;
				}
			}

			int tileWidth, tileHeight;
			this->getLayerTileSize(layer, &tileWidth, &tileHeight);

			// Calculate the dimensions of the tiles in the selection rectangle
			int x1, y1, x2, y2;
			bool selectPartial;
			if (this->selectFromX > ev.m_x) {
				// Negative rectangle
				x1 = ev.m_x;
				x2 = this->selectFromX;
				selectPartial = true;
			} else {
				x1 = this->selectFromX;
				x2 = ev.m_x;
				selectPartial = false;
			}
			if (this->selectFromY > ev.m_y) {
				y1 = ev.m_y;
				y2 = this->selectFromY;
			} else {
				y1 = this->selectFromY;
				y2 = ev.m_y;
			}
			// (x1,y1) is now always top-left, (x2,y2) is always bottom-right

			x1 = x1 / this->zoomFactor + this->offX;
			y1 = y1 / this->zoomFactor + this->offY;
			x2 = x2 / this->zoomFactor + this->offX;
			y2 = y2 / this->zoomFactor + this->offY;

			// Convert the selection rectangle from pixel coords to tile coords,
			// enlarging or shrinking it as necessary to accommodate selectPartial.
			if (selectPartial) {
				x2 += tileWidth - 1;
				y2 += tileHeight - 1;
			} else {
				x1 += tileWidth - 1;
				y1 += tileHeight - 1;
			}
			x1 /= tileWidth;
			y1 /= tileHeight;
			x2 /= tileWidth;
			y2 /= tileHeight;

			if (x1 < 0) x1 = 0;
			if (y1 < 0) y1 = 0;
			if (x2 < 0) x2 = 0;
			if (y2 < 0) y2 = 0;

			int minX = x2;
			int minY = y2;
			int maxX = x1;
			int maxY = y1;
			Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
			for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
				if (
					((*c)->x >= x1) && ((*c)->x < x2) &&
					((*c)->y >= y1) && ((*c)->y < y2)
				) {
					// This tile is contained within the selection rectangle
					if (minX > (*c)->x) minX = (*c)->x;
					if (minY > (*c)->y) minY = (*c)->y;
					if (maxX < (*c)->x + 1) maxX = (*c)->x + 1;
					if (maxY < (*c)->y + 1) maxY = (*c)->y + 1;
				}
			}
			if (this->selection.tiles) delete[] this->selection.tiles;
			this->selection.width = maxX - minX;
			this->selection.height = maxY - minY;
			if ((this->selection.width <= 0) || (this->selection.height <= 0)) {
				// Empty selection
				this->selection.tiles = NULL; // already deleted above
			} else {
				this->selection.tiles = new int[this->selection.width * this->selection.height];
				for (int i = 0; i < this->selection.width * this->selection.height; i++) {
					this->selection.tiles[i] = -1; // no tile present here
				}

				// Run through the tiles again, but this tile copy them into the selection
				for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
					if (
						((*c)->x >= minX) && ((*c)->x < maxX) &&
						((*c)->y >= minY) && ((*c)->y < maxY)
					) {
						// This tile is contained within the selection rectangle
						int offset = ((*c)->y - minY) * this->selection.width + (*c)->x - minX;
						assert(offset < this->selection.width * this->selection.height);
						this->selection.tiles[offset] = (*c)->code;
					}
				}

				// Remember the location of the original selection
				this->selection.x = minX;
				this->selection.y = minY;
			}
			break;
		}
		case ObjectMode:
			// TODO: Select multiple objects?
			break;
	}

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

		case WXK_NUMPAD_DELETE:
		case WXK_DELETE:
			// Delete the currently selected tiles
			if ((this->activeLayer >= 0) && (this->selection.tiles)) {
				int x1 = this->selection.x;
				int y1 = this->selection.y;
				int x2 = x1 + this->selection.width;
				int y2 = y1 + this->selection.height;
				Map2D::LayerPtr layer = this->map->getLayer(this->activeLayer);
				Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
				for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); ) {
					if (
						((*c)->x >= x1) && ((*c)->x < x2) &&
						((*c)->y >= y1) && ((*c)->y < y2)
					) {
						// Remove this tile
						c = content->erase(c);
						needRedraw = true;
					} else c++;
				}
			}
			break;

	}
	if (needRedraw) this->redraw();
	return;
}
