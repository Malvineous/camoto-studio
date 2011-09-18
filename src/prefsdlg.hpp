/**
 * @file   prefsdlg.hpp
 * @brief  Dialog box for the preferences window.
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

#ifndef _PREFSDLG_HPP_
#define _PREFSDLG_HPP_

#include <wx/dialog.h>

/// Main window.
class PrefsDialog: public wxDialog {
	public:

		wxString *pathDOSBox;     ///< Path of DOSBox .exe
		bool *pauseAfterExecute;  ///< Pause in DOSBox after running game?

		/// Initialise main window.
		/**
		 * @param parent
		 *   Display the dialog within this window.
		 */
		PrefsDialog(wxWindow *parent)
			throw ();

		~PrefsDialog()
			throw ();

		/// Update controls to reflect public vars.
		void setControls()
			throw ();

		/// Event handler for OK button.
		void onOK(wxCommandEvent& ev);

		/// Event handler for DOSBox .exe browse button.
		void onBrowseDOSBox(wxCommandEvent& ev);

	protected:
		wxTextCtrl *txtDOSBoxPath;
		wxCheckBox *chkDOSBoxPause;

		DECLARE_EVENT_TABLE();
};

#endif // _PREFSDLG_HPP_
