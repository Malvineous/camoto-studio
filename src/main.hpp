/**
 * @file   main.hpp
 * @brief  Entry point for Camoto Studio.
 *
 * Copyright (C) 2010-2013 Adam Nielsen <malvineous@shikadi.net>
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

#define CAMOTO_HEADER \
	"Camoto Game Modding Studio\n" \
	"Copyright (C) 2010-2013 Adam Nielsen <malvineous@shikadi.net>\n" \
	"http://www.shikadi.net/camoto\n" \
	"\n" \
	"This program comes with ABSOLUTELY NO WARRANTY.  This \n" \
	"is free software, and you are welcome to change and \n" \
	"redistribute it under certain conditions; see \n" \
	"<http://www.gnu.org/licenses/> for details.\n"

#include <wx/string.h>

/// File paths
struct paths
{
	wxString dataRoot;          ///< Main data folder
	wxString gameData;          ///< Location of XML game description files
	wxString gameScreenshots;   ///< Game screenshots used in 'new project' dialog
	wxString gameIcons;         ///< Icons used to represent each game
	wxString guiIcons;          ///< Icons used for GUI elements
	wxString mapIndicators;     ///< Icons used for map editor indicators
	wxString lastUsed;          ///< Path last used in open/save dialogs
};

extern paths path;

/// User preferences
struct config_data
{
	/// Path to DOSBox binary
	wxString dosboxPath;

	/// True to add a DOS 'pause' command before exiting DOSBox
	bool dosboxExitPause;

	/// Index of MIDI device to use
	int midiDevice;

	/// Digital output delay (relative to MIDI output) in milliseconds
	int pcmDelay;
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

/// Call wxApp::Yield()
void yield();

#endif // _MAIN_HPP_
