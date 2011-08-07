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

MapCanvas::MapCanvas(MapDocument *parent, Map2DPtr map, VC_TILESET tileset, int *attribList)
	throw () :
		wxGLCanvas(parent, wxID_ANY, wxDefaultPosition,
			wxDefaultSize, 0, wxEmptyString, attribList),
		doc(parent),
		map(map),
		tileset(tileset),
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

	// TEMP
	MapObject test(1, 1, 200, 200, 1, 1);
	test.x = 10;
	test.y = 10;
	test.width = 100;
	test.height = 100;
	this->objects.push_back(test);

	MapObject test2(50, 50, 100, 100, 32, 32);
	test2.x = 90;
	test2.y = 10;
	test2.width = 90;
	test2.height = 20;
	this->objects.push_back(test2);

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

	int *tile = new int[layerWidth * layerHeight];
	Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
	for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
		tile[(*c)->y * layerWidth + (*c)->x] = (*c)->code;
	}

	for (int y = 0; y < layerHeight; y++) {
		int *row = tile + y * layerWidth;
		for (int x = 0; x < layerWidth; x++) {
			if (row[x] == 0x4290) {
				MapObject plant(32, 8, 32, 0, tileWidth, tileHeight);
				plant.x = x * tileWidth;
				plant.y = y * tileHeight;
				plant.width = 0;
				plant.height = tileHeight;
				int x2;
				for (x2 = x; x2 < layerWidth; x2++) {
					if (
						(row[x2] == 0x4290) ||
						(row[x2] == 0x42B8) ||
						(row[x2] == 0x42E0) ||
						(row[x2] == 0x4308)
					) {
						plant.width += tileWidth;
					}
				}
				for (int y2 = y + 1; y2 < layerHeight; y2++) {
					int *row2 = tile + y2 * layerWidth;
					if (
						(row2[x+0] == 0x48D0) ||
						(row2[x+1] == 0x48F8) ||
						(row2[x+2] == 0x4920) ||
						(row2[x+3] == 0x4948) ||

						(row2[x+0] == 0x4F10) ||
						(row2[x+1] == 0x4F38) ||
						(row2[x+2] == 0x4F60) ||
						(row2[x+3] == 0x4F88) ||

						(row2[x+0] == 0x5550) ||
						(row2[x+2] == 0x5578) ||
						(row2[x+2] == 0x55A0) ||
						(row2[x+3] == 0x55C8) ||

						(row2[x+0] == 0x5B90) ||
						(row2[x+2] == 0x5BB8) ||
						(row2[x+2] == 0x5BE0) ||
						(row2[x+3] == 0x5C08) ||

						(row2[x+0] == 0x61D0) ||
						(row2[x+2] == 0x61F8) ||
						(row2[x+2] == 0x6220) ||
						(row2[x+3] == 0x6248)
					) {
						plant.height += tileHeight;
					}
				}
				x = x2; // skip scanned tiles
				this->objects.push_back(plant);
			}
		}
	}
	// ENDTEMP

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

	// If the grid is visible, draw it using the tile size of the active layer
	if ((this->gridVisible) && (this->activeLayer >= 0)) {
		Map2D::LayerPtr layer = this->map->getLayer(this->activeLayer);
		int tileWidth, tileHeight;
		this->getLayerTileSize(layer, &tileWidth, &tileHeight);
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

	// If an object is currently focused, draw a box around it
	if (this->focusedObject != this->objects.end()) {
		glColor4f(1.0, 1.0, 1.0, 0.75);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(3, 0xAAAA);
		glBegin(GL_LINE_LOOP);
		int x1 = this->focusedObject->x - this->offX;
		int y1 = this->focusedObject->y - this->offY;
		int x2 = x1 + this->focusedObject->width;
		int y2 = y1 + this->focusedObject->height;
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

bool MapCanvas::focusObject(MapObjectVector::iterator start)
{
	bool needRedraw = false;

	// See if the mouse is over an object
	int mapPointerX = this->pointerX / this->zoomFactor + this->offX;
	int mapPointerY = this->pointerY / this->zoomFactor + this->offY;
	MapObjectVector::iterator oldFocusedObject = this->focusedObject;
	MapObjectVector::iterator end = this->objects.end();
	this->focusedObject = this->objects.end(); // default to nothing focused
	for (int n = 0; n < 2; n++) {
		for (MapObjectVector::iterator i = start; i != end; i++) {
			if (
				(mapPointerX >= i->x - FOCUS_BOX_PADDING) &&
				(mapPointerX < i->x + i->width + FOCUS_BOX_PADDING) &&
				(mapPointerY >= i->y - FOCUS_BOX_PADDING) &&
				(mapPointerY < i->y + i->height + FOCUS_BOX_PADDING)
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
			(mapPointerX >= this->focusedObject->x - FOCUS_BOX_PADDING) &&
			(mapPointerX < this->focusedObject->x + FOCUS_BOX_PADDING)
		) {
			this->resizeX = -1;
		} else if (
			(mapPointerX >= this->focusedObject->x + this->focusedObject->width - FOCUS_BOX_PADDING) &&
			(mapPointerX < this->focusedObject->x + this->focusedObject->width + FOCUS_BOX_PADDING)
		) {
			this->resizeX = 1;
		}

		if (
			(mapPointerY >= this->focusedObject->y - FOCUS_BOX_PADDING) &&
			(mapPointerY < this->focusedObject->y + FOCUS_BOX_PADDING)
		) {
			this->resizeY = -1;
		} else if (
			(mapPointerY >= this->focusedObject->y + this->focusedObject->height - FOCUS_BOX_PADDING) &&
			(mapPointerY < this->focusedObject->y + this->focusedObject->height + FOCUS_BOX_PADDING)
		) {
			this->resizeY = 1;
		}

		// Prevent the resize in one or more axes if the object's dimensions in that
		// axis are fixed.  This is just UI sugar as the object couldn't be resized
		// anyway.
		if (
			(this->focusedObject->maxWidth == this->focusedObject->minWidth) &&
			(this->focusedObject->maxWidth == this->focusedObject->width)
		) {
			this->resizeX = 0;
		}
		if (
			(this->focusedObject->maxHeight == this->focusedObject->minHeight) &&
			(this->focusedObject->maxHeight == this->focusedObject->height)
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

	int mapPointerX = this->pointerX / this->zoomFactor + this->offX;
	int mapPointerY = this->pointerY / this->zoomFactor + this->offY;
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
	} else { // no active layer
		this->doc->setStatusText(wxString());
	}

	bool needRedraw = false;

	// Scroll the map if the user is middle-dragging
	if (this->scrollFromX >= 0) {
		this->offX = this->scrollFromX - ev.m_x;
		this->offY = this->scrollFromY - ev.m_y;
		needRedraw = true;
	}

	// Redraw the selection area if the user is right-dragging
	if (this->selectFromX >= 0) {
		needRedraw = true;
	}

	if (this->actionFromX >= 0) {
		// Primary action is active
		if (this->resizeX || this->resizeY) {
			int deltaX = (ev.m_x - this->actionFromX) * this->resizeX / this->zoomFactor;
			int deltaY = (ev.m_y - this->actionFromY) * this->resizeY / this->zoomFactor;

			// Make sure the delta values are even multiple of the tile size
			if (deltaX) deltaX -= deltaX % this->focusedObject->tileWidth;
			if (deltaY) deltaY -= deltaY % this->focusedObject->tileHeight;

			// Make sure the delta values are within range so we can apply them
			// with no further checks.
			int finalX = this->focusedObject->width + deltaX;
			if ((this->focusedObject->maxWidth > 0) && (finalX > this->focusedObject->maxWidth)) {
				int overflowX = finalX - this->focusedObject->maxWidth;
				if (overflowX < deltaX) deltaX -= overflowX;
				else deltaX = 0;
			} else if ((this->focusedObject->minWidth > 0) && (finalX < this->focusedObject->minWidth)) {
				int overflowX = finalX - this->focusedObject->minWidth;
				if (overflowX > deltaX) deltaX -= overflowX;
				else deltaX = 0;
			}
			int finalY = this->focusedObject->height + deltaY;
			if ((this->focusedObject->maxHeight > 0) && (finalY > this->focusedObject->maxHeight)) {
				int overflowY = finalY - this->focusedObject->maxHeight;
				if (overflowY < deltaY) deltaY -= overflowY;
				else deltaY = 0;
			} else if ((this->focusedObject->minHeight > 0) && (finalY < this->focusedObject->minHeight)) {
				int overflowY = finalY - this->focusedObject->minHeight;
				if (overflowY > deltaY) deltaY -= overflowY;
				else deltaY = 0;
			}
			switch (this->resizeX) {
				case -1:
					this->focusedObject->x -= deltaX;
					// fall through
				case 1:
					this->focusedObject->width += deltaX;
					break;
				case 0:
					break;
			}
			switch (this->resizeY) {
				case -1:
					this->focusedObject->y -= deltaY;
					// fall through
				case 1:
					this->focusedObject->height += deltaY;
					break;
				case 0:
					break;
			}
			this->actionFromX += deltaX * this->resizeX * this->zoomFactor;
			this->actionFromY += deltaY * this->resizeY * this->zoomFactor;

			needRedraw = true;
		}
	} else {
		// Not performing an action

		MapObjectVector::iterator start;
		if (this->focusedObject != this->objects.end()) {
			start = this->focusedObject;
		} else {
			start = this->objects.begin();
		}
		if (this->focusObject(start)) needRedraw = true;

	} // if (not performing primary action)

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
