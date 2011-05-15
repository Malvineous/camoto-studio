/**
 * @file   project.hpp
 * @brief  Project data management and manipulation.
 *
 * Copyright (C) 2010-2011 Adam Nielsen <malvineous@shikadi.net>
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

#include <wx/wx.h>
#include <wx/fileconf.h>
#include <wx/filename.h>

#include "exceptions.hpp"

/// Name of subfolder inside project dir storing the game files to be edited
#define PROJECT_GAME_DATA  "data"

/// Name of the .ini file storing project settings, inside the project dir
#define PROJECT_FILENAME   "project.camoto"

/// Value to use in the config file version.  Projects with a newer version
/// than this will not be opened.
#define CONFIG_FILE_VERSION 1

/// Used when project could not be opened.
class EProjectOpenFailure: public EBase
{
	public:
		EProjectOpenFailure(const wxString& msg);
};

/// Used when game data files can't be copied to new project's folder.
class EProjectCopyFailure: public EBase
{
	public:
		EProjectCopyFailure();
};

/// Interface to a project.
class Project
{
	public:

		/// Access to config data saved with the project.
		wxFileConfig config;

		/// Create a new project in the given folder.
		/**
		 * @param targetPath
		 *   Folder where project data is to be stored.  This folder must exist.
		 *
		 * @param gameSource
		 *   Path to the original game files.  These will be copied recursively into
		 *   the 'data' subdirectory inside targetPath.
		 *
		 * @pre targetPath must exist or an assertion failure will occur.
		 *
		 * @throw EProjectOpenFailure if the project could not be created, or
		 *   EProjectCopyFailure if the game data could not be copied into the
		 *   project folder.
		 *
		 * @return New Project instance.
		 */
		static Project *create(const wxString& targetPath, const wxString& gameSource)
			throw (EProjectOpenFailure, EProjectCopyFailure);

		/// Open the project at the given path.
		/**
		 * @param path
		 *   Full name and path of 'project.camoto' file.  This file should exist,
		 *   however it will be created if not (e.g. when creating a new project.)
		 *
		 * @throw EProjectOpenFailure if the project could not be created.
		 */
		Project(const wxString& path)
			throw (EProjectOpenFailure);

		~Project()
			throw ();

		bool save()
			throw ();

		/// Retrieve the base path of the project.
		wxString getBasePath()
			throw ();

		/// Retrieve the path to the local copy of the game files.
		wxString getDataPath()
			throw ();


	protected:
		wxFileName path;

};

#endif // _PROJECT_HPP_
