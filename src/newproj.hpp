/**
 * @file   newproj.hpp
 * @brief  Dialog box for creating a new project.
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

#ifndef _NEWPROJ_HPP_
#define _NEWPROJ_HPP_

#include <wx/treectrl.h>

#include "gamelist.hpp"

/// Main window.
class NewProjectDialog: public wxDialog {
	public:

		/// Initialise main window.
		/**
		 * @param parent
		 *   Display the dialog within this window.
		 */
		NewProjectDialog(wxWindow *parent);

		~NewProjectDialog();

		/// Event handler for OK button.
		void onOK(wxCommandEvent& ev);

		/// Event handler for project folder browse button.
		void onBrowseDest(wxCommandEvent& ev);

		/// Event handler for game source folder browse button.
		void onBrowseSrc(wxCommandEvent& ev);

		/// Event handler for OK button.
		void onTreeSelChanged(wxTreeEvent& ev);

		/// Get the path of the newly created project.
		/**
		 * @pre ShowModal() has returned wxID_OK.
		 *
		 * @return Path to new project, including 'project.camoto' filename.
		 */
		const wxString& getProjectPath() const;

	protected:
		wxTreeCtrl *treeCtrl;
		wxStaticBitmap *screenshot;
		wxTextCtrl *txtDest;
		wxTextCtrl *txtSrc;
		wxStaticText *txtGame;
		wxStaticText *txtDeveloper;
		wxTextCtrl *txtReverser;

		GameInfoMap games;

		wxString projectPath;
		wxString idGame;

		DECLARE_EVENT_TABLE();
};

#endif // _NEWPROJ_HPP_
