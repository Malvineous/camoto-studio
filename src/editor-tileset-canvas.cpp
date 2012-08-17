/**
 * @file   editor-tileset-canvas.cpp
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

#include "editor-tileset-canvas.hpp"

using namespace camoto;
using namespace camoto::gamegraphics;

BEGIN_EVENT_TABLE(TilesetCanvas, wxGLCanvas)
	EVT_PAINT(TilesetCanvas::onPaint)
	EVT_ERASE_BACKGROUND(TilesetCanvas::onEraseBG)
	EVT_SIZE(TilesetCanvas::onResize)
END_EVENT_TABLE()

TilesetCanvas::TilesetCanvas(wxWindow *parent, wxGLContext *glcx,
	int *attribList, int zoomFactor)
	:	wxGLCanvas(parent, glcx, wxID_ANY, wxDefaultPosition,
			wxDefaultSize, 0, wxEmptyString, attribList),
		zoomFactor(zoomFactor),
		tilesX(0), // must be updated before first redraw() call
		offset(0)
{
	this->SetCurrent();
	glClearColor(0.5, 0.5, 0.5, 1);
	glShadeModel(GL_FLAT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	this->glReset();
}

TilesetCanvas::~TilesetCanvas()
{
}

void TilesetCanvas::setTextures(TEXTURE_MAP& tm)
{
	this->tm = tm;
	return;
}

void TilesetCanvas::onEraseBG(wxEraseEvent& ev)
{
	return;
}

void TilesetCanvas::onPaint(wxPaintEvent& ev)
{
	wxPaintDC dummy(this);
	this->redraw();
	return;
}

void TilesetCanvas::onResize(wxSizeEvent& ev)
{
	this->Layout();
	this->glReset();
	return;
}

void TilesetCanvas::glReset()
{
	this->SetCurrent();
	wxSize s = this->GetClientSize();
	glViewport(0, 0, s.x, s.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, s.x/this->zoomFactor, s.y/this->zoomFactor, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	return;
}

void TilesetCanvas::redraw()
{
	this->SetCurrent();
this->glReset();
	glClearColor(0.5, 0.5, 0.5, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Enable on/off transparency
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0);

	wxSize s = this->GetClientSize();

	//const Tileset::VC_ENTRYPTR& tiles = tileset->getItems();
	unsigned int x = 0, y = 0, nx = 0, maxHeight = 0;
	TEXTURE_MAP::iterator t = this->tm.begin();
	std::advance(t, this->offset);
	for (; t != this->tm.end(); t++) {
		glBindTexture(GL_TEXTURE_2D, t->second.glid);

		if (this->tilesX == 0) { // fit width
			if ((x + t->second.width) * this->zoomFactor > (unsigned)s.x) {
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
		glTexCoord2d(0.0, 1.0);  glVertex2i(x,  y + t->second.height);
		glTexCoord2d(1.0, 1.0);  glVertex2i(x + t->second.width, y + t->second.height);
		glTexCoord2d(1.0, 0.0);  glVertex2i(x + t->second.width, y);
		glEnd();

		if (t->second.height > maxHeight) maxHeight = t->second.height;
		x += t->second.width;
		nx++;
	}

	glDisable(GL_TEXTURE_2D);

	this->SwapBuffers();
	return;
}

void TilesetCanvas::setZoomFactor(int f)
{
	this->zoomFactor = f;
	this->glReset();
	//this->redraw();
	return;
}

void TilesetCanvas::setTilesX(int t)
{
	this->tilesX = t;
	this->redraw();
	return;
}

void TilesetCanvas::setOffset(unsigned int o)
{
	if (o < this->tm.size()) {
		this->offset = o;
		this->redraw();
	}
	return;
}
