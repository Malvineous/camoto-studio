/**
 * @file  tab-newproject.hpp
 * @brief Tab for creating a new project.
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

#ifndef STUDIO_TAB_NEWPROJECT_HPP_
#define STUDIO_TAB_NEWPROJECT_HPP_

#include <gtkmm.h>

class Tab_NewProject: public Gtk::Box
{
	public:
		Tab_NewProject(BaseObjectType *obj,
			const Glib::RefPtr<Gtk::Builder>& refBuilder);

		static const std::string tab_id;

	protected:
		class ModelGameColumns: public Gtk::TreeModel::ColumnRecord
		{
			public:
				ModelGameColumns();

				Gtk::TreeModelColumn<Glib::ustring> code;
				Gtk::TreeModelColumn<Glib::ustring> name;
				Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> icon;
				Gtk::TreeModelColumn<Glib::ustring> developer;
				Gtk::TreeModelColumn<Glib::ustring> reverser;
		};

		void on_game_selected(Glib::RefPtr<Gtk::TreeSelection> tvsel);
		void on_new();

		Glib::RefPtr<Gtk::Builder> refBuilder;
		ModelGameColumns cols;
};

#endif // STUDIO_TAB_NEWPROJECT_HPP_
