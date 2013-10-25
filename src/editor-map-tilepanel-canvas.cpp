/**
 * @file   editor-map-tilepanel-canvas.cpp
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

#include <boost/bind.hpp>
#include "editor-map-tilepanel-canvas.hpp"

inline bool clamp(int *val, int min, int max) {
	if (*val < min) {
		*val = min;
		return true;
	}
	if (*val > max) {
		*val = max;
		return true;
	}
	return false;
}

using namespace camoto;
using namespace camoto::gamemaps;

BEGIN_EVENT_TABLE(TilePanelCanvas, MapBaseCanvas)
	EVT_PAINT(TilePanelCanvas::onPaint)
	EVT_ERASE_BACKGROUND(TilePanelCanvas::onEraseBG)
	EVT_SIZE(TilePanelCanvas::onResize)
	EVT_RIGHT_DOWN(TilePanelCanvas::onRightClick)
END_EVENT_TABLE()

TilePanelCanvas::TilePanelCanvas(TilePanel *parent, wxGLContext *glcx,
	int *attribList)
	:	MapBaseCanvas(parent, glcx, attribList),
		tilePanel(parent),
		tilesX(0),
		offset(0)
{
	assert(glcx);
}

TilePanelCanvas::~TilePanelCanvas()
{
}

void TilePanelCanvas::notifyNewDoc()
{
	this->calcCurrentExtents();
	this->redraw();
	return;
}

void TilePanelCanvas::onEraseBG(wxEraseEvent& ev)
{
	return;
}

void TilePanelCanvas::onPaint(wxPaintEvent& ev)
{
	wxPaintDC dummy(this);
	this->redraw();
	return;
}

void TilePanelCanvas::onResize(wxSizeEvent& ev)
{
	this->Layout();
	this->calcCurrentExtents();
	return;
}

void TilePanelCanvas::glReset()
{
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

void TilePanelCanvas::redraw()
{
	if (!this->IsShownOnScreen()) return;

	// Have to call glReset() every time, since the context is shared with the
	// main map canvas and it will change the projection since its window will
	// be a different size to us.
	this->glReset();

	glClear(GL_COLOR_BUFFER_BIT);

	// This may not do anything if no valid layer is selected.
	this->drawCanvas();

	// But we want to swap buffers anyway so the canvas is at least cleared if
	// there are no tiles available for selection.
	this->SwapBuffers();

	this->needRedraw = false;
	return;
}

void TilePanelCanvas::drawCanvas()
{
	// Some things for code readability
	MapDocument *& doc = this->tilePanel->doc;
	if (!doc) return;
	Map2DPtr& map = this->tilePanel->doc->map;
	if (!map) return;
	MapCanvas *& mapCanvas = this->tilePanel->doc->canvas;

	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Enable on/off transparency
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0);

	// Draw all the visible tiles
	this->forAllTiles(boost::bind(&TilePanelCanvas::drawMapItem, this, _1, _2, _3, _4, _5, _6), false);

	glDisable(GL_TEXTURE_2D);

	// If the grid is visible, draw it using the tile size
	if (this->gridVisible) {
		// Can't select tiles for an element layer
		if (mapCanvas->primaryLayer < MapCanvas::ElementCount) return;
		unsigned int layerIndex = mapCanvas->primaryLayer - MapCanvas::ElementCount;
		Map2D::LayerPtr layer = map->getLayer(layerIndex);

		unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
		getLayerDims(map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);
		if ((tileWidth > 0) && (tileHeight > 0)) {
			this->drawGrid(tileWidth, tileHeight);
		}
	}

	return;
}

void TilePanelCanvas::setTilesX(int t)
{
	this->tilesX = t;
	this->calcCurrentExtents();
	this->redraw();
	return;
}

void TilePanelCanvas::setOffset(unsigned int o)
{
	this->offset = o;
	this->calcCurrentExtents();
	this->redraw();
	return;
}

void TilePanelCanvas::onRightClick(wxMouseEvent& ev)
{
	// Some things for code readability
	MapDocument *& doc = this->tilePanel->doc;
	if (!doc) return;
	Map2DPtr& map = this->tilePanel->doc->map;
	if (!map) return;
	MapCanvas *& mapCanvas = this->tilePanel->doc->canvas;

	// Can't select tiles for an element layer
	if (mapCanvas->primaryLayer < MapCanvas::ElementCount) return;
	unsigned int layerIndex = mapCanvas->primaryLayer - MapCanvas::ElementCount;

	unsigned int pointerX = CLIENT_TO_MAP_X(ev.m_x);
	unsigned int pointerY = CLIENT_TO_MAP_Y(ev.m_y);
	Map2D::Layer::ItemPtrVector& selItems = mapCanvas->selection.layers[layerIndex];
	selItems.clear();
	mapCanvas->selection.x = 0;
	mapCanvas->selection.y = 0;
	mapCanvas->selection.width = 0;
	mapCanvas->selection.height = 0;

	TSMIDATA tsmi;
	tsmi.mapCanvas = mapCanvas;
	tsmi.selItems = &selItems;
	tsmi.pointerX = pointerX;
	tsmi.pointerY = pointerY;
	// Check all the visible tiles to see which one was clicked
	this->forAllTiles(boost::bind(&TilePanelCanvas::testSelectMapItem, this,
		tsmi, _1, _2, _3, _4, _5, _6), false);

	return;
}

void TilePanelCanvas::onMouseMove(wxMouseEvent& ev)
{
	this->MapBaseCanvas::onMouseMove(ev);
	this->clampScroll();
	if (this->needRedraw) this->redraw();
	return;
}

void TilePanelCanvas::clampScroll()
{
	if (clamp(&this->offX, 0, this->maxScrollX)) this->needRedraw = true;
	if (clamp(&this->offY, 0, this->maxScrollY)) this->needRedraw = true;
	return;
}

void TilePanelCanvas::calcCurrentExtents()
{
	this->fullWidth = 0;
	this->fullHeight = 0;

	// Recalculate fullWidth and fullHeight
	this->forAllTiles(boost::bind(
		&TilePanelCanvas::updateExtent, this, _1, _2, _3, _4, _5, _6
	), true);

	wxSize s = this->GetClientSize();
	s.x /= this->zoomFactor;
	s.y /= this->zoomFactor;

	this->maxScrollX = std::max(0, (signed)this->fullWidth - s.x);
	this->maxScrollY = std::max(0, (signed)this->fullHeight - s.y);

	this->clampScroll();
	return;
}

void TilePanelCanvas::forAllTiles(fn_foralltiles fnCallback, bool inclHidden)
{
	// Some things for code readability
	MapDocument *& doc = this->tilePanel->doc;
	if (!doc) return;
	Map2DPtr& map = this->tilePanel->doc->map;
	if (!map) return;
	MapCanvas *& mapCanvas = this->tilePanel->doc->canvas;

	// Can't draw tiles for an element layer
	if (mapCanvas->primaryLayer < MapCanvas::ElementCount) return;
	unsigned int layerIndex = mapCanvas->primaryLayer - MapCanvas::ElementCount;

	// Get the OpenGL texture map from the editor canvas
	if (mapCanvas->textureMap.size() < layerIndex) this->SwapBuffers();
	const TEXTURE_MAP& tm = mapCanvas->textureMap[layerIndex];

	Map2D::LayerPtr layer = map->getLayer(layerIndex);
	int layerCaps = layer->getCaps();
	unsigned int layerWidth, layerHeight, layerTileWidth, layerTileHeight;
	getLayerDims(map, layer, &layerWidth, &layerHeight, &layerTileWidth, &layerTileHeight);
	const Map2D::Layer::ItemPtrVectorPtr items = layer->getValidItemList();

	wxSize s = this->GetClientSize();
	s.x /= this->zoomFactor;
	s.y /= this->zoomFactor;

	unsigned int skip = this->offset;
	unsigned int pixelX = 0, pixelY = 0, nx = 0, maxHeight = 0, maxWidth = 0;
	for (Map2D::Layer::ItemPtrVector::const_iterator
		i = items->begin(); i != items->end(); i++
	) {
		if (skip) {
			skip--;
			continue;
		}
		const Texture& t = tm.at((*i)->code);

		unsigned int tileWidth, tileHeight;
		if (layerCaps & Map2D::Layer::UseImageDims) {
			tileWidth = t.width;
			tileHeight = t.height;
			if ((tileWidth == 0) || (tileHeight == 0)) {
				// Fall back to the layer tile size e.g. for blank images
				tileWidth = layerTileWidth;
				tileHeight = layerTileHeight;
			}
		} else {
			tileWidth = layerTileWidth;
			tileHeight = layerTileHeight;
		}

		if (this->tilesX == 0) { // fit width
			if ((pixelX + tileWidth) > (unsigned)s.x) {
				// This tile would go past the edge, start it on a new row
				if (pixelX > maxWidth) maxWidth = pixelX;
				pixelX = 0;
				nx = 0;
				pixelY += maxHeight;
				maxHeight = 0;
			}
		} else if (nx >= this->tilesX) { // fit to specific number of tiles wide
			if (pixelX > maxWidth) maxWidth = pixelX;
			pixelX = 0;
			nx = 0;
			pixelY += maxHeight;
			maxHeight = 0;
		}

		// If we've gone past the end of the window and the caller isn't interested
		// in these "hidden" tiles, finish now.
		if ((!inclHidden) && (pixelY > (unsigned)s.y + this->offY)) break;

		if (inclHidden
			|| (POINT_IN_RECT((signed)pixelX, (signed)pixelY,
				this->offX - (signed)tileWidth, this->offY - (signed)tileHeight,
				this->offX + s.x, this->offY + s.y
			))
		) {
			if (fnCallback(pixelX, pixelY, tileWidth, tileHeight, *i, &t)) break;
		}

		if (tileHeight > maxHeight) maxHeight = tileHeight;
		pixelX += tileWidth;
		nx++;
	}
	return;
}

bool TilePanelCanvas::updateExtent(int pixelX, int pixelY,
	unsigned int tileWidth, unsigned int tileHeight,
	const camoto::gamemaps::Map2D::Layer::ItemPtr item,
	const Texture *texture)
{
	if (pixelY + tileHeight > this->fullHeight) {
		this->fullHeight = pixelY + tileHeight;
	}
	if (pixelX + tileWidth > this->fullWidth) {
		this->fullWidth = pixelX + tileWidth;
	}
	return false;
}

bool TilePanelCanvas::testSelectMapItem(TSMIDATA tsmi, int pixelX,
	int pixelY, unsigned int tileWidth, unsigned int tileHeight,
	const camoto::gamemaps::Map2D::Layer::ItemPtr item, const Texture *texture)
{
	if (POINT_IN_RECT(tsmi.pointerX, tsmi.pointerY, pixelX, pixelY,
		pixelX + (signed)tileWidth, pixelY + (signed)tileHeight)
	) {
		// Mouse clicked on this tile
		tsmi.mapCanvas->selection.width = tileWidth;
		tsmi.mapCanvas->selection.height = tileHeight;
		tsmi.selItems->push_back(item);
		return true;
	}
	return false;
}
