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
#include <fstream>
#include <map>

#include <camoto/gamearchive.hpp>
#include <camoto/stream_file.hpp>
#include <camoto/util.hpp>

#include "main.hpp"
#include "project.hpp"
#include "gamelist.hpp"
#include "newproj.hpp"
#include "prefsdlg.hpp"
#include "editor.hpp"
#include "efailure.hpp"
#include "audio.hpp"

#include "editor-map.hpp"
#include "editor-music.hpp"
#include "editor-tileset.hpp"
#include "editor-image.hpp"

#include <wx/wx.h>
#include <wx/cmdline.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/aui/aui.h>
#include <wx/cshelp.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

namespace ga = camoto::gamearchive;
namespace stream = camoto::stream;

paths path;
config_data config;

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

/// Callback function to set expanded/native file size.
void setNativeSize(camoto::gamearchive::ArchivePtr arch,
	camoto::gamearchive::Archive::EntryPtr id, stream::len newSize)
	throw (camoto::stream::error)
{
	arch->resize(id, newSize, id->iPrefilteredSize);
	arch->flush();
	return;
}

/// Main window.
class CamotoFrame: public IMainWindow
{
	public:

		/// Initialise main window.
		/**
		 * @param idStudio
		 *   true to use Studio interface (a project with multiple documents), or
		 *   false to use standalone document editor.
		 */
		CamotoFrame(bool isStudio) :
			IMainWindow(NULL, wxID_ANY, _T("Camoto Studio"), wxDefaultPosition,
				wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN),
			isStudio(isStudio),
			project(NULL),
			game(NULL),
			audio(new Audio(this, 48000)),
			archManager(ga::getManager())
		{
			this->aui.SetManagedWindow(this);

			wxMenu *menuFile = new wxMenu();
			if (isStudio) {
				menuFile->Append(wxID_NEW,    _T("&New project..."), _T("Choose a new game to work with"));
				menuFile->Append(wxID_OPEN,   _T("&Open project..."), _T("Resume work on a previous mod"));
				menuFile->Append(wxID_SAVE,   _T("&Save project"), _T("Save options and all open documents"));
				menuFile->Append(wxID_CLOSE,  _T("&Close project"), _("Close all open documents"));
			} else {
				menuFile->Append(wxID_OPEN,   _T("&Open..."), _T("Open a new file"));
				menuFile->Append(wxID_SAVE,   _T("&Save"), _T("Save changes"));
				menuFile->Append(wxID_SAVEAS, _T("Save &As..."), _T("Save changes to a separate file"));
			}
			menuFile->AppendSeparator();
			menuFile->Append(wxID_EXIT, _T("E&xit"));

			wxMenu *menuView = new wxMenu();
			menuView->Append(IDC_RESET, _T("&Reset layout"), _T("Reset all windows to default sizes"));
			menuView->Append(wxID_SETUP, _T("&Options..."), _T("Change settings"));

			this->menuTest = new wxMenu();

			wxMenu *menuHelp = new wxMenu();
			menuHelp->Append(wxID_ABOUT, _T("&About..."));

			this->menubar = new wxMenuBar();
			menubar->Append(menuFile, _T("&File"));
			menubar->Append(menuView, _T("&View"));
			menubar->Append(menuTest, _T("&Test"));
			menubar->Append(menuHelp, _T("&Help"));
			this->SetMenuBar(menubar);

			this->status = this->CreateStatusBar(1, wxST_SIZEGRIP);
			this->status->SetStatusText(_T("Ready"));

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
			this->editors[_T("image")] = new ImageEditor(this);

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

			this->popup = new wxMenu();
			this->popup->Append(IDM_EXTRACT,       _T("&Extract file..."), _T("Save this file in its native format"));
			this->popup->Append(IDM_EXTRACT_RAW,   _T("&Extract raw..."), _T("Save this file in its raw format (no decompression or decryption)"));
			this->popup->Append(IDM_OVERWRITE,     _T("&Overwrite file..."), _T("Replace this item with the contents of another file"));
			this->popup->Append(IDM_OVERWRITE_RAW, _T("&Overwrite raw..."), _T("Replace this item with a file already encrypted or compressed in the correct format"));

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

			delete this->popup;
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
			if (this->project) {
				if (!this->confirmSave(_T("Close current project"))) return;
			}
			NewProjectDialog newProj(this);
			if (newProj.ShowModal() == wxID_OK) {
				// The project has been created successfully, so open it.
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
			return;
		}

		/// Reset the perspective in case some panels have become inaccessible.
		void onViewReset(wxCommandEvent& ev)
		{
			this->aui.LoadPerspective(this->defaultPerspective);
			return;
		}

		/// Show the preferences window.
		void onSetPrefs(wxCommandEvent& ev)
		{
			PrefsDialog prefs(this);
			prefs.pathDOSBox = &::config.dosboxPath;
			prefs.pauseAfterExecute = &::config.dosboxExitPause;
			prefs.setControls();
			if (prefs.ShowModal() == wxID_OK) {
				// Save the user's preferences
				wxConfigBase *configFile = wxConfigBase::Get(true);
				configFile->Write(_T("camoto/dosbox"), ::config.dosboxPath);
				configFile->Write(_T("camoto/pause"), ::config.dosboxExitPause);
				configFile->Flush();
			}
			return;
		}

		/// Load DOSBox and run the game
		void onRunGame(wxCommandEvent& ev)
		{
			std::map<long, wxString>::iterator i = this->commandMap.find(ev.GetId());
			if (i == this->commandMap.end()) {
				std::cerr << "[main] Menu item has no command associated with this ID!?"
					<< std::endl;
				return;
			}

			// Create a batch file to run the game with any parameters needed
			wxFileName exe;
			exe.AssignDir(this->project->getDataPath());
			exe.SetFullName(_T("camoto.bat"));
			wxString batName = exe.GetFullPath();

			wxFile bat;
			if (!bat.Create(batName, true)) {
				wxMessageDialog dlg(this, _T("Could not create 'camoto.bat' in the "
					"project data directory!  Make sure you have sufficient disk space "
					"and write access to the folder."),
					_T("Run game"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
			if (::config.dosboxExitPause) bat.Write(_T("@echo off\n"));
			bat.Write(i->second);
			if (::config.dosboxExitPause) bat.Write(_T("\npause > nul\n"));
			bat.Close();

			// Launch the batch file in DOSBox
			const wxChar **argv = new const wxChar*[5];
			argv[0] = ::config.dosboxPath.c_str();
			argv[1] = batName.c_str();
			argv[2] = _T("-noautoexec");
			argv[3] = _T("-exit");
			argv[4] = NULL;
			if (::wxExecute((wxChar **)argv, wxEXEC_ASYNC) == 0) {
				wxMessageDialog dlg(this, _T("Unable to launch DOSBox!  Make sure the "
					"path is correct in the Camoto preferences window."),
					_T("Run game"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
			}
			delete argv;
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

		/// Event handler for extracting an item in the tree view.
		void onExtractItem(wxCommandEvent& ev)
		{
			TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(this->treeCtrl->GetSelection());
			if (!data) return;
			this->extractItem(data->id, true); // true == use filters
			return;
		}

		/// Event handler for extracting an item without filters.
		void onExtractUnfilteredItem(wxCommandEvent& ev)
		{
			TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(this->treeCtrl->GetSelection());
			if (!data) return;
			this->extractItem(data->id, false); // false == don't use filters
			return;
		}

		/// Event handler for overwriting an item in the tree view.
		void onOverwriteItem(wxCommandEvent& ev)
		{
			TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(this->treeCtrl->GetSelection());
			if (!data) return;
			this->overwriteItem(data->id, true); // true == use filters
			return;
		}

		/// Event handler for overwriting an item without filtering.
		void onOverwriteUnfilteredItem(wxCommandEvent& ev)
		{
			TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(this->treeCtrl->GetSelection());
			if (!data) return;
			this->overwriteItem(data->id, false); // false == don't use filters
			return;
		}

		void extractItem(const wxString& id, bool useFilters)
		{
			if (!this->project) return; // just in case

			// Make sure the ID is valid
			GameObjectMap::iterator io = this->game->objects.find(id);
			if (io == this->game->objects.end()) {
				throw EFailure(wxString::Format(_T("Cannot open this item.  It refers "
					"to an entry in the game description XML file with an ID of \"%s\", "
					"but there is no item with this ID."),
					id.c_str()));
			}
			GameObjectPtr& o = io->second;

			wxString path = wxFileSelector(_T("Save file"),
				::path.lastUsed, o->filename, wxEmptyString,
#ifdef __WXMSW__
				_T("All files (*.*)|*.*"),
#else
				_T("All files (*)|*"),
#endif
				wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
			if (!path.empty()) {
				::path.lastUsed = wxFileName::FileName(path).GetPath();
				try {
					stream::inout_sptr extract = this->openFile(o, useFilters);
					assert(extract); // should have thrown an exception on error

					stream::output_file_sptr out(new stream::output_file());
					out->create(path.fn_str());

					stream::copy(out, extract);
					out->flush();

				} catch (const stream::open_error& e) {
					wxMessageDialog dlg(this,
						wxString::Format(_T("Unable to create file!\n\n[%s]"),
							wxString(e.what(), wxConvUTF8).c_str()),
						_T("Extract item"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return;

				} catch (const std::exception& e) {
					wxMessageDialog dlg(this,
						wxString::Format(_T("Unexpected error while extracting the item!\n\n[%s]"),
							wxString(e.what(), wxConvUTF8).c_str()),
						_T("Extract item"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return;
				}
			}
			return;
		}

		void overwriteItem(const wxString& id, bool useFilters)
		{
			if (!this->project) return; // just in case

			// Make sure the ID is valid
			GameObjectMap::iterator io = this->game->objects.find(id);
			if (io == this->game->objects.end()) {
				throw EFailure(wxString::Format(_T("Cannot open this item.  It refers "
					"to an entry in the game description XML file with an ID of \"%s\", "
					"but there is no item with this ID."),
					id.c_str()));
			}
			GameObjectPtr& o = io->second;

			wxString path = wxFileSelector(_T("Open file"),
				::path.lastUsed, o->filename, wxEmptyString,
#ifdef __WXMSW__
				_T("All files (*.*)|*.*"),
#else
				_T("All files (*)|*"),
#endif
				wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
			if (!path.empty()) {
				::path.lastUsed = wxFileName::FileName(path).GetPath();
				try {
					stream::inout_sptr dest = this->openFile(o, useFilters);
					assert(dest); // should have thrown an exception on error

					stream::input_file_sptr in(new stream::input_file());
					in->open(path.fn_str());

					stream::copy(dest, in);
					dest->flush();

				} catch (const stream::open_error& e) {
					wxMessageDialog dlg(this,
						wxString::Format(_T("Unable to open source file!\n\n[%s]"),
							wxString(e.what(), wxConvUTF8).c_str()),
						_T("Overwrite item"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return;

				} catch (const std::exception& e) {
					wxMessageDialog dlg(this,
						wxString::Format(_T("Unexpected error while replacing the item!\n\n[%s]"),
							wxString(e.what(), wxConvUTF8).c_str()),
						_T("Overwrite item"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return;
				}
			}
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

			// Make sure the ID is valid
			GameObjectMap::iterator io = this->game->objects.find(data->id);
			if (io == this->game->objects.end()) {
				throw EFailure(wxString::Format(_T("Cannot open this item.  It refers "
					"to an entry in the game description XML file with an ID of \"%s\", "
					"but there is no item with this ID."),
					data->id.c_str()));
			}
			GameObjectPtr &o = io->second;

			// Find an editor for the item
			EditorMap::iterator itEditor = this->editors.find(o->typeMajor);
			if (itEditor == this->editors.end()) {
				wxString msg = wxString::Format(_T("Sorry, there is no editor "
					"available for \"%s\" items."), o->typeMajor.c_str());
				wxMessageDialog dlg(this, msg, _T("Open item"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}

			camoto::stream::inout_sptr stream;
			SuppMap supp;
			try {
				this->openObject(data->id, &stream, &supp);

				IDocument *doc = itEditor->second->openObject(o->typeMinor, stream,
					o->filename, supp, this->game);
				this->notebook->AddPage(doc, this->game->objects[data->id]->friendlyName,
					true, wxNullBitmap);
			} catch (const EFailure& e) {
				wxMessageDialog dlg(this, e.getMessage(), _T("Open failure"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}

			return;
		}

		/// Open an object by ID, complete with supp items.
		void openObject(const wxString& id, camoto::stream::inout_sptr *stream,
			SuppMap *supp)
			throw (EFailure)
		{
			// Make sure the ID is valid
			GameObjectMap::iterator io = this->game->objects.find(id);
			if (io == this->game->objects.end()) {
				throw EFailure(wxString::Format(_T("Cannot open this item.  It refers "
					"to an entry in the game description XML file with an ID of \"%s\", "
					"but there is no item with this ID."),
					id.c_str()));
			}
			GameObjectPtr& o = io->second;

			*stream = this->openFile(o, true); // true == apply filters
			assert(*stream); // should have thrown an exception on error

			// Load any supplementary files
			for (std::map<wxString, wxString>::iterator i = o->supp.begin(); i != o->supp.end(); i++) {
				OpenedSuppItem& si = (*supp)[i->first];
				this->openObject(i->second, &si.stream, supp);

				// Have to put this after openObject() otherwise if i->second is an
				// invalid ID an empty entry will be created, making it look like a
				// valid ID when openObject() checks it.
				si.typeMinor = this->game->objects[i->second]->typeMinor;
			}

			return;
		}

		/// Open an file by object, without suppitems.
		stream::inout_sptr openFile(const GameObjectPtr& o, bool useFilters)
			throw (EFailure)
		{
			stream::inout_sptr s;

			// Open the file containing the item's data
			if (!o->idParent.empty()) {
				// This file is contained within an archive
				try {
					s = this->openFileFromArchive(o->idParent, o->filename, useFilters);
				} catch (const camoto::stream::error& e) {
					throw EFailure(wxString::Format(_T("Could not open this item:\n\n%s"),
							wxString(e.what(), wxConvUTF8).c_str()));
				}
			} else {
				// This is an actual file to open
				wxFileName fn;
				fn.AssignDir(this->project->getDataPath());
				fn.SetFullName(o->filename);

				// Make sure the filename isn't empty.  If it is, either the XML file
				// has a blank filename (not allowed) or some code has accidentally
				// accessed the object map with an invalid ID, creating an empty entry
				// for that ID which we're now trying to load.
				assert(!o->filename.empty());

				std::cout << "[main] Opening " << fn.GetFullPath().ToAscii() << "\n";
				if (!::wxFileExists(fn.GetFullPath())) {
					throw EFailure(wxString::Format(_T("Cannot open this item.  There is a "
						"file missing from the project's copy of the game data files:\n\n%s"),
						fn.GetFullPath().c_str()));
				}
				stream::file_sptr pf(new stream::file());
				try {
					pf->open(fn.GetFullPath().fn_str());
					s = pf;
				} catch (camoto::stream::open_error& e) {
					throw EFailure(wxString::Format(_T("Unable to open %s\n\nReason: %s"),
						fn.GetFullPath().c_str(), wxString(e.what(), wxConvUTF8).c_str()));
				}
			}
			return s;
		}

		/// Event handler for moving between open documents.
		void onDocTabChanged(wxAuiNotebookEvent& event)
		{
			IDocument *doc = (IDocument *)this->notebook->GetPage(event.GetSelection());
			this->updateToolPanes(doc);
			return;
		}

		/// Event handler for tab closing
		void onDocTabClose(wxAuiNotebookEvent& event)
		{
			IDocument *doc = (IDocument *)this->notebook->GetPage(event.GetSelection());
			if (doc->isModified) {
				wxMessageDialog dlg(this, _T("Save changes?"),
					_T("Close document"), wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_QUESTION);
				int r = dlg.ShowModal();
				if (r == wxID_YES) {
					try {
						doc->save();
					} catch (const camoto::stream::error& e) {
						wxString msg = _T("Unable to save document: ");
						msg.Append(wxString(e.what(), wxConvUTF8));
						wxMessageDialog dlg(this, msg, _T("Save failed"), wxOK | wxICON_ERROR);
						dlg.ShowModal();
						event.Veto();
					}
				} else if (r == wxID_NO) {
					// do nothing
				} else { // cancel
					event.Veto();
				}
			} // else doc hasn't been modified
			return;
		}

		void onItemRightClick(wxTreeEvent& ev)
		{
			// Find the item that was opened
			TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(ev.GetItem());
			if (!data) return;

			// Make sure the ID is valid
			GameObjectMap::iterator io = this->game->objects.find(data->id);
			if (io == this->game->objects.end()) {
				return;
			}
			GameObjectPtr& o = io->second;

			bool hasFilters = false;
			try {
				if (!o->idParent.empty()) {
					// This file is contained within an archive
					ga::ArchivePtr arch = this->getArchive(o->idParent);
					if (arch) {
						std::string nativeFilename(o->filename.fn_str());
						ga::Archive::EntryPtr f = ga::findFile(arch, nativeFilename);
						if (f) {
							// Found file
							hasFilters = !f->filter.empty();
						}
					}
				}
			} catch (...) {
				// just ignore any errors
			}
			this->popup->Enable(IDM_EXTRACT_RAW, hasFilters);
			this->popup->Enable(IDM_OVERWRITE_RAW, hasFilters);
			this->PopupMenu(this->popup);
			return;
		}

		/// Event handler for tab just been removed
		void onDocTabClosed(wxAuiNotebookEvent& event)
		{
			// Hide the tool panes if this was the last document
			if (!this->notebook->GetPage(0) && !this->notebook->GetPage(1)) {
				this->updateToolPanes(NULL);
			}
			return;
		}

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
		void openProject(const wxString& path)
			throw ()
		{
			std::cout << "[main] Opening project " << path.ToAscii() << "\n";
			assert(wxFileExists(path));

			try {
				Project *newProj = new Project(path);
				if (this->project) this->closeProject();
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

			// Add all the run commands to the Test menu
			this->clearTestMenu();
			for (std::map<wxString, wxString>::const_iterator i = this->game->dosCommands.begin();
				i != this->game->dosCommands.end(); i++
			) {
				long id = ::wxNewId();
				this->menuTest->Append(id, i->first, _T("Run the game through DOSBox"));
				this->commandMap[id] = i->second;
				this->Connect(id, wxEVT_COMMAND_MENU_SELECTED,
					wxCommandEventHandler(CamotoFrame::onRunGame));
			}

			// Load settings for each tool pane
			for (PaneMap::iterator t = this->editorPanes.begin(); t != this->editorPanes.end(); t++) {
				PaneVector& panes = this->editorPanes[t->first];
				for (PaneVector::iterator i = panes.begin(); i != panes.end(); i++) {
					(*i)->loadSettings(this->project);
				}
			}

			// Load settings for each editor
			for (EditorMap::iterator e = this->editors.begin(); e != this->editors.end(); e++) {
				e->second->loadSettings(this->project);
			}

			return;
		}

		void populateTreeItems(wxTreeItemId& root, tree<wxString>& items)
			throw ()
		{
			for (tree<wxString>::children_t::iterator i = items.children.begin(); i != items.children.end(); i++) {
				if (i->children.size() == 0) {
					// This is a document as it has no children

					// Make sure the ID is valid
					GameObjectMap::iterator io = this->game->objects.find(i->item);
					if (io == this->game->objects.end()) {
						std::cout << "[main] Tried to add non-existent item ID \"" <<
							i->item.mb_str() << "\" to tree list, skipping\n";
						continue;
					}

					wxTreeItemData *d = new TreeItemData(i->item);
					wxTreeItemId newItem = this->treeCtrl->AppendItem(root,
						this->game->objects[i->item]->friendlyName, IMG_FILE, IMG_FILE, d);

					// If this file doesn't have an editor, colour it grey
					EditorMap::iterator ed = this->editors.find(this->game->objects[i->item]->typeMajor);
					if (ed == this->editors.end()) {
						// No editor
						this->treeCtrl->SetItemTextColour(newItem, *wxLIGHT_GREY);
					} else if (!ed->second->isFormatSupported(this->game->objects[i->item]->typeMinor)) {
						// Have editor, but it doesn't support this filetype
						this->treeCtrl->SetItemTextColour(newItem, *wxLIGHT_GREY);
					}
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

			// Save all open documents
			for (int i = this->notebook->GetPageCount() - 1; i >= 0; i--) {
				IDocument *doc = static_cast<IDocument *>(this->notebook->GetPage(i));
				try {
					doc->save();
				} catch (const camoto::stream::error& e) {
					this->notebook->SetSelection(i); // focus the cause of the error
					wxString msg = _T("Unable to save document: ");
					msg.Append(wxString(e.what(), wxConvUTF8));
					wxMessageDialog dlg(this, msg, _T("Save failed"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return false;
				}
			}

			// Flush all open archives
			for (ArchiveMap::iterator itArch = this->archives.begin(); itArch != this->archives.end(); itArch++) {
				itArch->second->flush();
			}

			// Update the current perspective
			if (!this->project->config.Write(_T("camoto/perspective"),
				this->aui.SavePerspective())) return false;

			// Save settings for each tool pane
			for (PaneMap::iterator t = this->editorPanes.begin(); t != this->editorPanes.end(); t++) {
				PaneVector& panes = this->editorPanes[t->first];
				for (PaneVector::iterator i = panes.begin(); i != panes.end(); i++) {
					(*i)->saveSettings(this->project);
				}
			}

			// Save settings for each editor
			for (EditorMap::iterator e = this->editors.begin(); e != this->editors.end(); e++) {
				e->second->saveSettings(this->project);
			}

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
		 * @note This function does not attempt to save the project, or warn if
		 *   unsaved changes will be lost.
		 */
		void closeProject()
			throw ()
		{
			assert(this->project);
			assert(this->game);

			// Close all open docs and return false if needed
			this->notebook->SetSelection(0); // minimise page change events

			this->updateToolPanes(NULL); // hide tool windows

			// Close all open documents
			for (int i = this->notebook->GetPageCount() - 1; i >= 0; i--) {
				this->notebook->DeletePage(i);
			}
			this->treeCtrl->DeleteAllItems();

			// Flush all open archives
			for (ArchiveMap::iterator itArch = this->archives.begin(); itArch != this->archives.end(); itArch++) {
				itArch->second->flush();
			}

			// Unload any cached archives
			this->archives.clear();

			// Remove any run commands from the menu
			this->clearTestMenu();

			// Release the project
			delete this->project;
			this->project = NULL;

			// Release the game structure
			delete this->game;
			this->game = NULL;

			this->setControlStates();
			return;
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

		virtual void setStatusText(const wxString& text)
			throw ()
		{
			this->txtMessage = text;
			this->updateStatusBar();
			return;
		}

		virtual void setHelpText(const wxString& text)
			throw ()
		{
			this->txtHelp = text;
			this->updateStatusBar();
		}

		void updateStatusBar()
		{
			wxString text = this->txtHelp;
			if (!this->txtMessage.empty()) {
				text.append(_T(" ["));
				text.append(this->txtMessage);
				text.append(_T("]"));
			}
			this->status->SetStatusText(text, 0);
			return;
		}

		/// Remove any run commands from the 'Test' menu
		void clearTestMenu()
			throw ()
		{
			for (std::map<long, wxString>::iterator i = this->commandMap.begin();
				i != this->commandMap.end(); i++
			) {
				this->menuTest->Delete(i->first);
			}
			this->commandMap.clear();
			return;
		}

		/// Get an Archive instance for the given ID.
		/**
		 * This function will return the existing instance if the archive has
		 * already been opened, otherwise it will open it and return the new
		 * instance, caching it for future use.
		 *
		 * @param idArchive
		 *   ID of the archive to open.
		 *
		 * @return A shared pointer to the archive instance.
		 */
		ga::ArchivePtr getArchive(const wxString& idArchive)
			throw (EFailure)
		{
			// See if idArchive is open
			ArchiveMap::iterator itArch = this->archives.find(idArchive);
			ga::ArchivePtr arch;
			if (itArch == this->archives.end()) {
				// Not open, so open it, possibly recursing back here if it's inside
				// another archive

				camoto::stream::inout_sptr archStream;
				SuppMap supp;
				this->openObject(idArchive, &archStream, &supp);
				assert(archStream);

				// Now the archive file is open, so create an Archive object around it

				// No need to check if idArchive is valid, as openObject() just did that
				const wxString& typeMinor = this->game->objects[idArchive]->typeMinor;
				if (typeMinor.Cmp(_T(ARCHTYPE_MINOR_FIXED)) == 0) {
					// This is a fixed archive, with its files described in the XML
					std::vector<ga::FixedArchiveFile> items;
					for (GameObjectMap::iterator i = this->game->objects.begin(); i != this->game->objects.end(); i++) {
						if (idArchive.Cmp(i->second->idParent) == 0) {
							// This item is a subfile
							ga::FixedArchiveFile next;
							next.offset = i->second->offset;
							next.size = i->second->size;
							next.name = i->second->filename.ToUTF8();
							std::cout << "name is " << next.name << std::endl;
							// next.filter is unused
							items.push_back(next);
						}
					}
					arch.reset(new ga::FixedArchive(archStream, items));
				} else {
					// Normal archive file
					std::string strType(typeMinor.ToUTF8());
					ga::ArchiveTypePtr pArchType(this->archManager->getArchiveTypeByCode(strType));
					if (!pArchType) {
						throw EFailure(wxString::Format(_T("Cannot open this item.  The "
									"archive \"%s\" is in the unsupported format \"%s\""),
								idArchive.c_str(), typeMinor.c_str()));
					}

					// Collect any supplemental files supplied
					camoto::SuppData suppData;
					suppMapToData(supp, suppData);

					std::string baseFilename(this->game->objects[idArchive]->filename.mb_str());
					camoto::SuppFilenames reqd = pArchType->getRequiredSupps(archStream, baseFilename);
					for (camoto::SuppFilenames::iterator i = reqd.begin(); i != reqd.end(); i++) {
						if (suppData.find(i->first) == suppData.end()) {
							throw EFailure(wxString::Format(_T("Unable to open archive \"%s\" "
										"as the XML description file does not specify the required "
										"supplementary file \"%s\""),
									idArchive.c_str(),
									wxString(i->second.c_str(), wxConvUTF8).c_str()
								));
						}
					}

					try {
						arch = pArchType->open(archStream, suppData);
					} catch (const camoto::stream::error& e) {
						wxString msg = _T("Library exception opening archive \"");
						msg += idArchive;
						msg += _T("\":\n\n");
						msg += wxString(e.what(), wxConvUTF8);
						throw EFailure(msg);
					}
				}

				// Cache for future access
				this->archives[idArchive] = arch;
			} else {
				arch = itArch->second;
			}

			return arch;
		}

		camoto::stream::inout_sptr openFileFromArchive(const wxString& idArchive,
			const wxString& filename, bool useFilters)
			throw (EFailure)
		{
			ga::ArchivePtr arch = this->getArchive(idArchive);

			// Now we have the archive containing our file, so find and open it
			std::string nativeFilename(filename.fn_str());
			ga::Archive::EntryPtr f = ga::findFile(arch, nativeFilename);
			if (!f) {
				throw EFailure(wxString::Format(_T("Cannot open this item.  The file "
					"\"%s\" could not be found inside the archive \"%s\""),
					filename.c_str(), idArchive.c_str()));
			}

			// Open the file
			camoto::stream::inout_sptr file = arch->open(f);
			assert(file);

			// If it has any filters, apply them
			if (useFilters && (!f->filter.empty())) {
				// The file needs to be filtered first
				ga::FilterTypePtr pFilterType(this->archManager->getFilterTypeByCode(f->filter));
				if (!pFilterType) {
					throw EFailure(wxString::Format(_T("This file requires decoding, but "
						"the \"%s\" filter to do this couldn't be found (is your installed "
						"version of libgamearchive too old?)"),
						f->filter.c_str()));
				}
				try {
					file = pFilterType->apply(file,
						// Set the truncation function for the postfiltered (uncompressed) size.
						boost::bind<void>(&setNativeSize, arch, f, _1)
					);
				} catch (const camoto::filter_error& e) {
					throw EFailure(wxString::Format(_T("Error decoding this file: %s"),
						wxString(e.what(), wxConvUTF8).c_str()));
				}
			}

			return file;
		}

	protected:
		wxAuiManager aui;
		wxStatusBar *status;
		wxMenuBar *menubar;
		wxMenu *menuTest;
		wxAuiNotebook *notebook;
		wxTreeCtrl *treeCtrl;
		wxImageList *treeImages;
		wxMenu *popup;     ///< Popup menu when right-clicking in tree view

		wxString defaultPerspective;
		wxString txtMessage; ///< Last hint set for status bar
		wxString txtHelp;    ///< Last keyboard help set for status bar

		bool isStudio;     ///< true for full studio view, false for standalone editor
		Project *project;  ///< Currently open project or NULL
		Game *game;        ///< Game being edited in current project
		AudioPtr audio;    ///< Common audio output

		std::map<long, wxString> commandMap;

		/// Map containing typeMajor -> IEditor instance used to edit that type
		typedef std::map<wxString, IEditor *> EditorMap;
		EditorMap editors; ///< List of editors currently available

		typedef std::vector<IToolPanel *> PaneVector;
		typedef std::map<wxString, PaneVector> PaneMap;
		PaneMap editorPanes;

		ga::ManagerPtr archManager; ///< Manager object for libgamearchive
		typedef std::map<wxString, camoto::gamearchive::ArchivePtr> ArchiveMap;
		ArchiveMap archives; ///< List of currently open archives

		/// Control IDs
		enum {
			IDC_RESET = wxID_HIGHEST + 1,
			IDC_NOTEBOOK,
			IDC_TREE,
			IDM_EXTRACT,
			IDM_EXTRACT_RAW,
			IDM_OVERWRITE,
			IDM_OVERWRITE_RAW,
		};

		DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(CamotoFrame, wxFrame)
	EVT_MENU(wxID_NEW, CamotoFrame::onNewProject)
	EVT_MENU(wxID_OPEN, CamotoFrame::onOpen)
	EVT_MENU(wxID_SAVE, CamotoFrame::onSave)
	EVT_MENU(wxID_CLOSE, CamotoFrame::onCloseProject)
	EVT_MENU(IDC_RESET, CamotoFrame::onViewReset)
	EVT_MENU(wxID_SETUP, CamotoFrame::onSetPrefs)
	EVT_MENU(wxID_ABOUT, CamotoFrame::onHelpAbout)
	EVT_MENU(wxID_EXIT, CamotoFrame::onExit)
	EVT_MENU(IDM_EXTRACT, CamotoFrame::onExtractItem)
	EVT_MENU(IDM_EXTRACT_RAW, CamotoFrame::onExtractUnfilteredItem)
	EVT_MENU(IDM_OVERWRITE, CamotoFrame::onOverwriteItem)
	EVT_MENU(IDM_OVERWRITE_RAW, CamotoFrame::onOverwriteUnfilteredItem)
	EVT_TREE_ITEM_ACTIVATED(IDC_TREE, CamotoFrame::onItemOpened)
	EVT_TREE_ITEM_MENU(IDC_TREE, CamotoFrame::onItemRightClick)
	EVT_CLOSE(CamotoFrame::onClose)

	EVT_AUINOTEBOOK_PAGE_CHANGED(IDC_NOTEBOOK, CamotoFrame::onDocTabChanged)
	EVT_AUINOTEBOOK_PAGE_CLOSE(IDC_NOTEBOOK, CamotoFrame::onDocTabClose)
	EVT_AUINOTEBOOK_PAGE_CLOSED(IDC_NOTEBOOK, CamotoFrame::onDocTabClosed)
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

			// Load the user's preferences
			wxConfigBase *configFile = wxConfigBase::Get(true);
			configFile->Read(_T("camoto/dosbox"), &::config.dosboxPath,
#ifndef __WXMSW__
				_T("/usr/bin/dosbox")
#else
				_T("dosbox.exe")
#endif
			);
			configFile->Read(_T("camoto/pause"), &::config.dosboxExitPause, false);
			configFile->Read(_T("camoto/lastpath"), &::path.lastUsed, wxEmptyString);

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

		virtual int OnExit()
		{
			wxConfigBase *configFile = wxConfigBase::Get(true);
			configFile->Write(_T("camoto/lastpath"), ::path.lastUsed);
			configFile->Flush();
			return 0;
		}

};

const wxCmdLineEntryDesc CamotoApp::cmdLineDesc[] = {
	{wxCMD_LINE_OPTION, NULL, _T("project"), _T("Open the given project")},
	{wxCMD_LINE_OPTION, NULL, _T("music"), _T("Open a song in a standalone editor")},
	{wxCMD_LINE_NONE}
};

IMPLEMENT_APP(CamotoApp)
