/**
 * @file  tab-openfile.hpp
 * @brief Tab for opening a standalone file.
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

#ifndef STUDIO_TAB_OPENFILE_HPP_
#define STUDIO_TAB_OPENFILE_HPP_

#include <gtkmm.h>

class Tab_OpenFile: public Gtk::Box
{
	public:
		Tab_OpenFile(BaseObjectType *obj,
			const Glib::RefPtr<Gtk::Builder>& refBuilder);

		static const std::string tab_id;

	protected:
		void on_open();

	protected:
		Glib::RefPtr<Gtk::Builder> refBuilder;
};

#endif // STUDIO_TAB_OPENFILE_HPP_
