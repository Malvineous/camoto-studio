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
#include <camoto/suppitem.hpp>
#include "mainwindow.hpp"
#include "project.hpp"
#include "gamelist.hpp"
#include "efailure.hpp"

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
		IDocument(IMainWindow *parent, const wxString& typeMajor)
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

		/// Save changes to the document.
		virtual void save()
			throw (std::ios::failure) = 0;

	protected:
		IMainWindow *frame;
		wxString typeMajor;

};

/// Small window for additional information about the document being edited.
class IToolPanel: public wxPanel
{
	public:
		IToolPanel(IMainWindow *parent)
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
	camoto::FN_TRUNCATE fnTrunc;
};

/// Map between string and input stream, used for supp data.
typedef std::map<wxString, OpenedSuppItem> SuppMap;

/// Convert a SuppMap structure into a SuppData one.
void suppMapToData(SuppMap& supp, camoto::SuppData &suppData)
	throw ();

class IEditor
{
	public:
		/// Create tool windows that this editor uses.
		/**
		 * This is called when the editor is first created.  It should create any
		 * tool windows it needs, store them in a vector and return it.
		 *
		 * @return Vector of tool windows.
		 */
		virtual IToolPanelVector createToolPanes() const
			throw () = 0;

		/// Does this editor support this file type?
		/**
		 * @param type
		 *   typeMinor from XML file.
		 *
		 * @return true if supported, false if not.
		 */
		virtual bool isFormatSupported(const wxString& type) const
			throw () = 0;

		/// Open an object in this editor.
		/**
		 * @param typeMinor
		 *   File format minor code.  Major code is the editor type (map, audio,
		 *   etc.) while the minor code is the actual file format.
		 *
		 * @param data
		 *   Contents of the file to edit.
		 *
		 * @param filename
		 *   Name of the file.
		 *
		 * @param supp
		 *   Streams for each required supplementary file.
		 *
		 * @param game
		 *   Game description read in from XML file.
		 *
		 * @return IDocument interface to an editor instance.
		 */
		virtual IDocument *openObject(const wxString& typeMinor,
			camoto::iostream_sptr data, camoto::FN_TRUNCATE fnTrunc,
			const wxString& filename, SuppMap supp, const Game *game) const
			throw (EFailure) = 0;

};

#endif // _EDITOR_HPP_
