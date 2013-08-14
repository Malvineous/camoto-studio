/**
 * @file   editor-map-canvas.cpp
 * @brief  OpenGL surface where map is drawn.
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

#include <camoto/gamemaps/util.hpp>
#include "editor-map-canvas.hpp"

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

// How many *pixels* (i.e. irrespective of zoom) the mouse pointer can be moved
// out of the focus box without losing focus.  This is also the number of pixels
// either side of the border that the mouse will perform a resize action.
#define FOCUS_BOX_PADDING 2

// Size of the grips when positioning path points
#define PATH_ARROWHEAD_SIZE 5

// Some colours (R, G, B, A)
#define CLR_PATH              0.0, 0.0, 1.0, 1.0  ///< Path lines
#define CLR_ARROWHEAD_SEL     1.0, 0.0, 0.0, 1.0  ///< Path arrow, selected
#define CLR_ARROWHEAD_NORMAL  CLR_PATH            ///< Path arrow, normal
#define CLR_ARROWHEAD_HOT     1.0, 1.0, 0.0, 1.0  ///< Path arrow, highlighted
#define CLR_PATH_PREVIEW      0.7, 0.7, 0.7, 0.7  ///< Changed path preview

// Is the given point within a few pixels of the other point?
#define CURSOR_OVER_POINT(mx, my, px, py) \
	POINT_IN_RECT(mx, my, \
		px - PATH_ARROWHEAD_SIZE / this->zoomFactor, \
		py - PATH_ARROWHEAD_SIZE / this->zoomFactor, \
		px + PATH_ARROWHEAD_SIZE / this->zoomFactor, \
		py + PATH_ARROWHEAD_SIZE / this->zoomFactor \
	)

#define SIN45 0.85090352453
#define COS45 0.52532198881

int matchTileRun(unsigned int *tile, const MapObject::TileRun *run)
{
	int matched = 0;
	for (MapObject::TileRun::const_iterator t = run->begin(); t != run->end(); t++) {
		if ((*t != INVALID_TILECODE) && (*t != *tile)) break; // no more matches
		tile++; matched++;
	}
	return matched;
}

int matchAnyTileRun(unsigned int *tile, const MapObject::TileRun *run)
{
	int matched = 0;
	bool found;
	do {
		found = false;
		for (MapObject::TileRun::const_iterator t = run->begin(); t != run->end(); t++) {
			if ((*t == INVALID_TILECODE) || (*t == *tile)) {
				found = true;
				matched++;
				tile++;
				break;
			}
		}
	} while (found);
	return matched;
}

BEGIN_EVENT_TABLE(MapCanvas, MapBaseCanvas)
	EVT_PAINT(MapCanvas::onPaint)
	EVT_ERASE_BACKGROUND(MapCanvas::onEraseBG)
	EVT_SIZE(MapCanvas::onResize)
	EVT_LEFT_DOWN(MapCanvas::onMouseDownLeft)
	EVT_LEFT_UP(MapCanvas::onMouseUpLeft)
	EVT_RIGHT_DOWN(MapCanvas::onMouseDownRight)
	EVT_RIGHT_UP(MapCanvas::onMouseUpRight)
	EVT_KEY_DOWN(MapCanvas::onKeyDown)
END_EVENT_TABLE()

MapCanvas::MapCanvas(MapDocument *parent, wxGLContext *glcx, Map2DPtr map,
	TilesetCollectionPtr tilesets, int *attribList, const MapObjectVector *mapObjects)
	:	MapBaseCanvas(parent, attribList),
		doc(parent),
		map(map),
		glcx(glcx),
		tilesets(tilesets),
		mapObjects(mapObjects),
		editingMode(TileMode),
		primaryLayer(0),
		selectFromX(-1),
		// selectFromY doesn't matter when X is -1
		actionFromX(-1),
		// actionFromY doesn't matter when X is -1
		pointerX(0),
		pointerY(0),
		resizeX(0),
		resizeY(0),
		nearestPathPointOff(-1)
{
	assert(tilesets->size() > 0);
	// Initial state is all layers visible and active
	unsigned int layerCount = map->getLayerCount();
	for (unsigned int i = 0; i < layerCount; i++) {
		this->visibleLayers.push_back(true);
		this->activeLayers.push_back(true);
	}
	this->visibleElements[ElViewport] = this->map->getCaps() & Map2D::HasViewport;
	this->visibleElements[ElPaths] = this->map->getCaps() & Map2D::HasPaths;
	this->activeElement = 0; // no element layers are active

	this->SetCurrent(*this->glcx);

	for (unsigned int i = 0; i < layerCount; i++) {
		// Prepopulate an empty selection vector for each layer, so there is
		// somewhere to put the tiles when some are selected.
		this->selection.layers.push_back(Map2D::Layer::ItemPtrVector());

		Map2D::LayerPtr layer = this->map->getLayer(i);

		unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
		getLayerDims(this->map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);

		Texture unknownTile;
		// TODO: Load unknown/default tile the same size (if possible) as the grid
		unknownTile.glid = 0;
		unknownTile.width = tileWidth;
		unknownTile.height = tileHeight;

		PaletteTablePtr palDefault;
		if (layer->getCaps() & Map2D::Layer::HasPalette) {
			// Use the palette supplied by the map layer
			palDefault = layer->getPalette(this->tilesets);
			if (!palDefault) {
				std::cerr << "ERROR: Map said it had a palette but then didn't "
					"supply one!" << std::endl;
			}
		} else {
			for (TilesetCollection::const_iterator
				i = tilesets->begin(); i != tilesets->end(); i++
			) {
				if (i->second->getCaps() & Tileset::HasPalette) {
					palDefault = i->second->getPalette();
					break;
				}
			}
		}
		if (!palDefault) {
			// Load a default palette.  The VGA one is backwards compatible with
			// 16-color EGA/CGA images.
			palDefault = createPalette_DefaultVGA();
		}
		assert(palDefault);

		TEXTURE_MAP tm;

		// Load all the allowed tiles first, so the tile list will always be in the
		// same order
		Map2D::Layer::ItemPtrVectorPtr valid = layer->getValidItemList();
		for (Map2D::Layer::ItemPtrVector::iterator c = valid->begin(); c != valid->end(); c++) {
			unsigned int code = (*c)->code;
			if (tm.find(code) == tm.end()) {
				this->loadTileImage(tm, palDefault, *c, layer, tilesets, unknownTile);
			}
		}

		// Load the tiles used in the map next, in case any are missing from the
		// list of allowed tiles
		Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
		for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
			unsigned int code = (*c)->code;
			if (tm.find(code) == tm.end()) {
				this->loadTileImage(tm, palDefault, *c, layer, tilesets, unknownTile);
			}
		}

		this->textureMap.push_back(tm);
	}

	this->glReset();

	// Search tiles to find objects for object-mode

	// TEMP: this does layer 0 only!
	Map2D::LayerPtr layer = this->map->getLayer(0); // TEMP

	unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
	getLayerDims(this->map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);

	// Load the layer into an array
	unsigned int *tile = new unsigned int[layerWidth * layerHeight];
	for (unsigned int i = 0; i < layerWidth * layerHeight; i++) tile[i] = INVALID_TILECODE;
	Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
	for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
		if (((*c)->x >= layerWidth) || ((*c)->y >= layerHeight)) {
			std::cerr << "ERROR: This map has items outside its boundary!" << std::endl;
		} else {
			tile[(*c)->y * layerWidth + (*c)->x] = (*c)->code;
		}
	}
	// tile[] now uses -1 for no-tile-present and the actual tile code otherwise.

	for (unsigned int y = 0; y < layerHeight; y++) {
		for (unsigned int x = 0; x < layerWidth; x++) {
			unsigned int startCode = tile[y * layerWidth + x];
			if (startCode == 0) continue; // skip empty tiles
			for (MapObjectVector::const_iterator i = this->mapObjects->begin();
				i != this->mapObjects->end();
				i++
			) {
				unsigned int y2 = y;
				unsigned int x2 = x;

				// Start tracking this in case it turns out to be an actual object
				Object newObject;
				newObject.obj = &(*i);
				newObject.x = x;
				newObject.y = y;
				newObject.height = 0;
				newObject.width = 0;

				bool done = false;

				for (unsigned int s = 0; s < MapObject::SectionCount; s++) {
					if ((!done) && (i->section[s].size() > 0)) {
						// There are top/mid/bot rows, see if they match the map
						for (MapObject::RowVector::const_iterator oY = i->section[s].begin(); oY != i->section[s].end(); oY++) {
							unsigned int leftMatched = matchTileRun(&tile[y2 * layerWidth + x2], &oY->segment[MapObject::Row::L]);

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
							unsigned int width = x2 - x;
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
{
	for (std::vector<TEXTURE_MAP>::iterator l = this->textureMap.begin(); l != this->textureMap.end(); l++) {
		for (TEXTURE_MAP::iterator t = l->begin(); t != l->end(); t++) {
			glDeleteTextures(1, &t->second.glid);
		}
	}
}

void MapCanvas::loadTileImage(TEXTURE_MAP& tm, PaletteTablePtr& palDefault,
	const Map2D::Layer::ItemPtr& item, Map2D::LayerPtr& layer,
	const TilesetCollectionPtr& tilesets, Texture& unknownTile)
{
	// This tile code doesn't have an associated image yet
	ImagePtr image = layer->imageFromCode(item, tilesets);
	if (image) {
		// Got an image for this tile code

		Texture t;
		glGenTextures(1, &t.glid);
		glBindTexture(GL_TEXTURE_2D, t.glid);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		image->getDimensions(&t.width, &t.height);
		if ((t.width > 512) || (t.height > 512)) {
			// Image too large
			std::cerr << "[editor-map-canvas] Tile too large, using default image" << std::endl;
			tm[item->code] = unknownTile;
			return;
		}

		PaletteTablePtr pal;
		if (image->getCaps() & Image::HasPalette) {
			pal = image->getPalette();
		} else {
			pal = palDefault;
		}
		assert(pal);
		StdImageDataPtr data = image->toStandard();
		StdImageDataPtr mask = image->toStandardMask();

		// Convert the 8-bit data into 32-bit BGRA (so it is possible to have a
		// 256-colour image with transparency as a "257th colour".)
		boost::shared_array<uint32_t> combined(new uint32_t[t.width * t.height]);
		uint8_t *d = data.get(), *m = mask.get();
		uint8_t *c = (uint8_t *)combined.get();
		for (unsigned int p = 0; p < t.width * t.height; p++) {
			*c++ = *m & 0x01 ? 255 : (*pal)[*d].blue;
			*c++ = *m & 0x01 ?   0 : (*pal)[*d].green;
			*c++ = *m & 0x01 ? 255 : (*pal)[*d].red;
			*c++ = *m & 0x01 ?   0 : (*pal)[*d].alpha;
			m++; d++;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t.width, t.height, 0, GL_BGRA,
			GL_UNSIGNED_BYTE, combined.get());
		if (glGetError()) {
			std::cerr << "[editor-tileset] GL error loading texture "
				"into id " << t.glid << std::endl;
		}

		tm[item->code] = t;
	} else {
		std::cerr << "[editor-map] Could not get an image for tile "
			"code " << item->code << std::endl;

		tm[item->code] = unknownTile;
	}

	return;
}

void MapCanvas::setTileMode()
{
	this->editingMode = TileMode;

	// Deselect anything that was selected
	this->focusedObject = this->objects.end();

	// Remove any selection markers
	this->redraw();

	this->updateHelpText();
	return;
}

void MapCanvas::setObjMode()
{
	this->editingMode = ObjectMode;

	// Deselect anything that was selected
	this->clearSelection();

	// Remove any selection markers
	this->redraw();

	this->updateHelpText();
	return;
}

void MapCanvas::toggleActiveLayer(unsigned int layerIndex)
{
	assert(layerIndex < MapCanvas::ElementCount + this->activeLayers.size());

	if (layerIndex == MapCanvas::ElViewport) return;

	if (layerIndex < MapCanvas::ElementCount) {
		this->activeElement = layerIndex + 1;
		// Deactivate all normal layers as an element layer is active
		unsigned int layerCount = this->map->getLayerCount();
		for (unsigned int i = 0; i < layerCount; i++) {
			this->activeLayers[i] = false;
		}
	} else {
		layerIndex -= MapCanvas::ElementCount;
		bool newState = !this->activeLayers[layerIndex]; // ^=1 doesn't work :-(
		this->activeLayers[layerIndex] = newState;
		// Deactivate any element layer
		this->activeElement = 0;
	}

	this->redraw();
	this->updateHelpText();
	return;
}

void MapCanvas::setPrimaryLayer(unsigned int layerIndex)
{
	assert(layerIndex < MapCanvas::ElementCount + this->activeLayers.size());

	this->primaryLayer = layerIndex;
	this->redraw();

	this->updateHelpText();
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
	if (!this->GetParent()->IsShown()) return;
	this->SetCurrent(*this->glcx);
	wxSize s = this->GetClientSize();
	glViewport(0, 0, s.x, s.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, s.x / this->zoomFactor, s.y / this->zoomFactor, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.5, 0.5, 0.5, 1);
	glShadeModel(GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	return;
}

void MapCanvas::redraw()
{
	wxSize winSize = this->GetClientSize();
	winSize.x /= this->zoomFactor;
	winSize.y /= this->zoomFactor;

	this->needRedraw = false;

	// Limit scroll range (this is here for efficiency as we already have the
	// window size, but it means you can scroll past the bounds of the map
	// during the scroll operation and the limits don't kick in until the
	// the scroll is complete.)  It does mean you won't get limited while
	// scrolling though, so I'm calling it a feature.
	if (this->offX < -winSize.x) this->offX = -winSize.x;
	if (this->offY < -winSize.y) this->offY = -winSize.y;
	// TODO: Prevent scrolling past the end of the map too

	// Have to call glReset() every time, since the context is shared with the
	// tile selector canvas and it will change the projection since its window
	// will be a different size to us.
	this->glReset();

	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_LINE_STIPPLE);
	glEnable(GL_TEXTURE_2D);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Enable on/off transparency
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0);

	int mapPointerX = CLIENT_TO_MAP_X(this->pointerX);
	int mapPointerY = CLIENT_TO_MAP_Y(this->pointerY);

	unsigned int layerCount = this->map->getLayerCount();
	for (unsigned int layerNum = 0; layerNum < layerCount; layerNum++) {
		if (!this->visibleLayers[layerNum]) continue;

		Map2D::LayerPtr layer = this->map->getLayer(layerNum);
		int layerCaps = layer->getCaps();
		unsigned int layerWidth, layerHeight, layerTileWidth, layerTileHeight;
		getLayerDims(this->map, layer, &layerWidth, &layerHeight, &layerTileWidth, &layerTileHeight);

		// Origin of potential paste area
		int oX = (mapPointerX - this->selection.width / 2) / (signed)layerTileWidth;
		int oY = (mapPointerY - this->selection.height / 2) / (signed)layerTileHeight;

		// Selected tiles in this layer
		const Map2D::Layer::ItemPtrVector& selItems = this->selection.layers[layerNum];

		// Figure out whether there is a current selection to draw (i.e. highlight
		// the tiles that have been selected)
		bool drawSelection = false;
		int selX1, selY1, selX2, selY2;
		int pasteX1, pasteY1, pasteX2, pasteY2;
		if (
			(this->activeLayers[layerNum]) &&  // if this is the active layer
			(this->haveSelection()) &&   // and there are selected tiles
			(this->selectFromX < 0)      // and we're not selecting new tiles
		) {
			// Draw the current selection
			drawSelection = true;

			selX1 = this->selection.x / layerTileWidth;
			selY1 = this->selection.y / layerTileHeight;
			selX2 = /*selX1 + */this->selection.width / layerTileWidth;
			selY2 = /*selY1 + */this->selection.height / layerTileHeight;

			pasteX1 = oX;
			pasteY1 = oY;
			pasteX2 = pasteX1 + selX2;
			pasteY2 = pasteY1 + selY2;

			selX2 += selX1;
			selY2 += selY1;
		}
		// Draw the layer
		Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
		for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {

			int pixelX = (*c)->x * layerTileWidth;
			int pixelY = (*c)->y * layerTileHeight;
			unsigned int tileWidth, tileHeight;
			if (layerCaps & Map2D::Layer::UseImageDims) {
				tileWidth = this->textureMap[layerNum][(*c)->code].width;
				tileHeight = this->textureMap[layerNum][(*c)->code].height;
			} else {
				tileWidth = layerTileWidth;
				tileHeight = layerTileHeight;
			}

			if (pixelX > this->offX + winSize.x) continue; // tile is off viewport to the right
			if (pixelX + (signed)tileWidth < this->offX) continue; // tile is off viewport to the left
			if (pixelY > this->offY + winSize.y) continue; // tile is off viewport to the bottom
			if (pixelY + (signed)tileHeight < this->offY) continue; // tile is off viewport to the top

			// Is this tile within the selection rectangle?
			bool withinSelection = drawSelection &&
				POINT_IN_RECT((signed)(*c)->x, (signed)(*c)->y, selX1, selY1, selX2, selY2);

			// Is this tile within the potential paste area?
			bool withinPaste = drawSelection &&
				POINT_IN_RECT((signed)(*c)->x, (signed)(*c)->y, pasteX1, pasteY1, pasteX2, pasteY2);

			// If there's a selection, see whether the current tile is where one of
			// the selected tiles should be drawn.  If so we skip it, so there's an
			// empty space ready to draw the selection later.  This way any
			// selection will temporarily replace the actual map tiles as the mouse
			// moves around.  This is best because it will make it obvious what
			// will happen when the mouse is clicked.  Otherwise with a nice alpha
			// blend over the top of the map tiles, the user may get a surprise when
			// the pasted selection overwrites tiles which were previously blended.
			if (withinPaste) {
				// This item is contained within the selection rendering area, so
				// don't draw the map tile - unless the selection has no tile for
				// that bit.

				// Would this cell be overwritten if a paste operation happened now?
				bool wouldOverwrite = false;

				// See if there's a tile in the selection area at the same offset
				for (Map2D::Layer::ItemPtrVector::const_iterator
					s = selItems.begin(); s != selItems.end(); s++
				) {
					if (
						(((*s)->x - selX1) == (*c)->x - pasteX1) &&
						(((*s)->y - selY1) == (*c)->y - pasteY1)
					) {
						// The current tile would be overwritten by a paste operation,
						// so don't draw it now.  The spot will be filled later by a
						// semitransparent version of the selected tile to show what
						// would happen if a paste operation occurred right now.
						unsigned int maxInstances;
						if (layer->tilePermittedAt(*s, (*c)->x, (*c)->y, &maxInstances)) {
							// Only prevent the original tile from being drawn if we can
							// actually paste it.
							wouldOverwrite = true;
						}
						break;
					}
				}
				// Don't draw these tiles
				if (wouldOverwrite) continue;
			}

			// Now draw the tile at this map location
			if (
				withinSelection
				&& (std::find(selItems.begin(), selItems.end(), *c) != selItems.end())
			) {
				// This tile is contained within the original selection, i.e. the area
				// that will be removed if the delete key is pressed, so colour it.
				glDisable(GL_BLEND);
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
				glColor4f(1.0, 0.2, 0.2, 1.0);
			} else {
				glDisable(GL_BLEND);
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}

			this->drawMapItem(pixelX, pixelY, *c,
				&this->textureMap[layerNum][(*c)->code]);
		} // for (each item in layer)

		// Turn off any remaining red selection colour
		glDisable(GL_BLEND);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		// If there's a selection, draw it now to show where it would be pasted.
		// We have to do this after the other tiles are drawn (not at the same time)
		// because the loop above only runs through actual tiles in the level.
		// Since we might be pasting tiles that *don't* overwrite existing ones
		// (i.e. pasting on an empty background cell), we still want to draw those,
		// even though those cells won't be checked in the above loop.
		if (drawSelection) {
			// Enable semitransparency
			if (glBlendColor) {
				glEnable(GL_BLEND);
				glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
				glBlendColor(0.0, 0.0, 0.0, 0.65);
			}

			for (Map2D::Layer::ItemPtrVector::const_iterator
				s = selItems.begin(); s != selItems.end(); s++
			) {
				int tileX = pasteX1 + (*s)->x - selX1;
				int tileY = pasteY1 + (*s)->y - selY1;

				int pixelX = tileX * layerTileWidth;
				int pixelY = tileY * layerTileHeight;
				unsigned int tileWidth, tileHeight;
				Texture& texture = this->textureMap[layerNum][(*s)->code];
				if (layerCaps & Map2D::Layer::UseImageDims) {
					tileWidth = texture.width;
					tileHeight = texture.height;
				} else {
					tileWidth = layerTileWidth;
					tileHeight = layerTileHeight;
				}

				if (pixelX > this->offX + winSize.x) continue; // tile is off viewport to the right
				if (pixelX + (signed)tileWidth < this->offX) continue; // tile is off viewport to the left
				if (pixelY > this->offY + winSize.y) continue; // tile is off viewport to the bottom
				if (pixelY + (signed)tileHeight < this->offY) continue; // tile is off viewport to the top

				int x1 = pixelX - this->offX;
				int x2 = x1 + tileWidth;
				int y1 = pixelY - this->offY;
				int y2 = y1 + tileHeight;

				unsigned int maxInstances;
				bool allowed = layer->tilePermittedAt(*s, tileX, tileY, &maxInstances);
				if (maxInstances) {
					// There is a count limit on this code
					unsigned int curInstances = 0;
					for (Map2D::Layer::ItemPtrVector::iterator
						c = content->begin(); c != content->end(); c++
					) {
						if ((*s)->code == (*c)->code) curInstances++;
						if (curInstances >= maxInstances) break;
					}
					// Don't place this tile if we have already reached the maximum allowed
					if (curInstances >= maxInstances) allowed = false;
				}

				if (
					!allowed ||
					(tileX < 0) ||
					(tileY < 0) ||
					(tileX >= (signed)layerWidth) ||
					(tileY >= (signed)layerHeight)
				) {
					// This tile cannot be placed here, draw a box with a red X in it
					glDisable(GL_TEXTURE_2D);
					glColor4f(1.0, 0.0, 0.0, 1.0);
					glBegin(GL_LINE_LOOP);
					glVertex2i(x1, y1);
					glVertex2i(x1, y2);
					glVertex2i(x2, y2);
					glVertex2i(x1, y2);
					glVertex2i(x2, y1);
					glVertex2i(x2, y2);
					glVertex2i(x1, y1);
					glVertex2i(x2, y1);
					glEnd();
					glEnable(GL_TEXTURE_2D);
				} else {
					// This tile can be placed here, draw the tile
					glBindTexture(GL_TEXTURE_2D, texture.glid);
					glBegin(GL_QUADS);
					glTexCoord2d(0.0, 0.0);
					glVertex2i(x1, y1);
					glTexCoord2d(0.0, 1.0);
					glVertex2i(x1, y2);
					glTexCoord2d(1.0, 1.0);
					glVertex2i(x2, y2);
					glTexCoord2d(1.0, 0.0);
					glVertex2i(x2, y1);
					glEnd();
				}
			}
		} // if (selection exists)
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Draw any visible paths
	if (this->visibleElements[ElPaths] && (this->map->getCaps() & Map2D::HasPaths)) {
		Map2D::PathPtrVectorPtr paths = this->map->getPaths();
		assert(paths);

		glEnable(GL_LINE_SMOOTH);
		glLineWidth(2.0);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glTranslatef(-this->offX, -this->offY, 0.0);

		int selDeltaX = mapPointerX - this->selHotX;
		int selDeltaY = mapPointerY - this->selHotY;
		for (Map2D::PathPtrVector::iterator p = paths->begin(); p != paths->end(); p++) {
			// TODO: Some way of selecting one of many overlapping path points
			Map2D::PathPtr path = *p;
			unsigned int stnum = 0;
			bool entirePathSelected = false;
			for (Map2D::Path::point_vector::iterator st = (*p)->start.begin(); st != (*p)->start.end(); st++) {
				unsigned int ptnum = 1;
				bool previewNextSegment = false;

				int lastX = st->first;
				int lastY = st->second;
				int lastSelX = st->first;
				int lastSelY = st->second;

				// Draw a line at the starting pos so it can be coloured on select
				if (!(*p)->points.empty()) {
					int dX = (*p)->points[0].first, dY = (*p)->points[0].second;
					float angle = atan2((double)dY, (double)dX) * 180 / M_PI - 90;
					glPushMatrix();
					glTranslatef(lastX, lastY, 0.0);
					glScalef(1.0 / this->zoomFactor, 1.0 / this->zoomFactor, 1.0);
					glRotatef(angle, 0.0, 0.0, 1.0);

					glColor4f(CLR_ARROWHEAD_NORMAL);
					for (std::vector<path_point>::iterator spt = this->pathSelection.begin();
						spt != this->pathSelection.end(); spt++
					) {
						if ((spt->path == path) && (spt->point == 0) && (spt->start == stnum)) {
							// This point has been selected
							glColor4f(CLR_ARROWHEAD_SEL);
							lastSelX += selDeltaX;
							lastSelY += selDeltaY;
							previewNextSegment = true;
							// Selecting the start point selects the entire path (effectively,
							// but the other points shouldn't be in the selection data.)
							entirePathSelected = true;
							break;
						}
					}

					glBegin(GL_LINE_STRIP);
					glVertex2i(-PATH_ARROWHEAD_SIZE, 0);
					glVertex2i(PATH_ARROWHEAD_SIZE, 0);
					glEnd();

					glPopMatrix();
				}

				for (Map2D::Path::point_vector::iterator pt = (*p)->points.begin(); pt != (*p)->points.end(); pt++) {
					int nextX = st->first + pt->first;
					int nextY = st->second + pt->second;

					// Where this point will go if it's selected and placed at the
					// current cursor location (ish).
					int nextSelX = nextX, nextSelY = nextY;

					glColor4f(CLR_PATH);
					glBegin(GL_LINES);
					glVertex2i(lastX, lastY);
					glVertex2i(nextX, nextY);
					glEnd();

					// While we're here, see if the mouse cursor is over the point
					if (this->activeElement == 1 + ElPaths) {

						// Don't draw a preview (false) unless the last point was selected,
						// in which case we'll need to draw a preview line from it to us,
						// even if we're not in the selection.
						bool drawPreview = previewNextSegment;

						// But initially we don't want our next point to be part of a
						// preview line, unless it turns up in the selection which we're
						// about to check...
						previewNextSegment = entirePathSelected; // usually false

						// Don't draw the arrow head in the selection colour by default.
						// This is separate to drawPreview because an unselected point
						// following a selected point will have drawPreview=true but
						// selected=false.
						bool selected = entirePathSelected; // usually false

						// Check to see if this point is selected
						for (std::vector<path_point>::iterator spt = this->pathSelection.begin();
							spt != this->pathSelection.end(); spt++
						) {
							if (entirePathSelected || ((spt->path == path) && (spt->point == ptnum))) {
								// This point has been selected
								nextSelX += selDeltaX;
								nextSelY += selDeltaY;

								// We now need to draw the next segment in the preview colour
								// too, whether that point has moved or not (because the line
								// needs to be drawn from this moved point to the next one no
								// matter what.)
								previewNextSegment = true;

								// Draw this point as a preview line
								drawPreview = true;

								// Draw the arrow head in the selection colour
								selected = true;

								break;
							}
						}

						if (drawPreview) {
							// Draw a preview while we're here, showing where this point
							// would go if the left mouse button was clicked right now.
							glColor4f(CLR_PATH_PREVIEW);
							glBegin(GL_LINES);
							glVertex2i(lastSelX, lastSelY);
							glVertex2i(nextSelX, nextSelY);
							glEnd();

							int dX = nextSelX - lastSelX, dY = nextSelY - lastSelY;
							float angle = atan2((double)dY, (double)dX) * 180 / M_PI - 90;
							glPushMatrix();
							glTranslatef(nextSelX, nextSelY, 0.0);
							glScalef(1.0 / this->zoomFactor, 1.0 / this->zoomFactor, 1.0);
							glRotatef(angle, 0.0, 0.0, 1.0);

							glBegin(GL_LINE_STRIP);
							glVertex2i(-PATH_ARROWHEAD_SIZE, -PATH_ARROWHEAD_SIZE);
							glVertex2i(0, 0);
							glVertex2i(PATH_ARROWHEAD_SIZE, -PATH_ARROWHEAD_SIZE);
							glEnd();

							glPopMatrix();
						}

						if (selected) {
							// Set the selection colour for the arrow head
							glColor4f(CLR_ARROWHEAD_SEL);
						} else {
							// Normal colour
							glColor4f(CLR_ARROWHEAD_NORMAL);
						}
					}

					int dX = nextX - lastX, dY = nextY - lastY;
					float angle = atan2((double)dY, (double)dX) * 180 / M_PI - 90;
					glPushMatrix();
					glTranslatef(nextX, nextY, 0.0);
					glScalef(1.0 / this->zoomFactor, 1.0 / this->zoomFactor, 1.0);
					glRotatef(angle, 0.0, 0.0, 1.0);

					glBegin(GL_LINE_STRIP);
					glVertex2i(-PATH_ARROWHEAD_SIZE, -PATH_ARROWHEAD_SIZE);
					glVertex2i(0, 0);
					glVertex2i(PATH_ARROWHEAD_SIZE, -PATH_ARROWHEAD_SIZE);
					glEnd();

					glPopMatrix();

					lastX = nextX;
					lastY = nextY;
					lastSelX = nextSelX;
					lastSelY = nextSelY;
					ptnum++;
				}

				// If the path is treated as a closed loop, draw a final joining line
				if ((*p)->forceClosed) {
					// TODO: Include this dashed line in the live preview?
					glEnable(GL_LINE_STIPPLE);
					glLineStipple(3, 0xAAAA);
					glColor4f(CLR_PATH);
					glBegin(GL_LINES);
					glVertex2i(lastX, lastY);
					glVertex2i(st->first, st->second);
					glEnd();
					glDisable(GL_LINE_STIPPLE);
				}
			}
			stnum++;
		}

		// Highlight any proposed position for inserting a new point
		if (this->nearestPathPointOff >= 0) {
			Map2D::PathPtr path = this->nearestPathPoint.path;
			assert(path);
			assert(path->points.size() > 1);
			assert(path->start.size() > this->nearestPathPoint.start);
			Map2D::Path::point ptOrigin = path->start[this->nearestPathPoint.start];
			Map2D::Path::point ptA, ptB;
			if (this->nearestPathPoint.point == 0) {
				ptA.first = ptA.second = 0;
				ptB = path->points[0];
			} else {
				unsigned int index = this->nearestPathPoint.point - 1;
				ptA = path->points[index];
				index++;
				if (index < path->points.size()) {
					ptB = path->points[index];
				} else {
					ptB.first = ptB.second = 0; // loop back to origin
					// We don't need to check whether the path is a closed-loop type,
					// because if it isn't we'd never have the last point in the path
					// selected as the nearest point.
				}
			}
			int dX = ptB.first - ptA.first, dY = ptB.second - ptA.second;
			float angle = atan2((double)dY, (double)dX) * 180 / M_PI - 90;
			glPushMatrix();
			glTranslatef(ptOrigin.first + ptA.first, ptOrigin.second + ptA.second, 0.0);
			glScalef(1.0 / this->zoomFactor, 1.0 / this->zoomFactor, 1.0);
			glRotatef(angle, 0.0, 0.0, 1.0);

			glColor4f(CLR_ARROWHEAD_HOT);
			int offset = this->nearestPathPointOff * this->zoomFactor;
			glBegin(GL_LINE_STRIP);
			glVertex2i(-PATH_ARROWHEAD_SIZE, -PATH_ARROWHEAD_SIZE + offset);
			glVertex2i(0, offset);
			glVertex2i(PATH_ARROWHEAD_SIZE, -PATH_ARROWHEAD_SIZE + offset);
			glEnd();

			glPopMatrix();
		}

		glPopMatrix();

		glLineWidth(1.0);
		glDisable(GL_LINE_SMOOTH);
	}

	// Draw the viewport overlay
	if (this->visibleElements[ElViewport]) {
		unsigned int vpX, vpY;
		this->map->getViewport(&vpX, &vpY);
		int vpOffX = (winSize.x - (signed)vpX) / 2;
		int vpOffY = (winSize.y - (signed)vpY) / 2;

		// Draw a line around the viewport
		glColor4f(1.0, 1.0, 1.0, 0.3);
		glBegin(GL_LINE_LOOP);
		glVertex2i(vpOffX, vpOffY + 0);
		glVertex2i(vpOffX, vpOffY + (signed)vpY);
		glVertex2i(vpOffX + (signed)vpX, vpOffY + (signed)vpY);
		glVertex2i(vpOffX + (signed)vpX, vpOffY + 0);
		glEnd();

		// Darken the area outside the viewport
		glColor4f(0.0, 0.0, 0.0, 0.3);
		glBegin(GL_QUAD_STRIP);
		glVertex2i(0, 0);
		glVertex2i(vpOffX, vpOffY);
		glVertex2i(0, winSize.y);
		glVertex2i(vpOffX, vpOffY + (signed)vpY);
		glVertex2i(winSize.x, winSize.y);
		glVertex2i(vpOffX + (signed)vpX, vpOffY + (signed)vpY);
		glVertex2i(winSize.x, 0);
		glVertex2i(vpOffX + (signed)vpX, vpOffY);
		glVertex2i(0, 0);
		glVertex2i(vpOffX, vpOffY);
		glEnd();
	}

	// If the grid is visible, draw it using the tile size of the primary layer
	if ((this->gridVisible) && (this->primaryLayer >= ElementCount)) {
		Map2D::LayerPtr layer = this->map->getLayer(this->primaryLayer - ElementCount);
		unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
		getLayerDims(this->map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);

		this->drawGrid(tileWidth, tileHeight);
	}
/*
	if (this->editingMode == ObjectMode) {
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
	}
*/
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

bool MapCanvas::clearSelection()
{
	bool cleared = false;
	for (ItemsInLayers::iterator
		itSelLayer = this->selection.layers.begin();
		itSelLayer != this->selection.layers.end(); itSelLayer++
	) {
		if (!itSelLayer->empty()) {
			cleared = true;
			itSelLayer->clear();
		}
	}
	this->selection.width = 0;
	this->selection.height = 0;
	return cleared;
}

bool MapCanvas::haveSelection()
{
	return (this->selection.width != 0) && (this->selection.height != 0);
}

bool MapCanvas::isTileSelected(unsigned int layerNum,
	const Map2D::Layer::ItemPtr& c)
{
	const Map2D::Layer::ItemPtrVector& items = this->selection.layers[layerNum];
	return std::find(items.begin(), items.end(), c) != items.end();
}

bool MapCanvas::focusObject(ObjectVector::iterator start)
{
	bool needRedraw = false;

	// See if the mouse is over an object
	int mapPointerX = CLIENT_TO_MAP_X(this->pointerX);
	int mapPointerY = CLIENT_TO_MAP_Y(this->pointerY);

	ObjectVector::iterator oldFocusedObject = this->focusedObject;
	ObjectVector::iterator end = this->objects.end();
	this->focusedObject = this->objects.end(); // default to nothing focused

	// No active layer.  Only return true (redraw) if this caused
	// the selected object to be deselected.
	if (this->activeLayer < ElementCount) return (this->focusedObject != oldFocusedObject);

	Map2D::LayerPtr layer = this->map->getLayer(this->activeLayer - ElementCount);
	unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
	getLayerDims(this->map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);

	for (int n = 0; n < 2; n++) {
		for (ObjectVector::iterator i = start; i != end; i++) {
			if (
				(mapPointerX >= (signed)(i->x * tileWidth) - FOCUS_BOX_PADDING) &&
				(mapPointerX < (signed)((i->x + i->width) * tileWidth + FOCUS_BOX_PADDING)) &&
				(mapPointerY >= (signed)(i->y * tileHeight) - FOCUS_BOX_PADDING) &&
				(mapPointerY < (signed)((i->y + i->height) * tileHeight + FOCUS_BOX_PADDING))
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
			(mapPointerX >= (signed)(this->focusedObject->x * tileWidth) - FOCUS_BOX_PADDING) &&
			(mapPointerX < (signed)(this->focusedObject->x * tileWidth) + FOCUS_BOX_PADDING)
		) {
			this->resizeX = -1;
		} else if (
			(mapPointerX >= (signed)((this->focusedObject->x + this->focusedObject->width) * tileWidth) - FOCUS_BOX_PADDING) &&
			(mapPointerX < (signed)((this->focusedObject->x + this->focusedObject->width) * tileWidth) + FOCUS_BOX_PADDING)
		) {
			this->resizeX = 1;
		}

		if (
			(mapPointerY >= (signed)(this->focusedObject->y * tileHeight) - FOCUS_BOX_PADDING) &&
			(mapPointerY < (signed)(this->focusedObject->y * tileHeight) + FOCUS_BOX_PADDING)
		) {
			this->resizeY = -1;
		} else if (
			(mapPointerY >= (signed)((this->focusedObject->y + this->focusedObject->height) * tileHeight) - FOCUS_BOX_PADDING) &&
			(mapPointerY < (signed)((this->focusedObject->y + this->focusedObject->height) * tileHeight) + FOCUS_BOX_PADDING)
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

void MapCanvas::putSelection(int x, int y)
{
	unsigned int layerCount = map->getLayerCount();
	for (unsigned int l = 0; l < layerCount; l++) {
		// Skip layers with no selected tiles
		if (this->selection.layers[l].size() == 0) continue;

		// Skip inactive layers
		if (!this->activeLayers[l]) continue;

		const Map2D::Layer::ItemPtrVector& selItems = this->selection.layers[l];

		Map2D::LayerPtr layer = this->map->getLayer(l);
		unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
		getLayerDims(this->map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);

		// Origin of paste area
		int oX = (CLIENT_TO_MAP_X(x) - this->selection.width / 2) / (signed)tileWidth;
		int oY = (CLIENT_TO_MAP_Y(y) - this->selection.height / 2) / (signed)tileHeight;

		Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();

		// Make a copy of the selection, so we can remove tiles as we paste them,
		// without affecting the original selection.
		Map2D::Layer::ItemPtrVector selCopy = selItems;

		// Paste area rectangle (same size as selection area, transposed to paste origin)
		int pasteX1 = oX;
		int pasteY1 = oY;
		int pasteX2 = oX + this->selection.width / (signed)tileWidth;
		int pasteY2 = oY + this->selection.height / (signed)tileHeight;

		// Coordinates of upper-left corner of selection rectangle.
		int selOriginX = this->selection.x / tileWidth;
		int selOriginY = this->selection.y / tileHeight;

		// Overwrite any existing tiles
		for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
			if (POINT_IN_RECT((signed)(*c)->x, (signed)(*c)->y, pasteX1, pasteY1, pasteX2, pasteY2)) {
				// This tile is within the paste area, so see if we can paste it.

				// Calculate the offset into the selection where this tile resides.  For
				// example, if the click is at (10,10) and this tile is at (11,12) then
				// the offset will be (1,2).
				int selOffX = (*c)->x - pasteX1;
				int selOffY = (*c)->y - pasteY1;

				// Now find if we have a tile to place here.
				int dstX = (*c)->x;
				int dstY = (*c)->y;
				for (Map2D::Layer::ItemPtrVector::iterator p = selCopy.begin(); p != selCopy.end(); p++) {
					if (
						((signed)(*p)->x - selOriginX == selOffX) &&
						((signed)(*p)->y - selOriginY == selOffY)
					) {
						// This tile should replace the current one

						unsigned int maxInstances;
						if (!layer->tilePermittedAt(*p, dstX, dstY, &maxInstances)) continue;
						if (maxInstances) {
							// There is a count limit on this code
							unsigned int curInstances = 0;
							for (Map2D::Layer::ItemPtrVector::iterator
								c2 = content->begin(); c2 != content->end(); c2++
							) {
								if ((*p)->code == (*c2)->code) curInstances++;
								if (curInstances >= maxInstances) break;
							}
							// Don't place this tile if we have already reached the maximum allowed
							if (curInstances >= maxInstances) continue;
						}

						// Overwrite the cell
						c->reset(new Map2D::Layer::Item);
						**c = **p;
						(*c)->x = dstX;
						(*c)->y = dstY;
						this->doc->isModified = true;

						// Remove the tile from the copy of the selection, since we have
						// now placed it (and won't need to check/place it again.)
						selCopy.erase(p);
						break;
					}
				}
			}
		}

		// Now the tiles left in selCopy are those that did not overwrite any
		// existing tiles, so we have to add them.
		for (Map2D::Layer::ItemPtrVector::iterator
			p = selCopy.begin(); p != selCopy.end(); p++
		) {
			int tileX = pasteX1 + (*p)->x - selOriginX;
			int tileY = pasteY1 + (*p)->y - selOriginY;

			unsigned int maxInstances;
			if (!(
				!layer->tilePermittedAt(*p, tileX, tileY, &maxInstances) ||
				(tileX < 0) ||
				(tileY < 0) ||
				(tileX >= (signed)layerWidth) ||
				(tileY >= (signed)layerHeight)
			)) {
				if (maxInstances) {
					// There is a count limit on this code
					unsigned int curInstances = 0;
					for (Map2D::Layer::ItemPtrVector::iterator
						c = content->begin(); c != content->end(); c++
					) {
						if ((*p)->code == (*c)->code) curInstances++;
						if (curInstances >= maxInstances) break;
					}
					// Don't place this tile if we have already reached the maximum allowed
					if (curInstances >= maxInstances) continue;
				}

				Map2D::Layer::ItemPtr newItem(new Map2D::Layer::Item);
				*newItem = **p;
				newItem->x = tileX;
				newItem->y = tileY;
				content->push_back(newItem);
				this->doc->isModified = true;
			}
		}

	} // for (each layer)
	return;
}

void MapCanvas::onMouseMove(wxMouseEvent& ev)
{
	this->pointerX = ev.m_x;
	this->pointerY = ev.m_y;

	int mapPointerX = CLIENT_TO_MAP_X(this->pointerX);
	int mapPointerY = CLIENT_TO_MAP_Y(this->pointerY);

	// Perform actions that require a real primary layer
	if (this->primaryLayer >= ElementCount) {
		Map2D::LayerPtr layer = this->map->getLayer(this->primaryLayer - ElementCount);
		unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
		getLayerDims(this->map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);

		int pointerTileX = mapPointerX / (signed)tileWidth;
		int pointerTileY = mapPointerY / (signed)tileHeight;

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
					(tileX <= mapPointerX) && (tileX + (signed)tileWidth > mapPointerX) &&
					(tileY <= mapPointerY) && (tileY + (signed)tileHeight > mapPointerY)
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
	} else { // primary layer is an element layer
		this->doc->setStatusText(wxString());
	}

		// Perform the primary action (left mouse drag) if it is active
		if (this->actionFromX >= 0) {
			switch (this->editingMode) {
				case TileMode: {
					this->putSelection(ev.m_x, ev.m_y);
					this->needRedraw = true;
					break;
				}
			}
		} else {
			// Not performing the primary action

			// Redraw the selection area if the user is right-dragging
			if (this->selectFromX >= 0) {
				this->needRedraw = true;

			} else {
				// Not selecting
				switch (this->editingMode) {
					case TileMode:
						// If there's a current selection, overlay it
						if (this->haveSelection()) {
							this->needRedraw = true;
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
						if (this->focusObject(start)) this->needRedraw = true;
						break;
				}
			}

		} // if (not performing primary action)

	if (this->activeElement == 1 + ElPaths) {

		if (this->pathSelection.size() > 0) {
			// There's a selection, so redraw on every mouse movement because the
			// preview will need to be updated to show where the path will go on
			// a left-click.
			this->needRedraw = true;

		} else if (this->selectFromX >= 0) {
			// The user is right-dragging, redraw the selection box
			this->needRedraw = true;

		} else {
			// No selection or right dragging, see how close the mouse is
			bool bWasVisible = this->nearestPathPointOff >= 0;
			this->nearestPathPointOff = -1;
			Map2D::PathPtrVectorPtr paths = this->map->getPaths();
			for (Map2D::PathPtrVector::iterator p = paths->begin(); p != paths->end(); p++) {
				Map2D::PathPtr path = *p;
				int stnum = 0;
				for (Map2D::Path::point_vector::iterator st = (*p)->start.begin(); st != (*p)->start.end(); st++) {
					int ptnum = 0;

					int lastX = st->first;
					int lastY = st->second;

					for (Map2D::Path::point_vector::iterator pt = (*p)->points.begin(); pt != (*p)->points.end(); pt++) {
						int nextX = st->first + pt->first;
						int nextY = st->second + pt->second;

						int dX = nextX - lastX, dY = nextY - lastY;
						float lineLength = sqrt((double)(dX*dX + dY*dY));
						float angle = acos(dY / lineLength);
						if (dX < 0) angle *= -1;

						// Construct a rectangle with the current line segment running down
						// the middle, but rotate it so the line segment is completely
						// vertical.
						int x1 = lastX - (10 / this->zoomFactor);
						int y1 = lastY;
						int x2 = lastX + (10 / this->zoomFactor);
						int y2 = lastY + lineLength;

						// Rotate the mouse cursor around the point, so we can pretend the
						// line between the two points is straight.
						int rPointerX = (mapPointerX - lastX) * cos(angle) - (mapPointerY - lastY) * sin(angle) + lastX;
						int rPointerY = (mapPointerX - lastX) * sin(angle) + (mapPointerY - lastY) * cos(angle) + lastY;
						if (POINT_IN_RECT(rPointerX, rPointerY, x1, y1, x2, y2)) {
							// The cursor is close enough to this line that we should show
							// an insertion point.
							this->nearestPathPoint.path = path;
							this->nearestPathPoint.start = stnum;
							this->nearestPathPoint.point = ptnum;
							this->nearestPathPointOff = rPointerY - y1;
							this->needRedraw = true;
							break;
						}

						lastX = nextX;
						lastY = nextY;
						ptnum++;
					}
					if (this->nearestPathPointOff >= 0) break; // done
					stnum++;
				}
				if (this->nearestPathPointOff >= 0) break; // done
			}
			if ((bWasVisible) && (this->nearestPathPointOff == -1)) {
				// There is currently a path point highlight on the screen but the mouse
				// has now moved too far away, so we need to do a final redraw to hide
				// the highlight.
				this->needRedraw = true;
				this->updateHelpText(); // hide mention of the key to add a point
			} else if ((!bWasVisible) && (this->nearestPathPointOff >= 0)) {
				// The mouse has just gotten close enough to for the highlight to
				// reappear, so update the status bar to show that a new point can
				// now be inserted.
				this->updateHelpText();
			}
		}
	}

	this->MapBaseCanvas::onMouseMove(ev);

	if (this->needRedraw) this->redraw();
	return;
}

void MapCanvas::onMouseDownLeft(wxMouseEvent& ev)
{
	bool needRedraw = false;
	if (this->activeElement == 1 + ElPaths) {
		// Should never be able to set this layer as active if there are no paths
		// in the map

		if (!this->pathSelection.empty()) {
			int mapPointerX = CLIENT_TO_MAP_X(ev.m_x);
			int mapPointerY = CLIENT_TO_MAP_Y(ev.m_y);
			int selDeltaX = mapPointerX - this->selHotX;
			int selDeltaY = mapPointerY - this->selHotY;
			for (std::vector<path_point>::iterator spt = this->pathSelection.begin();
				spt != this->pathSelection.end(); spt++
			) {
				if (spt->point == 0) {
					// Need to move a starting point
					if (spt->path->fixed) {
						this->doc->setStatusText(_("Path starting point is fixed in this map"));
					} else {
						assert(spt->start < spt->path->start.size());
						Map2D::Path::point &pt = spt->path->start[spt->start];
						pt.first += selDeltaX;
						pt.second += selDeltaY;
						needRedraw = true;
						this->doc->isModified = true;
					}
				} else {
					// Just a normal point
					Map2D::Path::point_vector::iterator pt = spt->path->points.begin() + (spt->point - 1);
					if (pt != spt->path->points.end()) {
						pt->first += selDeltaX;
						pt->second += selDeltaY;
						this->doc->isModified = true;
					}
				}
			}
			// Update the hotspot so points can be moved again
			this->selHotX += selDeltaX;
			this->selHotY += selDeltaY;
		}

	} else { // normal layer

		switch (this->editingMode) {

			case TileMode: {
				// Paint the current selection, if any
				if (
					(this->activeElement == 0) && // if a real layer is active
					(this->haveSelection()) &&    // and there are selected tiles
					(this->selectFromX < 0)       // and we're not selecting new tiles
				) {
					this->actionFromX = ev.m_x;
					this->actionFromY = ev.m_y;
					this->CaptureMouse();
					this->putSelection(ev.m_x, ev.m_y);
					needRedraw = true;
				}
				break;
			}

			case ObjectMode:
				if (this->resizeX || this->resizeY) {
					// Mouse was over focus border, start a resize
					this->actionFromX = ev.m_x;
					this->actionFromY = ev.m_y;
					this->CaptureMouse();
				}
				break;
		}
	}
	ev.Skip();

	if (needRedraw) this->redraw();
	this->updateHelpText();
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
	this->nearestPathPointOff = -1; // hide highlight
	return;
}

void MapCanvas::onMouseUpRight(wxMouseEvent& ev)
{
	this->ReleaseMouse();

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

	x1 = CLIENT_TO_MAP_X(x1);
	y1 = CLIENT_TO_MAP_Y(y1);
	x2 = CLIENT_TO_MAP_X(x2);
	y2 = CLIENT_TO_MAP_Y(y2);

	// Set the selection hotspot to be the middle of the rectangle
	this->selHotX = CLIENT_TO_MAP_X(ev.m_x);
	this->selHotY = CLIENT_TO_MAP_Y(ev.m_y);

	if (this->activeElement == 1 + ElPaths) { // select some points on a path
		// Should never be able to set this layer as active if there are no paths
		// in the map
		Map2D::PathPtrVectorPtr paths = this->map->getPaths();
		assert(paths);

		this->pathSelection.clear();
		this->doc->setStatusText(wxString()); // hide any previous error message
		for (Map2D::PathPtrVector::iterator p = paths->begin();
			p != paths->end(); p++
		) {
			int stnum = 0;
			for (Map2D::Path::point_vector::iterator st = (*p)->start.begin();
				st != (*p)->start.end(); st++
			) {
				if (POINT_IN_RECT(
					st->first, st->second,
					x1, y1, x2, y2)
				) {
					path_point pp;
					pp.path = *p;
					pp.start = stnum;
					pp.point = 0;
					this->pathSelection.push_back(pp);
					if ((*p)->fixed) {
						this->doc->setStatusText(_("You have selected the path's starting"
							" point, but it cannot be moved in this map."));
					}
				} else {
					// Starting point wasn't selected, add individual points for this path
					int ptnum = 1;
					for (Map2D::Path::point_vector::iterator pt = (*p)->points.begin(); pt != (*p)->points.end(); pt++) {
						if (POINT_IN_RECT(
							st->first + pt->first, st->second + pt->second,
							x1, y1, x2, y2)
						) {
							// This point was inside the selection rectangle
							path_point pp;
							pp.path = *p;
							pp.start = stnum;
							pp.point = ptnum;
							this->pathSelection.push_back(pp);
						}
						ptnum++;
					}
				}
			}
			stnum++;
		}

	} else if (this->activeElement == 0) { // normal map layer(s)

		switch (this->editingMode) {
			case TileMode: {
				// Select the tiles contained within the rectangle

				if (this->haveSelection()) this->clearSelection();

				// Remember the location of the original selection
				this->selection.x = x1;
				this->selection.y = y1;
				this->selection.width = 0; // updated below
				this->selection.height = 0;

				unsigned int layerCount = this->map->getLayerCount();
				for (unsigned int l = 0; l < layerCount; l++) {
					// Skip inactive layers
					if (!this->activeLayers[l]) continue;

					Map2D::LayerPtr layer = this->map->getLayer(l);
					unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
					getLayerDims(this->map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);

					Map2D::Layer::ItemPtrVector& selItems = this->selection.layers[l];

					// Convert the selection rectangle from pixel coords to tile coords,
					// enlarging or shrinking it as necessary to accommodate selectPartial.
					int x1t = x1;
					int y1t = y1;
					int x2t = x2;
					int y2t = y2;
					if (selectPartial) {
						x2t += tileWidth - 1;
						y2t += tileHeight - 1;
					} else {
						x1t += tileWidth - 1;
						y1t += tileHeight - 1;
					}

					if (x1t < 0) x1t = 0;
					if (y1t < 0) y1t = 0;
					if (x2t < 0) x2t = 0;
					if (y2t < 0) y2t = 0;

					x1t /= tileWidth;
					y1t /= tileHeight;
					x2t /= tileWidth;
					y2t /= tileHeight;

					unsigned int minX = x2t;
					unsigned int minY = y2t;
					unsigned int maxX = x1t;
					unsigned int maxY = y1t;
					Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
					for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
						if (POINT_IN_RECT(
								(signed)(*c)->x, (signed)(*c)->y,
								x1t, y1t, x2t, y2t
							)) {
							// This tile is contained within the selection rectangle
							if (minX > (*c)->x) minX = (*c)->x;
							if (minY > (*c)->y) minY = (*c)->y;
							if (maxX < (*c)->x + 1) maxX = (*c)->x + 1;
							if (maxY < (*c)->y + 1) maxY = (*c)->y + 1;
						}
					}
					if ((maxX < minX) || (maxY < minY)) {
						// Empty selection
						selItems.clear();
						//this->clearSelection();
					} else {
						this->selection.width = std::max(this->selection.width, (signed)((maxX - minX) * tileWidth));
						this->selection.height = std::max(this->selection.height, (signed)((maxY - minY) * tileHeight));

						// Run through the tiles again, but this time copy them into the selection
						for (Map2D::Layer::ItemPtrVector::iterator c = content->begin(); c != content->end(); c++) {
							if (POINT_IN_RECT((*c)->x, (*c)->y, minX, minY, maxX, maxY)) {
								// This tile is contained within the selection rectangle
								selItems.push_back(*c);
							}
						}
					}
				}
				break;
			}
			case ObjectMode:
				// TODO: Select multiple objects?
				break;
		}
	}

	this->selectFromX = -1;
	this->redraw();
	this->updateHelpText();
	return;
}

void MapCanvas::onMouseCaptureLost(wxMouseCaptureLostEvent& ev)
{
	this->MapBaseCanvas::onMouseCaptureLost(ev);
	this->selectFromX = -1;
	this->updateHelpText();
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

		case WXK_NUMPAD_INSERT:
		case WXK_INSERT:
			if ((this->activeElement == 1 + ElPaths) && (this->nearestPathPointOff >= 0)) {
				Map2D::PathPtr path = this->nearestPathPoint.path;
				assert(path);
				assert(path->points.size() > 1);
				assert(path->start.size() > this->nearestPathPoint.start);
				if ((path->maxPoints > 0) && (path->points.size() >= path->maxPoints)) {
					this->doc->setStatusText(_("Path is at maximum size, cannot insert "
						"another point"));
					break;
				}
				Map2D::Path::point ptA, ptB;
				Map2D::Path::point_vector::iterator beforeThis;
				if (this->nearestPathPoint.point == 0) {
					beforeThis = path->points.begin();
					ptA.first = ptA.second = 0;
					ptB = path->points[0];
				} else {
					unsigned int index = this->nearestPathPoint.point - 1;
					ptA = path->points[index];
					index++;
					if (index < path->points.size()) {
						beforeThis = path->points.begin() + index;
						ptB = *beforeThis;//path->points[index];
					} else {
						beforeThis = path->points.end();
						ptB.first = ptB.second = 0; // loop back to origin
						// We don't need to check whether the path is a closed-loop type,
						// because if it isn't we'd never have the last point in the path
						// selected as the nearest point.
					}
				}
				int dX = ptB.first - ptA.first, dY = ptB.second - ptA.second;
				float lineLength = sqrt((double)(dX*dX + dY*dY));
				float angle = acos(dY / lineLength);
				if (dX < 0) angle *= -1;
				Map2D::Path::point newPoint;
				newPoint.first = ptA.first + this->nearestPathPointOff * sin(angle);
				newPoint.second = ptA.second + this->nearestPathPointOff * cos(angle);
				path->points.insert(beforeThis, newPoint);
				this->doc->isModified = true;
				this->nearestPathPointOff = -1; // hide highlight
				needRedraw = true;
			}
			break;

		case WXK_NUMPAD_DELETE:
		case WXK_DELETE:
			// Delete the currently selected tiles or path points
			if (this->activeElement == 1 + ElPaths) {
				if (!this->pathSelection.empty()) {
					for (std::vector<path_point>::iterator spt = this->pathSelection.begin();
						spt != this->pathSelection.end(); spt++
					) {
						if (spt->point == 0) {
							if (this->map->getCaps() && Map2D::FixedPathCount) {
								this->doc->setStatusText(_("Can't delete whole paths in this "
									"map - you tried to delete the starting point"));
							} else {
								wxMessageDialog dlg(this, _("You are about to delete the "
									"starting point for a path.  If you proceed, the entire path "
									"will be removed.\n\nDelete the entire path?"),
									_("Delete path?"), wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION);
								int r = dlg.ShowModal();
								if (r == wxID_NO) return;

								// Remove this path from the selection first.
								Map2D::PathPtr path = spt->path;
								unsigned int start = spt->start;
								for (std::vector<path_point>::iterator spt2 = this->pathSelection.begin();
									spt2 != this->pathSelection.end(); spt2++
								) {
									if ((spt2->path == path) && (spt2->start = start)) {
										spt2 = this->pathSelection.erase(spt2) - 1;
										// spt is now invalid too
										needRedraw = true;
									}
								}

								// Remove the instance
								Map2D::Path::point_vector::iterator x = path->start.begin() + start;
								if (x != path->start.end()) {
									path->start.erase(x);
									this->doc->isModified = true;
									needRedraw = true;
								}

								if (path->start.empty()) {
									// No start points, the path is now unused, delete it
									Map2D::PathPtrVectorPtr allPaths = this->map->getPaths();
									for (Map2D::PathPtrVector::iterator x = allPaths->begin();
										x != allPaths->end(); x++
									) {
										if (*x == path) {
											allPaths->erase(x);
											this->doc->isModified = true;
											break;
										}
									}
								}

								// Start the main loop again in case there are other paths to delete
								spt = this->pathSelection.begin();
							}
						} else {
							// Just a normal point to remove
							Map2D::Path::point_vector::iterator pt = spt->path->points.begin() + (spt->point - 1);
							if (pt != spt->path->points.end()) {
								// Remove it from the path
								spt->path->points.erase(pt);
								this->doc->isModified = true;

								// Update the index numbers in the remaining selection to take
								// into account the point we're about to delete
								Map2D::PathPtr path = spt->path;
								unsigned int start = spt->start;
								unsigned int point = spt->point;
								for (std::vector<path_point>::iterator spt2 = this->pathSelection.begin();
									spt2 != this->pathSelection.end(); spt2++
								) {
									if ((spt2->path == path) && (spt2->start == start) && (spt2->point > point)) {
										spt2->point--;
									}
								}

								// Remove it from the selection
								spt = this->pathSelection.erase(spt) - 1;

								needRedraw = true;
							}
						}
					}
				}
			} else if ((this->activeElement == 0) && (this->haveSelection())) {
				unsigned int layerCount = map->getLayerCount();
				for (unsigned int l = 0; l < layerCount; l++) {
					// Skip layers with no selection
					if (this->selection.layers[l].size() == 0) continue;

					Map2D::LayerPtr layer = this->map->getLayer(l);
					Map2D::Layer::ItemPtrVectorPtr content = layer->getAllItems();
					for (Map2D::Layer::ItemPtrVector::iterator
						c = content->begin(); c != content->end();
					) {
						bool erased = false;
						for (Map2D::Layer::ItemPtrVector::iterator
							s = this->selection.layers[l].begin();
							s != this->selection.layers[l].end();
							s++
						) {
							if (*c == *s) {
								c = content->erase(c);
								erased = true;
								this->doc->isModified = true;
								needRedraw = true;
								break;
							}
						}
						if (!erased) c++;
					}
				}
			}
			// Hide any nearest point so it will be recalculated on the next mouse move
			// TODO: Ideally we'd recalculate it here before doing the redraw...
			this->nearestPathPointOff = -1;
			break;

		case WXK_ESCAPE:
			// Clear the active selection
			if (this->clearSelection()) needRedraw = true;
			if (!this->pathSelection.empty()) {
				this->pathSelection.clear();
				this->doc->setStatusText(wxString()); // hide any previous error message
				needRedraw = true;
			}
			break;

		case WXK_SPACE:
			if (this->activeElement == 1 + ElPaths) {
				// Close all currently selected paths, by moving the last point in each
				// selected path to (0,0).
				if (!this->pathSelection.empty()) {
					for (std::vector<path_point>::iterator spt = this->pathSelection.begin();
						spt != this->pathSelection.end(); spt++
					) {
						if (spt->path->forceClosed) {
							this->doc->setStatusText(_("Can't close a continuous path!"));
						} else {
							if (!spt->path->points.empty()) {
								int num = spt->path->points.size();
								Map2D::Path::point &p = spt->path->points[num - 1];
								p.first = 0;
								p.second = 0;
								this->doc->isModified = true;
								needRedraw = true;
							}
						}
					}
				}
			}
			break;

	}
	if (needRedraw) this->redraw();
	this->updateHelpText();
	return;
}

void MapCanvas::updateHelpText()
{
	if (this->activeElement == 1 + ElPaths) { // select some points on a path
		if (this->pathSelection.empty()) {
			if (this->nearestPathPointOff >= 0) {
				Map2D::PathPtr path = this->nearestPathPoint.path;
				assert(path);
				if ((path->maxPoints == 0) || (path->points.size() < path->maxPoints)) {
					return this->doc->setHelpText(wxString(HT_INS) + __ + HT_SELECT + __ +  HT_SCROLL);
				}
			}
			return this->doc->setHelpText(wxString(HT_SELECT) + __ + HT_SCROLL);
		} else {
			return this->doc->setHelpText(wxString(HT_APPLY) + __ + HT_CLOSEPATH + __ + HT_DEL
				+ __ + HT_SELECT + __ + HT_ESC);
		}

	} else if (this->activeElement == 1 + ElViewport) {
		return this->doc->setHelpText(HT_SCROLL);

	} else if (this->activeElement == 0) { // normal map layer
		if (this->haveSelection()) {
			return this->doc->setHelpText(wxString(HT_PASTE) + __ + HT_DEL + __ + HT_COPY + __
				+ HT_ESC);
		} else { // no selection
			return this->doc->setHelpText(wxString(HT_COPY) + __ + HT_SCROLL);
		}
	}

	this->doc->setHelpText(HT_DEFAULT);
	return;
}
