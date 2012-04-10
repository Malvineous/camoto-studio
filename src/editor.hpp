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
#include <camoto/stream.hpp>
#include <camoto/suppitem.hpp>
#include "mainwindow.hpp"
#include "project.hpp"
#include "gamelist.hpp"
#include "efailure.hpp"

/// Base class for a document editor.
class IDocument: public wxPanel
{
	public:
		/// Has the document changed?
		/**
		 * Set to true whenever a change is made, and to false whenever the document
		 * is saved or the change is otherwise reverted.  When the document is
		 * closed and this is true, the user is prompted to save changes.
		 *
		 * @note Automatically set to false in the IDocument constructor.
		 */
		bool isModified;

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
			throw (camoto::stream::error) = 0;

		/// Set the text in the hint part of the status bar.
		/**
		 * @param text
		 *   Text to display, or wxString() to display nothing.
		 */
		void setStatusText(const wxString& text)
			throw ();

		/// Set the text in the keyboard help part of the status bar.
		/**
		 * @param text
		 *   Text to display, or wxString() to display nothing.
		 */
		void setHelpText(const wxString& text)
			throw ();

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
	camoto::stream::inout_sptr stream;
};

/// Map between string and input stream, used for supp data.
typedef std::map<wxString, OpenedSuppItem> SuppMap;

/// Convert a SuppMap structure into a SuppData one.
void suppMapToData(SuppMap& supp, camoto::SuppData &suppData)
	throw ();

class IEditor
{
	public:
		virtual ~IEditor()
			throw ();

		/// Create tool windows that this editor uses.
		/**
		 * This is called when the editor is first created.  It should create any
		 * tool windows it needs, store them in a vector and return it.
		 *
		 * @return Vector of tool windows.
		 */
		virtual IToolPanelVector createToolPanes() const
			throw () = 0;

		/// Load this editor's settings from the project.
		/**
		 * This is called when a project is loaded, to update the editor's view
		 * settings with those in the project.
		 */
		virtual void loadSettings(Project *proj)
			throw () = 0;

		/// Save this editor's settings to the project.
		/**
		 * This is called when a project is saved, to store the editor's current
		 * view settings in the project.
		 */
		virtual void saveSettings(Project *proj) const
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
			camoto::stream::inout_sptr data, const wxString& filename, SuppMap supp,
			const Game *game)
			throw (EFailure) = 0;

};

#endif // _EDITOR_HPP_
