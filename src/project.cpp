/**
 * @file   project.cpp
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

#include <wx/dir.h>
#include <wx/filename.h>

#include "project.hpp"

EProjectOpenFailure::EProjectOpenFailure(const wxString& msg) :
	EBase(msg)
{
}

EProjectCopyFailure::EProjectCopyFailure() :
	EBase(_T("Unable to copy the game files into the project folder."))
{
}

/// Recursively copy files from one folder to another.
class CopyTo: public wxDirTraverser
{
	public:
		bool error;  ///< Set to true if the copy failed

		CopyTo(const wxString& dest, const wxString& src) :
			error(false),
			dest(dest),
			src(src)
		{
			std::cout << "[copy] Creating " << dest.ToAscii();
			if (!wxMkdir(dest, 0755)) {
				std::cout << " - failed." << std::endl;
				this->error = true;
			} else {
				std::cout << " - ok.\n";
			}
		}

		virtual wxDirTraverseResult OnFile(const wxString& filename)
		{
			wxFileName full;
			full.Assign(filename);

			wxFileName target;
			target.AssignDir(this->dest);
			target.SetFullName(full.GetFullName());
			std::cout << "[copy] " << filename.ToAscii() << " -> "
				<< target.GetFullPath().ToAscii();
			if (!wxCopyFile(filename, target.GetFullPath())) {
				std::cout << " - failed." << std::endl;
				this->error = true;
				return wxDIR_STOP;
			}
			std::cout << " - ok.\n";

			return wxDIR_CONTINUE;
		}

		virtual wxDirTraverseResult OnDir(const wxString& filename)
		{
			wxFileName nextSource;
			nextSource.AssignDir(filename);

			wxFileName nextDest;
			nextDest.AssignDir(this->dest);
			nextDest.AppendDir(nextSource.GetDirs()[nextSource.GetDirCount() - 1]);

			wxDir nextSourceDir(filename);
			CopyTo nextDestCallback(nextDest.GetFullPath(), filename);
			nextSourceDir.Traverse(nextDestCallback);
			if (nextDestCallback.error) {
				this->error = true;
				return wxDIR_STOP;
			}

			return wxDIR_IGNORE;
		}

	protected:
		wxString dest;
		wxString src;

};

Project *Project::create(const wxString& targetPath, const wxString& gameSource)
	throw (EProjectOpenFailure, EProjectCopyFailure)
{
	assert(wxDirExists(targetPath));

	// Copy everything from the source game's directory into the 'data'
	// subdirectory within the project folder.
	std::cout << "[Project::create] Creating new project in " <<
		targetPath.ToAscii() << "\n";
	wxFileName fnCopyDest;
	fnCopyDest.AssignDir(targetPath);
	fnCopyDest.AppendDir(_T(PROJECT_GAME_DATA));
	wxDir dir(gameSource);
	CopyTo copy(fnCopyDest.GetFullPath(), gameSource); // creates 'data' dir
	if ((dir.Traverse(copy) < 0) || (copy.error)) throw EProjectCopyFailure();

	// Create the project config file.
	wxFileName projFilename;
	projFilename.AssignDir(targetPath);
	projFilename.SetFullName(_T(PROJECT_FILENAME));

	wxString strProjFilename = projFilename.GetFullPath();
	{
		wxFileConfig config(wxEmptyString, wxEmptyString, strProjFilename,
			wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
		config.Write(_T("camoto/version"), CONFIG_FILE_VERSION);
		config.Write(_T("camoto/revision"), 1);
		config.Flush();
		// The file will be closed when we leave this block
	}

	Project *p = new Project(strProjFilename); // may throw EProjectOpenFailure
	return p;
}

Project::Project(const wxString& path)
	throw (EProjectOpenFailure) :
	path(path),
	config(wxEmptyString, wxEmptyString, path, wxEmptyString, wxCONFIG_USE_LOCAL_FILE)
{
	long version;
	if (!this->config.Read(_T("camoto/version"), &version)) {
		throw EProjectOpenFailure(
			_T("The project file could not be opened or is corrupted!"));
	}
	if (version > CONFIG_FILE_VERSION) throw EProjectOpenFailure(_T(
			"This project was created by a newer version of Camoto Studio.  You "
			"will need to upgrade before you can open it."));
}

Project::~Project()
	throw ()
{
}

bool Project::save()
	throw ()
{
	return this->config.Flush();
}

wxString Project::getBasePath()
	throw ()
{
	return this->path.GetPath();
}

wxString Project::getDataPath()
	throw ()
{
	wxFileName p;
	p.AssignDir(this->path.GetPath());
	p.AppendDir(_T(PROJECT_GAME_DATA));
	return p.GetFullPath();
}