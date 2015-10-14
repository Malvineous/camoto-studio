/**
 * @file  tab-openfile.cpp
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

#include <cassert>
#include <gtkmm.h>
#include "main.hpp"
#include "gamelist.hpp"
#include "tab-openfile.hpp"

using namespace camoto;

const std::string Tab_OpenFile::tab_id = "tab-openfile";

Tab_OpenFile::Tab_OpenFile(BaseObjectType *obj,
	const Glib::RefPtr<Gtk::Builder>& refBuilder)
	:	Gtk::Box(obj),
		refBuilder(refBuilder)
{
	Glib::RefPtr<Gio::SimpleActionGroup> refActionGroup =
		Gio::SimpleActionGroup::create();
	refActionGroup->add_action("open", sigc::mem_fun(this, &Tab_OpenFile::on_open));
	this->insert_action_group("tab_openfile", refActionGroup);
}

void Tab_OpenFile::on_open()
{
	// Create a game item outside of the project describing the file we want to
	// open
	GameObject item;

	item.filename = "todo";
	item.editor = "todo";
	item.format = "todo";
	item.filter = "todo";
	item.friendlyName = item.filename;

	// Populate suppData from supp browse boxes
	SuppData suppData;

	// Open main content stream from main browse box
	std::unique_ptr<stream::inout> content;

	Gtk::Window *parent = dynamic_cast<Gtk::Window *>(this->get_toplevel());
	if (!parent) {
		Gtk::MessageDialog dlg("Can't find parent window!", false,
			Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
		dlg.set_title(_("Open failure"));
		dlg.run();
		return;
	}

	auto studio = static_cast<Studio *>(this->get_toplevel());
	studio->openItem(item, std::move(content), suppData, nullptr);
	return;
}
