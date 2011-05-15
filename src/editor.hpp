/**
 * @file   editor.hpp
 * @brief  Interface definition for a format editor.
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

#ifndef _EDITOR_HPP_
#define _EDITOR_HPP_

#include <map>
#include <vector>
#include <wx/wx.h>
#include <camoto/types.hpp>
#include "project.hpp"

/// Base class for a document editor.
class IDocument: public wxPanel
{
	public:
		/// Required to pass parameters to wxPanel parent class.
		/**
		 * @param parent
		 *   Parent window that will display this document.
		 *
		 * @param typeMajor
		 *   Major type of this document (e.g. "map", "tileset", etc.)
		 */
		IDocument(wxWindow *parent, const wxString& typeMajor)
			throw ();

		/// Get the major type (editor ID) of this document.
		/**
		 * This is used when switching to a new document to work out which tool
		 * panes to show.
		 *
		 * @return The document's major type (map, music, tileset, etc.)
		 */
		const wxString& getTypeMajor() const
			throw ();

	protected:
		wxString typeMajor;

};

/// Small window for additional information about the document being edited.
class IToolPanel: public wxPanel
{
	public:
		IToolPanel(wxWindow *parent)
			throw ();

		virtual void getPanelInfo(wxString *id, wxString *label) const
			throw () = 0;

		virtual void switchDocument(IDocument *doc)
			throw () = 0;

		virtual void loadSettings(Project *proj)
			throw () = 0;

		virtual void saveSettings(Project *proj) const
			throw () = 0;
};

/// List of tool panels.
typedef std::vector<IToolPanel *> IToolPanelVector;

/// Details about an open supplementary item.
struct OpenedSuppItem
{
	wxString typeMinor;
	camoto::iostream_sptr stream;
};

/// Map between string and input stream, used for supp data.
typedef std::map<wxString, OpenedSuppItem> SuppMap;

class IEditor
{
	public:
		virtual IToolPanelVector createToolPanes() const
			throw () = 0;

		virtual IDocument *openObject(const wxString& typeMinor,
			camoto::iostream_sptr data, const wxString& filename, SuppMap supp) const
			throw () = 0;

};

#endif // _EDITOR_HPP_
