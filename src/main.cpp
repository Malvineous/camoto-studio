/**
 * @file   main.cpp
 * @brief  Entry point for Camoto Studio.
 *
 * Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>
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

#ifdef WIN32
#include <stdlib.h>
#include <crtdbg.h>
#endif

#ifndef WIN32
#include <config.h>
#endif

#include <iostream>
#include <cassert>
#include <gtkmm.h>
#include <glibmm/i18n.h>
#include <camoto/util.hpp> // make_unique
#include "main.hpp"
#include "tab-graphics.hpp"
#include "tab-newproject.hpp"
#include "tab-openfile.hpp"
#include "tab-project.hpp"

using namespace camoto;
using namespace camoto::gamegraphics;

paths path;
config_data config;

Studio::Studio(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder>& refBuilder)
	:	Gtk::Window(obj),
		refBuilder(refBuilder)
{
	Glib::RefPtr<Gio::SimpleActionGroup> refActionGroup =
		Gio::SimpleActionGroup::create();
	refActionGroup->add_action("new_file", sigc::mem_fun(this, &Studio::on_menuitem_file_new));
	refActionGroup->add_action("open_file", sigc::mem_fun(this, &Studio::on_menuitem_file_open));
	refActionGroup->add_action("quit", sigc::mem_fun(this, &Studio::on_menuitem_file_exit));
	refActionGroup->add_action("new_project", sigc::mem_fun(this, &Studio::on_menuitem_project_new));
	refActionGroup->add_action("open_project", sigc::mem_fun(this, &Studio::on_menuitem_project_open));
	this->insert_action_group("app", refActionGroup);

	auto ctInfo = Glib::RefPtr<Gtk::InfoBar>::cast_dynamic(
		this->refBuilder->get_object("infobar"));
	assert(ctInfo);
	ctInfo->hide();

	ctInfo->signal_response().connect(sigc::mem_fun(this, &Studio::on_infobar_button));
}

void Studio::on_menuitem_file_new()
{
//	this->openTab<TabDocument>("tab-document", "Document");
}

void Studio::on_menuitem_file_open()
{
	// Translators: Tab label for opening a standalone file
	this->openTab<Tab_OpenFile>(_("Open file"));
	return;
}

void Studio::on_menuitem_file_exit()
{
	this->hide(); // will cause run() to return
	return;
}

void Studio::on_menuitem_project_new()
{
	// Translators: Tab label for creating a new project
	this->openTab<Tab_NewProject>(_("New project"));
	return;
}

void Studio::on_menuitem_project_open()
{
	Gtk::FileChooserDialog browseProj(_("Select a project folder"),
		Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	browseProj.set_transient_for(*this);
	browseProj.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	browseProj.add_button("_Open", Gtk::RESPONSE_OK);
	auto result = browseProj.run();
	if (result == Gtk::RESPONSE_OK) {
		this->openProjectByFilename(browseProj.get_filename());
	}
	return;
}

void Studio::on_infobar_button(int response_id)
{
	if (response_id == Gtk::RESPONSE_CLOSE) {
		auto ctInfo = Glib::RefPtr<Gtk::InfoBar>::cast_dynamic(
			this->refBuilder->get_object("infobar"));
		assert(ctInfo);
		ctInfo->hide();
	}
	return;
}

void Studio::openProject(std::unique_ptr<Project> proj)
{
	auto tab = this->openTab<Tab_Project>(proj->getProjectTitle());
	tab->content(std::move(proj));
	return;
}

void Studio::openProjectByFilename(const std::string& folder)
{
	try {
		this->openProject(
			std::make_unique<Project>(folder)
		);
	} catch (const EProjectOpenFailure& e) {
		Gtk::MessageDialog dlg(*this, Glib::ustring::compose(
			// Translators: %1 is the path to the project being opened, %2 is the
			// reason why the project can't be opened
			_("Unable to open project %1: %2"), folder, e.what()),
			false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dlg.set_title(_("Open project"));
		dlg.run();
	}
	return;
}

void Studio::openItem(const GameObject& item,
	std::unique_ptr<stream::inout> content, SuppData& suppData, Project *proj)
{
	try {
		if (
			(item.typeMajor.compare("image") == 0)
			|| (item.typeMajor.compare("palette") == 0)
		) {
			auto obj = ::openObject<ImageType>(this, item, std::move(content),
				suppData, proj);
			auto tab = this->openTab<Tab_Graphics>(item.friendlyName);
			tab->content(std::move(obj));

		} else if (item.typeMajor.compare("tileset") == 0) {
			auto obj = ::openObject<TilesetType>(this, item, std::move(content),
				suppData, proj);
			auto tab = this->openTab<Tab_Graphics>(item.friendlyName);
			tab->content(std::move(obj));

		} else {
			Gtk::MessageDialog dlg(*this,
				Glib::ustring::compose(
					"%1\n\n[%2]",
					_("Sorry, this type of item cannot be edited yet!"),
					Glib::ustring::compose(
						// Translators: %1 is the item major type from the XML file, such as
						// "tileset" or "map2d", and %2 is the item ID from the XML file.
						_("No editor for typeMajor \"%1\", as specified by item \"%2\""),
						item.typeMajor,
						item.id
					)
				), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			dlg.set_title(_("Open failure"));
			dlg.run();
		}

	} catch (const EFailure& e) {
		Gtk::MessageDialog dlg(*this, e.getMessage(), false,
			Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
		dlg.set_title(_("Open failure"));
		dlg.run();
		return;
	}
	return;
}

template <class T>
T* Studio::openTab(const Glib::ustring& title)
{
	Gtk::Notebook* tabs = 0;
	this->refBuilder->get_widget("tabs", tabs);
	assert(tabs);

	Glib::RefPtr<Gtk::Builder> tabBuilder = Gtk::Builder::create();
	try {
		tabBuilder->add_from_file("gui/" + T::tab_id + ".glade");
		T* nextTab;
		tabBuilder->get_widget_derived(T::tab_id, nextTab);
		if (!nextTab) return nullptr;
		int index = tabs->append_page(*nextTab, title);
		tabs->set_current_page(index);
		return nextTab;
	} catch (const Glib::Error& e) {
		Gtk::MessageDialog dlg(*this, Glib::ustring::compose(
			// Translators: %1 is internal tab ID (not translated), %2 is reason for error
			_("Unable to create new tab \"%1\": %2"), T::tab_id, e.what()),
			false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dlg.set_title(_("New tab"));
		dlg.run();
		return nullptr;
	}
}

void Studio::closeTab(Gtk::Box *tab)
{
	Gtk::Notebook* tabs = 0;
	this->refBuilder->get_widget("tabs", tabs);
	assert(tabs);
	tabs->remove_page(*tab);
	return;
}

void Studio::infobar(const Glib::ustring& content)
{
	auto msg = Glib::RefPtr<Gtk::Label>::cast_dynamic(
		this->refBuilder->get_object("ctInfoMsg"));
	assert(msg);
	msg->set_text(content);

	auto ctInfo = Glib::RefPtr<Gtk::InfoBar>::cast_dynamic(
		this->refBuilder->get_object("infobar"));
	assert(ctInfo);
	ctInfo->show();
	return;
}

int main(int argc, char *argv[])
{
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv,
		"net.shikadi.camoto");
	// Translators: "Camoto" should not be translated but "Studio" should be
	Glib::set_application_name(_("Camoto Studio"));

	// Set all the standard paths
#ifdef WIN32
	// Use the 'data' subdir in the executable's dir
	::path.dataRoot = Glib::build_filename(Glib::get_current_dir(), "data");
#else
	// Use the value given to the configure script by --datadir
	::path.dataRoot = DATA_PATH;
#endif
	std::cout << "[init] Data root is " << ::path.dataRoot << "\n";

	if (!Glib::file_test(::path.dataRoot, Glib::FILE_TEST_IS_DIR)) {
		Gtk::MessageDialog dlg(Glib::ustring::compose(
			// Translators: %1 is the folder path that should exist but does not
			_("Cannot find Camoto Studio data directory: %1"), ::path.dataRoot),
			false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dlg.set_title(_("Cannot find data directory"));
		dlg.run();
		return 1;
	}
	::path.gameData = Glib::build_filename(::path.dataRoot, "games");
	::path.gameScreenshots = Glib::build_filename(::path.gameData, "screenshots");
	::path.gameIcons = Glib::build_filename(::path.gameData, "icons");
	::path.guiIcons = Glib::build_filename(::path.dataRoot, "icons");
	::path.mapIndicators = Glib::build_filename(::path.dataRoot, "maps");

	// Display the main window
	Glib::RefPtr<Gtk::Builder> refBuilder = Gtk::Builder::create();
	Studio *winMain = 0;
	try {
		refBuilder->add_from_file("gui/win-main.glade");
		refBuilder->get_widget_derived("main", winMain);
		if (!winMain) {
			auto msg = _("Unable to find main window in Glade file.");
			std::cerr << msg << std::endl;
			Gtk::MessageDialog dlg(msg, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			dlg.set_title(_("Unhandled error"));
			dlg.run();
			return 1;
		}

		app->run(*winMain);
		delete winMain;
	} catch (const Glib::Exception& e) {
		auto msg = Glib::ustring::compose(_("Unhandled GTK exception: %1"),
			e.what());
		std::cerr << msg << std::endl;
		Gtk::MessageDialog dlg(msg, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dlg.set_title(_("Unhandled error"));
		dlg.run();
		return 1;
	} catch (const stream::error& e) {
		auto msg = Glib::ustring::compose(_("Unhandled Camoto exception: %1"),
			e.what());
		std::cerr << msg << std::endl;

		Gtk::MessageDialog dlg(msg, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dlg.set_title(_("Unhandled error"));
		dlg.run();
		return 1;
	}

	return 0;
}
