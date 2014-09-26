/**
 * @file   studio.cpp
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

#include <GL/glew.h>

#include <wx/wx.h>
#include <wx/cmdline.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/uri.h>

#include <camoto/gamearchive.hpp>
#include <camoto/stream_file.hpp>
#include <camoto/util.hpp>

#include "main.hpp"
#include "project.hpp"
#include "newproj.hpp"
#include "prefsdlg.hpp"
#include "editor.hpp"
#include "exceptions.hpp"
#include "studio.hpp"
#include "gamelist.hpp"
#include "editor-map.hpp"
#include "editor-music.hpp"
#include "editor-tileset.hpp"
#include "editor-image.hpp"
#include "dlg-map-attr.hpp"

using namespace camoto;

/// Attribute list for all OpenGL surfaces
int glAttribList[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 0, 0};

/// Data for each item in the tree view
class TreeItemData: public wxTreeItemData {
	public:
		wxString id;

		TreeItemData(const wxString& id)
			:	id(id)
		{
		}

};

BEGIN_EVENT_TABLE(Studio, wxFrame)
	EVT_MENU(wxID_NEW, Studio::onNewProject)
	EVT_MENU(wxID_OPEN, Studio::onOpen)
	EVT_MENU(wxID_SAVE, Studio::onSave)
	EVT_MENU(wxID_CLOSE, Studio::onCloseProject)
	EVT_MENU(IDC_RESET, Studio::onViewReset)
	EVT_MENU(wxID_SETUP, Studio::onSetPrefs)
	EVT_MENU(IDC_OPENWIKI, Studio::onHelpWiki)
	EVT_MENU(wxID_ABOUT, Studio::onHelpAbout)
	EVT_MENU(wxID_EXIT, Studio::onExit)
	EVT_MENU(IDM_EXTRACT, Studio::onExtractItem)
	EVT_MENU(IDM_EXTRACT_RAW, Studio::onExtractUnfilteredItem)
	EVT_MENU(IDM_OVERWRITE, Studio::onOverwriteItem)
	EVT_MENU(IDM_OVERWRITE_RAW, Studio::onOverwriteUnfilteredItem)
	EVT_MENU(wxID_PROPERTIES, Studio::onItemProperties)
	EVT_TREE_ITEM_ACTIVATED(IDC_TREE, Studio::onItemOpened)
	EVT_TREE_ITEM_MENU(IDC_TREE, Studio::onItemRightClick)
	EVT_CLOSE(Studio::onClose)

	EVT_AUINOTEBOOK_PAGE_CHANGED(IDC_NOTEBOOK, Studio::onDocTabChanged)
	EVT_AUINOTEBOOK_PAGE_CLOSE(IDC_NOTEBOOK, Studio::onDocTabClose)
	EVT_AUINOTEBOOK_PAGE_CLOSED(IDC_NOTEBOOK, Studio::onDocTabClosed)
END_EVENT_TABLE()

Studio::Studio(bool isStudio)
	:	wxFrame(NULL, wxID_ANY, _T("Camoto Studio"), wxDefaultPosition,
			wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN),
		CamotoLibs(this),
		glcx(NULL),
		popup(NULL),
		isStudio(isStudio),
		project(NULL),
		audio(new Audio(this, 48000))
{
	this->aui.SetManagedWindow(this);

	// Load all the images into the shared image list, in the order given by
	// ImageListIndex::Type.
	this->smallImages = new wxImageList(16, 16, true, 5);
	this->smallImages->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16)));
	this->smallImages->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16, 16)));
	this->smallImages->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16)));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("inst-mute.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("inst-opl.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("inst-midi.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("inst-pcm.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("gtk-media-previous-ltr.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("gtk-media-play-ltr.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("gtk-media-pause.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("gtk-media-next-ltr.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("gtk-zoom-in.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("gtk-zoom-100.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("gtk-zoom-out.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("import.png"), wxBITMAP_TYPE_PNG));
	this->smallImages->Add(wxImage(::path.guiIcons + _T("export.png"), wxBITMAP_TYPE_PNG));

	wxMenu *menuFile = new wxMenu();
	if (isStudio) {
		menuFile->Append(wxID_NEW,    _("&New project..."), _("Choose a new game to work with"));
		menuFile->Append(wxID_OPEN,   _("&Open project..."), _("Resume work on a previous mod"));
		menuFile->Append(wxID_SAVE,   _("&Save project"), _("Save options and all open documents"));
		menuFile->Append(wxID_CLOSE,  _("&Close project"), _("Close all open documents"));
	} else {
		menuFile->Append(wxID_OPEN,   _("&Open..."), _("Open a new file"));
		menuFile->Append(wxID_SAVE,   _("&Save"), _("Save changes"));
		menuFile->Append(wxID_SAVEAS, _("Save &As..."), _("Save changes to a separate file"));
	}
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT, _("E&xit"));

	wxMenu *menuView = new wxMenu();
	menuView->Append(IDC_RESET, _("&Reset layout"), _("Reset all windows to default sizes"));
	menuView->Append(wxID_SETUP, _("&Options..."), _("Change settings"));

	this->menuTest = new wxMenu();

	wxMenu *menuHelp = new wxMenu();
	menuHelp->Append(IDC_OPENWIKI, _("Help for this &game..."));
	menuHelp->Append(wxID_ABOUT, _("&About..."));

	this->menubar = new wxMenuBar();
	this->menubar->Append(menuFile, _("&File"));
	this->menubar->Append(menuView, _("&View"));
	this->menubar->Append(menuTest, _("&Test"));
	this->menubar->Append(menuHelp, _("&Help"));
	this->SetMenuBar(this->menubar);

	this->status = this->CreateStatusBar(1, wxST_SIZEGRIP);
	this->status->SetStatusText(_("Ready"));

	this->SetMinSize(wxSize(400, 300));

	this->notebook = new wxAuiNotebook(this, IDC_NOTEBOOK);
	this->aui.AddPane(this->notebook, wxAuiPaneInfo().Name(_T("documents")).
		CenterPane().PaneBorder(false));

	if (isStudio) {
		this->treeCtrl = new wxTreeCtrl(this, IDC_TREE, wxDefaultPosition,
			wxDefaultSize, wxTR_DEFAULT_STYLE | wxNO_BORDER);
		this->treeCtrl->SetImageList(this->smallImages);
		this->aui.AddPane(this->treeCtrl, wxAuiPaneInfo().Name(_T("tree")).Caption(_("Items")).
			Left().Layer(1).Position(1).CloseButton(true).MaximizeButton(true));
	}

	// Save the current view state as that which will be used on a reset
	this->defaultPerspective = this->aui.SavePerspective();

	// Load all the editors
	this->editors[_T("map")] = new MapEditor(this);
	this->editors[_T("music")] = new MusicEditor(this, this->audio);
	this->editors[_T("tileset")] = new TilesetEditor(this);
	this->editors[_T("image")] = new ImageEditor(this);

	this->popup = new wxMenu();
	this->popup->Append(IDM_EXTRACT,       _("&Extract file..."), _("Save this file in its native format"));
	this->popup->Append(IDM_EXTRACT_RAW,   _("&Extract raw..."), _("Save this file in its raw format (no decompression or decryption)"));
	this->popup->Append(IDM_OVERWRITE,     _("&Overwrite file..."), _("Replace this item with the contents of another file"));
	this->popup->Append(IDM_OVERWRITE_RAW, _("&Overwrite raw..."), _("Replace this item with a file already encrypted or compressed in the correct format"));
	this->popup->Append(wxID_PROPERTIES,   _("&Properties..."), _("View this item's properties directly"));

	// Create a base OpenGL context to share among all editors
	this->Show(true);
	wxGLCanvas *canvas = new wxGLCanvas(this, wxID_ANY,
		wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxWANTS_CHARS,
		wxEmptyString, ::glAttribList);
	this->glcx = new wxGLContext(canvas, NULL);
	canvas->SetCurrent(*this->glcx);
	GLenum err = glewInit();
	delete canvas;
	assert(this->glcx);

	if (err != GLEW_OK) {
		this->Show(false);
		wxString msg = wxString::Format(_("Unable to load OpenGL.  Make sure "
			"you have OpenGL drivers available for your video card.\n\n"
			"[glewInit() failed: %s]"),
			wxString((const char *)glewGetErrorString(err), wxConvUTF8).c_str());
		wxMessageDialog dlg(NULL, msg, _("Startup failure"),
			wxOK | wxICON_ERROR);
		dlg.ShowModal();
		throw EFailure(msg);
	}
	std::cout << "Using GLEW " << glewGetString(GLEW_VERSION) << "\n";

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
				Right().CloseButton(true).MaximizeButton(true).Hide();
			this->aui.AddPane(*p, pane);
			v.push_back(*p);
		}
		this->editorPanes[e->first] = v;
		std::cout << "\n";
	}

	this->aui.Update();
	this->setControlStates();

	wxMessageDialog dlg(NULL, _(
		"Hi, Malvineous here.  Thanks for trying out Camoto Studio!  Please "
		"remember that this a very early preview release - it's not even anywhere "
		"near what you might call a beta version.  There are still heaps of things "
		"that are unfinished or broken.  This is very definitely a 'work in "
		"progress.'  If you are brave (foolish?) enough to actually try to make a "
		"mod with this version of Camoto, make sure you back up your project "
		"folder regularly in case your whole mod gets corrupted for some reason.\n\n"
		"There is still a huge amount of work to do before it becomes really "
		"user-friendly, so please bear with me and try not to get too frustrated "
		"that your favourite game/feature doesn't work yet.  If you run into "
		"problems (which aren't on the to-do list) by all means stop by the forum "
		"to discuss:\n\nhttp://www.classicdosgames.com/forum/viewforum.php?f=25\n\n"
		"With your expectations suitably lowered, enjoy! :-)"),
		_("Welcome"),
		wxOK | wxICON_WARNING);
	dlg.ShowModal();
}

Studio::~Studio()
{
	if (this->project) delete this->project;
	if (this->game) delete this->game;
	// this->treeImages will be destroyed by the tree control

	for (EditorMap::iterator e = this->editors.begin(); e != this->editors.end(); e++) {
		delete e->second;
	}

	if (this->popup) delete this->popup;
	if (this->glcx) delete this->glcx;
}

wxString Studio::getGameFilesPath()
{
	assert(this->project);
	return this->project->getDataPath();
}

void Studio::setControlStates()
{
	this->menubar->Enable(wxID_SAVE, this->project);
	this->menubar->Enable(wxID_CLOSE, this->project);
	return;
}

GameObjectPtr Studio::getSelectedGameObject()
{
	return this->getSelectedGameObject(this->treeCtrl->GetSelection());
}

GameObjectPtr Studio::getSelectedGameObject(wxTreeEvent& ev)
{
	return this->getSelectedGameObject(ev.GetItem());
}

GameObjectPtr Studio::getSelectedGameObject(wxTreeItemId id)
{
	// Find the item that is currently selected in the tree view
	TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(id);
	if (!data) return GameObjectPtr();

	// Make sure the ID is valid
	GameObjectMap::iterator io = this->game->objects.find(data->id);
	if (io == this->game->objects.end()) {
		throw EFailure(wxString::Format(_("Cannot open this item.  It refers "
			"to an entry in the game description XML file with an ID of \"%s\", "
			"but there is no item with this ID."),
			data->id.c_str()));
	}
	return io->second;
}

void Studio::onNewProject(wxCommandEvent& ev)
{
	if (this->project) {
		if (!this->confirmSave(_("Close current project"))) return;
	}
	NewProjectDialog newProj(this);
	if (newProj.ShowModal() == wxID_OK) {
		// The project has been created successfully, so open it.
		this->openProject(newProj.getProjectPath());
	}
	return;
}

void Studio::onOpen(wxCommandEvent& ev)
{
	if (this->project) {
		if (!this->confirmSave(_("Close current project"))) return;
	}
	wxDirDialog dlg(this, _("Open project"), wxEmptyString,
		wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	dlg.SetMessage(_("Select the folder where the project is located."));
	if (dlg.ShowModal() == wxID_OK) {
		wxFileName fn;
		fn.AssignDir(dlg.GetPath());
		fn.SetFullName(_T("project.camoto"));
		if (!::wxFileExists(fn.GetFullPath())) {
			wxString msg = wxString::Format(_("This folder does not contain a "
				"Camoto Studio project.\n\n[File not found: %s]"),
				fn.GetFullPath().c_str());
			wxMessageDialog dlg(this, msg, _("Open project"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
		} else {
			this->openProject(fn.GetFullPath());
		}
	}
	return;
}

void Studio::onSave(wxCommandEvent& ev)
{
	if (!this->project) return; // just in case
	this->saveProjectNoisy(); // will display error if needed
	return;
}

void Studio::onCloseProject(wxCommandEvent& ev)
{
	if (!this->project) return;
	if (!this->confirmSave(_("Close project"))) return;
	this->closeProject();
	return;
}

void Studio::onViewReset(wxCommandEvent& ev)
{
	this->aui.LoadPerspective(this->defaultPerspective);
	return;
}

void Studio::onSetPrefs(wxCommandEvent& ev)
{
	PrefsDialog prefs(this, this->audio);
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

void Studio::onRunGame(wxCommandEvent& ev)
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
		wxMessageDialog dlg(this, _("Could not create 'camoto.bat' in the "
			"project data directory!  Make sure you have sufficient disk space "
			"and write access to the folder."),
			_("Run game"), wxOK | wxICON_ERROR);
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
		wxMessageDialog dlg(this, _("Unable to launch DOSBox!  Make sure the "
			"path is correct in the Camoto preferences window."),
			_("Run game"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
	delete argv;
	return;
}

void Studio::onHelpWiki(wxCommandEvent& ev)
{
	if (!this->game) {
		wxMessageDialog dlg(this, _(
			"This option will open a web page with help specific to game being "
			"edited.  You will need to open a game for editing before you can use "
			"this menu item."
		), _("Modding Help"), wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		return;
	}

	wxURI u(_T("http://www.shikadi.net/camoto/help.php?game=" + this->game->title));
	wxString url = u.BuildURI();
	wxMessageDialog dlg(this, _(
		"Tips and helpful guides are available for many games on the ModdingWiki "
		"web site.\n"
		"\n"
		"Would you like to visit the page for this game now?\n"
		"\n") +
		_T("<") + url + _T(">")
		,
		_("Modding Help"), wxYES_NO | wxICON_QUESTION);
	if (dlg.ShowModal() == wxID_YES) {
		if (!::wxLaunchDefaultBrowser(url)) {
			wxMessageDialog dlg(this, _(
				"Unable to open the default browser!"
			), _("Modding Help"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	}
	return;
}

void Studio::onHelpAbout(wxCommandEvent& ev)
{
	wxMessageDialog dlg(this, _(
		CAMOTO_HEADER
		"\n"
		"Camoto Studio is an integrated editing environment for \n"
		"modifying games from the early 1990s DOS era.  This is \n"
		"the central component in the Camoto suite of utilities.\n"
		"\n"
		"Special thanks to NY00123 for helping with the Windows port."
		),
		_("About Camoto Studio"), wxOK | wxICON_INFORMATION);
	dlg.ShowModal();
	return;
}

void Studio::onExit(wxCommandEvent& ev)
{
	this->Close();
	return;
}

void Studio::onExtractItem(wxCommandEvent& ev)
{
	TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(this->treeCtrl->GetSelection());
	if (!data) return;
	this->extractItem(data->id, true); // true == use filters
	return;
}

void Studio::onExtractUnfilteredItem(wxCommandEvent& ev)
{
	TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(this->treeCtrl->GetSelection());
	if (!data) return;
	this->extractItem(data->id, false); // false == don't use filters
	return;
}

void Studio::onOverwriteItem(wxCommandEvent& ev)
{
	TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(this->treeCtrl->GetSelection());
	if (!data) return;
	this->overwriteItem(data->id, true); // true == use filters
	return;
}

void Studio::onOverwriteUnfilteredItem(wxCommandEvent& ev)
{
	TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(this->treeCtrl->GetSelection());
	if (!data) return;
	this->overwriteItem(data->id, false); // false == don't use filters
	return;
}

void Studio::onItemProperties(wxCommandEvent& ev)
{
	GameObjectPtr o;
	try {
		o = this->getSelectedGameObject();
	} catch (const EFailure& e) {
		wxMessageDialog dlg(this, e.getMessage(), _("Error retrieving properties"),
			wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	if (o && o->typeMajor.IsSameAs(_T("map"))) {
		gamemaps::MapPtr map;
		fn_write fnWriteMap;
		try {
			map = this->openMap(o, &fnWriteMap);
			if (map) {
				DlgMapAttr dlg(this, map);
				if (dlg.ShowModal() == wxID_OK) {
					fnWriteMap();
				}
			}
		} catch (const camoto::stream::error& e) {
			throw EFailure(wxString::Format(_("Library exception: %s"),
					wxString(e.what(), wxConvUTF8).c_str()));
		}
	} else {
		wxMessageDialog dlg(this, _("This item has no properties."),
			_("Properties"), wxOK | wxICON_WARNING);
		dlg.ShowModal();
		return;
	}
	return;
}

void Studio::extractItem(const wxString& id, bool useFilters)
{
	if (!this->project) return; // just in case

	// Make sure the ID is valid
	GameObjectMap::iterator io = this->game->objects.find(id);
	if (io == this->game->objects.end()) {
		throw EFailure(wxString::Format(_("Cannot open this item.  It refers "
			"to an entry in the game description XML file with an ID of \"%s\", "
			"but there is no item with this ID."),
			id.c_str()));
	}
	GameObjectPtr& o = io->second;

	wxString path = wxFileSelector(_("Save file"),
		::path.lastUsed, o->filename, wxEmptyString,
#ifdef __WXMSW__
		_("All files (*.*)|*.*"),
#else
		_("All files (*)|*"),
#endif
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (!path.empty()) {
		::path.lastUsed = wxFileName::FileName(path).GetPath();
		try {
			stream::inout_sptr extract = this->openFile(o, useFilters);
			assert(extract); // should have thrown an exception on error

			stream::output_file_sptr out(new stream::output_file());
			out->create(path.mb_str());

			stream::copy(out, extract);
			out->flush();

		} catch (const stream::open_error& e) {
			wxMessageDialog dlg(this, wxString::Format(_(
				"Unable to create file!\n\n[%s]"
				), wxString(e.what(), wxConvUTF8).c_str()),
				_("Extract item"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;

		} catch (const std::exception& e) {
			wxMessageDialog dlg(this, wxString::Format(_(
				"Unexpected error while extracting the item!\n\n[%s]"
				), wxString(e.what(), wxConvUTF8).c_str()),
				_("Extract item"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	}
	return;
}

void Studio::overwriteItem(const wxString& id, bool useFilters)
{
	if (!this->project) return; // just in case

	// Make sure the ID is valid
	GameObjectMap::iterator io = this->game->objects.find(id);
	if (io == this->game->objects.end()) {
		throw EFailure(wxString::Format(_("Cannot open this item.  It refers "
			"to an entry in the game description XML file with an ID of \"%s\", "
			"but there is no item with this ID."),
			id.c_str()));
	}
	GameObjectPtr& o = io->second;

	wxString path = wxFileSelector(_("Open file"),
		::path.lastUsed, o->filename, wxEmptyString,
#ifdef __WXMSW__
		_("All files (*.*)|*.*"),
#else
		_("All files (*)|*"),
#endif
		wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
	if (!path.empty()) {
		::path.lastUsed = wxFileName::FileName(path).GetPath();
		try {
			stream::inout_sptr dest = this->openFile(o, useFilters);
			assert(dest); // should have thrown an exception on error

			stream::input_file_sptr in(new stream::input_file());
			in->open(path.mb_str());

			stream::copy(dest, in);
			dest->flush();

		} catch (const stream::open_error& e) {
			wxMessageDialog dlg(this, wxString::Format(_(
				"Unable to open source file!\n\n[%s]"
				), wxString(e.what(), wxConvUTF8).c_str()),
				_("Overwrite item"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;

		} catch (const std::exception& e) {
			wxMessageDialog dlg(this, wxString::Format(_(
				"Unexpected error while replacing the item!\n\n[%s]"
				), wxString(e.what(), wxConvUTF8).c_str()),
				_("Overwrite item"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	}
	return;
}

void Studio::onClose(wxCloseEvent& ev)
{
	if (this->project) {
		if (!this->confirmSave(_("Exit"))) return;
		this->closeProject();
	}
	this->aui.UnInit();
	this->Destroy();
	return;
}

void Studio::onItemOpened(wxTreeEvent& ev)
{
	GameObjectPtr o;
	try {
		o = this->getSelectedGameObject(ev);
	} catch (const EFailure& e) {
		wxMessageDialog dlg(this, e.getMessage(), _("Open failure"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	if (!o) {
		// not an item (probably a folder double-clicked)
		ev.Skip();
		return;
	}

	if (o->typeMajor.IsSameAs(_("unknown"))) {
		wxMessageDialog dlg(this, _("Sorry, this item is in an unknown "
			"format so it cannot be edited yet.  If you can help work out the file "
			"format, please document it on the ModdingWiki "
			"<http://www.shikadi.net/moddingwiki/> so we can add support for it to "
			"Camoto."),
			_("Unable to open item"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	// Find an editor for the item
	EditorMap::iterator itEditor = this->editors.find(o->typeMajor);
	if (itEditor == this->editors.end()) {
		wxString msg = wxString::Format(_("Sorry, there is no editor "
			"available for \"%s\" items."), o->typeMajor.c_str());
		wxMessageDialog dlg(this, msg, _("Open item"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	//camoto::stream::inout_sptr stream;
	//SuppMap supp;
	try {
		//this->openObject(data->id, &stream, &supp);

		//IDocument *doc = itEditor->second->openObject(o->typeMinor, stream,
		//	o->filename, supp, this->game);
		IDocument *doc = itEditor->second->openObject(o);
		if (doc) {
			this->notebook->AddPage(doc, this->game->objects[o->id]->friendlyName,
				true, wxNullBitmap);
		}
	} catch (const EFailure& e) {
		wxMessageDialog dlg(this, e.getMessage(), _("Open failure"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	return;
}

void Studio::onDocTabChanged(wxAuiNotebookEvent& event)
{
	IDocument *doc;
	if (this->project) {
		doc = (IDocument *)this->notebook->GetPage(event.GetSelection());
	} else {
		doc = NULL;
	}
	this->updateToolPanes(doc);
	return;
}

void Studio::onDocTabClose(wxAuiNotebookEvent& event)
{
	IDocument *doc = (IDocument *)this->notebook->GetPage(event.GetSelection());
	if (doc->isModified) {
		wxMessageDialog dlg(this, _("Save changes?"),
			_("Close document"), wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_QUESTION);
		int r = dlg.ShowModal();
		if (r == wxID_YES) {
			try {
				doc->save();
			} catch (const camoto::stream::error& e) {
				wxString msg = _("Unable to save document: ");
				msg.Append(wxString(e.what(), wxConvUTF8));
				wxMessageDialog dlg(this, msg, _("Save failed"), wxOK | wxICON_ERROR);
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

void Studio::onItemRightClick(wxTreeEvent& ev)
{
	bool hasFilters = false, hasProperties = false;
	try {
		GameObjectPtr o = this->getSelectedGameObject(ev);
		if (o) {
			if (!o->idParent.empty()) {
				// This file is contained within an archive
				gamearchive::ArchivePtr arch = this->getArchive(o->idParent);
				if (arch) {
					std::string nativeFilename(o->filename.mb_str());
					gamearchive::Archive::EntryPtr f = gamearchive::findFile(arch, nativeFilename);
					if (f) {
						// Found file
						hasFilters = !f->filter.empty();
					}
				}
			}
			if (o->typeMajor.IsSameAs(_T("map"))) {
				hasProperties = true;
			}
		}
	} catch (...) {
		// just ignore any errors
	}
	this->popup->Enable(IDM_EXTRACT_RAW, hasFilters);
	this->popup->Enable(IDM_OVERWRITE_RAW, hasFilters);
	this->popup->Enable(wxID_PROPERTIES, hasProperties);
	this->PopupMenu(this->popup);
	return;
}

void Studio::onDocTabClosed(wxAuiNotebookEvent& event)
{
	// Hide the tool panes if this was the last document
	if (this->notebook->GetPageCount() == 0) {
		this->updateToolPanes(NULL);
	}
	return;
}

void Studio::openProject(const wxString& path)
{
	std::cout << "[main] Opening project " << path.ToAscii() << "\n";
	assert(wxFileExists(path));

	try {
		Project *newProj = new Project(path);
		if (this->project) this->closeProject();
		this->project = newProj;
	} catch (EFailure& e) {
		wxString msg = _("Unable to open project: ");
		msg.Append(e.getMessage());
		wxMessageDialog dlg(this, msg, _("Open project"), wxOK | wxICON_ERROR);
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
			_("Unable to open, project file is corrupted (missing game ID)"),
			_("Open project"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	if (this->game) delete this->game;
	this->game = ::loadGameStructure(idGame);
	if (!this->game) {
		delete this->project;
		this->project = NULL;

		wxMessageDialog dlg(this,
			_("Unable to parse XML description file for this game!"),
			_("Open project"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	// Add the game icon to the treeview's image list
	wxFileName fn(::path.gameIcons);
	fn.SetName(this->game->id);
	fn.SetExt(_T("png"));
	if (::wxFileExists(fn.GetFullPath())) {
		wxImage *image = new wxImage(fn.GetFullPath(), wxBITMAP_TYPE_PNG);
		wxBitmap bitmap(*image);
		delete image;
		this->smallImages->Replace(ImageListIndex::Game, bitmap);
	} else {
		// If the image file couldn't be opened, use a generic folder image
		this->smallImages->Replace(ImageListIndex::Game, wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16)));
	}

	// Populate the treeview
	this->treeCtrl->DeleteAllItems();
	wxTreeItemId root = this->treeCtrl->AddRoot(this->game->title, ImageListIndex::Game);
	this->populateTreeItems(root, this->game->treeItems);
	this->treeCtrl->ExpandAll();

	// Add all the run commands to the Test menu
	this->clearTestMenu();
	for (std::map<wxString, wxString>::const_iterator i = this->game->dosCommands.begin();
		i != this->game->dosCommands.end(); i++
	) {
		long id = ::wxNewId();
		this->menuTest->Append(id, i->first, _("Run the game through DOSBox"));
		this->commandMap[id] = i->second;
		this->Connect(id, wxEVT_COMMAND_MENU_SELECTED,
			wxCommandEventHandler(Studio::onRunGame));
	}

	// Load settings for each tool pane
	for (PaneMap::iterator t = this->editorPanes.begin(); t != this->editorPanes.end(); t++) {
		PaneVector& panes = this->editorPanes[t->first];
		for (PaneVector::iterator i = panes.begin(); i != panes.end(); i++) {
			(*i)->loadSettings(this->project);
			wxAuiPaneInfo& pane = this->aui.GetPane(*i);
			pane.Hide(); // don't want the pane visible until an editor is loaded
		}
	}

	// Load settings for each editor
	for (EditorMap::iterator e = this->editors.begin(); e != this->editors.end(); e++) {
		e->second->loadSettings(this->project);
	}

	this->aui.Update();
	return;
}

void Studio::populateTreeItems(wxTreeItemId& root, tree<wxString>& items)
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
				this->game->objects[i->item]->friendlyName, ImageListIndex::File,
				ImageListIndex::File, d);

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
				i->item, ImageListIndex::Folder, ImageListIndex::Folder, NULL);
			this->populateTreeItems(folder, *i);
		}
	}
	return;
}

bool Studio::saveProjectNoisy()
{
	if (!this->saveProjectQuiet()) {
		wxMessageDialog dlg(this, _("Unable to save the project!  Make sure "
			"there is enough disk space and the project folder hasn't been moved "
			"or deleted."),
			_("Save project"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return false;
	}
	return true;
}

bool Studio::saveProjectQuiet()
{
	assert(this->project);

	// Save all open documents
	for (int i = this->notebook->GetPageCount() - 1; i >= 0; i--) {
		IDocument *doc = static_cast<IDocument *>(this->notebook->GetPage(i));
		try {
			doc->save();
		} catch (const camoto::stream::error& e) {
			this->notebook->SetSelection(i); // focus the cause of the error
			wxString msg = _("Unable to save document: ");
			msg.Append(wxString(e.what(), wxConvUTF8));
			wxMessageDialog dlg(this, msg, _("Save failed"), wxOK | wxICON_ERROR);
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
	for (PaneMap::iterator
		t = this->editorPanes.begin(); t != this->editorPanes.end(); t++
	) {
		PaneVector& panes = this->editorPanes[t->first];
		for (PaneVector::iterator i = panes.begin(); i != panes.end(); i++) {
			(*i)->saveSettings(this->project);
		}
	}

	// Save settings for each editor
	for (EditorMap::iterator
		e = this->editors.begin(); e != this->editors.end(); e++
	) {
		e->second->saveSettings(this->project);
	}

	return this->project->save();
}

bool Studio::confirmSave(const wxString& title)
{
	if (!this->project) return true;

	wxMessageDialog dlg(this, _("Save project?"),
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

void Studio::closeProject()
{
	assert(this->project);
	assert(this->game);

	// Close all open docs and return false if needed
	this->notebook->SetSelection(0); // minimise page change events

	this->updateToolPanes(NULL); // hide tool windows

	// Mark the project as closed, but let the data hang around until last
	Project *projDeleteLater = this->project;
	this->project = NULL;

	// Close all open documents
	for (int i = this->notebook->GetPageCount() - 1; i >= 0; i--) {
		this->notebook->DeletePage(i);
	}
	this->treeCtrl->DeleteAllItems();

	// Flush all open archives
	for (ArchiveMap::iterator
		itArch = this->archives.begin(); itArch != this->archives.end(); itArch++
	) {
		itArch->second->flush();
	}

	// Unload any cached archives
	this->archives.clear();

	// Remove any run commands from the menu
	this->clearTestMenu();

	// Release the project
	delete projDeleteLater;

	// Release the game structure
	delete this->game;
	this->game = NULL;

	this->setControlStates();
	return;
}

void Studio::updateToolPanes(IDocument *doc)
{
	// If there's an open document, show its panes
	wxString typeMajor;
	if (doc) {
		typeMajor = doc->getTypeMajor();
	} // else typeMajor will be empty

	for (PaneMap::iterator
		t = this->editorPanes.begin(); t != this->editorPanes.end(); t++
	) {
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

void Studio::setStatusText(const wxString& text)
{
	this->txtMessage = text;
	this->updateStatusBar();
	return;
}

void Studio::setHelpText(const wxString& text)
{
	this->txtHelp = text;
	this->updateStatusBar();
}

wxGLContext *Studio::getGLContext()
{
	return this->glcx;
}

int *Studio::getGLAttributes()
{
	return ::glAttribList;
}

void Studio::updateStatusBar()
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

void Studio::clearTestMenu()
{
	for (std::map<long, wxString>::iterator
		i = this->commandMap.begin(); i != this->commandMap.end(); i++
	) {
		this->menuTest->Delete(i->first);
	}
	this->commandMap.clear();
	return;
}
