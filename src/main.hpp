/**
 * @file   main.hpp
 * @brief  Entry point for Camoto Studio.
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

#ifndef _MAIN_HPP_
#define _MAIN_HPP_

#ifdef __WIN32
int truncate(const char *path, off_t length);
#endif

#define CAMOTO_HEADER \
	"Camoto Game Modding Studio\n" \
	"Copyright (C) 2010-2011 Adam Nielsen <malvineous@shikadi.net>\n" \
	"http://www.shikadi.net/camoto\n" \
	"\n" \
	"This program comes with ABSOLUTELY NO WARRANTY.  This is free software,\n" \
	"and you are welcome to change and redistribute it under certain conditions;\n" \
	"see <http://www.gnu.org/licenses/> for details.\n"

#include <wx/string.h>

struct paths {
	wxString dataRoot;          ///< Main data folder
	wxString gameData;          ///< Location of XML game description files
	wxString gameScreenshots;   ///< Game screenshots used in 'new project' dialog
	wxString gameIcons;         ///< Icons used to represent each game
	wxString guiIcons;          ///< Icons used for GUI elements
	wxString lastUsed;          ///< Path last used in open/save dialogs
};

extern paths path;

struct config_data {
	wxString dosboxPath;
	bool dosboxExitPause;
};

extern config_data config;

/// Show additional errors.
/**
 * If this is defined, additional error message popups will appear.  Usually
 * these are suppressed as wxWidgets calls wxLogError() which notifies the user
 * instead.  If this facility is ever disabled, uncomment this to provide the
 * user with feedback.
 */
#undef NO_WXLOG_POPUPS

#endif // _MAIN_HPP_
