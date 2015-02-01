/**
 * @file   editor-music-eventpanel.cpp
 * @brief  Single channel list of events UI control for the music editor.
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

#include "editor-music-eventpanel.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

BEGIN_EVENT_TABLE(EventPanel, wxPanel)
	EVT_PAINT(EventPanel::onPaint)
	EVT_SIZE(EventPanel::onResize)
END_EVENT_TABLE()

EventPanel::EventPanel(MusicDocument *parent, unsigned int trackIndex)
	:	wxPanel(parent, wxID_ANY),
		doc(parent),
		trackIndex(trackIndex)
{
	this->SetBackgroundColour(*wxWHITE);
}

EventPanel::~EventPanel()
{
}

void EventPanel::onPaint(wxPaintEvent& pev)
{
	this->pdc = new wxPaintDC(this);
	wxSize s = this->GetClientSize();
	this->pdc->SetFont(this->doc->font);
	int height = s.GetHeight();

	if (this->doc->lastAudiblePos.order >= this->doc->music->patternOrder.size()) {
		// past EOF or at EOF if loop is off
		return;
	}
	unsigned int patternIndex =
		this->doc->music->patternOrder[this->doc->lastAudiblePos.order];
	gamemusic::TrackPtr& track =
		this->doc->music->patterns.at(patternIndex)->at(this->trackIndex);
	Track::const_iterator ev = track->begin();

	int midY = height / 2;
	int rowOffset = midY / this->doc->fontHeight;
	unsigned int rowHighlight = this->doc->lastAudiblePos.row;
	int firstRow = -rowOffset + rowHighlight;
	unsigned int lastRow = this->doc->music->ticksPerTrack;

	this->paintY = 0;
	if (firstRow < 0) {
		this->paintY = -firstRow * this->doc->fontHeight;
		// draw rectangle from 0,0 to width,this->paintY
		firstRow = 0;
	}
	unsigned int rowDraw = firstRow;
	unsigned int curRow = 0;
	for (; this->paintY < height; this->paintY += this->doc->fontHeight) {
		// Get the next event for this view
		while (
			(ev != track->end()) &&
			(curRow < rowDraw)
		) {
			curRow += ev->delay;
			ev++;
		}

		if ((rowHighlight >= rowDraw) && (rowHighlight < rowDraw + this->doc->ticksPerRow)) {
			// This row is currently being played, so highlight it
			this->pdc->SetTextBackground(*wxLIGHT_GREY);
			this->pdc->SetBackgroundMode(wxSOLID);
		} else {
			// Normal row
			this->pdc->SetTextBackground(wxNullColour);
			this->pdc->SetBackgroundMode(wxTRANSPARENT);
		}

		if (
			(ev != track->end())
			&& (curRow < rowDraw + this->doc->ticksPerRow)
		) {
			// This event occurs on the current row, use the processEvent()
			// wrapper to call this->handleEvent() with the appropriate type.
			ev->event->processEvent(ev->delay, this->trackIndex, patternIndex, this);
		} else {
			this->pdc->SetTextForeground(*wxLIGHT_GREY);
			this->pdc->DrawText(_T("--- -- -"), 0, this->paintY);
		}
		rowDraw += this->doc->ticksPerRow;
		if (rowDraw >= lastRow) break;
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

void EventPanel::handleEvent(unsigned long delay, unsigned int trackIndex,
	unsigned int patternIndex, const TempoEvent *ev)
{
	this->pdc->SetTextForeground(*wxGREEN);
	wxString txt;
	txt.Printf(_T("T-%09lu"), (unsigned long)ev->tempo.usPerTick);
	this->pdc->DrawText(txt, 0, this->paintY);
	return;
}

void EventPanel::handleEvent(unsigned long delay, unsigned int trackIndex,
	unsigned int patternIndex, const NoteOnEvent *ev)
{
	this->pdc->SetTextForeground(*wxBLACK);
	this->drawNoteOn(ev->milliHertz, ev->instrument, ev->velocity);
	return;
}

void EventPanel::handleEvent(unsigned long delay, unsigned int trackIndex,
	unsigned int patternIndex, const NoteOffEvent *ev)
{
	this->pdc->SetTextForeground(*wxBLACK);
	this->pdc->DrawText(_T("^^  -- -"), 0, this->paintY);
	return;
}

void EventPanel::handleEvent(unsigned long delay, unsigned int trackIndex,
	unsigned int patternIndex, const EffectEvent *ev)
{
	switch (ev->type) {
		case EffectEvent::PitchbendNote:
			this->pdc->SetTextForeground(*wxBLUE);
			this->drawNoteOn(ev->data, -1, -1);
			break;
		case EffectEvent::Volume:
			this->pdc->SetTextForeground(*wxBLACK);
			this->drawNoteOn(-1, -1, ev->data);
			break;
	}
	return;
}

void EventPanel::handleEvent(unsigned long delay, unsigned int trackIndex,
	unsigned int patternIndex, const GotoEvent *ev)
{
	this->pdc->SetTextForeground(*wxRED);
	this->pdc->DrawText(_T("JUMP"), 0, this->paintY);
	return;
}

void EventPanel::handleEvent(unsigned long delay, unsigned int trackIndex,
	unsigned int patternIndex, const ConfigurationEvent *ev)
{
	this->pdc->SetTextForeground(*wxCYAN);
	wxString txt;
	switch (ev->configType) {
		case ConfigurationEvent::EmptyEvent:
			this->pdc->DrawText("... -- -", 0, this->paintY);
			break;
		case ConfigurationEvent::EnableOPL3:
			txt.Append(_T("OPL3"));
			break;
		case ConfigurationEvent::EnableDeepTremolo:
			txt.Append(_T("Tremolo"));
			break;
		case ConfigurationEvent::EnableDeepVibrato:
			txt.Append(_T("Vibrato"));
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

void EventPanel::endOfTrack(unsigned long delay)
{
	return;
}

void EventPanel::endOfPattern(unsigned long delay)
{
	return;
}

void EventPanel::drawNoteOn(int milliHertz, int instrument, int velocity)
{
	double dbHertz = milliHertz / 440000.0;
	double midiNote, midiNoteFrac;
	int midiNoteInt;
	if (dbHertz > 0) {
		midiNote = 69 + 12 * log(dbHertz) / log(2.0);
	} else {
		midiNote = 0;
	}
	midiNoteInt = (int)midiNote;
	midiNoteFrac = midiNote - midiNoteInt;

	const wxChar *names[] = {_T("C-"), _T("C#"), _T("D-"), _T("D#"), _T("E-"),
		_T("F-"), _T("F#"), _T("G-"), _T("G#"), _T("A-"), _T("A#"), _T("B-")};

	wxString txt;
	if (milliHertz >= 0) {
		txt.Printf(_T("%s%1d "), names[midiNoteInt % 12], midiNoteInt / 12);
	} else {
		txt.Append(_T("--- "));
	}

	if (instrument >= 0) {
		txt.Append(wxString::Format(_T("%02X "), instrument));
	} else {
		txt.Append(_T("-- "));
	}

	if (velocity >= 0) {
		txt.Append(wxString::Format(_T("%01X"), velocity >> 4));
	} else {
		txt.Append(_T("-"));
	}
/*
	if (midiNoteFrac == 0) {
		txt.Append(_T("----"));
	} else {
		txt.Append(wxString::Format(_T("%0+4d"), (int)(255.0 * midiNoteFrac)));
	}
*/
	this->pdc->DrawText(txt, 0, this->paintY);
	return;
}
