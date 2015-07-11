/**
 * @file  tab-newproject.cpp
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

#include <cassert>
#include <gtkmm.h>
#include <glibmm/i18n.h>
#include "gamelist.hpp"
#include "main.hpp"
#include "project.hpp"
#include "tab-newproject.hpp"

const std::string Tab_NewProject::tab_id = "tab-newproject";

Tab_NewProject::ModelGameColumns::ModelGameColumns()
{
	this->add(this->code);
	this->add(this->name);
	this->add(this->icon);
	this->add(this->developer);
	this->add(this->reverser);
}

Tab_NewProject::Tab_NewProject(BaseObjectType *obj,
	const Glib::RefPtr<Gtk::Builder>& refBuilder)
	:	Gtk::Box(obj),
		refBuilder(refBuilder)
{
	Glib::RefPtr<Gio::SimpleActionGroup> refActionGroup =
		Gio::SimpleActionGroup::create();
	refActionGroup->add_action("new", sigc::mem_fun(this, &Tab_NewProject::on_new));
	this->insert_action_group("tab_newproject", refActionGroup);

	// Populate tree view with games
	auto listGames = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(this->refBuilder->get_object("listGames"));
	assert(listGames);

	std::map<std::string, GameInfo> games;

	try {
		games = ::getAllGames();
	} catch (const Glib::FileError& e) {
		Gtk::MessageDialog dlg(Glib::ustring::compose(_("Unable to access folder "
			"containing XML data files: %1"), e.what()), false, Gtk::MESSAGE_ERROR,
			Gtk::BUTTONS_OK, true);
		dlg.set_title(_("Unable to populate list of games"));
		Gtk::Window *parent = dynamic_cast<Gtk::Window *>(this->get_toplevel());
		if (parent) dlg.set_transient_for(*parent);
		dlg.run();
	}

	for (const auto& i : games) {
		auto& gameInfo = i.second;
		auto row = *(listGames->append());
		row[this->cols.code] = gameInfo.id;
		row[this->cols.name] = gameInfo.title;
		try {
			row[this->cols.icon] = Gdk::Pixbuf::create_from_file(
				Glib::build_filename(::path.gameIcons, gameInfo.id + ".png")
			);
		} catch (const Glib::FileError& e) {
		} catch (const Gdk::PixbufError& e) {
		}
		row[this->cols.developer] = gameInfo.developer;
		row[this->cols.reverser] = gameInfo.reverser;
	}

	Gtk::TreeView* tvGames = nullptr;
	this->refBuilder->get_widget("tvGames", tvGames);
	assert(tvGames);

	auto tvsel = tvGames->get_selection();
	tvsel->signal_changed().connect(sigc::bind<Glib::RefPtr<Gtk::TreeSelection>>(
		sigc::mem_fun(this, &Tab_NewProject::on_game_selected),
		tvsel)
	);
}

void Tab_NewProject::on_game_selected(Glib::RefPtr<Gtk::TreeSelection> tvsel)
{
	auto& row = *tvsel->get_selected();
	auto idGame = row[this->cols.code];
	Gtk::Image *ctScreenshot = nullptr;
	this->refBuilder->get_widget("screenshot", ctScreenshot);
	if (ctScreenshot) {
		ctScreenshot->set(Glib::build_filename(::path.gameScreenshots, idGame + ".png"));
	}
	Gtk::Label *txtDeveloper = nullptr;
	this->refBuilder->get_widget("txtDeveloper", txtDeveloper);
	if (txtDeveloper) {
		txtDeveloper->set_label(row[this->cols.developer]);
	}
	Gtk::Label *txtReverser = nullptr;
	this->refBuilder->get_widget("txtReverser", txtReverser);
	if (txtReverser) {
		txtReverser->set_label(row[this->cols.reverser]);
	}
	return;
}

void Tab_NewProject::on_new()
{
	Gtk::TreeView* tvGames = nullptr;
	this->refBuilder->get_widget("tvGames", tvGames);
	assert(tvGames);

	auto tvsel = tvGames->get_selection();
	auto& row = *tvsel->get_selected();
	if (!row) {
		Gtk::MessageDialog dlg(_("You must select the game you wish to edit from "
			"the list of games!"), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dlg.set_title(_("Error"));
		Gtk::Window *parent = dynamic_cast<Gtk::Window *>(this->get_toplevel());
		if (parent) dlg.set_transient_for(*parent);
		dlg.run();
		return;
	}
	Glib::ustring idGame = row[this->cols.code];

	auto studio = static_cast<Studio *>(this->get_toplevel());

	Gtk::FileChooserButton *ctBrowseGame = nullptr;
	this->refBuilder->get_widget("browseGame", ctBrowseGame);
	assert(ctBrowseGame);

	Gtk::FileChooserButton *ctBrowseProject = nullptr;
	this->refBuilder->get_widget("browseProject", ctBrowseProject);
	assert(ctBrowseProject);

	try {
		auto proj = Project::create(
			ctBrowseProject->get_filename(),
			ctBrowseGame->get_filename(),
			idGame
		);
		studio->openProject(std::move(proj));
		studio->closeTab(this);
	} catch (const EProjectOpenFailure& e) {
		Gtk::MessageDialog dlg(e.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK,
			true);
		dlg.set_title(_("New project"));
		Gtk::Window *parent = dynamic_cast<Gtk::Window *>(this->get_toplevel());
		if (parent) dlg.set_transient_for(*parent);
		dlg.run();
	}
	return;
}
