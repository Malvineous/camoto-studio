/**
 * @file   project.cpp
 * @brief  Project data management and manipulation.
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

#include <cassert>
#include <iostream>
#include <glibmm/fileutils.h>
#include <glibmm/i18n.h>
#include <glibmm/keyfile.h>
#include <glibmm/miscutils.h>
#include <giomm/file.h>
#include <camoto/util.hpp> // make_unique
#include <camoto/stream_file.hpp>
#include <camoto/gamearchive/fixedarchive.hpp>
#include <camoto/gamearchive/manager.hpp>
#include <camoto/gamearchive/util.hpp>
#include "gamelist.hpp"
#include "project.hpp"

using namespace camoto;
using namespace camoto::gamearchive;

void noopTruncate()
{
	return;
}

EProjectOpenFailure::EProjectOpenFailure(const std::string& msg)
	:	EFailure(msg)
{
}

void copyDir(const std::string& pathDst, const std::string& pathSrc)
{
	Glib::Dir dir(pathSrc);
	for (const auto& i : dir) {
		auto fileSrc = Gio::File::create_for_path(Glib::build_filename(pathSrc, i));
		auto fileDst = Gio::File::create_for_path(Glib::build_filename(pathDst, i));
		std::cout << fileDst->get_path() << " <= " << fileSrc->get_path() << std::endl;
		if (fileSrc->query_file_type() == Gio::FILE_TYPE_DIRECTORY) {
			fileDst->make_directory();
			copyDir(fileDst->get_path(), fileSrc->get_path());
		} else {
			fileSrc->copy(fileDst);
		}
	}
	return;
}

std::unique_ptr<Project> Project::create(const std::string& targetPath,
	const std::string& gameSource, const std::string& gameId)
{
	std::cout << "[Project::create] Creating new project in " <<
		targetPath << "\n";

	// Copy everything from the source game's directory into the 'data'
	// subdirectory within the project folder.
	auto pathDest = Glib::build_filename(targetPath, PROJECT_GAME_DATA);
	auto filePathDest = Gio::File::create_for_path(pathDest);
	try {
		filePathDest->make_directory(); // create 'data' directory inside project folder
		copyDir(pathDest, gameSource);
	} catch (const Glib::Error& e) {
		throw EProjectOpenFailure(Glib::ustring::compose(
			// Translators: %1 is the error reason
			_("Unable to copy game files: %1"), e.what()
		));
	}

	// Create the project config file.
	auto proj = std::make_unique<Project>(targetPath, true); // may throw EProjectOpenFailure

	proj->cfg_game = gameId;
	proj->cfg_orig_game = gameSource;
	proj->save();
	return proj;
}

Project::Project(const std::string& path, bool create)
	:	path(path)
{
	if (create) {
		this->cfg_projrevision = 0;
	} else {
		this->load();
	}
	this->game = std::make_unique<Game>(this->cfg_game);
}

Project::~Project()
{
}

void Project::load()
{
	Glib::KeyFile config;
	config.load_from_file(this->getProjectFile());
	int version = config.get_integer("camoto", "version");
	if (version > CONFIG_FILE_VERSION) throw EProjectOpenFailure(_(
		"This project was created by a newer version of Camoto Studio.  You "
		"will need to upgrade before you can open it."));

	try {
		this->cfg_game = config.get_string("camoto", "game");
	} catch (const Glib::KeyFileError& e) {
		throw EProjectOpenFailure(_("Project file does not specify which game to edit!"));
	}
	try {
		this->cfg_orig_game = config.get_string("camoto", "orig_game_path");
	} catch (...) {
		// do nothing
	}
	try {
		this->cfg_projrevision = config.get_integer("camoto", "projrevision");
	} catch (...) {
		this->cfg_projrevision = 0;
	}

	try {
		for (auto i : config.get_keys("lastExtractPath")) {
			this->cfg_lastExtract[i].path = config.get_string("lastExtractPath", i);
		}
		for (auto i : config.get_keys("lastExtractDecoded")) {
			auto it = this->cfg_lastExtract.find(i);
			if (it != this->cfg_lastExtract.end()) {
				it->second.applyFilters = config.get_boolean("lastExtractDecoded", i);
			}
		}
	} catch (...) {
	}

	try {
		for (auto i : config.get_keys("lastReplacePath")) {
			this->cfg_lastReplace[i].path = config.get_string("lastReplacePath", i);
		}
		for (auto i : config.get_keys("lastReplaceDecoded")) {
			auto it = this->cfg_lastReplace.find(i);
			if (it != this->cfg_lastReplace.end()) {
				it->second.applyFilters = config.get_boolean("lastReplaceDecoded", i);
			}
		}
	} catch (...) {
	}
	return;
}

void Project::save()
{
	this->cfg_projrevision++;
	Glib::KeyFile config;
	config.set_integer("camoto", "version", CONFIG_FILE_VERSION);
	config.set_string("camoto", "game", this->cfg_game);
	config.set_string("camoto", "orig_game_path", this->cfg_orig_game);
	config.set_integer("camoto", "projrevision", this->cfg_projrevision);
	for (auto& i : this->cfg_lastExtract) {
		config.set_string("lastExtractPath", i.first, i.second.path);
		config.set_boolean("lastExtractDecoded", i.first, i.second.applyFilters);
	}
	for (auto& i : this->cfg_lastReplace) {
		config.set_string("lastReplacePath", i.first, i.second.path);
		config.set_boolean("lastReplaceDecoded", i.first, i.second.applyFilters);
	}
	config.save_to_file(this->getProjectFile());
	return;
}

std::string Project::getBasePath() const
{
	return this->path;
}

std::string Project::getDataPath() const
{
	return Glib::build_filename(this->path, PROJECT_GAME_DATA);
}

std::string Project::getProjectFile() const
{
	return Glib::build_filename(this->path, PROJECT_FILENAME);
}

Glib::ustring Project::getProjectTitle() const
{
	auto projDir = Gio::File::create_for_path(this->path);
	auto inf = projDir->query_info(G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
	return inf->get_display_name();
}

std::unique_ptr<stream::inout> Project::openFile(Gtk::Window* win,
	const GameObject& o, bool useFilters)
{
	std::unique_ptr<stream::inout> s;

	// Open the file containing the item's data
	if (!o.idParent.empty()) {
		// This file is contained within an archive
		try {
			s = this->openFileFromArchive(win, o.idParent, o.filename, useFilters);
		} catch (const stream::error& e) {
			throw EFailure(Glib::ustring::compose(
				"%1\n\n[%2]",
				Glib::ustring::compose(
					_("Could not open file \"%1\" (id \"%2\") inside archive \"%3\"."),
					o.filename,
					o.id,
					o.idParent
				),
				e.what()
			));
		}
	} else {
		// This is an actual file to open
		auto fn = Glib::build_filename(this->getDataPath(), o.filename);

		// Make sure the filename isn't empty.  If it is, either the XML file
		// has a blank filename (not allowed) or some code has accidentally
		// accessed the object map with an invalid ID, creating an empty entry
		// for that ID which we're now trying to load.
		if (o.filename.empty()) {
			throw EFailure(Glib::ustring::compose(_("Cannot open item \"%1\".  The "
				"game description XML file is missing the filename for this item!"),
				o.id));
		}

		std::cout << "[project] Opening " << fn << "\n";
		auto file = Gio::File::create_for_path(fn);
		if (!file->query_exists()) {
			throw EFailure(Glib::ustring::compose(
				// Translators: %1 is the XML item ID, %2 is the full absolute filename
				// to the missing game file.
				_("Cannot open item \"%1\".  There is a file missing from the "
					"project's copy of the game data files:\n\n%2"),
				o.id,
				fn
			));
		}
		try {
			s = std::make_unique<stream::file>(fn, false/*no create*/);
		} catch (stream::open_error& e) {
			throw EFailure(Glib::ustring::compose(
				_("Unable to open file \"%1\": %2"),
				fn,
				e.what()
			));
		}

		// If it has any filters, apply them
		if (useFilters && (!o.filter.empty())) {
			// The file needs to be filtered first
			auto pFilterType = FilterManager::byCode(o.filter);
			if (!pFilterType) {
				throw EFailure(Glib::ustring::compose(
					_("This file requires decoding with the \"%1\" filter, which could "
						"not be found (is your libgamearchive too old?)"),
					o.filter
				));
			}
			try {
				std::cout << "[project] Applying filter " << o.filter << "\n";
				s = pFilterType->apply(std::move(s), std::bind<void>(&noopTruncate));
			} catch (const camoto::filter_error& e) {
				throw EFailure(Glib::ustring::compose(
					// Translators: %1 is the item ID, %2 is the filter ID, %3 is the
					// error message.
					_("Filter error decoding item \"%1\" with filter \"%2\": %3"),
					o.id,
					o.filter,
					e.what()
				));
			}
		}
	}
	return s;
}

void Project::openSuppsById(Gtk::Window* win, SuppData *suppOut,
	const GameObject& o)
{
	// Load any supplementary files specified in the XML
	for (const auto& i : o.supp) {
		auto os = this->game->findObjectById(i.second);
		if (!os) {
			throw EFailure(Glib::ustring::compose(
				_("Cannot open item \"%1\".  It has a supplementary item in the game "
					"description XML file with an ID of \"%2\", but there is no item "
					"with this ID."),
				o.id,
				i.second
			));
		}
		// Save this item as the main object's supp
		(*suppOut)[i.first] = this->openFile(win, *os, true); // true == apply filters
	}
	return;
}

void Project::openSuppsByFilename(Gtk::Window* win, SuppData *suppOut,
	const SuppFilenames& suppItem)
{
	for (const auto& i : suppItem) {
		if (suppOut->find(i.first) != suppOut->end()) {
			std::cout << "[camotolibs] SuppItem type #" << suppToString(i.first)
				<< " has already been loaded, not loading again from "
				<< i.second << ".\n"
				"[camotolibs]   This could be because a SuppItem was specified in "
				"the game description XML file and it doesn't need to be."
				<< std::endl;
			continue;
		}
		auto os = this->game->findObjectByFilename(i.second, {});
		if (!os) {
			throw EFailure(Glib::ustring::compose(
				"%1\n\n[%2]",
				_("Cannot open this item due to a bug in the Camoto data files."),
				Glib::ustring::compose(
					_("An entry is missing from the game description XML file for the "
						"filename \"%1\""),
					i.second
				)
			));
		}
		(*suppOut)[i.first] = this->openFile(win, *os, true); // true == apply filters
	}
	return;
}

std::shared_ptr<Archive> Project::getArchive(Gtk::Window* win,
	const itemid_t& idArchive)
{
	// See if idArchive is open
	auto itArch = this->archives.find(idArchive);
	if (itArch != this->archives.end()) return itArch->second;

	// Not open, so open it, possibly recursing back here if it's inside
	// another archive

	auto o = this->game->findObjectById(idArchive);
	if (!o) {
		throw EFailure(Glib::ustring::compose(
			_("This item (or one related to it) is supposed to be inside an "
				"archive with an ID of \"%1\", but there's no entry in the game "
				"description XML for an archive with that ID!"),
			idArchive
		));
	}

	auto content = this->openFile(win, *o, true);
	assert(content);
	SuppData suppData;
	this->openSuppsById(win, &suppData, *o);

	// Now the archive file is open, so create an Archive object around it

	std::shared_ptr<Archive> arch;

	// No need to check if idArchive is valid, as openObject() just did that
	if (o->typeMinor.compare(ARCHTYPE_MINOR_FIXED) == 0) {
		// This is a fixed archive, with its files described in the XML
		std::vector<FixedArchiveFile> items;
		for (auto& i : this->game->objects) {
			if (idArchive.compare(i.second.idParent) == 0) {
				// This item is a subfile
				FixedArchiveFile next;
				next.offset = i.second.offset;
				next.size = i.second.size;
				next.name = i.second.filename;
				next.filter = {};
				items.push_back(next);
			}
		}
		arch = gamearchive::make_FixedArchive(std::move(content), items);
	} else {
		// Normal archive file
		arch = ::openObject<ArchiveType>(win, *o, std::move(content), suppData, this);
		if (arch) {
			// Cache for future access
			this->archives[idArchive] = arch;
		}
	}

	return arch; // may be nullptr
}

std::unique_ptr<stream::inout> Project::openFileFromArchive(Gtk::Window* win,
	const itemid_t& idArchive, const std::string& filename, bool useFilters)
{
	auto arch = this->getArchive(win, idArchive);
	// If the user cancels the operation (e.g. after being warned the file might
	// not be in the right format) we'll get null here, so just return silently.
	if (!arch) return nullptr;

	// Now we have the archive containing our file, so find and open it
	Archive::FileHandle f;
	gamearchive::findFile(&arch, &f, filename);
	if (!f) {
		throw EFailure(Glib::ustring::compose(
			// Translators: %1 is the filename of the item being opened, %2 is the
			// XML ID of the archive that is supposed to contain that file
			_("Cannot open this item.  The file \"%1\" could not be found inside the "
				"archive \"%2\"."),
			filename,
			idArchive
		));
	}

	// Open the file
	auto file = arch->open(f, useFilters);
	assert(file);
/*
	// If it has any filters, apply them
	if (useFilters && (!f->filter.empty())) {
		// The file needs to be filtered first
		FilterTypePtr pFilterType(this->mgrArchive->getFilterTypeByCode(f->filter));
		if (!pFilterType) {
			throw EFailure(Glib::ustring::compose(
				_("This file requires decoding with the \"%1\" filter, which could "
					"not be found (is your libgamearchive too old?)"),
				f->filter
			));
		}
		try {
			file = pFilterType->apply(file,
				// Set the truncation function for the prefiltered (uncompressed) size.
				boost::bind<void>(&setRealSize, arch, f, _1)
			);
		} catch (const camoto::filter_error& e) {
			throw EFailure(wxString::Format(_("Error decoding this file: %s"),
				wxString(e.what(), wxConvUTF8).c_str()));
		}
	}
*/
	return file;
}
