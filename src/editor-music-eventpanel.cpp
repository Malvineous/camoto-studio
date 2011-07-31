/**
 * @file   editor-music-eventpanel.cpp
 * @brief  Single channel list of events UI control for the music editor.
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

#include "editor-music-eventpanel.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

BEGIN_EVENT_TABLE(EventPanel, wxPanel)
	EVT_PAINT(EventPanel::onPaint)
	EVT_SIZE(EventPanel::onResize)
END_EVENT_TABLE()

EventPanel::EventPanel(MusicDocument *parent, int channel)
	throw () :
		wxPanel(parent, wxID_ANY),
		doc(parent),
		channel(channel)
{
	this->SetBackgroundColour(*wxWHITE);
}

EventPanel::~EventPanel()
	throw ()
{
}

void EventPanel::onPaint(wxPaintEvent& pev)
{
	this->pdc = new wxPaintDC(this);
	wxSize s = this->GetClientSize();
	this->pdc->SetFont(this->doc->font);
	int height = s.GetHeight();
	int rowTime = this->doc->absTimeStart;
	EventVector::iterator ev = this->doc->events.begin();

	for (this->paintY = 0; this->paintY < height; this->paintY += this->doc->fontHeight) {
		// Get the next event for this view
		while (
			(ev != this->doc->events.end()) &&
			(
				((*ev)->absTime < rowTime) ||
				(
					((*ev)->channel != 0) &&
					((*ev)->channel != this->channel)
				)
			)
		) {
			ev++;
		}
		if (ev == this->doc->events.end()) break; // no more events

		// At this point we have an event at or after the current row time
		assert((*ev)->absTime >= rowTime);

		if ((this->doc->playPos >= rowTime) && (this->doc->playPos < rowTime + this->doc->ticksPerRow)) {
			// This row is currently being played, so highlight it
			this->pdc->SetTextBackground(*wxLIGHT_GREY);
			this->pdc->SetBackgroundMode(wxSOLID);
		} else {
			// Normal row
			this->pdc->SetTextBackground(wxNullColour);
			this->pdc->SetBackgroundMode(wxTRANSPARENT);
		}

		if ((*ev)->absTime < rowTime + this->doc->ticksPerRow) {
			// This event occurs on the current row, use the processEvent()
			// wrapper to call this->handleEvent() with the appropriate type.
			(*ev)->processEvent(this);
		} else {
			this->pdc->SetTextForeground(*wxLIGHT_GREY);
			this->pdc->DrawText(_T("--- -- ----"), 0, this->paintY);
		}
		rowTime += this->doc->ticksPerRow;
	}

	delete this->pdc;
	this->pdc = NULL;  // just in case
	return;
}

void EventPanel::onResize(wxSizeEvent& ev)
{
	this->Layout();
	return;
}

void EventPanel::handleEvent(TempoEvent *ev)
	throw (std::ios::failure)
{
	this->pdc->SetTextForeground(*wxGREEN);
	wxString txt;
	txt.Printf(_T("T-%09d"), ev->usPerTick);
	this->pdc->DrawText(txt, 0, this->paintY);
	return;
}

void EventPanel::handleEvent(NoteOnEvent *ev)
	throw (std::ios::failure, EChannelMismatch, EBadPatchType)
{
	this->pdc->SetTextForeground(*wxBLACK);
	this->drawNoteOn(ev->milliHertz, ev->instrument);
	return;
}

void EventPanel::handleEvent(NoteOffEvent *ev)
	throw (std::ios::failure)
{
	this->pdc->SetTextForeground(*wxBLACK);
	this->pdc->DrawText(_T("    -- ----"), 0, this->paintY);
	return;
}

void EventPanel::handleEvent(PitchbendEvent *ev)
	throw (std::ios::failure)
{
	this->pdc->SetTextForeground(*wxBLUE);
	this->drawNoteOn(ev->milliHertz, -1);
	return;
}

void EventPanel::handleEvent(ConfigurationEvent *ev)
	throw (std::ios::failure)
{
	this->pdc->SetTextForeground(*wxCYAN);
	wxString txt;
	switch (ev->configType) {
		case ConfigurationEvent::EnableOPL3:
			txt.Append(_T("OPL3"));
			break;
		case ConfigurationEvent::EnableDeepTremolo:
			txt.Append(_T("DeepTrem"));
			break;
		case ConfigurationEvent::EnableDeepVibrato:
			txt.Append(_T("DeepVibr"));
			break;
		case ConfigurationEvent::EnableRhythm:
			txt.Append(_T("Rhythm"));
			break;
		case ConfigurationEvent::EnableWaveSel:
			txt.Append(_T("WaveSel"));
			break;
		default:
			txt.Append(_T("(Unknown)"));
			break;
	}
	txt.Append((ev->value == 1) ? _T("+") : _T("-"));
	this->pdc->DrawText(txt, 0, this->paintY);
	return;
}

void EventPanel::drawNoteOn(int milliHertz, int instrument)
	throw ()
{
	double dbHertz = milliHertz / 440000.0;
	double midiNote, midiNoteFrac;
	int midiNoteInt;
	if (dbHertz > 0) {
		midiNote = 69 + 12 * log(dbHertz) / log(2);
	} else {
		midiNote = 0;
	}
	midiNoteInt = (int)midiNote;
	midiNoteFrac = midiNote - midiNoteInt;

	const wxChar *names[] = {_T("C-"), _T("C#"), _T("D-"), _T("D#"), _T("E-"),
		_T("F-"), _T("F#"), _T("G-"), _T("G#"), _T("A-"), _T("A#"), _T("B-")};

	wxString txt;
	txt.Printf(_T("%s%1d "), names[midiNoteInt % 12], midiNoteInt / 12);

	if (instrument >= 0) {
		txt.Append(wxString::Format(_T("%02X "), instrument));
	} else {
		txt.Append(_T("-- "));
	}

	if (midiNoteFrac == 0) {
		txt.Append(_T("----"));
	} else {
		txt.Append(wxString::Format(_T("%0+4d"), (int)(255.0 * midiNoteFrac)));
	}
	this->pdc->DrawText(txt, 0, this->paintY);
	return;
}
