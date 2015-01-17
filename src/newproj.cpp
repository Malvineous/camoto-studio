/**
 * @file   newproj.cpp
 * @brief  Dialog box for creating a new project.
 *
 * Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>
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

#include <wx/filename.h>
#include <wx/statline.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#ifndef __WXMSW__
#include <wx/cshelp.h>
#endif

#include "main.hpp"
#include "gamelist.hpp"
#include "project.hpp"
#include "newproj.hpp"

/// Control IDs
enum {
	IDC_TREE = wxID_HIGHEST + 1,
	IDC_DEST,
	IDC_BROWSE_DEST,
	IDC_SRC,
	IDC_BROWSE_SRC,
	IDC_GAME,
	IDC_AUTHOR,
	IDC_REVERSER,
};

BEGIN_EVENT_TABLE(NewProjectDialog, wxDialog)
	EVT_BUTTON(wxID_OK, NewProjectDialog::onOK)
	EVT_BUTTON(IDC_BROWSE_DEST, NewProjectDialog::onBrowseDest)
	EVT_BUTTON(IDC_BROWSE_SRC, NewProjectDialog::onBrowseSrc)
	EVT_TREE_SEL_CHANGED(IDC_TREE, NewProjectDialog::onTreeSelChanged)
END_EVENT_TABLE()

/// Data for each item in the tree view
class TreeItemData: public wxTreeItemData {
	public:
		wxString id;

		TreeItemData(const wxString& id)
			:	id(id)
		{
		}

};

NewProjectDialog::NewProjectDialog(wxWindow *parent)
	:	wxDialog(parent, wxID_ANY, _("New project"), wxDefaultPosition,
			wxDefaultSize, wxDIALOG_EX_CONTEXTHELP | wxRESIZE_BORDER)
{
	wxImageList *imgList = new wxImageList(16, 16, true, 2);
	imgList->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16)));
	imgList->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16, 16)));
	this->treeCtrl = new wxTreeCtrl(this, IDC_TREE, wxDefaultPosition,
		wxDefaultSize, wxGROW | wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);
	this->treeCtrl->AssignImageList(imgList);

	wxTreeItemId root = this->treeCtrl->AddRoot(_("Games"), 0);
	this->games = ::getAllGames();
	for (GameInfoMap::iterator i = games.begin(); i != games.end(); i++) {
		wxFileName fn(::path.gameIcons);
		fn.SetName(i->first);
		fn.SetExt(_T("png"));
		int imgIndex = 1;
		if (::wxFileExists(fn.GetFullPath())) {
			wxImage *image = new wxImage(fn.GetFullPath(), wxBITMAP_TYPE_PNG);
			wxBitmap bitmap(*image);
			imgIndex = imgList->Add(bitmap);
		}
		this->treeCtrl->AppendItem(root, i->second.title, imgIndex, imgIndex,
			new TreeItemData(i->first));
	}
	this->treeCtrl->ExpandAll();

	// Have to create the static box *before* the controls that go inside it
	wxStaticBoxSizer *infoBox = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, _("Game information")), wxVERTICAL);
	this->screenshot = new wxStaticBitmap(this, wxID_ANY, wxBitmap(),
		wxDefaultPosition, wxSize(320, 200));
	infoBox->Add(this->screenshot, 0, wxALIGN_CENTER | wxALL, 10);

	wxFlexGridSizer *details = new wxFlexGridSizer(4, 2, 3, 2);
	details->AddGrowableCol(1, 1);
	details->AddGrowableRow(2, 1);
	this->txtGame = new wxStaticText(this, IDC_GAME, wxEmptyString);
	this->txtDeveloper = new wxStaticText(this, IDC_AUTHOR, wxEmptyString);
	this->txtReverser = new wxTextCtrl(this, IDC_REVERSER, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE);
	this->txtReverser->SetHelpText(_("These are the people who deciphered the "
		"file formats this game uses, making modification possible!"));
	details->Add(new wxStaticText(this, wxID_ANY, _("Game:")), 0, wxALIGN_RIGHT | wxALL, 2);
	details->Add(this->txtGame, 1, wxEXPAND | wxALIGN_LEFT | wxALL, 2);
	details->Add(new wxStaticText(this, wxID_ANY, _("Developer:")), 0, wxALIGN_RIGHT | wxALL, 2);
	details->Add(this->txtDeveloper, 1, wxEXPAND | wxALIGN_LEFT | wxALL, 2);
	details->Add(new wxStaticText(this, wxID_ANY, _("Reversed by:")), 0, wxALIGN_RIGHT | wxALL, 2);
	details->Add(this->txtReverser, 1, wxEXPAND | wxALIGN_LEFT | wxALL, 2);
	infoBox->Add(details, 1, wxEXPAND);

	wxSizer *szOptions = new wxBoxSizer(wxVERTICAL);
	szOptions->Add(infoBox, 1, wxEXPAND | wxALIGN_CENTER | wxALL, 8);

	// First text field
	wxString helpText = _("A folder in which to store a copy of the game, along "
		"with your changes to it.  The folder should be empty.");
	wxStaticText *label = new wxStaticText(this, wxID_ANY, _("Project location:"));
	label->SetHelpText(helpText);
	szOptions->Add(label, 0, wxEXPAND | wxALIGN_LEFT | wxLEFT, 4);

	wxBoxSizer *textbox = new wxBoxSizer(wxVERTICAL);
	textbox->AddStretchSpacer(1);
	this->txtDest = new wxTextCtrl(this, IDC_DEST);
	this->txtDest->SetHelpText(helpText);
	textbox->Add(this->txtDest, 0, wxEXPAND | wxALIGN_CENTRE);
	textbox->AddStretchSpacer(1);

	wxBoxSizer *field = new wxBoxSizer(wxHORIZONTAL);
	field->Add(textbox, 1, wxEXPAND);
	wxButton *button = new wxButton(this, IDC_BROWSE_DEST, _("Browse..."));
	button->SetHelpText(helpText);
	field->Add(button, 0, wxALIGN_CENTRE);

	szOptions->Add(field, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 4);

	// Second text field
	helpText = _("Location of the game and its data files.  The files "
		"in here will not be altered in any way, they will be copied to the "
		"project folder.");
	label = new wxStaticText(this, wxID_ANY, _("Copy game files from:"));
	label->SetHelpText(helpText);
	szOptions->Add(label, 0, wxEXPAND | wxALIGN_LEFT | wxLEFT, 4);

	textbox = new wxBoxSizer(wxVERTICAL);
	textbox->AddStretchSpacer(1);
	this->txtSrc = new wxTextCtrl(this, IDC_SRC);
	this->txtSrc->SetHelpText(helpText);
	textbox->Add(this->txtSrc, 0, wxEXPAND | wxALIGN_CENTRE);
	textbox->AddStretchSpacer(1);

	field = new wxBoxSizer(wxHORIZONTAL);
	field->Add(textbox, 1, wxEXPAND);
	button = new wxButton(this, IDC_BROWSE_SRC, _("Browse..."));
	button->SetHelpText(helpText);
	field->Add(button, 0, wxALIGN_CENTRE);

	szOptions->Add(field, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 4);

	// Add dialog buttons (ok/cancel/help)
	wxStdDialogButtonSizer *szButtons = new wxStdDialogButtonSizer();
#ifndef __WXMSW__
	szButtons->AddButton(new wxContextHelpButton(this));
#endif

	wxButton *b = new wxButton(this, wxID_OK);
	b->SetDefault();
	szButtons->AddButton(b);

	b = new wxButton(this, wxID_CANCEL);
	szButtons->AddButton(b);
	szButtons->Realize();

	wxSizer *szContent = new wxBoxSizer(wxHORIZONTAL);
	szContent->Add(this->treeCtrl, 1, wxEXPAND);
	szContent->Add(szOptions, 2, wxEXPAND);

	wxSizer *szMain = new wxBoxSizer(wxVERTICAL);
	szMain->Add(szContent, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 2);
	szMain->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxSize(20, -1), wxLI_HORIZONTAL), 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP, 5);
	szMain->Add(szButtons, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 2);
	this->SetSizer(szMain);
	szMain->Fit(this);

	wxFileName defaultProjDir;
	wxStandardPathsBase& std = wxStandardPaths::Get();
	defaultProjDir.AssignDir(std.GetDocumentsDir());
	defaultProjDir.AppendDir(_("my_project"));
	this->txtDest->SetValue(defaultProjDir.GetFullPath());
}

NewProjectDialog::~NewProjectDialog()
{
}

void NewProjectDialog::onOK(wxCommandEvent& ev)
{
	if (!this->idGame) {
		wxMessageDialog dlg(this, _("You must select the game you would like to "
			"edit!"), _("Create project"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	wxString gameSource = this->txtSrc->GetValue();
	if (!::wxDirExists(gameSource)) {
		wxMessageDialog dlg(this, _("The game folder does not exist!  It "
			"must contain the game files so a copy can be made."),
			_("Create project"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	wxString projFolder = this->txtDest->GetValue();
	if (!::wxDirExists(projFolder)) {
		// Create the main project folder.
		if (!wxMkdir(projFolder, 0755)) {
#ifdef NO_WXLOG_POPUPS
			wxMessageDialog dlg(this, _("Could not create project directory!"),
				_("Create project"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
#endif
			return;
		}
	} else {
		wxDir dir(projFolder);
		if (dir.HasFiles() || dir.HasSubDirs()) {
			wxMessageDialog dlg(this, _("The project folder already has files in it!  "
					"Please choose an empty folder."), _("Create project"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	}

	// Set the final project name
	wxFileName projFile;
	projFile.AssignDir(projFolder);
	projFile.SetFullName(_T("project.camoto"));
	this->projectPath = projFile.GetFullPath();

	// Create the project file
	Project *newProj = Project::create(projFolder, gameSource);
	newProj->config.Write(_T("camoto/game"), this->idGame);
	newProj->save();
	delete newProj;

	this->EndModal(wxID_OK);
	return;
}

void NewProjectDialog::onBrowseDest(wxCommandEvent& ev)
{
	wxString path = wxDirSelector(_("Project folder selection"),
		wxEmptyString, 0, wxDefaultPosition, this);
	if (!path.empty()) this->txtDest->SetValue(path);
	return;
}

void NewProjectDialog::onBrowseSrc(wxCommandEvent& ev)
{
	wxString path = wxDirSelector(_("Game location"),
		wxEmptyString, 0, wxDefaultPosition, this);
	if (!path.empty()) this->txtSrc->SetValue(path);
	return;
}

void NewProjectDialog::onTreeSelChanged(wxTreeEvent& ev)
{
	TreeItemData *data = (TreeItemData *)this->treeCtrl->GetItemData(ev.GetItem());
	if (data) {
		wxFileName fn(::path.gameScreenshots);
		fn.SetName(data->id);
		fn.SetExt(_T("png"));
		if (::wxFileExists(fn.GetFullPath())) {
			wxImage *image = new wxImage(fn.GetFullPath(), wxBITMAP_TYPE_PNG);
			wxBitmap bitmap(*image);
			this->screenshot->SetBitmap(bitmap);
		}
		this->idGame = data->id;
		this->txtGame->SetLabel(this->games[data->id].title);
		this->txtDeveloper->SetLabel(this->games[data->id].developer);
		this->txtReverser->SetValue(this->games[data->id].reverser);
	}

	return;
}

const wxString& NewProjectDialog::getProjectPath() const
{
	return this->projectPath;
}
