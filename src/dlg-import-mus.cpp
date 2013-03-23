/**
 * @file   dlg-import-mus.cpp
 * @brief  Dialog box for the preferences window.
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

#include <iostream>
#include <wx/filename.h>
#include <wx/notebook.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/filedlg.h>
#include <wx/button.h>
#include <wx/tglbtn.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#ifndef __WXMSW__
#include <wx/cshelp.h>
#endif
#include <camoto/stream_file.hpp>
#include <camoto/gamemusic/musictype.hpp>
#include "main.hpp"
#include "dlg-import-mus.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

/// Control IDs
enum {
	IDC_PATH = wxID_HIGHEST + 1,
	IDC_BROWSE,
	IDC_FILETYPE,
	IDC_PITCHBENDS,
};

BEGIN_EVENT_TABLE(DlgImportMusic, wxDialog)
	EVT_BUTTON(wxID_OK, DlgImportMusic::onOK)
	EVT_BUTTON(wxID_CANCEL, DlgImportMusic::onCancel)
	EVT_BUTTON(IDC_BROWSE, DlgImportMusic::onBrowse)
END_EVENT_TABLE()

DlgImportMusic::DlgImportMusic(Studio *parent, ManagerPtr pManager)
	:	wxDialog(parent, wxID_ANY, _T("Preferences"), wxDefaultPosition,
			wxDefaultSize, wxDIALOG_EX_CONTEXTHELP | wxRESIZE_BORDER),
		pManager(pManager)
{
	wxBoxSizer *szMain = new wxBoxSizer(wxVERTICAL);

	// Have to create the static box *before* the controls that go inside it
	wxStaticBoxSizer *szFilename = new wxStaticBoxSizer(wxVERTICAL, this, _T("Import"));

	// First text field
	wxString helpText = _T("The file to open.");
	wxStaticText *label = new wxStaticText(this, wxID_ANY, _T("Open:"));
	label->SetHelpText(helpText);
	szFilename->Add(label, 0, wxEXPAND | wxALIGN_LEFT | wxLEFT, 4);

	wxBoxSizer *textbox = new wxBoxSizer(wxVERTICAL);
	textbox->AddStretchSpacer(1);
	this->txtPath = new wxTextCtrl(this, IDC_PATH);
	this->txtPath->SetHelpText(helpText);
	textbox->Add(this->txtPath, 0, wxEXPAND | wxALIGN_CENTRE);
	textbox->AddStretchSpacer(1);

	wxBoxSizer *field = new wxBoxSizer(wxHORIZONTAL);
	field->Add(textbox, 1, wxEXPAND);
	wxButton *button = new wxButton(this, IDC_BROWSE, _T("Browse..."));
	button->SetHelpText(helpText);
	field->Add(button, 0, wxALIGN_CENTRE);

	szFilename->Add(field, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 4);

	// Generate a list of available file types
	MusicTypePtr nextType;
	unsigned int numFormats = 0;
	wxArrayString formats;
	while ((nextType = pManager->getMusicType(numFormats++))) {
		std::string code = nextType->getCode();
		std::string desc = nextType->getFriendlyName();
		std::vector<std::string> exts = nextType->getFileExtensions();
		std::string all_exts;
		if (exts.size() > 0) {
			std::string sep;
			for (std::vector<std::string>::iterator x =
				exts.begin(); x != exts.end(); x++)
			{
				all_exts += sep;
				all_exts += "*.";
				all_exts += *x;
				if (sep.empty()) sep = "; ";
			}
			desc += " (";
			desc += all_exts;
			desc += ")";
		}
		formats.Add(wxString(desc.c_str(), wxConvUTF8));
	}

	this->cbFileType = new wxComboBox(this, IDC_FILETYPE, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, formats,
		wxCB_DROPDOWN | wxCB_READONLY);

	szFilename->Add(this->cbFileType, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 4);

	szMain->Add(szFilename, 0, wxEXPAND | wxALL, 8);

	// Add dialog buttons (ok/cancel/help)
//	wxStdDialogButtonSizer *szButtons = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
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

	if (szButtons) szMain->Add(szButtons, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 8);
	this->SetSizer(szMain);
	szMain->Fit(this);
}

void DlgImportMusic::setControls()
{
	this->txtPath->SetValue(this->filename);

	MusicTypePtr nextType;
	unsigned int fileTypeIndex = 0;
	while ((nextType = pManager->getMusicType(fileTypeIndex))) {
		wxString code = wxString(nextType->getCode().c_str(), wxConvUTF8);
		if (this->fileType.IsSameAs(code)) {
			this->cbFileType->SetSelection(fileTypeIndex);
			break;
		}
		fileTypeIndex++;
	}
	// Unknown fileType will result in no default setting (combobox will be blank)
	return;
}

DlgImportMusic::~DlgImportMusic()
{
}

void DlgImportMusic::onOK(wxCommandEvent& ev)
{
	this->filename = this->txtPath->GetValue();

	MusicTypePtr nextType = pManager->getMusicType(this->cbFileType->GetSelection());
	if (nextType) {
		this->fileType = wxString(nextType->getCode().c_str(), wxConvUTF8);
	} else {
		this->fileType = wxEmptyString;
	}

	this->EndModal(wxID_OK);
	return;
}

void DlgImportMusic::onCancel(wxCommandEvent& ev)
{
	this->EndModal(wxID_CANCEL);
	return;
}

void DlgImportMusic::onBrowse(wxCommandEvent& ev)
{
	wxFileName fn(this->filename, wxPATH_NATIVE);
	wxString path = wxFileSelector(_T("Import song"), fn.GetPath(),
		wxEmptyString, wxEmptyString, _T("All files|*"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
	if (!path.empty()) {
		this->txtPath->SetValue(path);

		// Test filetype and set combobox accordingly
		this->cbFileType->SetValue(wxEmptyString); // default to blank
		stream::input_file_sptr infile(new stream::input_file());
		infile->open(path.mb_str());

		MusicTypePtr pTestType;
		int i = 0;
		bool tryUnsure = true;
		while ((pTestType = pManager->getMusicType(i))) {
			MusicType::Certainty cert = pTestType->isInstance(infile);
			switch (cert) {
				case MusicType::DefinitelyNo:
					break;
				case MusicType::Unsure:
					std::cout << "File could be: " << pTestType->getFriendlyName()
						<< " [" << pTestType->getCode() << "]" << std::endl;
					// If we haven't found a match already, use this one
					if (tryUnsure) {
						this->cbFileType->SetSelection(i);
						tryUnsure = false;
					}
					break;
				case MusicType::PossiblyYes:
					std::cout << "File is likely to be: " << pTestType->getFriendlyName()
						<< " [" << pTestType->getCode() << "]" << std::endl;
					// Take this one as it's better than an uncertain match
					this->cbFileType->SetSelection(i);
					tryUnsure = false;
					break;
				case MusicType::DefinitelyYes:
					std::cout << "File is definitely: " << pTestType->getFriendlyName()
						<< " [" << pTestType->getCode() << "]" << std::endl;
					this->cbFileType->SetSelection(i);
					// Don't bother checking any other formats if we got a 100% match
					goto finishTesting;
			}
			i++;
		}
finishTesting:
		;
	}

	return;
}
