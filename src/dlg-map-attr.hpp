/**
 * @file   dlg-map-attr.hpp
 * @brief  Dialog box for changing map attributes.
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

#ifndef _DLG_MAP_ATTR_HPP_
#define _DLG_MAP_ATTR_HPP_

#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include "studio.hpp"

/// Map attributes dialog box.
class DlgMapAttr: public wxDialog
{
	public:
		/// Initialise the dialog box.
		/**
		 * @param parent
		 *   Display the dialog within this window.
		 *
		 * @param map
		 *   Map to show and edit attributes in.
		 */
		DlgMapAttr(Studio *parent, camoto::gamemaps::MapPtr map);
		~DlgMapAttr();

		/// Event handler for OK button.
		void onOK(wxCommandEvent& ev);

		/// Event handler for cancel/close button.
		void onCancel(wxCommandEvent& ev);

		/// Event handler for list box selection change.
		void onAttrSelected(wxCommandEvent& ev);

		/// Save the current selection back to newAttr.
		void saveCurrent();

	protected:
		Studio *studio;               ///< Main window
		camoto::gamemaps::MapPtr map; ///< Map to change
		wxStaticText *txtDesc;        ///< Control to display attr description
		wxTextCtrl *ctText;           ///< Control to collect new text value
		wxSpinCtrl *ctInt;            ///< Control to collect new number value
		wxComboBox *ctEnum;           ///< Control to collect new enum value
		wxComboBox *ctFilename;       ///< Control to collect new filename value
		wxStaticBoxSizer *szControls; ///< Sizer for ct* controls

		/// The changes are stored here until saved on OK.
		camoto::gamemaps::Map::AttributePtrVectorPtr newAttr;
		int curSel;   ///< Index into list box of current selection

		DECLARE_EVENT_TABLE();
};

#endif // _DLG_MAP_ATTR_HPP_
