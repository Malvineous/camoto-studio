/**
 * @file  tab-map2d.hpp
 * @brief Tab for editing 2D tile-based maps.
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

#ifndef STUDIO_TAB_MAP2D_HPP_
#define STUDIO_TAB_MAP2D_HPP_

#include <gtkmm.h>
#include <camoto/gamemaps/map2d.hpp>
#include "ct-map2d-canvas.hpp"

class Tab_Map2D: public Gtk::Box
{
	public:
		Tab_Map2D(BaseObjectType *obj,
			const Glib::RefPtr<Gtk::Builder>& refBuilder);

		/// Set a 2D tile-based map to display in this tab.
		void content(std::unique_ptr<camoto::gamemaps::Map2D> obj,
			DepData& depData);

		static const std::string tab_id;

	protected:
		class ModelTilesetColumns: public Gtk::TreeModel::ColumnRecord
		{
			public:
				ModelTilesetColumns();

				Gtk::TreeModelColumn<Glib::ustring> name;
				Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> icon;
				Gtk::TreeModelColumn<std::shared_ptr<camoto::gamegraphics::Tileset>> tileset;
				Gtk::TreeModelColumn<int> index;
		};

		void on_undo();
		void on_redo();
		void on_save();

		Glib::RefPtr<Gtk::Builder> refBuilder;
		Glib::RefPtr<Gtk::TreeStore> ctItems;
		Glib::RefPtr<Gio::SimpleActionGroup> agItems;
		DrawingArea_Map2D *ctCanvas;
		ModelTilesetColumns cols;
		std::shared_ptr<camoto::gamemaps::Map2D> obj;
};

#endif // STUDIO_TAB_MAP2D_HPP_
