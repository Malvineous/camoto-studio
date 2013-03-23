/**
 * @file   editor-music-instrumentpanel.cpp
 * @brief  Instrument adjustment panel for music editor.
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

#include "editor-music-instrumentpanel.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

enum {
	IDC_INST_TYPE = wxID_HIGHEST + 1,
	IDC_INST_NAME,

	IDC_C_FREQMULT,
	IDC_C_SCALE,
	IDC_C_OUTPUT,
	IDC_C_ATTACK,
	IDC_C_DECAY,
	IDC_C_SUSTAIN,
	IDC_C_RELEASE,
	IDC_C_WAVESEL,

	IDC_M_FREQMULT,
	IDC_M_SCALE,
	IDC_M_OUTPUT,
	IDC_M_ATTACK,
	IDC_M_DECAY,
	IDC_M_SUSTAIN,
	IDC_M_RELEASE,
	IDC_M_WAVESEL,

	IDC_PATCH,
};

BEGIN_EVENT_TABLE(InstrumentPanel, IToolPanel)
	EVT_CHOICEBOOK_PAGE_CHANGED(IDC_INST_TYPE, InstrumentPanel::onTypeChanged)
	EVT_TEXT(IDC_INST_NAME, InstrumentPanel::onRenamed)

	EVT_SPINCTRL(IDC_C_FREQMULT, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_C_SCALE, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_C_OUTPUT, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_C_ATTACK, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_C_DECAY, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_C_SUSTAIN, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_C_RELEASE, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_C_WAVESEL, InstrumentPanel::onValueChanged)

	EVT_SPINCTRL(IDC_M_FREQMULT, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_M_SCALE, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_M_OUTPUT, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_M_ATTACK, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_M_DECAY, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_M_SUSTAIN, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_M_RELEASE, InstrumentPanel::onValueChanged)
	EVT_SPINCTRL(IDC_M_WAVESEL, InstrumentPanel::onValueChanged)

	EVT_SPINCTRL(IDC_PATCH, InstrumentPanel::onValueChanged)
END_EVENT_TABLE()

InstrumentPanel::InstrumentPanel(Studio *parent)
	:	IToolPanel(parent),
		instList(NULL),
		instIndex(0)
{
	this->tabs = new wxChoicebook(this, IDC_INST_TYPE);

	wxScrolledWindow *pnlOPL = new wxScrolledWindow(this->tabs);
	wxGridBagSizer *s = new wxGridBagSizer(2, 2);
	s->AddGrowableCol(0, 2);
	s->AddGrowableCol(1, 1);
	int row = 0;
	wxStaticText *label = new wxStaticText(pnlOPL, wxID_ANY, _("Carrier:"));
	s->Add(label, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND | wxALIGN_CENTRE_VERTICAL);

	this->addNumberControl(pnlOPL, s, &row, IDC_C_FREQMULT, _("Freq. multiplier"), 0,  7);
	this->addNumberControl(pnlOPL, s, &row, IDC_C_SCALE,    _("Scale level"),      0,  3);
	this->addNumberControl(pnlOPL, s, &row, IDC_C_OUTPUT,   _("Output level"),     0, 63);
	this->addNumberControl(pnlOPL, s, &row, IDC_C_ATTACK,   _("Attack rate"),      0, 15);
	this->addNumberControl(pnlOPL, s, &row, IDC_C_DECAY,    _("Decay rate"),       0, 15);
	this->addNumberControl(pnlOPL, s, &row, IDC_C_SUSTAIN,  _("Sustain rate"),     0, 15);
	this->addNumberControl(pnlOPL, s, &row, IDC_C_RELEASE,  _("Release rate"),     0, 15);
	this->addNumberControl(pnlOPL, s, &row, IDC_C_WAVESEL,  _("Waveform"),         0,  7);

	label = new wxStaticText(pnlOPL, wxID_ANY, _("Modulator:"));
	s->Add(label, wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND | wxALIGN_CENTRE_VERTICAL);

	this->addNumberControl(pnlOPL, s, &row, IDC_M_FREQMULT, _("Freq. multiplier"), 0,  7);
	this->addNumberControl(pnlOPL, s, &row, IDC_M_SCALE,    _("Scale level"),      0,  3);
	this->addNumberControl(pnlOPL, s, &row, IDC_M_OUTPUT,   _("Output level"),     0, 63);
	this->addNumberControl(pnlOPL, s, &row, IDC_M_ATTACK,   _("Attack rate"),      0, 15);
	this->addNumberControl(pnlOPL, s, &row, IDC_M_DECAY,    _("Decay rate"),       0, 15);
	this->addNumberControl(pnlOPL, s, &row, IDC_M_SUSTAIN,  _("Sustain rate"),     0, 15);
	this->addNumberControl(pnlOPL, s, &row, IDC_M_RELEASE,  _("Release rate"),     0, 15);
	this->addNumberControl(pnlOPL, s, &row, IDC_M_WAVESEL,  _("Waveform"),         0,  7);

	this->tabs->AddPage(pnlOPL, _("FM/OPL"));

	pnlOPL->SetSizer(s);
	pnlOPL->FitInside();
	pnlOPL->SetScrollRate(0, 16);

	wxScrolledWindow *pnlMIDI = new wxScrolledWindow(this->tabs);
	s = new wxGridBagSizer(2, 2);
	row = 0;
	this->addNumberControl(pnlMIDI, s, &row, IDC_PATCH, _("Patch"), 1,  128);

	this->tabs->AddPage(pnlMIDI, _("MIDI"));
	pnlMIDI->SetSizer(s);
	pnlMIDI->FitInside();
	pnlMIDI->SetScrollRate(0, 16);

	this->txtInstNum = new wxStaticText(this, wxID_ANY, _T("??: "));
	this->txtInstName = new wxTextCtrl(this, IDC_INST_NAME, _("[no selection]"));
	wxBoxSizer *titleSizer = new wxBoxSizer(wxHORIZONTAL);
	titleSizer->Add(this->txtInstNum, 0, wxEXPAND | wxRIGHT, 2);
	titleSizer->Add(this->txtInstName, 1, wxEXPAND);

	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(titleSizer, 0, wxEXPAND | wxALL, 2);
	mainSizer->Add(this->tabs, 1, wxEXPAND);
	this->SetSizer(mainSizer);
}

void InstrumentPanel::addNumberControl(wxWindow *parent, wxGridBagSizer *sizer,
	int *row, unsigned int id, const wxString& title, unsigned int min,
	unsigned int max)
{
	wxStaticText *label = new wxStaticText(parent, wxID_ANY, title);
	sizer->Add(label, wxGBPosition(*row, 0), wxGBSpan(1, 1), wxEXPAND | wxALIGN_CENTRE_VERTICAL);

	wxSpinCtrl *val = new wxSpinCtrl(parent, id, wxEmptyString, wxDefaultPosition, wxSize(50, -1));
	val->SetRange(min, max);
	sizer->Add(val, wxGBPosition(*row, 1), wxGBSpan(1, 1), wxEXPAND);

	(*row)++;
	return;
}

void InstrumentPanel::getPanelInfo(wxString *id, wxString *label) const
{
	*id = _T("music.instrument");
	*label = _("Instrument settings");
	return;
}

void InstrumentPanel::switchDocument(IDocument *doc)
{
	this->oplInst.reset();
	this->midiInst.reset();
	//this->pcmInst.reset();
	// We don't have to do anything here, because the InstrumentListPanel will
	// call setInstrumentList() and setInstrument() in its switchDocument().
	return;
}

void InstrumentPanel::loadSettings(Project *proj)
{
	return;
}

void InstrumentPanel::saveSettings(Project *proj) const
{
	return;
}

void InstrumentPanel::setInstrumentList(InstrumentListPanel *instList, PatchBankPtr patchBank)
{
	this->instList = instList;
	this->patchBank = patchBank;
	return;
}

void InstrumentPanel::setInstrument(unsigned int newIndex)
{
	this->instIndex = newIndex;
	if (!this->patchBank) return; // just in case

	if (newIndex >= this->patchBank->size()) {
		std::cerr << "ERROR: Tried to edit patch #" << newIndex << " but only "
			<< this->patchBank->size() << " instruments in bank!"
			<< std::endl;
		return;
	}

	this->inst = patchBank->at(newIndex);
	if (!this->inst) {
		std::cerr << "ERROR: Tried to edit in-range patch #" << newIndex
			<< " but bank returned a NULL pointer!" << std::endl;
		return;
	}

	this->oplInst = boost::dynamic_pointer_cast<OPLPatch>(this->inst);
	this->midiInst = boost::dynamic_pointer_cast<MIDIPatch>(this->inst);
	//this->pcmInst = boost::dynamic_pointer_cast<PCMPatch>(this->inst);
	if (this->oplInst) {
		// Show and update the OPL controls
		((wxSpinCtrl *)this->FindWindow(IDC_C_FREQMULT))->SetValue(this->oplInst->c.freqMult);
		((wxSpinCtrl *)this->FindWindow(IDC_C_SCALE))->SetValue(this->oplInst->c.scaleLevel);
		((wxSpinCtrl *)this->FindWindow(IDC_C_OUTPUT))->SetValue(this->oplInst->c.outputLevel);
		((wxSpinCtrl *)this->FindWindow(IDC_C_ATTACK))->SetValue(this->oplInst->c.attackRate);
		((wxSpinCtrl *)this->FindWindow(IDC_C_DECAY))->SetValue(this->oplInst->c.decayRate);
		((wxSpinCtrl *)this->FindWindow(IDC_C_SUSTAIN))->SetValue(this->oplInst->c.sustainRate);
		((wxSpinCtrl *)this->FindWindow(IDC_C_RELEASE))->SetValue(this->oplInst->c.releaseRate);
		((wxSpinCtrl *)this->FindWindow(IDC_C_WAVESEL))->SetValue(this->oplInst->c.waveSelect);

		((wxSpinCtrl *)this->FindWindow(IDC_M_FREQMULT))->SetValue(this->oplInst->m.freqMult);
		((wxSpinCtrl *)this->FindWindow(IDC_M_SCALE))->SetValue(this->oplInst->m.scaleLevel);
		((wxSpinCtrl *)this->FindWindow(IDC_M_OUTPUT))->SetValue(this->oplInst->m.outputLevel);
		((wxSpinCtrl *)this->FindWindow(IDC_M_ATTACK))->SetValue(this->oplInst->m.attackRate);
		((wxSpinCtrl *)this->FindWindow(IDC_M_DECAY))->SetValue(this->oplInst->m.decayRate);
		((wxSpinCtrl *)this->FindWindow(IDC_M_SUSTAIN))->SetValue(this->oplInst->m.sustainRate);
		((wxSpinCtrl *)this->FindWindow(IDC_M_RELEASE))->SetValue(this->oplInst->m.releaseRate);
		((wxSpinCtrl *)this->FindWindow(IDC_M_WAVESEL))->SetValue(this->oplInst->m.waveSelect);
		this->instType = 0; // Choicebook page index
	} else if (this->midiInst) {
		// Show and update the MIDI controls
		this->instType = 1; // Choicebook page index
		((wxSpinCtrl *)this->FindWindow(IDC_PATCH))->SetValue(this->midiInst->midiPatch + 1);
	}
	this->tabs->SetSelection(this->instType);

	wxString newNum;
	newNum.Printf(_T("%02X: "), newIndex);
	this->txtInstNum->SetLabel(newNum);
	if (this->inst->name.empty()) {
		this->txtInstName->ChangeValue(_("[no name]"));
	} else {
		this->txtInstName->ChangeValue(wxString(this->inst->name.c_str(), wxConvUTF8));
	}
	return;
}

void InstrumentPanel::onTypeChanged(wxChoicebookEvent& ev)
{
	this->instType = ev.GetSelection();
	if (this->instList) {
		PatchPtr newInst;
		switch (this->instType) {
			case 0:
				if (!this->oplInst) {
					this->oplInst.reset(new OPLPatch());
					this->oplInst->name = this->inst->name;
					// TODO: Default settings
				}
				newInst = this->oplInst;
				this->midiInst.reset();
				this->inst = newInst;
				break;
			case 1:
				if (!this->midiInst) {
					this->midiInst.reset(new MIDIPatch());
					this->midiInst->name = this->inst->name;
					this->midiInst->midiPatch = 0; // Default to Piano
				}
				newInst = this->midiInst;
				this->oplInst.reset();
				this->inst = newInst;
				break;
			default:
				return;
		}
		this->instList->replaceInstrument(this->instIndex, newInst);
		// Update the controls with the new instrument
		this->setInstrument(this->instIndex);
	}
	return;
}

void InstrumentPanel::onValueChanged(wxSpinEvent& ev)
{
	unsigned int val = ev.GetPosition();
	if (this->oplInst) {
		switch (ev.GetId()) {
			case IDC_C_FREQMULT: this->oplInst->c.freqMult = val; break;
			case IDC_C_SCALE: this->oplInst->c.scaleLevel = val; break;
			case IDC_C_OUTPUT: this->oplInst->c.outputLevel = val; break;
			case IDC_C_ATTACK: this->oplInst->c.attackRate = val; break;
			case IDC_C_DECAY: this->oplInst->c.decayRate = val; break;
			case IDC_C_SUSTAIN: this->oplInst->c.sustainRate = val; break;
			case IDC_C_RELEASE: this->oplInst->c.releaseRate = val; break;
			case IDC_C_WAVESEL: this->oplInst->c.waveSelect = val; break;

			case IDC_M_FREQMULT: this->oplInst->m.freqMult = val; break;
			case IDC_M_SCALE: this->oplInst->m.scaleLevel = val; break;
			case IDC_M_OUTPUT: this->oplInst->m.outputLevel = val; break;
			case IDC_M_ATTACK: this->oplInst->m.attackRate = val; break;
			case IDC_M_DECAY: this->oplInst->m.decayRate = val; break;
			case IDC_M_SUSTAIN: this->oplInst->m.sustainRate = val; break;
			case IDC_M_RELEASE: this->oplInst->m.releaseRate = val; break;
			case IDC_M_WAVESEL: this->oplInst->m.waveSelect = val; break;
		}
	}
	if (this->midiInst) {
		switch (ev.GetId()) {
			case IDC_PATCH: this->midiInst->midiPatch = val - 1; break;
		}
	}
	return;
}

void InstrumentPanel::onRenamed(wxCommandEvent& ev)
{
	if (!this->inst) return;
	std::string newName = ev.GetString().ToUTF8().data();
	this->inst->name = newName;
	this->instList->updateInstrument(this->instIndex);
	return;
}
