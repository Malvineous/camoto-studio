/**
 * @file  ct-map2d-canvas.hpp
 * @brief GTK DrawingArea widget for drawing Map2D objects.
 *
 * Copyright (C) 2013-2015 Adam Nielsen <malvineous@shikadi.net>
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

#ifndef STUDIO_CT_MAP2D_CANVAS_HPP_
#define STUDIO_CT_MAP2D_CANVAS_HPP_

#include <camoto/gamegraphics/image.hpp> // Point
#include <camoto/gamemaps/map2d.hpp>
#include <gtkmm/drawingarea.h>

class DrawingArea_Map2D: public Gtk::DrawingArea
{
	public:
		DrawingArea_Map2D(BaseObjectType *obj,
			const Glib::RefPtr<Gtk::Builder>& refBuilder);
		virtual ~DrawingArea_Map2D();

		/// Set a 2D tile-based map to display in this tab.
		void content(std::shared_ptr<camoto::gamemaps::Map2D> obj,
			camoto::gamemaps::TilesetCollection& allTilesets);

	protected:
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

		std::shared_ptr<camoto::gamemaps::Map2D> obj;
		camoto::gamemaps::TilesetCollection allTilesets;

		struct TileImage {
			bool loaded;
			camoto::gamegraphics::Point dims;
			Cairo::RefPtr<Cairo::SurfacePattern> surfacePattern;
		};
		std::map<
			decltype(camoto::gamemaps::Map2D::Layer::Item::code),
			TileImage
		> imgCache;
};

#endif // STUDIO_CT_MAP2D_CANVAS_HPP_
