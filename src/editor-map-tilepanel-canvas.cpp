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

#include "editor-map-tilepanel-canvas.hpp"

using namespace camoto;
using namespace camoto::gamemaps;

BEGIN_EVENT_TABLE(TilePanelCanvas, wxGLCanvas)
	EVT_PAINT(TilePanelCanvas::onPaint)
	EVT_ERASE_BACKGROUND(TilePanelCanvas::onEraseBG)
	EVT_SIZE(TilePanelCanvas::onResize)
	EVT_RIGHT_DOWN(TilePanelCanvas::onRightClick)
END_EVENT_TABLE()

TilePanelCanvas::TilePanelCanvas(TilePanel *parent, wxGLContext *glcx,
	int *attribList)
	: wxGLCanvas(parent, wxID_ANY, attribList),
	  glcx(glcx),
	  tilePanel(parent),
	  tilesX(0), // must be updated before first redraw() call
	  offset(0)
{
	assert(glcx);
	this->glReset();
}

TilePanelCanvas::~TilePanelCanvas()
{
}

void TilePanelCanvas::notifyNewDoc()
{
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
	this->glReset();
	return;
}

void TilePanelCanvas::glReset()
{
	if (!this->GetParent()->IsShown()) return;

	MapDocument *& doc = this->tilePanel->doc;
	if (!doc) return;
	MapCanvas *& mapCanvas = this->tilePanel->doc->canvas;

	this->SetCurrent(*this->glcx);
	wxSize s = this->GetClientSize();
	glViewport(0, 0, s.x, s.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, s.x / mapCanvas->zoomFactor, s.y / mapCanvas->zoomFactor, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.5, 0.5, 0.5, 1);
	glShadeModel(GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	return;
}

void TilePanelCanvas::redraw()
{
	// Have to call glReset() every time, since the context is shared with the
	// main map canvas and it will change the projection since its window will
	// be a different size to us.
	this->glReset();

	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Enable on/off transparency
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0);

	wxSize s = this->GetClientSize();

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
	if (mapCanvas->textureMap.size() < layerIndex) return;
	const TEXTURE_MAP& tm = mapCanvas->textureMap[layerIndex];

	Map2D::LayerPtr layer = map->getLayer(layerIndex);
	const Map2D::Layer::ItemPtrVectorPtr items = layer->getValidItemList();

	unsigned int x = 0, y = 0, nx = 0, maxHeight = 0;
	for (Map2D::Layer::ItemPtrVector::const_iterator
		i = items->begin(); i != items->end(); i++
	) {
		const Texture& t = tm.at((*i)->code);
		glBindTexture(GL_TEXTURE_2D, t.glid);

		if (this->tilesX == 0) { // fit width
			if ((x + t.width) * mapCanvas->zoomFactor > (unsigned)s.x) {
				// This tile would go past the edge, start it on a new row
				x = 0;
				nx = 0;
				y += maxHeight;
				maxHeight = 0;
			}
		} else if (nx >= this->tilesX) { // fit to specific number of tiles wide
			x = 0;
			nx = 0;
			y += maxHeight;
			maxHeight = 0;
		}

		if (y > (unsigned)s.y) break; // gone past bottom of window

		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 0.0);  glVertex2i(x,  y);
		glTexCoord2d(0.0, 1.0);  glVertex2i(x,  y + t.height);
		glTexCoord2d(1.0, 1.0);  glVertex2i(x + t.width, y + t.height);
		glTexCoord2d(1.0, 0.0);  glVertex2i(x + t.width, y);
		glEnd();

		if (t.height > maxHeight) maxHeight = t.height;
		x += t.width;
		nx++;
	}
	glDisable(GL_TEXTURE_2D);

	this->SwapBuffers();
	return;
}

void TilePanelCanvas::setTilesX(int t)
{
	this->tilesX = t;
	this->redraw();
	return;
}

void TilePanelCanvas::setOffset(unsigned int o)
{
	return;
}

void TilePanelCanvas::onRightClick(wxMouseEvent& ev)
{
	return;
}
