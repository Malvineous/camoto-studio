/**
 * @file   prefsdlg.cpp
 * @brief  Dialog box for the preferences window.
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

#include <wx/statline.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#ifndef __WXMSW__
#include <wx/cshelp.h>
#endif

#include "prefsdlg.hpp"

/// Control IDs
enum {
	IDC_DOSBOX_PATH = wxID_HIGHEST + 1,
	IDC_BROWSE_DOSBOX,
	IDC_PAUSE,
};

BEGIN_EVENT_TABLE(PrefsDialog, wxDialog)
	EVT_BUTTON(wxID_OK, PrefsDialog::onOK)
	EVT_BUTTON(IDC_BROWSE_DOSBOX, PrefsDialog::onBrowseDOSBox)
END_EVENT_TABLE()

PrefsDialog::PrefsDialog(wxWindow *parent)
	throw () :
	wxDialog(parent, wxID_ANY, _T("Preferences"), wxDefaultPosition,
		wxDefaultSize, wxDIALOG_EX_CONTEXTHELP | wxRESIZE_BORDER)
{
	// Have to create the static box *before* the controls that go inside it
	wxStaticBoxSizer *szDOSBox = new wxStaticBoxSizer(wxVERTICAL, this, _T("Game testing"));

	// First text field
	wxString helpText = _T("The location of the DOSBox .exe file.");
	wxStaticText *label = new wxStaticText(this, wxID_ANY, _T("DOSBox executable:"));
	label->SetHelpText(helpText);
	szDOSBox->Add(label, 0, wxEXPAND | wxALIGN_LEFT | wxLEFT, 4);

	wxBoxSizer *textbox = new wxBoxSizer(wxVERTICAL);
	textbox->AddStretchSpacer(1);
	this->txtDOSBoxPath = new wxTextCtrl(this, IDC_DOSBOX_PATH);
	this->txtDOSBoxPath->SetHelpText(helpText);
	textbox->Add(this->txtDOSBoxPath, 0, wxEXPAND | wxALIGN_CENTRE);
	textbox->AddStretchSpacer(1);

	wxBoxSizer *field = new wxBoxSizer(wxHORIZONTAL);
	field->Add(textbox, 1, wxEXPAND);
	wxButton *button = new wxButton(this, IDC_BROWSE_DOSBOX, _T("Browse..."));
	button->SetHelpText(helpText);
	field->Add(button, 0, wxALIGN_CENTRE);

	szDOSBox->Add(field, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 4);

	this->chkDOSBoxPause = new wxCheckBox(this, IDC_PAUSE, _T("Pause after "
		"exiting game, before closing DOSBox?"));
	this->chkDOSBoxPause->SetHelpText(_T("This option will cause DOSBox to wait "
		"for you to press a key after the game has exited, so you can see the "
		"exit screen."));

	szDOSBox->Add(this->chkDOSBoxPause, 0, wxEXPAND | wxALIGN_LEFT | wxALL, 10);

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

	wxBoxSizer *szMain = new wxBoxSizer(wxVERTICAL);
	szMain->Add(szDOSBox, 1, wxEXPAND | wxALIGN_CENTER | wxALL, 4);
	szMain->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxSize(20, -1), wxLI_HORIZONTAL), 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP, 5);
	szMain->Add(szButtons, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 2);
	this->SetSizer(szMain);
	szMain->Fit(this);
}

void PrefsDialog::setControls()
	throw ()
{
	this->txtDOSBoxPath->SetValue(*this->pathDOSBox);
	this->chkDOSBoxPause->SetValue(*this->pauseAfterExecute);
	return;
}

PrefsDialog::~PrefsDialog()
	throw ()
{
}

void PrefsDialog::onOK(wxCommandEvent& ev)
{
	*this->pathDOSBox = this->txtDOSBoxPath->GetValue();
	*this->pauseAfterExecute = this->chkDOSBoxPause->IsChecked();

	this->EndModal(wxID_OK);
	return;
}

void PrefsDialog::onBrowseDOSBox(wxCommandEvent& ev)
{
	wxString path = wxFileSelector(_T("DOSBox executable location"),
		wxEmptyString,
#ifdef __WXMSW__
		_T("dosbox.exe"), _T(".exe"), _T("Executable files (*.exe)|*.exe|All files (*.*)|*.*"),
#else
		_T("dosbox"), wxEmptyString, _T("All files|*"),
#endif
		wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
	if (!path.empty()) this->txtDOSBoxPath->SetValue(path);
	return;
}
