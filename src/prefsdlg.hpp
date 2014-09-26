/**
 * @file   prefsdlg.hpp
 * @brief  Dialog box for the preferences window.
 *
 * Copyright (C) 2010-2014 Adam Nielsen <malvineous@shikadi.net>
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

#include <boost/thread/thread.hpp>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include "studio.hpp"
#include "audio.hpp"

/// User preferences dialog box.
class PrefsDialog: public wxDialog
{
	public:
		wxString *pathDOSBox;     ///< Path of DOSBox .exe
		bool *pauseAfterExecute;  ///< Pause in DOSBox after running game?

		/// Initialise main window.
		/**
		 * @param parent
		 *   Display the dialog within this window.
		 */
		PrefsDialog(Studio *parent, AudioPtr audio);

		~PrefsDialog();

		/// Update controls to reflect public vars.
		void setControls();

		/// Event handler for OK button.
		void onOK(wxCommandEvent& ev);

		/// Event handler for cancel/close button.
		void onCancel(wxCommandEvent& ev);

		/// Event handler for DOSBox .exe browse button.
		void onBrowseDOSBox(wxCommandEvent& ev);

	protected:
		AudioPtr audio;

		wxTextCtrl *txtDOSBoxPath;
		wxCheckBox *chkDOSBoxPause;
		wxListCtrl *portList;

		DECLARE_EVENT_TABLE();
};

#endif // _PREFSDLG_HPP_
