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

#include <GL/glew.h>
#include "editor-map-basecanvas.hpp"

// Size of the grips when positioning path points
#define MOVEMENT_ARROWHEAD_SIZE 5

#define SIN45 0.85090352453
#define COS45 0.52532198881

using namespace camoto;
using namespace camoto::gamemaps;
using namespace camoto::gamegraphics;

MapBaseCanvas::MapBaseCanvas(wxWindow *parent, int *attribList)
	: wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition,
	  	wxDefaultSize, wxTAB_TRAVERSAL | wxWANTS_CHARS),
	  zoomFactor(2),
	  offX(0),
	  offY(0)
{
}

MapBaseCanvas::~MapBaseCanvas()
{
}

void MapBaseCanvas::setZoomFactor(int f)
{
	// Keep the viewport centred after the zoom
	wxSize s = this->GetClientSize();
	int centreX = s.x / 2, centreY = s.y / 2;
	int absX = this->offX + centreX / this->zoomFactor;
	int absY = this->offY + centreY / this->zoomFactor;
	this->zoomFactor = f;
	this->offX = absX - centreX / this->zoomFactor;
	this->offY = absY - centreY / this->zoomFactor;
	this->redraw();
	return;
}

void MapBaseCanvas::drawMapItem(int pixelX, int pixelY, unsigned int tileWidth,
	unsigned int tileHeight, const Map2D::Layer::ItemPtr& item, GLuint textureId)
{
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
		glVertex2i(x1, y1);
		glTexCoord2d(0.0, 1.0);
		glVertex2i(x1, y2);
		glTexCoord2d(1.0, 1.0);
		glVertex2i(x2, y2);
		glTexCoord2d(1.0, 0.0);
		glVertex2i(x2, y1);
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

	return;
}
