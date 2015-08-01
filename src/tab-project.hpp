/**
 * @file  tab-project.hpp
 * @brief Tab for working on an existing project.
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

#ifndef STUDIO_TAB_PROJECT_HPP_
#define STUDIO_TAB_PROJECT_HPP_

#include <map>
#include <gtkmm.h>
#include "project.hpp"

class Tab_Project: public Gtk::Box
{
	public:
		Tab_Project(BaseObjectType *obj,
			const Glib::RefPtr<Gtk::Builder>& refBuilder);

		/// Set the project to display in this tab.
		void content(std::unique_ptr<Project> obj);

		static const std::string tab_id;

	protected:
		class ModelItemColumns: public Gtk::TreeModel::ColumnRecord
		{
			public:
				ModelItemColumns();

				Gtk::TreeModelColumn<itemid_t> code;
				Gtk::TreeModelColumn<Glib::ustring> name;
				Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> icon;
		};

		void appendChildren(const tree<itemid_t>& treeItems,
			Gtk::TreeModel::Row& root);
		void on_row_activated(const Gtk::TreeModel::Path& path,
			Gtk::TreeViewColumn* column);
		void on_open_item();
		void on_item_selected();
		void on_extract_again();
		void on_extract_raw();
		void on_extract_decoded();
		void on_replace_again();
		void on_replace_raw();
		void on_replace_decoded();
		void openItemById(const itemid_t& idItem);
		void promptExtract(bool applyFilters);
		void promptReplace(bool applyFilters);
		void extractAgain(const itemid_t& idItem);
		void replaceAgain(const itemid_t& idItem);
		/// Enable/disable toolbar buttons depending on the currently selected item
		void syncControlStates();

		Glib::RefPtr<Gtk::Builder> refBuilder;
		Glib::RefPtr<Gtk::TreeView> ctTree;
		Glib::RefPtr<Gtk::TreeStore> ctItems;
		Glib::RefPtr<Gio::SimpleActionGroup> agItems;
		ModelItemColumns cols;
		std::unique_ptr<Project> proj;
		Glib::ustring loadErrors; ///< List of errors encountered when loading project
};

#endif // STUDIO_TAB_PROJECT_HPP_
