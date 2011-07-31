/**
 * @file   main.cpp
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

#include <config.h>

#include <boost/bind.hpp>
#include <iostream>
#include <fstream>
#include <map>

#include "main.hpp"
#include "project.hpp"
#include "gamelist.hpp"
#include "newproj.hpp"
#include "editor.hpp"

#include "audio.hpp"

#include "editor-map.hpp"
#include "editor-music.hpp"
#include "editor-tileset.hpp"

#ifdef __WIN32
#include <windows.h>
int truncate(const char *path, off_t length)
{
	HANDLE f = CreateFile(path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer(f, length, NULL, FILE_BEGIN);
	BOOL b = SetEndOfFile(f);
	CloseHandle(f);
	return b ? -1 : 0;
}

#endif

#include <wx/wx.h>
#include <wx/cmdline.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/aui/aui.h>
#include <wx/cshelp.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

paths path;

/// Data for each item in the tree view
class TreeItemData: public wxTreeItemData {
	public:
		wxString id;

		TreeItemData(const wxString& id)
			throw () :
			id(id)
		{
		}

};

enum {
	IMG_FOLDER = 0,
	IMG_FILE,
};

/// Main window.
class CamotoFrame: public wxFrame {
	public:

		/// Initialise main window.
		/**
		 * @param idStudio
		 *   true to use Studio interface (a project with multiple documents), or
		 *   false to use standalone document editor.
		 */
		CamotoFrame(bool isStudio) :
			wxFrame(NULL, wxID_ANY, _T("Camoto Studio"), wxDefaultPosition,
				wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN),
			isStudio(isStudio),
			project(NULL),
			game(NULL),
			audio(new Audio(this, 48000))
		{
			this->aui.SetManagedWindow(this);

			wxMenu *menuFile = new wxMenu();
			if (isStudio) {
				menuFile->Append(wxID_NEW,    _T("&New project..."));
				menuFile->Append(wxID_OPEN,   _T("&Open project..."));
				menuFile->Append(wxID_SAVE,   _T("&Save project"));
				menuFile->Append(wxID_CLOSE,  _T("&Close project"));
			} else {
				menuFile->Append(wxID_OPEN,   _T("&Open..."));
				menuFile->Append(wxID_SAVE,   _T("&Save"));
				menuFile->Append(wxID_SAVEAS, _T("Save &As..."));
			}
			menuFile->AppendSeparator();
			menuFile->Append(wxID_EXIT, _T("E&xit"));

			wxMenu *menuView = new wxMenu();
			menuView->Append(IDC_RESET, _T("&Reset layout"));

			wxMenu *menuHelp = new wxMenu();
			menuHelp->Append(wxID_ABOUT, _T("&About..."));

			this->menubar = new wxMenuBar();
			menubar->Append(menuFile, _T("&File"));
			menubar->Append(menuView, _T("&View"));
			menubar->Append(menuHelp, _T("&Help"));
			this->SetMenuBar(menubar);

			this->status = this->CreateStatusBar(2, wxST_SIZEGRIP);
			int sbWidths[] = {-2, -3};
			this->status->SetStatusWidths(sizeof(sbWidths) / sizeof(int), sbWidths);
			this->status->SetStatusText(_T("Ready"), 0);

			this->SetMinSize(wxSize(400, 300));

			this->notebook = new wxAuiNotebook(this, IDC_NOTEBOOK);
			this->aui.AddPane(this->notebook, wxAuiPaneInfo().Name(_T("documents")).
				CenterPane().PaneBorder(false));

			if (isStudio) {
				this->treeImages = new wxImageList(16, 16, true, 3);
				this->treeImages->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16)));
				this->treeImages->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16, 16)));

				this->treeCtrl = new wxTreeCtrl(this, IDC_TREE, wxDefaultPosition,
					wxDefaultSize, wxTR_DEFAULT_STYLE | wxNO_BORDER);
				this->treeCtrl->AssignImageList(this->treeImages);
				this->aui.AddPane(this->treeCtrl, wxAuiPaneInfo().Name(_T("tree")).Caption(_T("Items")).
					Left().Layer(1).Position(1).CloseButton(true).MaximizeButton(true));
			}

			// Save the current view state as that which will be used on a reset
			this->defaultPerspective = this->aui.SavePerspective();

			// Load all the editors
			this->editors[_T("map")] = new MapEditor(this);
			this->editors[_T("music")] = new MusicEditor(this, this->audio);
			this->editors[_T("tileset")] = new TilesetEditor(this);

			// Prepare each editor
			for (EditorMap::iterator e = this->editors.begin(); e != this->editors.end(); e++) {
				IToolPanelVector toolPanels = e->second->createToolPanes();
				std::cout << "[main] Creating tool panes for '" << e->first.ToAscii() << "': ";
				PaneVector v;
				for (IToolPanelVector::iterator p = toolPanels.begin(); p != toolPanels.end(); p++) {
					wxString name, label;
					(*p)->getPanelInfo(&name, &label);
					std::cout << name.ToAscii() << " ";
					wxAuiPaneInfo pane;
					pane.Name(name).Caption(label).MinSize(200, 100).
						Right().CloseButton(true).MaximizeButton(true).Hide();//Show();
					this->aui.AddPane(*p, pane); // show is temp!
					v.push_back(*p);
				}
				this->editorPanes[e->first] = v;
				std::cout << "\n";
			}

			this->aui.Update();
			this->setControlStates();
		}

		~CamotoFrame()
			throw ()
		{
			if (this->project) delete this->project;
			if (this->game) delete this->game;
			// this->treeImages will be destroyed by the tree control

			for (EditorMap::iterator e = this->editors.begin(); e != this->editors.end(); e++) {
				delete e->second;
			}
		}

		/// Enable/disable menu items according to current state.
		void setControlStates()
		{
			this->menubar->Enable(wxID_SAVE, this->project);
			this->menubar->Enable(wxID_CLOSE, this->project);
			return;
		}

		/// Event handler for creating a new project.
		void onNewProject(wxCommandEvent& ev)
		{
			//getAllGames(this->gameDataPath);
			if (this->project) {
				if (!this->confirmSave(_T("Close current project"))) return;
			}
			NewProjectDialog newProj(this);
			if (newProj.ShowModal() == wxID_OK) {
				// The project has been created successfully, so open it.
				if (this->project) this->closeProject();
				this->openProject(newProj.getProjectPath());
			}
			return;
		}

		/// Event handler for opening a new file/project.
		void onOpen(wxCommandEvent& ev)
		{
			if (this->project) {
				if (!this->confirmSave(_T("Close current project"))) return;
			}
			wxDirDialog dlg(this, _T("Open project"), wxEmptyString,
				wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
			dlg.SetMessage(_T("Select the folder where the project is located."));
			if (dlg.ShowModal() == wxID_OK) {
				wxFileName fn;
				fn.AssignDir(dlg.GetPath());
				fn.SetFullName(_T("project.camoto"));
				if (!::wxFileExists(fn.GetFullPath())) {
					wxMessageDialog dlg(this, _T("This folder does not contain a camoto project."),
						_T("Open project"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
				} else {
					this->openProject(fn.GetFullPath());
				}
			}
			return;
		}

		/// Event handler for saving the open project/file.
		void onSave(wxCommandEvent& ev)
		{
			if (!this->project) return; // just in case
			this->saveProjectNoisy(); // will display error if needed
			return;
		}

		/// Event handler for closing the open project.
		void onCloseProject(wxCommandEvent& ev)
		{
			if (!this->project) return;
			if (!this->confirmSave(_T("Close project"))) return;
			this->closeProject();
		}

		/// Reset the perspective in case some panels have become inaccessible.
		void onViewReset(wxCommandEvent& ev)
		{
			this->aui.LoadPerspective(this->defaultPerspective);
			return;
		}

		/// Event handler for Help | About.
		void onHelpAbout(wxCommandEvent& ev)
		{
			wxMessageDialog dlg(this, _T(
				CAMOTO_HEADER
				"\n"
				"Camoto Studio is an integrated editing environment for modifying "
				"games from the early 1990s DOS era.  This is the central component in "
				"the Camoto suite of utilities."),
				_T("About Camoto Studio"), wxOK | wxICON_INFORMATION);
			dlg.ShowModal();
			return;
		}

		void onExit(wxCommandEvent& ev)
		{
			this->Close();
			return;
		}

		/// Event handler for main window closing.
		void onClose(wxCloseEvent& ev)
		{
			if (this->project) {
				if (!this->confirmSave(_T("Exit"))) return;
				this->closeProject();
			}
			this->aui.UnInit();
			this->Destroy();
			return;
		}

		void onItemOpened(wxTreeEvent& ev)
		{
			// Find the item that was opened
			TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(ev.GetItem());
			if (!data) return;

			GameObject &o = this->game->objects[data->id];

			// Find an editor for the item
			EditorMap::iterator itEditor = this->editors.find(o.typeMajor);
			if (itEditor == this->editors.end()) {
				wxString msg = wxString::Format(_T("Sorry, there is no editor "
					"available for \"%s\" items."), o.typeMajor.c_str());
				wxMessageDialog dlg(this, msg, _T("Open item"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}

			camoto::iostream_sptr stream;
			SuppMap supp;
			if (!this->openObject(data->id, &stream, &supp)) return;

			IDocument *doc = itEditor->second->openObject(o.typeMinor, stream, o.filename, supp);
			this->notebook->AddPage(doc, this->game->objects[data->id].friendlyName,
				true, wxNullBitmap);
			return;
		}

		bool openObject(const wxString& id, camoto::iostream_sptr *stream, SuppMap *supp)
			throw ()
		{
			GameObject &o = this->game->objects[id];

			// Open the file containing the item's data
			wxFileName fn;
			fn.AssignDir(this->project->getDataPath());
			fn.SetFullName(o.filename);
			std::cout << "[main] Opening " << fn.GetFullPath().ToAscii() << "\n";
			if (!::wxFileExists(fn.GetFullPath())) {
				wxString msg = wxString::Format(_T("Cannot open this item.  There is a "
					"file missing from the project's copy of the game data files:\n\n%s"),
					fn.GetFullPath().c_str());
				wxMessageDialog dlg(this, msg, _T("Open item"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return false;
			}
			std::fstream *pf = new std::fstream;
			stream->reset(pf);
			try {
				pf->exceptions(std::ios::badbit | std::ios::failbit);
				pf->open(fn.GetFullPath().fn_str(), std::ios::in | std::ios::out | std::ios::binary);
			} catch (std::ios::failure& e) {
				wxString msg = wxString::Format(_T("Unable to open %s\n\nReason: %s"),
					fn.GetFullPath().c_str(), wxString(e.what(), wxConvUTF8).c_str());
				wxMessageDialog dlg(this, msg, _T("Open item"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return false;
			}

			// Load any supplementary files
			for (std::map<wxString, wxString>::iterator i = o.supp.begin(); i != o.supp.end(); i++) {
				OpenedSuppItem& si = (*supp)[i->first];
				si.typeMinor = this->game->objects[i->second].typeMinor;
				if (!this->openObject(i->second, &si.stream, supp)) {
					return false;
				}
			}
			return true;
		}

		/// Event handler for moving between open documents.
		void onDocTabChanged(wxAuiNotebookEvent& event)
		{
			IDocument *doc = (IDocument *)this->notebook->GetPage(event.GetSelection());
			this->updateToolPanes(doc);
			return;
		}

		/// Open a new project, replacing the current project.
		/**
		 * @param path
		 *   Full path of 'project.camoto' including filename.
		 *
		 * @pre path must be a valid (existing) file.
		 */
		void openProject(const wxString& path)
			throw ()
		{
			std::cout << "[main] Opening project " << path.ToAscii() << "\n";
			assert(wxFileExists(path));

			try {
				Project *newProj = new Project(path);
				if (this->project) {
					// Close open project
					if (!this->closeProject()) return;
				}
				this->project = newProj;
			} catch (EBase& e) {
				wxString msg = _T("Unable to open project: ");
				msg.Append(e.getMessage());
				wxMessageDialog dlg(this, msg, _T("Open project"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}

			// Load the project's perspective
			wxString ps;
			if (
				(!this->project->config.Read(_T("camoto/perspective"), &ps)) ||
				(!this->aui.LoadPerspective(ps))
			) {
				std::cout << "[main] Couldn't load perspective, using default\n";
				this->aui.LoadPerspective(this->defaultPerspective);
			}
			this->setControlStates();

			// Populate the tree view
			wxString idGame;
			if (!this->project->config.Read(_T("camoto/game"), &idGame)) {
				std::cout << "[main] Project doesn't specify which game to use!\n";
				delete this->project;
				this->project = NULL;

				wxMessageDialog dlg(this,
					_T("Unable to open, project file is corrupted (missing game ID)"),
					_T("Open project"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}

			if (this->game) delete this->game;
			this->game = ::loadGameStructure(idGame);
			if (!this->game) {
				delete this->project;
				this->project = NULL;

				wxMessageDialog dlg(this,
					_T("Unable to parse XML description file for this game!"),
					_T("Open project"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}

			// Add the game icon to the treeview's image list
			wxFileName fn(::path.gameIcons);
			fn.SetName(this->game->id);
			fn.SetExt(_T("png"));
			int imgIndexGame = IMG_FOLDER; // fallback image
			if (::wxFileExists(fn.GetFullPath())) {
				wxImage *image = new wxImage(fn.GetFullPath(), wxBITMAP_TYPE_PNG);
				wxBitmap bitmap(*image);
				imgIndexGame = this->treeImages->Add(bitmap);
			}

			// Populate the treeview
			this->treeCtrl->DeleteAllItems();
			wxTreeItemId root = this->treeCtrl->AddRoot(this->game->title, imgIndexGame);
			this->populateTreeItems(root, this->game->treeItems);
			this->treeCtrl->ExpandAll();
			return;
		}

		void populateTreeItems(wxTreeItemId& root, tree<wxString>& items)
			throw ()
		{
			for (tree<wxString>::children_t::iterator i = items.children.begin(); i != items.children.end(); i++) {
				if (i->children.size() == 0) {
					// This is a document as it has no children
					wxTreeItemData *d = new TreeItemData(i->item);
					this->treeCtrl->AppendItem(root,
						this->game->objects[i->item].friendlyName, IMG_FILE, IMG_FILE, d);
				} else {
					// This is a folder as it has children
					wxTreeItemId folder = this->treeCtrl->AppendItem(root,
						i->item, IMG_FOLDER, IMG_FOLDER, NULL);
					this->populateTreeItems(folder, *i);
				}
			}
			return;
		}

		/// Save the open project, displaying any errors as popups.
		/**
		 * @return true if the save operation succeeded, false on failure (e.g.
		 *   disk full)
		 */
		bool saveProjectNoisy()
			throw ()
		{
			if (!this->saveProjectQuiet()) {
				wxMessageDialog dlg(this, _T("Unable to save the project!  Make sure "
					"there is enough disk space and the project folder hasn't been moved "
					"or deleted."),
					_T("Save project"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return false;
			}
			return true;
		}

		/// Try to save the project but don't display errors on failure.
		/**
		 * @return true if the save operation succeeded, false on failure (e.g.
		 *   disk full)
		 */
		bool saveProjectQuiet()
			throw ()
		{
			assert(this->project);

			// TODO: Save all open documents

			// Update the current perspective
			if (!this->project->config.Write(_T("camoto/perspective"),
				this->aui.SavePerspective())) return false;

			return this->project->save();
		}

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
		bool confirmSave(const wxString& title)
			throw ()
		{
			if (!this->project) return true;

			wxMessageDialog dlg(this, _T("Save project?"),
				title, wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_QUESTION);
			int r = dlg.ShowModal();
			if (r == wxID_YES) {
				// Save project and open documents
				return this->saveProjectNoisy();
			} else if (r == wxID_NO) {
				// Don't save project
				return true;
			}
			// Otherwise cancel was clicked, abort
			return false;
		}

		/// Close the active project and all open documents.
		/**
		 * @pre A project must be open.
		 *
		 * @return true if the project was closed, false if not (e.g. unsaved
		 *   changes in a document and the user opted to cancel.)
		 */
		bool closeProject()
			throw ()
		{
			assert(this->project);
			assert(this->game);

			// TODO: Close all open docs and return false if needed

			// Release the project.
			delete this->project;
			this->project = NULL;

			delete this->game;
			this->game = NULL;

			this->setControlStates();
			return true;
		}

		void updateToolPanes(IDocument *doc)
			throw ()
		{
			// If there's an open document, show its panes
			wxString typeMajor;
			if (doc) {
				typeMajor = doc->getTypeMajor();
			} // else typeMajor will be empty

			for (PaneMap::iterator t = this->editorPanes.begin(); t != this->editorPanes.end(); t++) {
				bool show = (t->first == typeMajor);
				PaneVector& panes = this->editorPanes[t->first];
				for (PaneVector::iterator i = panes.begin(); i != panes.end(); i++) {
					wxAuiPaneInfo& pane = this->aui.GetPane(*i);
					if (show) {
						(*i)->switchDocument(doc);
						pane.Show();
					} else {
						(*i)->switchDocument(NULL);
						pane.Hide();
					}
				}
			}
			this->aui.Update();
			return;
		}

	protected:
		wxAuiManager aui;
		wxStatusBar *status;
		wxMenuBar *menubar;
		wxAuiNotebook *notebook;
		wxTreeCtrl *treeCtrl;
		wxImageList *treeImages;

		wxString defaultPerspective;

		bool isStudio;     ///< true for full studio view, false for standalone editor
		Project *project;  ///< Currently open project or NULL
		Game *game;        ///< Game being edited in current project
		AudioPtr audio;    ///< Common audio output

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
		};

		DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(CamotoFrame, wxFrame)
	EVT_MENU(wxID_NEW, CamotoFrame::onNewProject)
	EVT_MENU(wxID_OPEN, CamotoFrame::onOpen)
	EVT_MENU(wxID_SAVE, CamotoFrame::onSave)
	EVT_MENU(wxID_CLOSE, CamotoFrame::onCloseProject)
	EVT_MENU(IDC_RESET, CamotoFrame::onViewReset)
	EVT_MENU(wxID_ABOUT, CamotoFrame::onHelpAbout)
	EVT_MENU(wxID_EXIT, CamotoFrame::onExit)
	EVT_TREE_ITEM_ACTIVATED(IDC_TREE, CamotoFrame::onItemOpened)
	EVT_CLOSE(CamotoFrame::onClose)

	EVT_AUINOTEBOOK_PAGE_CHANGED(IDC_NOTEBOOK, CamotoFrame::onDocTabChanged)
END_EVENT_TABLE()

class CamotoApp: public wxApp {
	public:

		static const wxCmdLineEntryDesc cmdLineDesc[];

		CamotoApp()
		{
		}

		void OnInitCmdLine(wxCmdLineParser& parser)
		{
			parser.SetDesc(CamotoApp::cmdLineDesc);
			return;
		}

		virtual bool OnCmdLineParsed(wxCmdLineParser& parser)
		{
			wxSimpleHelpProvider *helpProvider = new wxSimpleHelpProvider();
			wxHelpProvider::Set(helpProvider);

			wxImage::AddHandler(new wxPNGHandler());

			::path.dataRoot = _T(DATA_PATH);
			std::cout << "[init] Data root is " DATA_PATH "\n";
			wxFileName next;

			next.AssignDir(::path.dataRoot);
			next.AppendDir(_T("games"));
			::path.gameData = next.GetFullPath();

			next.AssignDir(::path.gameData);
			next.AppendDir(_T("screenshots"));
			::path.gameScreenshots = next.GetFullPath();

			next.AssignDir(::path.gameData);
			next.AppendDir(_T("icons"));
			::path.gameIcons = next.GetFullPath();

			next.AssignDir(::path.dataRoot);
			next.AppendDir(_T("icons"));
			::path.guiIcons = next.GetFullPath();

			CamotoFrame *f;
			wxString filename;
			if (parser.Found(_T("project"), &filename)) {
				f = new CamotoFrame(true);
				if (!::wxFileExists(filename)) {
					wxMessageDialog dlg(f, _T("The supplied project file does not exist!"
							"  The --project option must be given the full path (and "
							"filename) of the 'project.camoto' file inside the project "
							"directory."),
						_T("Open project"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
				} else {
					f->openProject(filename);
				}
			} else if (parser.Found(_T("music"), &filename)) {
				f = new CamotoFrame(false);
				//f->loadMusic(filename);
				wxMessageDialog dlg(f, _T("Sorry, standalone music editor not yet "
						"implemented!"),
					_T("Open song"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
			} else {
				f = new CamotoFrame(true);
			}
			f->Show(true);
			return true;
		}

		virtual bool OnInit()
		{
			std::cout << CAMOTO_HEADER "\n";
			if (!wxApp::OnInit()) return false;
			return true;
		}

};

const wxCmdLineEntryDesc CamotoApp::cmdLineDesc[] = {
	{wxCMD_LINE_OPTION, NULL, _T("project"), _T("Open the given project")},
	{wxCMD_LINE_OPTION, NULL, _T("music"), _T("Open a song in a standalone editor")},
	{wxCMD_LINE_NONE}
};

IMPLEMENT_APP(CamotoApp)
