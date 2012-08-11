/**
 * @file   prefsdlg.cpp
 * @brief  Dialog box for the preferences window.
 *
 * Copyright (C) 2010-2012 Adam Nielsen <malvineous@shikadi.net>
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
#include <camoto/gamemusic/patch-midi.hpp>
#include "prefsdlg.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

/// Control IDs
enum {
	IDC_DOSBOX_PATH = wxID_HIGHEST + 1,
	IDC_BROWSE_DOSBOX,
	IDC_PAUSE,
	IDC_MIDI_PORTS,
	IDC_PCM_DELAY,
	IDC_TEST_AUDIO,
};

BEGIN_EVENT_TABLE(PrefsDialog, wxDialog)
	EVT_BUTTON(wxID_OK, PrefsDialog::onOK)
	EVT_BUTTON(wxID_CANCEL, PrefsDialog::onCancel)
	EVT_BUTTON(IDC_BROWSE_DOSBOX, PrefsDialog::onBrowseDOSBox)
	EVT_SPINCTRL(IDC_PCM_DELAY, PrefsDialog::onDelayChange)
	EVT_TOGGLEBUTTON(IDC_TEST_AUDIO, PrefsDialog::onTestAudio)
END_EVENT_TABLE()

PrefsDialog::PrefsDialog(IMainWindow *parent, AudioPtr audio)
	throw ()
	: wxDialog(parent, wxID_ANY, _("Preferences"), wxDefaultPosition,
		wxDefaultSize, wxDIALOG_EX_CONTEXTHELP | wxRESIZE_BORDER),
	  audio(audio),
	  player(NULL)
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
	this->portList = new wxListCtrl(tabAudio, IDC_MIDI_PORTS, wxDefaultPosition,
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

	wxStaticBoxSizer *szSync = new wxStaticBoxSizer(wxVERTICAL, tabAudio, _("Synchronisation"));
	szAudio->Add(szSync, 0, wxEXPAND | wxALL, 8);

	label = new wxStaticText(tabAudio, wxID_ANY, _("Set the delay in "
		"milliseconds applied to compensate for differing latency between the "
		"digital audio stream and the MIDI device.  The Test button will play two "
		"notes at the same time (one digitised and one MIDI.)  If you hear the "
		"high note first (a synthesised bell), increase the value.  If you hear "
		"the low note first (a MIDI piano), decrease the value."));
	label->Wrap(480);
	szSync->Add(label, 0, wxEXPAND | wxALIGN_LEFT | wxALL, 8);

	field = new wxBoxSizer(wxHORIZONTAL);
	this->spinDelay = new wxSpinCtrl(tabAudio, IDC_PCM_DELAY);
	this->spinDelay->SetRange(-1000, 1000);
	field->Add(this->spinDelay, 0, wxEXPAND);

	wxToggleButton *tbutton = new wxToggleButton(tabAudio, IDC_TEST_AUDIO, _("Test"));
#if wxMINOR_VERSION > 8
	tbutton->SetBitmap(parent->smallImages->GetBitmap(ImageListIndex::Play));
#endif
	field->Add(tbutton, 0, wxALIGN_CENTRE);

	szSync->Add(field, 0, wxALIGN_CENTRE | wxALL, 8);

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

	try {
		this->midi.reset(new RtMidiOut());
	} catch (RtError& e) {
		std::cerr << "Unable to initialise RtMidi: " << e.getMessage() << std::endl;
		wxString msg(_("Unable to initialise MIDI support: "));
		msg += wxString::FromUTF8(e.getMessage().c_str());
		wxMessageDialog dlg(this, msg, _("RtMidi error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
}

void PrefsDialog::setControls()
	throw ()
{
	this->txtDOSBoxPath->SetValue(*this->pathDOSBox);
	this->chkDOSBoxPause->SetValue(*this->pauseAfterExecute);

	this->portList->DeleteAllItems();
	this->portList->InsertItem(0, _("Virtual MIDI port"),
		ImageListIndex::InstMIDI);
	unsigned int portCount = this->midi->getPortCount();
	for (unsigned int i = 0; i < portCount; i++) {
		this->portList->InsertItem(i + 1,
			wxString(this->midi->getPortName(i).c_str(), wxConvUTF8),
			ImageListIndex::InstMIDI);
	}
	this->portList->SetColumnWidth(0, wxLIST_AUTOSIZE);
	this->portList->SetItemState(*this->midiDevice + 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

	this->spinDelay->SetValue(*this->pcmDelay / 1000);

	// Remember original values of controls that cause live changes
	this->o_pcmDelay = *this->pcmDelay;
	this->o_midiDevice = *this->midiDevice;

	return;
}

PrefsDialog::~PrefsDialog()
	throw ()
{
	if (this->player) {
		// Tell the playback thread to gracefully terminate
		this->player->quit();

		// Wake the thread up if it's stuck in a delay
		this->thread.interrupt();

		// Wait for playback thread to terminate
		this->thread.join();

		delete this->player;
	}
}

void PrefsDialog::onOK(wxCommandEvent& ev)
{
	*this->pathDOSBox = this->txtDOSBoxPath->GetValue();
	*this->pauseAfterExecute = this->chkDOSBoxPause->IsChecked();

	long sel = this->portList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (sel == -1) sel = 0; // default to first device (-1 is virtual port)
	else sel--; // make first item (virtual port) become -1, and 0 first real dev
	*this->midiDevice = sel;

	*this->pcmDelay = this->spinDelay->GetValue() * 1000;

	this->EndModal(wxID_OK);
	return;
}

void PrefsDialog::onCancel(wxCommandEvent& ev)
{
	// Restore original values for things that were changed live
	*this->pcmDelay = this->o_pcmDelay;
	*this->midiDevice = this->o_midiDevice;

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

void PrefsDialog::onDelayChange(wxSpinEvent& ev)
{
	// Update this value live so the effect can be heard immediately
	*this->pcmDelay = ev.GetPosition() * 1000;
	return;
}

void PrefsDialog::onTestAudio(wxCommandEvent& ev)
{
	if (ev.IsChecked()) {
		// Start playback

		// Update the selected MIDI device
		long sel = this->portList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (sel == -1) sel = 0; // default to first device (-1 is virtual port)
		else sel--; // make first item (virtual port) become -1, and 0 first real dev
		*this->midiDevice = sel;

		// Test the MIDI device first
		try {
			if (*this->midiDevice == -1) {
				this->midi->openVirtualPort("Camoto Studio");
			} else {
				this->midi->openPort(*this->midiDevice);
			}
			this->midi->closePort();
		} catch (const RtError& e) {
			std::cerr << "Unable to open the selected MIDI device: "
				<< e.getMessage() << std::endl;
			wxString msg(_("Unable to open the selected MIDI device: "));
			msg += wxString::FromUTF8(e.getMessage().c_str());
			wxMessageDialog dlg(this, msg, _("MIDI error"), wxOK | wxICON_ERROR);
			dlg.ShowModal();

			// Undo the button toggle
			((wxToggleButton *)ev.GetEventObject())->SetValue(false);
			return;
		}

		this->portList->Enable(false);

		MusicPtr music(new Music());
		music->patches.reset(new PatchBank());
		music->events.reset(new EventVector());
		music->patches->reserve(2);

		OPLPatchPtr po(new OPLPatch());
		po->c.enableTremolo = false;
		po->c.enableVibrato = false;
		po->c.enableSustain = false;
		po->c.enableKSR = true;
		po->c.freqMult = 5;
		po->c.scaleLevel = 0;
		po->c.outputLevel = 63-63;
		po->c.attackRate = 15;
		po->c.decayRate = 3;
		po->c.sustainRate = 14;
		po->c.releaseRate = 3;
		po->c.waveSelect = 0;
		po->m.enableTremolo = false;
		po->m.enableVibrato = false;
		po->m.enableSustain = false;
		po->m.enableKSR = false;
		po->m.freqMult = 7;
		po->m.scaleLevel = 1;
		po->m.outputLevel = 63-48;
		po->m.attackRate = 15;
		po->m.decayRate = 2;
		po->m.sustainRate = 9;
		po->m.releaseRate = 1;
		po->m.waveSelect = 0;
		po->feedback = 4;
		po->connection = false;
		po->rhythm = 0;
		music->patches->push_back(po);

		MIDIPatchPtr pm(new MIDIPatch());
		pm->midiPatch = 0; // Piano
		music->patches->push_back(pm);

		TempoEvent *e0 = new TempoEvent();
		e0->absTime = 0;
		e0->channel = 0;
		e0->usPerTick = 500000; // 500ms per tick
		music->events->push_back(EventPtr(e0));

		for (int i = 0; i < 4; i++) {
			NoteOnEvent *e1 = new NoteOnEvent();
			e1->absTime = i * 2;
			e1->channel = 1;
			e1->instrument = 0;
			e1->milliHertz = 440000;
			e1->velocity = 0;
			music->events->push_back(EventPtr(e1));

			e1 = new NoteOnEvent();
			e1->absTime = i * 2;
			e1->channel = 2;
			e1->instrument = 1;
			e1->milliHertz = 110000;
			e1->velocity = 0;
			music->events->push_back(EventPtr(e1));

			NoteOffEvent *e2 = new NoteOffEvent();
			e2->absTime = i * 2 + 1;
			e2->channel = 1;
			music->events->push_back(EventPtr(e2));

			e2 = new NoteOffEvent();
			e2->absTime = i * 2 + 1;
			e2->channel = 2;
			music->events->push_back(EventPtr(e2));
		}

		// Some other event at the end to cause a delay after the previous note off
		e0 = new TempoEvent();
		e0->absTime = 10;
		e0->channel = 0;
		e0->usPerTick = 500000; // 500ms per tick
		music->events->push_back(EventPtr(e0));

		this->player = new PlayerThread(audio, music, this);
		this->thread = boost::thread(boost::ref(*this->player));
		this->player->resume();
	} else {
		// Stop playback

		if (this->player) {
			// Tell the playback thread to gracefully terminate
			this->player->quit();

			// Wake the thread up if it's stuck in a delay
			this->thread.interrupt();

			// Wait for playback thread to terminate
			this->thread.join();

			delete this->player;
			this->player = NULL;
		}

		this->portList->Enable(true);
	}
	return;
}

void PrefsDialog::notifyPosition(unsigned long absTime)
	throw ()
{
	return;
}
