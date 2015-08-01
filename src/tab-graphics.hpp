/**
 * @file  tab-graphics.hpp
 * @brief Tab for editing images, tilesets and palettes.
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

#ifndef STUDIO_TAB_GRAPHICS_HPP_
#define STUDIO_TAB_GRAPHICS_HPP_

#include <gtkmm.h>
#include <camoto/gamegraphics/image.hpp>
#include <camoto/gamegraphics/tileset.hpp>

class Tab_Graphics: public Gtk::Box
{
	public:
		Tab_Graphics(BaseObjectType *obj,
			const Glib::RefPtr<Gtk::Builder>& refBuilder);

		/// Set a tileset to display in this tab.
		void content(std::shared_ptr<camoto::gamegraphics::Tileset> obj);

		/// Set an image to display in this tab.
		void content(std::unique_ptr<camoto::gamegraphics::Image> obj);

		/// Set a palette to display in this tab.
		void content(std::unique_ptr<camoto::gamegraphics::Palette> obj);

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

		void appendChildren(const Glib::ustring& prefix,
			std::shared_ptr<camoto::gamegraphics::Tileset> tileset,
			Gtk::TreeModel::Row& root);
		void setImage(std::unique_ptr<camoto::gamegraphics::Image> img);

		void on_row_activated(const Gtk::TreeModel::Path& path,
			Gtk::TreeViewColumn* column);
		void on_undo();
		void on_redo();
		void on_tileset_add();
		void on_tileset_remove();
		void on_image_import();
		void on_image_export();
		void on_palette_import();
		void on_palette_export();
		void on_item_selected();

		Glib::RefPtr<Gtk::Builder> refBuilder;
		Glib::RefPtr<Gtk::TreeView> ctTileset;
		Glib::RefPtr<Gtk::TreeStore> ctItems;
		Glib::RefPtr<Gio::SimpleActionGroup> agItems;
		ModelTilesetColumns cols;
		std::unique_ptr<camoto::gamegraphics::Image> obj_image;
		std::shared_ptr<camoto::gamegraphics::Tileset> obj_tileset;
		std::unique_ptr<camoto::gamegraphics::Palette> obj_palette;
};

#endif // STUDIO_TAB_GRAPHICS_HPP_
