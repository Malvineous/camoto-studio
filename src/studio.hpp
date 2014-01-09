/**
 * @file   studio.hpp
 * @brief  Interface definition for callback functions implemented by the main
 *         window.
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

#ifndef _STUDIO_HPP_
#define _STUDIO_HPP_

#include <wx/frame.h>
#include <wx/glcanvas.h>
#include <wx/imaglist.h>
#include <wx/aui/aui.h>
#include <wx/treectrl.h>

#include <camoto/gamearchive/manager.hpp>
#include <camoto/gamegraphics/manager.hpp>
#include <camoto/gamemaps/manager.hpp>
#include <camoto/gamemusic/manager.hpp>

/// Order of images in Studio::smallImages
struct ImageListIndex
{
	enum Type {
		Folder,     ///< Directory
		File,       ///< Generic file
		Game,       ///< Game being edited
		InstMute,   ///< Muted instrument
		InstOPL,    ///< Adlib instrument
		InstMIDI,   ///< MIDI patch
		InstPCM,    ///< Sampled instrument
		SeekPrev,   ///< Media: Seek backwards
		Play,       ///< Media: Start playback
		Pause,      ///< Media: Pause playback
		SeekNext,   ///< Media: Seek forwards
		ZoomIn,     ///< Zoom closer
		ZoomNormal, ///< Zoom to 1:1
		ZoomOut,    ///< Zoom out
		Import,     ///< Import document
		Export,     ///< Export to file
	};
};

class Studio;

#include "camotolibs.hpp"
#include "editor.hpp"
#include "audio.hpp"
#include "project.hpp"

/// Main window.
class Studio: public wxFrame, public CamotoLibs
{
	public:
		wxImageList *smallImages;

		/// Initialise main window.
		/**
		 * @param idStudio
		 *   true to use Studio interface (a project with multiple documents), or
		 *   false to use standalone document editor.
		 */
		Studio(bool isStudio);
		virtual ~Studio();

		virtual wxString getGameFilesPath();

		/// Enable/disable menu items according to current state.
		void setControlStates();

		/// Get a GameObject describing the current tree view selection.
		GameObjectPtr getSelectedGameObject();
		/// Get a GameObject for the tree view selection that triggered an event.
		GameObjectPtr getSelectedGameObject(wxTreeEvent& ev);
		/// Get a GameObject describing the given tree view item.
		GameObjectPtr getSelectedGameObject(wxTreeItemId id);

		/// Event handler for creating a new project.
		void onNewProject(wxCommandEvent& ev);

		/// Event handler for opening a new file/project.
		void onOpen(wxCommandEvent& ev);

		/// Event handler for saving the open project/file.
		void onSave(wxCommandEvent& ev);

		/// Event handler for closing the open project.
		void onCloseProject(wxCommandEvent& ev);

		/// Reset the perspective in case some panels have become inaccessible.
		void onViewReset(wxCommandEvent& ev);

		/// Show the preferences window.
		void onSetPrefs(wxCommandEvent& ev);

		/// Load DOSBox and run the game
		void onRunGame(wxCommandEvent& ev);

		/// Event handler for Help | Modding help.
		void onHelpWiki(wxCommandEvent& ev);

		/// Event handler for Help | About.
		void onHelpAbout(wxCommandEvent& ev);

		void onExit(wxCommandEvent& ev);

		/// Event handler for extracting an item in the tree view.
		void onExtractItem(wxCommandEvent& ev);

		/// Event handler for extracting an item without filters.
		void onExtractUnfilteredItem(wxCommandEvent& ev);

		/// Event handler for overwriting an item in the tree view.
		void onOverwriteItem(wxCommandEvent& ev);

		/// Event handler for overwriting an item without filtering.
		void onOverwriteUnfilteredItem(wxCommandEvent& ev);

		/// Event handler for the right-click|properties menu item.
		void onItemProperties(wxCommandEvent& ev);

		void extractItem(const wxString& id, bool useFilters);

		void overwriteItem(const wxString& id, bool useFilters);

		/// Event handler for main window closing.
		void onClose(wxCloseEvent& ev);

		void onItemOpened(wxTreeEvent& ev);

		/// Event handler for moving between open documents.
		void onDocTabChanged(wxAuiNotebookEvent& event);

		/// Event handler for tab closing
		void onDocTabClose(wxAuiNotebookEvent& event);

		void onItemRightClick(wxTreeEvent& ev);

		/// Event handler for tab just been removed
		void onDocTabClosed(wxAuiNotebookEvent& event);

		/// Open a new project, replacing the current project.
		/**
		 * @param path
		 *   Full path of 'project.camoto' including filename.
		 *
		 * @pre path must be a valid (existing) file.
		 *
		 * @note This function does not prompt if there is an open project with
		 *   unsaved changes, it simply discards any unsaved data.
		 */
		void openProject(const wxString& path);

		void populateTreeItems(wxTreeItemId& root, tree<wxString>& items);

		/// Save the open project, displaying any errors as popups.
		/**
		 * @return true if the save operation succeeded, false on failure (e.g.
		 *   disk full)
		 */
		bool saveProjectNoisy();

		/// Try to save the project but don't display errors on failure.
		/**
		 * @return true if the save operation succeeded, false on failure (e.g.
		 *   disk full)
		 */
		bool saveProjectQuiet();

		/// Ask the user if they wish to save the project.
		/**
		 * This function is used to confirm whether the project should be saved
		 * before continuing with an operation that will affect the project (such
		 * as closing the project.)
		 *
		 * If the project has not been modified since it was last saved, no prompt
		 * will appear and the function will return true.
		 *
		 * @param title
		 *   The title to use in the prompt.  This should be the reason for the
		 *   save request, e.g. "Close project" or "Exit".
		 *
		 * @return true if safe to continue (project was saved or user didn't want
		 *   to save) or false if the user wants to abort or there was a problem
		 *   saving.
		 */
		bool confirmSave(const wxString& title);

		/// Close the active project and all open documents.
		/**
		 * @pre A project must be open.
		 *
		 * @note This function does not attempt to save the project, or warn if
		 *   unsaved changes will be lost.
		 */
		void closeProject();

		void updateToolPanes(IDocument *doc);
		virtual void setStatusText(const wxString& text);
		virtual void setHelpText(const wxString& text);
		virtual wxGLContext *getGLContext();
		virtual int *getGLAttributes();
		void updateStatusBar();

		/// Remove any run commands from the 'Test' menu
		void clearTestMenu();

	protected:
		wxGLContext *glcx; ///< Shared OpenGL context
		wxAuiManager aui;
		wxStatusBar *status;
		wxMenuBar *menubar;
		wxMenu *menuTest;
		wxAuiNotebook *notebook;
		wxTreeCtrl *treeCtrl;
		wxMenu *popup;     ///< Popup menu when right-clicking in tree view

		wxString defaultPerspective;
		wxString txtMessage; ///< Last hint set for status bar
		wxString txtHelp;    ///< Last keyboard help set for status bar

		bool isStudio;     ///< true for full studio view, false for standalone editor
		Project *project;  ///< Currently open project or NULL
		AudioPtr audio;    ///< Common audio output

		std::map<long, wxString> commandMap;

		/// Map containing typeMajor -> IEditor instance used to edit that type
		typedef std::map<wxString, IEditor *> EditorMap;
		EditorMap editors; ///< List of editors currently available

		typedef std::vector<IToolPanel *> PaneVector;
		typedef std::map<wxString, PaneVector> PaneMap;
		PaneMap editorPanes;

		/// Control IDs
		enum {
			IDC_RESET = wxID_HIGHEST + 1,
			IDC_NOTEBOOK,
			IDC_TREE,
			IDC_OPENWIKI,
			IDM_EXTRACT,
			IDM_EXTRACT_RAW,
			IDM_OVERWRITE,
			IDM_OVERWRITE_RAW,
		};

		DECLARE_EVENT_TABLE();
};

#endif // _STUDIO_HPP_
