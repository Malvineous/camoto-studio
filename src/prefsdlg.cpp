/**
 * @file   prefsdlg.cpp
 * @brief  Dialog box for the preferences window.
 *
 * Copyright (C) 2010-2014 Adam Nielsen <malvineous@shikadi.net>
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
#include "prefsdlg.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

/// Control IDs
enum {
	IDC_DOSBOX_PATH = wxID_HIGHEST + 1,
	IDC_BROWSE_DOSBOX,
	IDC_PAUSE,
	IDC_SOUNDFONTS,
};

BEGIN_EVENT_TABLE(PrefsDialog, wxDialog)
	EVT_BUTTON(wxID_OK, PrefsDialog::onOK)
	EVT_BUTTON(wxID_CANCEL, PrefsDialog::onCancel)
	EVT_BUTTON(IDC_BROWSE_DOSBOX, PrefsDialog::onBrowseDOSBox)
END_EVENT_TABLE()

PrefsDialog::PrefsDialog(Studio *parent, AudioPtr audio)
	:	wxDialog(parent, wxID_ANY, _("Preferences"), wxDefaultPosition,
			wxDefaultSize, wxDIALOG_EX_CONTEXTHELP | wxRESIZE_BORDER)
{
	wxNotebook *tabs = new wxNotebook(this, wxID_ANY);
	wxPanel *tabTesting = new wxPanel(tabs);
	wxPanel *tabAudio = new wxPanel(tabs);
	wxBoxSizer *szTesting = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szAudio = new wxBoxSizer(wxVERTICAL);
	tabTesting->SetSizer(szTesting);
	tabAudio->SetSizer(szAudio);

	// Have to create the static box *before* the controls that go inside it
	wxStaticBoxSizer *szDOSBox = new wxStaticBoxSizer(wxVERTICAL, tabTesting, _("DOSBox"));

	// First text field
	wxString helpText = _("The location of the DOSBox .exe file.");
	wxStaticText *label = new wxStaticText(tabTesting, wxID_ANY, _("DOSBox executable:"));
	label->SetHelpText(helpText);
	szDOSBox->Add(label, 0, wxEXPAND | wxALIGN_LEFT | wxLEFT, 4);

	wxBoxSizer *textbox = new wxBoxSizer(wxVERTICAL);
	textbox->AddStretchSpacer(1);
	this->txtDOSBoxPath = new wxTextCtrl(tabTesting, IDC_DOSBOX_PATH);
	this->txtDOSBoxPath->SetHelpText(helpText);
	textbox->Add(this->txtDOSBoxPath, 0, wxEXPAND | wxALIGN_CENTRE);
	textbox->AddStretchSpacer(1);

	wxBoxSizer *field = new wxBoxSizer(wxHORIZONTAL);
	field->Add(textbox, 1, wxEXPAND);
	wxButton *button = new wxButton(tabTesting, IDC_BROWSE_DOSBOX, _("Browse..."));
	button->SetHelpText(helpText);
	field->Add(button, 0, wxALIGN_CENTRE);

	szDOSBox->Add(field, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 4);

	this->chkDOSBoxPause = new wxCheckBox(tabTesting, IDC_PAUSE, _("Pause after "
		"exiting game, before closing DOSBox?"));
	this->chkDOSBoxPause->SetHelpText(_("This option will cause DOSBox to wait "
		"for you to press a key after the game has exited, so you can see the "
		"exit screen."));

	szDOSBox->Add(this->chkDOSBoxPause, 0, wxEXPAND | wxALIGN_LEFT | wxALL, 8);

	szTesting->Add(szDOSBox, 0, wxEXPAND | wxALL, 8);

	wxStaticBoxSizer *szMIDI = new wxStaticBoxSizer(wxVERTICAL, tabAudio, _("MIDI Output"));
	label = new wxStaticText(tabAudio, wxID_ANY, _("Select a device to use "
		"for MIDI output.  If you click the Test button below and you cannot hear "
		"a piano, try a different device."));
	label->Wrap(480);
	szMIDI->Add(label, 0, wxEXPAND | wxALIGN_LEFT | wxALL, 8);
	this->portList = new wxListCtrl(tabAudio, IDC_SOUNDFONTS, wxDefaultPosition,
		wxDefaultSize, wxBORDER_SUNKEN | wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);
	this->portList->SetImageList(parent->smallImages, wxIMAGE_LIST_SMALL);
	szMIDI->Add(this->portList, 1, wxEXPAND | wxALL, 8);
	szAudio->Add(szMIDI, 1, wxEXPAND | wxALL, 8);

	wxListItem info;
	info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE | wxLIST_MASK_FORMAT;
	info.m_image = 0;
	info.m_format = 0;
	info.m_text = _("MIDI Port");
	this->portList->InsertColumn(0, info);

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

	tabs->AddPage(tabTesting, _("Testing"));
	tabs->AddPage(tabAudio, _("Audio"));

	wxBoxSizer *szMain = new wxBoxSizer(wxVERTICAL);
	szMain->Add(tabs, 1, wxEXPAND | wxALIGN_CENTER | wxALL, 8);
	if (szButtons) szMain->Add(szButtons, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 8);
	this->SetSizer(szMain);
	szMain->Fit(this);
}

void PrefsDialog::setControls()
{
	this->txtDOSBoxPath->SetValue(*this->pathDOSBox);
	this->chkDOSBoxPause->SetValue(*this->pauseAfterExecute);

	this->portList->DeleteAllItems();
	this->portList->InsertItem(0, _("Virtual MIDI port"),
		ImageListIndex::InstMIDI);

	this->portList->SetColumnWidth(0, wxLIST_AUTOSIZE);

	return;
}

PrefsDialog::~PrefsDialog()
{
}

void PrefsDialog::onOK(wxCommandEvent& ev)
{
	*this->pathDOSBox = this->txtDOSBoxPath->GetValue();
	*this->pauseAfterExecute = this->chkDOSBoxPause->IsChecked();

	long sel = this->portList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel == -1) sel = 0; // default to first device (-1 is virtual port)
	else sel--; // make first item (virtual port) become -1, and 0 first real dev

	this->EndModal(wxID_OK);
	return;
}

void PrefsDialog::onCancel(wxCommandEvent& ev)
{
	this->EndModal(wxID_CANCEL);
	return;
}

void PrefsDialog::onBrowseDOSBox(wxCommandEvent& ev)
{
	wxString path = wxFileSelector(_("DOSBox executable location"),
		wxEmptyString,
#ifdef __WXMSW__
		_T("dosbox.exe"), _T(".exe"), _("Executable files (*.exe)|*.exe|All files (*.*)|*.*"),
#else
		_T("dosbox"), wxEmptyString, _("All files|*"),
#endif
		wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
	if (!path.empty()) this->txtDOSBoxPath->SetValue(path);
	return;
}
