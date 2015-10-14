/**
 * @file   project.hpp
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

#ifndef _PROJECT_HPP_
#define _PROJECT_HPP_

#include <memory>
#include "exceptions.hpp"
#include "gamelist.hpp"

/// Name of subfolder inside project dir storing the game files to be edited
#define PROJECT_GAME_DATA  "data"

/// Name of the .ini file storing project settings, inside the project dir
#define PROJECT_FILENAME   "project.camoto"

/// Value to use in the config file version.  Projects with a newer version
/// than this will not be opened.
#define CONFIG_FILE_VERSION 1

/// Used when project could not be opened.
class EProjectOpenFailure: public EFailure
{
	public:
		EProjectOpenFailure(const std::string& msg);
};

/// Used when game data files can't be copied to new project's folder.
class EProjectCopyFailure: public EFailure
{
	public:
		EProjectCopyFailure();
};

/// Interface to a project.
class Project
{
	public:
		/// Create a new project in the given folder.
		/**
		 * @param targetPath
		 *   Folder where project data is to be stored.  This folder must exist.
		 *
		 * @param gameSource
		 *   Path to the original game files.  These will be copied recursively into
		 *   the 'data' subdirectory inside targetPath.
		 *
		 * @param gameId
		 *   ID of the game being edited.
		 *
		 * @pre targetPath must exist and be a folder.
		 *
		 * @throw EProjectOpenFailure if the project could not be created, or
		 *   EProjectCopyFailure if the game data could not be copied into the
		 *   project folder.
		 *
		 * @return New Project instance.
		 */
		static std::unique_ptr<Project> create(const std::string& targetPath,
			const std::string& gameSource, const std::string& gameId);

		/// Open the project at the given path.
		/**
		 * @param path
		 *   Path to project (name and path of folder containing 'project.camoto',
		 *   but not including 'project.camoto' itself.)
		 *
		 * @param create
		 *   true to create the 'project.camoto', false to leave it alone.
		 *
		 * @throw EProjectOpenFailure if the project could not be created.
		 */
		Project(const std::string& path, bool create = false);

		~Project();

		/// Read project.camoto
		void load();

		/// Write project.camoto
		void save();

		/// Retrieve the base path of the project.
		std::string getBasePath() const;

		/// Retrieve the path to the local copy of the game files.
		std::string getDataPath() const;

		/// Retrieve the path and filename of project.camoto
		std::string getProjectFile() const;

		/// Retrieve the title of the project.
		Glib::ustring getProjectTitle() const;

		/// Get a stream to the given game object's data file.
		/**
		 * @param win
		 *   Parent window in case the user needs to be warned the file is not in
		 *   the right format, and given the choice to proceed or abort.
		 *
		 * @param o
		 *   GameObject of the item to open.
		 *
		 * @param useFilters
		 *   Set to true to apply any filters to the stream before returning.
		 *
		 * @return A stream to the item's data files.
		 *
		 * @throw EFailure if the item could not be opened.
		 */
		std::unique_ptr<camoto::stream::inout> openFile(Gtk::Window* win,
			const GameObject& o, bool useFilters);

		/// Find a game object by ID.
		/**
		 * @param idItem
		 *   ID of the item to open.
		 *
		 * @throw EFailure if the item could not be found.
		 */
		const GameObject& findItem(const itemid_t& idItem);

		void openSuppsByObj(Gtk::Window* win, camoto::SuppData *suppOut,
			const GameObject& o);

		void openSuppsByFilename(Gtk::Window* win, camoto::SuppData *suppOut,
			const camoto::SuppFilenames& suppItem);

		void openDeps(Gtk::Window* win, const GameObject& o,
			camoto::SuppData& suppData, DepData* depData);

		std::shared_ptr<camoto::gamearchive::Archive> getArchive(Gtk::Window* win,
			const itemid_t& idArchive);

		/// Open a file by filename from within an archive identified by ID.
		/**
		 * @return Stream of opened file, or nullptr if the operation was cancelled
		 *   by the user (in which case no messages need be displayed.)
		 */
		std::unique_ptr<camoto::stream::inout> openFileFromArchive(Gtk::Window* win,
			const itemid_t& idArchive, const std::string& filename, bool useFilters);

		// Saved config items
		std::string cfg_game;      ///< ID of the game being edited
		std::string cfg_orig_game; ///< Path to the original game files
		struct ExternalResource {
			Glib::ustring path;
			bool applyFilters;
		};
		std::map<itemid_t, ExternalResource> cfg_lastExtract;
		std::map<itemid_t, ExternalResource> cfg_lastReplace;

		// Shared working objects
		std::unique_ptr<Game> game; ///< Game instance for this project

	protected:
		std::string path;

		/// List of currently open archives
		std::map<itemid_t, std::shared_ptr<camoto::gamearchive::Archive>> archives;

		unsigned int cfg_projrevision;
};

#endif // _PROJECT_HPP_
