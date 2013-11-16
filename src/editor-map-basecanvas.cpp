/**
 * @file   editor-map-basecanvas.cpp
 * @brief  Shared functions for an OpenGL surface where map tiles are drawn.
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

#include <png++/png.hpp>
#include <GL/glew.h>
#include "main.hpp"
#include "editor-map-basecanvas.hpp"

// Size of the grips when positioning path points
#define MOVEMENT_ARROWHEAD_SIZE 5

#define SIN45 0.85090352453
#define COS45 0.52532198881

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

BEGIN_EVENT_TABLE(MapBaseCanvas, wxGLCanvas)
	EVT_MOTION(MapBaseCanvas::onMouseMove)
	EVT_MIDDLE_DOWN(MapBaseCanvas::onMouseDownMiddle)
	EVT_MIDDLE_UP(MapBaseCanvas::onMouseUpMiddle)
	EVT_MOUSE_CAPTURE_LOST(MapBaseCanvas::onMouseCaptureLost)
END_EVENT_TABLE()

MapBaseCanvas::MapBaseCanvas(wxWindow *parent, wxGLContext *glcx, int *attribList)
	:	wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition,
			wxDefaultSize, wxTAB_TRAVERSAL | wxWANTS_CHARS),
		glcx(glcx),
		zoomFactor(2),
		offX(0),
		offY(0),
		gridVisible(false),
		scrolling(false),
		scrollFromX(0),
		scrollFromY(0)
{
	this->SetCurrent(*this->glcx);

	// Load preset tiles and indicators
	this->indicators[Map2D::Layer::Supplied].width = 0;
	this->indicators[Map2D::Layer::Supplied].height = 0;
	this->indicators[Map2D::Layer::Supplied].glid = 0;
	this->indicators[Map2D::Layer::Blank].width = 0;
	this->indicators[Map2D::Layer::Blank].height = 0;
	this->indicators[Map2D::Layer::Blank].glid = 0;
	this->indicators[Map2D::Layer::Unknown] = this->loadTileFromFile("unknown-tile");
	this->indicators[Map2D::Layer::Digit0] = this->loadTileFromFile("number-0");
	this->indicators[Map2D::Layer::Digit1] = this->loadTileFromFile("number-1");
	this->indicators[Map2D::Layer::Digit2] = this->loadTileFromFile("number-2");
	this->indicators[Map2D::Layer::Digit3] = this->loadTileFromFile("number-3");
	this->indicators[Map2D::Layer::Digit4] = this->loadTileFromFile("number-4");
	this->indicators[Map2D::Layer::Digit5] = this->loadTileFromFile("number-5");
	this->indicators[Map2D::Layer::Digit6] = this->loadTileFromFile("number-6");
	this->indicators[Map2D::Layer::Digit7] = this->loadTileFromFile("number-7");
	this->indicators[Map2D::Layer::Digit8] = this->loadTileFromFile("number-8");
	this->indicators[Map2D::Layer::Digit9] = this->loadTileFromFile("number-9");
	this->indicators[Map2D::Layer::DigitA] = this->loadTileFromFile("number-a");
	this->indicators[Map2D::Layer::DigitB] = this->loadTileFromFile("number-b");
	this->indicators[Map2D::Layer::DigitC] = this->loadTileFromFile("number-c");
	this->indicators[Map2D::Layer::DigitD] = this->loadTileFromFile("number-d");
	this->indicators[Map2D::Layer::DigitE] = this->loadTileFromFile("number-e");
	this->indicators[Map2D::Layer::DigitF] = this->loadTileFromFile("number-f");
	this->indicators[Map2D::Layer::Interactive] = this->loadTileFromFile("interactive");
}

MapBaseCanvas::~MapBaseCanvas()
{
}

void MapBaseCanvas::setZoomFactor(int f)
{
	// Don't want any divisions by zero!
	assert(f != 0);

	// Keep the viewport centred after the zoom
	wxSize s = this->GetClientSize();
	int centreX = s.x / 2, centreY = s.y / 2;
	int absX = this->offX + centreX / this->zoomFactor;
	int absY = this->offY + centreY / this->zoomFactor;
	this->zoomFactor = f;
	this->offX = absX - centreX / this->zoomFactor;
	this->offY = absY - centreY / this->zoomFactor;

	this->calcCurrentExtents();
	this->redraw();
	return;
}

void MapBaseCanvas::setGridVisible(bool visible)
{
	this->gridVisible = visible;
	this->redraw();
	return;
}

void MapBaseCanvas::drawGrid(unsigned int divWidth, unsigned int divHeight)
{
	wxSize winSize = this->GetClientSize();
	winSize.x /= this->zoomFactor;
	winSize.y /= this->zoomFactor;

	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_AND_REVERSE);
	glColor4f(0.3, 0.3, 0.3, 0.5);
	glBegin(GL_LINES);
	for (int x = -this->offX % divWidth; x < winSize.x; x += divWidth) {
		glVertex2i(x, 0);
		glVertex2i(x, winSize.y);
	}
	for (int y = -this->offY % divHeight; y < winSize.y; y += divHeight) {
		glVertex2i(0,   y);
		glVertex2i(winSize.x, y);
	}
	glEnd();
	glDisable(GL_COLOR_LOGIC_OP);
	return;
}

bool MapBaseCanvas::drawMapItem(int pixelX, int pixelY, unsigned int tileWidth,
	unsigned int tileHeight, const Map2D::Layer::ItemPtr item,
	const Texture *texture)
{
	const GLuint& textureId = texture->glid;

	int x1 = pixelX - this->offX;
	int x2 = x1 + tileWidth;
	int xt = tileWidth/4;
	int y1 = pixelY - this->offY;
	int y2 = y1 + tileHeight;
	int yt = tileHeight/4;

	if (textureId) {
		// Normal image
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 0.0);
		glVertex2i(x1 - texture->hotspotX, y1 - texture->hotspotY);
		glTexCoord2d(0.0, 1.0);
		glVertex2i(x1 - texture->hotspotX, y2 - texture->hotspotY);
		glTexCoord2d(1.0, 1.0);
		glVertex2i(x2 - texture->hotspotX, y2 - texture->hotspotY);
		glTexCoord2d(1.0, 0.0);
		glVertex2i(x2 - texture->hotspotX, y1 - texture->hotspotY);
		glEnd();
	}

	if (item->type & Map2D::Layer::Item::Blocking) {
		// This tile has blocking attributes, so draw them
		glDisable(GL_TEXTURE_2D);
		glLineWidth(1.0);

		glColor4f(CLR_ATTR_BG);
		glBegin(GL_QUADS);
		if (item->blockingFlags & Map2D::Layer::Item::BlockLeft) {
			glVertex2i(x1, y1);
			glVertex2i(x1, y2);
			glVertex2i(x1 + xt, y2);
			glVertex2i(x1 + xt, y1);
		}
		if (item->blockingFlags & Map2D::Layer::Item::BlockRight) {
			glVertex2i(x2 - xt, y1);
			glVertex2i(x2 - xt, y2);
			glVertex2i(x2, y2);
			glVertex2i(x2, y1);
		}
		if (item->blockingFlags & Map2D::Layer::Item::BlockTop) {
			glVertex2i(x1, y1);
			glVertex2i(x1, y1 + yt);
			glVertex2i(x2, y1 + yt);
			glVertex2i(x2, y1);
		}
		if (item->blockingFlags & Map2D::Layer::Item::BlockBottom) {
			glVertex2i(x1, y2 - yt);
			glVertex2i(x1, y2);
			glVertex2i(x2, y2);
			glVertex2i(x2, y2 - yt);
		}
		if (item->blockingFlags & Map2D::Layer::Item::Slant45) {
			glVertex2i(x1, y2);
			glVertex2f(x1 + xt / SIN45, y2);
			glVertex2f(x2, y1 + yt / SIN45);
			glVertex2i(x2, y1);
		}
		if (item->blockingFlags & Map2D::Layer::Item::Slant135) {
			glVertex2i(x1, y1);
			glVertex2i(x1, y1 + yt / SIN45);
			glVertex2i(x2 - xt / SIN45, y2);
			glVertex2i(x2, y2);
		}
		glEnd();

		glColor4f(CLR_ATTR_FG);
		glBegin(GL_LINES);
		if (item->blockingFlags & Map2D::Layer::Item::BlockLeft) {
			glVertex2i(x1, y1);
			glVertex2i(x1, y2);
			for (unsigned int i = 0; i < tileHeight; i += 2) {
				glVertex2i(x1, y1 + i);
				glVertex2i(x1 + xt, y1 + i);
			}
		}
		if (item->blockingFlags & Map2D::Layer::Item::BlockRight) {
			glVertex2i(x2, y1);
			glVertex2i(x2, y2);
			for (unsigned int i = 0; i < tileHeight; i += 2) {
				glVertex2i(x2 - xt, y1 + i);
				glVertex2i(x2, y1 + i);
			}
		}
		if (item->blockingFlags & Map2D::Layer::Item::BlockTop) {
			glVertex2i(x1, y1);
			glVertex2i(x2, y1);
			for (unsigned int i = 0; i < tileHeight; i += 2) {
				glVertex2i(x1 + i, y1);
				glVertex2i(x1 + i, y1 + yt);
			}
		}
		if (item->blockingFlags & Map2D::Layer::Item::BlockBottom) {
			glVertex2i(x1, y2);
			glVertex2i(x2, y2);
			for (unsigned int i = 0; i < tileHeight; i += 2) {
				glVertex2i(x1 + i, y2 - yt);
				glVertex2i(x1 + i, y2);
			}
		}
		if (item->blockingFlags & Map2D::Layer::Item::Slant45) {
			glVertex2i(x1, y2);
			glVertex2i(x2, y1);
			for (float i = 2 * SIN45; i < tileWidth - 2 * SIN45; i += 2*SIN45) {
				glVertex2f(x1 + i, y2 - i);
				glVertex2f(x1 + i + xt * COS45, y2 - i + yt * COS45);
			}
		}
		if (item->blockingFlags & Map2D::Layer::Item::Slant135) {
			glVertex2i(x1, y1);
			glVertex2i(x2, y2);
			for (float i = 2 * SIN45; i < tileWidth - 2 * SIN45; i += 2*SIN45) {
				glVertex2f(x1 + i, y1 + i);
				glVertex2f(x1 + i - xt * COS45, y1 + i + yt * COS45);
			}
		}
		glEnd();
	}

	// If there are movement flags, draw little arrows in the
	// direction of movement.
	if (item->type & Map2D::Layer::Item::Movement) {
		// This tile has movement attributes, so draw them
		glDisable(GL_TEXTURE_2D);
		glLineWidth(2.0);

		glColor4f(CLR_ATTR_BG);
		if (item->movementFlags & Map2D::Layer::Item::DistanceLimit) {
			int x1 = pixelX - this->offX;
			int x2 = x1 + tileWidth;
			int xm = tileWidth/2;
			int y1 = pixelY - this->offY;
			int y2 = y1 + tileHeight;
			int ym = tileHeight/2;

			if (item->movementDistLeft == Map2D::Layer::Item::DistIndeterminate) {
				glPushMatrix();
				glTranslatef(x1, y1 + ym, 0.0);
				glRotatef(90.0, 0.0, 0.0, 1.0);
				glBegin(GL_LINE_STRIP);
				glVertex2i(-MOVEMENT_ARROWHEAD_SIZE, -MOVEMENT_ARROWHEAD_SIZE);
				glVertex2i(0, 0);
				glVertex2i(0, -ym);
				glVertex2i(0, 0);
				glVertex2i(MOVEMENT_ARROWHEAD_SIZE, -MOVEMENT_ARROWHEAD_SIZE);
				glEnd();
				glPopMatrix();
			}
			if (item->movementDistRight == Map2D::Layer::Item::DistIndeterminate) {
				glPushMatrix();
				glTranslatef(x2, y1 + ym, 0.0);
				glRotatef(270.0, 0.0, 0.0, 1.0);
				glBegin(GL_LINE_STRIP);
				glVertex2i(-MOVEMENT_ARROWHEAD_SIZE, -MOVEMENT_ARROWHEAD_SIZE);
				glVertex2i(0, 0);
				glVertex2i(0, -ym);
				glVertex2i(0, 0);
				glVertex2i(MOVEMENT_ARROWHEAD_SIZE, -MOVEMENT_ARROWHEAD_SIZE);
				glEnd();
				glPopMatrix();
			}
			if (item->movementDistUp == Map2D::Layer::Item::DistIndeterminate) {
				glPushMatrix();
				glTranslatef(x1 + xm, y1, 0.0);
				glRotatef(180.0, 0.0, 0.0, 1.0);
				glBegin(GL_LINE_STRIP);
				glVertex2i(-MOVEMENT_ARROWHEAD_SIZE, -MOVEMENT_ARROWHEAD_SIZE);
				glVertex2i(0, 0);
				glVertex2i(0, -ym);
				glVertex2i(0, 0);
				glVertex2i(MOVEMENT_ARROWHEAD_SIZE, -MOVEMENT_ARROWHEAD_SIZE);
				glEnd();
				glPopMatrix();
			}
			if (item->movementDistDown == Map2D::Layer::Item::DistIndeterminate) {
				glPushMatrix();
				glTranslatef(x1 + xm, y2, 0.0);
				glRotatef(0.0, 0.0, 0.0, 1.0);
				glBegin(GL_LINE_STRIP);
				glVertex2i(-MOVEMENT_ARROWHEAD_SIZE, -MOVEMENT_ARROWHEAD_SIZE);
				glVertex2i(0, 0);
				glVertex2i(0, -ym);
				glVertex2i(0, 0);
				glVertex2i(MOVEMENT_ARROWHEAD_SIZE, -MOVEMENT_ARROWHEAD_SIZE);
				glEnd();
				glPopMatrix();
			}
		}
		glLineWidth(1.0);
	}

	// If there are any general flags, draw those
	if (item->type & Map2D::Layer::Item::Flags) {
		if (item->generalFlags & Map2D::Layer::Item::Interactive) {
			const GLuint& textureId = this->indicators[Map2D::Layer::Interactive].glid;

			if (textureId) {
				// Normal image
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, textureId);
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
	}

	return false;
}

Texture MapBaseCanvas::loadTileFromFile(const char *name)
{
	png::image<png::rgba_pixel> png;
	wxString filename = ::path.mapIndicators + name + ".png";
	try {
		png.read(filename.mb_str());
	} catch (const std::exception& e) {
		std::cerr << "[editor-map-basecanvas] Error loading .png: " << e.what()
			<< std::endl;
		throw EFailure(e.what());
	}

	Texture t;
	t.width = png.get_width();
	t.height = png.get_height();
	t.hotspotX = 0;
	t.hotspotY = 0;
	glGenTextures(1, &t.glid);
	glBindTexture(GL_TEXTURE_2D, t.glid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	boost::shared_array<uint32_t> combined(new uint32_t[t.width * t.height]);
	uint8_t *c = (uint8_t *)combined.get();
	for (unsigned int y = 0; y < t.height; y++) {
		png::image<png::rgba_pixel>::row_type row = png[y];
		for (unsigned int x = 0; x < t.width; x++) {
			const png::rgba_pixel& px = row[x];
			*c++ = px.blue;
			*c++ = px.green;
			*c++ = px.red;
			*c++ = px.alpha;
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t.width, t.height, 0, GL_BGRA,
		GL_UNSIGNED_BYTE, combined.get());
	if (glGetError()) {
		std::cerr << "[editor-map-basecanvas] GL error loading file " << name <<
			"into texture id " << t.glid << std::endl;
	}

	return t;
}

void MapBaseCanvas::onMouseMove(wxMouseEvent& ev)
{
	// Scroll the map if the user is middle-dragging
	if (this->scrolling) {
		this->offX = (this->scrollFromX - ev.m_x) / this->zoomFactor;
		this->offY = (this->scrollFromY - ev.m_y) / this->zoomFactor;
		this->needRedraw = true;
	}
	return;
}

void MapBaseCanvas::onMouseDownMiddle(wxMouseEvent& ev)
{
	this->scrolling = true;
	this->scrollFromX = this->offX * this->zoomFactor + ev.m_x;
	this->scrollFromY = this->offY * this->zoomFactor + ev.m_y;
	this->CaptureMouse();
	return;
}

void MapBaseCanvas::onMouseUpMiddle(wxMouseEvent& ev)
{
	this->scrollFromX = 0;
	this->scrollFromY = 0;
	this->scrolling = false;
	this->ReleaseMouse();
	return;
}

void MapBaseCanvas::onMouseCaptureLost(wxMouseCaptureLostEvent& ev)
{
	this->scrollFromX = 0;
	this->scrollFromY = 0;
	this->scrolling = false;
	return;
}

void MapBaseCanvas::calcCurrentExtents()
{
	return;
}
