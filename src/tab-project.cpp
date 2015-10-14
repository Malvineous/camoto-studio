/**
 * @file  tab-project.cpp
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

#include <cassert>
#include <gtkmm.h>
#include <glibmm/i18n.h>
#include "gamelist.hpp"
#include "main.hpp"
#include "project.hpp"
#include "tab-project.hpp"

using namespace camoto;

const std::string Tab_Project::tab_id = "tab-project";

Tab_Project::ModelItemColumns::ModelItemColumns()
{
	this->add(this->code);
	this->add(this->name);
	this->add(this->icon);
}

Tab_Project::Tab_Project(BaseObjectType *obj,
	const Glib::RefPtr<Gtk::Builder>& refBuilder)
	:	Gtk::Box(obj),
		refBuilder(refBuilder),
		agItems(Gio::SimpleActionGroup::create())
{
	this->agItems->add_action("open", sigc::mem_fun(this, &Tab_Project::on_open_item));
	this->agItems->add_action("extract_again", sigc::mem_fun(this, &Tab_Project::on_extract_again));
	this->agItems->add_action("extract_raw", sigc::mem_fun(this, &Tab_Project::on_extract_raw));
	this->agItems->add_action("extract_decoded", sigc::mem_fun(this, &Tab_Project::on_extract_decoded));
	this->agItems->add_action("replace_again", sigc::mem_fun(this, &Tab_Project::on_replace_again));
	this->agItems->add_action("replace_raw", sigc::mem_fun(this, &Tab_Project::on_replace_raw));
	this->agItems->add_action("replace_decoded", sigc::mem_fun(this, &Tab_Project::on_replace_decoded));

	this->ctTree = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(
		this->refBuilder->get_object("tvItems"));
	assert(this->ctTree);

	this->ctItems = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic(
		this->refBuilder->get_object("project_items"));
	assert(this->ctItems);

	auto tvsel = this->ctTree->get_selection();
	tvsel->signal_changed().connect(sigc::mem_fun(this, &Tab_Project::on_item_selected));

	this->ctTree->signal_row_activated().connect(sigc::mem_fun(this, &Tab_Project::on_row_activated));
}

void Tab_Project::content(std::unique_ptr<Project> obj)
{
	this->proj = std::move(obj);

	this->loadErrors.clear();

	// Populate tree view with items
	auto row = *(this->ctItems->append());
	row[this->cols.code] = "";
	row[this->cols.name] = this->proj->game->title;
	try {
		row[this->cols.icon] = Gdk::Pixbuf::create_from_file(
			Glib::build_filename(::path.gameIcons, this->proj->cfg_game + ".png")
		);
	} catch (const Glib::FileError& e) {
	} catch (const Gdk::PixbufError& e) {
	}

	this->appendChildren(this->proj->game->treeItems, row);

	this->ctTree->expand_all();

	if (!this->loadErrors.empty()) {
		Gtk::MessageDialog dlg(_("There were errors while "
			"loading this game's XML description file:") + Glib::ustring("\n")
			+ this->loadErrors,
			false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
		dlg.set_title(_("Warning"));
		Gtk::Window *parent = dynamic_cast<Gtk::Window *>(this->get_toplevel());
		if (parent) dlg.set_transient_for(*parent);
		dlg.run();
	}
	return;
}

void Tab_Project::appendChildren(const tree<itemid_t>& treeItems,
	Gtk::TreeModel::Row& root)
{
	auto studio = static_cast<Studio *>(this->get_toplevel());
	for (auto& i : treeItems.children) {
		auto row = *(this->ctItems->append(root->children()));
		if (!i.children.empty()) {
			// This is a folder
			row[this->cols.code] = "";
			row[this->cols.name] = i.item;
			row[this->cols.icon] = studio->getIcon(Studio::Icon::Folder);
			this->appendChildren(i, row);
		} else {
			// This is a normal file/item
			row[this->cols.code] = i.item;

			auto gameObject = this->proj->game->objects.find(i.item);
			Glib::ustring type;
			if (gameObject == this->proj->game->objects.end()) {
				this->loadErrors += "\n";
				this->loadErrors += Glib::ustring::compose(
					_("Item \"%1\" does not exist but was added to the tree"),
					i.item);
				row[this->cols.name] = i.item;
				type = "invalid";
			} else {
				row[this->cols.name] = gameObject->second.friendlyName;
				type = gameObject->second.editor;
			}

			auto icon = studio->nameToIcon(type);
			if (icon == Studio::Icon::Invalid) {
				row[this->cols.icon] = studio->getIcon(Studio::Icon::Generic);
			} else {
				row[this->cols.icon] = studio->getIcon(icon);
			}
		}
	}
	return;
}

void Tab_Project::on_row_activated(const Gtk::TreeModel::Path& path,
	Gtk::TreeViewColumn* column)
{
	auto row = *this->ctItems->get_iter(path);
	itemid_t idItem = row[this->cols.code];
	if (idItem.empty()) return; // folder
	this->openItemById(idItem);
	return;
}

void Tab_Project::on_open_item()
{
	auto tvsel = this->ctTree->get_selection();
	auto& row = *tvsel->get_selected();
	itemid_t idItem = row[this->cols.code];
	if (idItem.empty()) return; // folder
	this->openItemById(idItem);
	return;
}

void Tab_Project::on_item_selected()
{
	this->syncControlStates();
	return;
}

void Tab_Project::on_extract_again()
{
	auto tvsel = this->ctTree->get_selection();
	auto& row = *tvsel->get_selected();
	itemid_t idItem = row[this->cols.code];
	if (idItem.empty()) return;

	this->extractAgain(idItem);
	return;
}

void Tab_Project::on_extract_raw()
{
	this->promptExtract(false);
	return;
}

void Tab_Project::on_extract_decoded()
{
	this->promptExtract(true);
	return;
}

void Tab_Project::on_replace_again()
{
	auto tvsel = this->ctTree->get_selection();
	auto& row = *tvsel->get_selected();
	itemid_t idItem = row[this->cols.code];
	if (idItem.empty()) return;

	this->replaceAgain(idItem);
	return;
}

void Tab_Project::on_replace_raw()
{
	this->promptReplace(false);
	return;
}

void Tab_Project::on_replace_decoded()
{
	this->promptReplace(true);
	return;
}

void Tab_Project::openItemById(const itemid_t& idItem)
{
	Gtk::Window *win = dynamic_cast<Gtk::Window *>(this->get_toplevel());
	if (!win) {
		Gtk::MessageDialog dlg("Can't find parent window!", false,
			Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
		dlg.set_title(_("Open failure"));
		dlg.run();
		return;
	}

	try {
		auto gameObj = this->proj->findItem(idItem);
		auto content = this->proj->openFile(win, gameObj, true);
		if (!content) {
			// File could not be opened, and the user has already been told why.
			return;
		}
		SuppData suppData;
		this->proj->openSuppsByObj(win, &suppData, gameObj);
		auto studio = static_cast<Studio *>(this->get_toplevel());
		studio->openItem(gameObj, std::move(content), suppData, this->proj.get());
	} catch (const EFailure& e) {
		Gtk::MessageDialog dlg(
			Glib::ustring::compose(
				// Translators: %1 is the XML ID of the item being opened, and %2 is
				// the reason why the item could not be opened.
				_("This item (\"%1\") could not be opened for the following reason:\n\n%2"),
				idItem,
				e.getMessage()
			),
			false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dlg.set_title(_("Open failure"));
		Gtk::Window *parent = dynamic_cast<Gtk::Window *>(this->get_toplevel());
		if (parent) dlg.set_transient_for(*parent);
		dlg.run();
		return;
	}
	return;
}

void Tab_Project::promptExtract(bool applyFilters)
{
	auto tvsel = this->ctTree->get_selection();
	auto& row = *tvsel->get_selected();
	itemid_t idItem = row[this->cols.code];
	if (idItem.empty()) return;

	Gtk::FileChooserDialog dlg(_("Save as"),
		Gtk::FILE_CHOOSER_ACTION_SAVE);
	dlg.set_transient_for(*static_cast<Gtk::Window *>(this->get_toplevel()));
	dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dlg.add_button("_Save", Gtk::RESPONSE_OK);
	auto result = dlg.run();
	if (result == Gtk::RESPONSE_OK) {
		this->proj->cfg_lastExtract[idItem].path = dlg.get_filename();
		this->proj->cfg_lastExtract[idItem].applyFilters = applyFilters;
		this->extractAgain(idItem);
		// "Extract again" is now possible, make sure the button is enabled
		this->syncControlStates();
		this->proj->save();
	}
	return;
}

void Tab_Project::promptReplace(bool applyFilters)
{
	auto tvsel = this->ctTree->get_selection();
	auto& row = *tvsel->get_selected();
	itemid_t idItem = row[this->cols.code];
	if (idItem.empty()) return;

	Gtk::FileChooserDialog dlg(_("Open"),
		Gtk::FILE_CHOOSER_ACTION_SAVE);
	dlg.set_transient_for(*static_cast<Gtk::Window *>(this->get_toplevel()));
	dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dlg.add_button("_Open", Gtk::RESPONSE_OK);
	auto result = dlg.run();
	if (result == Gtk::RESPONSE_OK) {
		this->proj->cfg_lastReplace[idItem].path = dlg.get_filename();
		this->proj->cfg_lastReplace[idItem].applyFilters = applyFilters;
		this->replaceAgain(idItem);
		// "Replace again" is now possible, make sure the button is enabled
		this->syncControlStates();
		this->proj->save();
	}
	return;
}

void Tab_Project::extractAgain(const itemid_t& idItem)
{
	auto studio = static_cast<Studio *>(this->get_toplevel());
	studio->infobar("TODO: Extract to " + this->proj->cfg_lastExtract[idItem].path);
	return;
}

void Tab_Project::replaceAgain(const itemid_t& idItem)
{
	auto studio = static_cast<Studio *>(this->get_toplevel());
	studio->infobar("TODO: Replace from " + this->proj->cfg_lastReplace[idItem].path);
	return;
}

void Tab_Project::syncControlStates()
{
	auto tvsel = this->ctTree->get_selection();
	auto& row = *tvsel->get_selected();
	itemid_t idItem = row[this->cols.code];
	if (idItem.empty()) {
		this->remove_action_group("item");
	} else {
		this->insert_action_group("item", this->agItems);

		auto itExtractItem = this->proj->cfg_lastExtract.find(idItem);
		bool canExtractAgain = itExtractItem != this->proj->cfg_lastExtract.end();
		auto actionExtract = Glib::RefPtr<Gio::SimpleAction>::cast_static(this->agItems->lookup("extract_again"));
		actionExtract->set_enabled(canExtractAgain);
		auto ttExtract = canExtractAgain ?
			Glib::ustring::compose(
				// Translators: %1 is the filename, %2 is "with" or "without" compression/encryption
				_("Extract this item to %1, %2 compression/encryption"),
				itExtractItem->second.path,
				itExtractItem->second.applyFilters ? _("with") : _("without")
			) : _(
				"Extract the file underlying this item to the same location as "
				"previously, overwriting without warning"
			);

		auto ctExtractAgainButton = Glib::RefPtr<Gtk::MenuToolButton>::cast_dynamic(
		this->refBuilder->get_object("tb_extract"));
		assert(ctExtractAgainButton);
		ctExtractAgainButton->set_tooltip_text(ttExtract);

		auto ctExtractAgainMenu = Glib::RefPtr<Gtk::ImageMenuItem>::cast_dynamic(
		this->refBuilder->get_object("tb_ex_again"));
		assert(ctExtractAgainMenu);
		ctExtractAgainMenu->set_tooltip_text(ttExtract);

		auto itReplaceItem = this->proj->cfg_lastReplace.find(idItem);
		bool canReplaceAgain = itReplaceItem != this->proj->cfg_lastReplace.end();
		auto actionReplace = Glib::RefPtr<Gio::SimpleAction>::cast_static(this->agItems->lookup("replace_again"));
		actionReplace->set_enabled(canReplaceAgain);

		auto ttReplace = canReplaceAgain ?
			Glib::ustring::compose(
				// Translators: %1 is the filename, %2 is "with" or "without" compression/encryption
				_("Replace this item from %1, %2 compression/encryption"),
				itReplaceItem->second.path,
				itReplaceItem->second.applyFilters ? _("with") : _("without")
			) : _(
				"Replace the file underlying this item with the same file it was "
				"replaced from previously"
			);

		auto ctReplaceAgainButton = Glib::RefPtr<Gtk::MenuToolButton>::cast_dynamic(
		this->refBuilder->get_object("tb_replace"));
		assert(ctReplaceAgainButton);
		ctReplaceAgainButton->set_tooltip_text(ttReplace);

		auto ctReplaceAgainMenu = Glib::RefPtr<Gtk::ImageMenuItem>::cast_dynamic(
		this->refBuilder->get_object("tb_rp_again"));
		assert(ctReplaceAgainMenu);
		ctReplaceAgainMenu->set_tooltip_text(ttReplace);
	}
	return;
}
