/**
 * @file   editor-music-document.cpp
 * @brief  Music IDocument interface.
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

#include <camoto/stream_file.hpp>
#include "editor-music-document.hpp"
#include "dlg-export-mus.hpp"
#include "dlg-import-mus.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

BEGIN_EVENT_TABLE(MusicDocument, IDocument)
	EVT_TOOL(IDC_SEEK_PREV, MusicDocument::onSeekPrev)
	EVT_TOOL(IDC_PLAY, MusicDocument::onPlay)
	EVT_TOOL(IDC_PAUSE, MusicDocument::onPause)
	EVT_TOOL(IDC_SEEK_NEXT, MusicDocument::onSeekNext)
	EVT_TOOL(wxID_ZOOM_IN, MusicDocument::onZoomIn)
	EVT_TOOL(wxID_ZOOM_100, MusicDocument::onZoomNormal)
	EVT_TOOL(wxID_ZOOM_OUT, MusicDocument::onZoomOut)
	EVT_TOOL(IDC_IMPORT, MusicDocument::onImport)
	EVT_TOOL(IDC_EXPORT, MusicDocument::onExport)
	EVT_MOUSEWHEEL(MusicDocument::onMouseWheel)
	EVT_SIZE(EventPanel::onResize)
END_EVENT_TABLE()

MusicDocument::MusicDocument(MusicEditor *editor, MusicTypePtr musicType,
	stream::inout_sptr musFile, SuppData suppData)
	throw (camoto::stream::error) :
		IDocument(editor->frame, _T("music")),
		editor(editor),
		musicType(musicType),
		musFile(musFile),
		suppData(suppData),
		playing(false),
		absTimeStart(0),
		font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL)
{
	this->music = this->musicType->read(this->musFile, this->suppData);
	assert(music);

	IMainWindow *parent = this->editor->frame;

	wxClientDC dc(this);
	dc.SetFont(this->font);
	dc.GetTextExtent(_T("X"), &this->fontWidth, &this->fontHeight);

	wxToolBar *tb = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
	tb->SetToolBitmapSize(wxSize(16, 16));

	tb->AddTool(IDC_SEEK_PREV, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::SeekPrev),
		wxNullBitmap, wxITEM_NORMAL, _T("Seek to start"),
		_T("Go back to the beginning of the song"));

	tb->AddTool(IDC_PLAY, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::Play),
		wxNullBitmap, wxITEM_NORMAL, _T("Play"),
		_T("Start playback"));
	tb->ToggleTool(wxID_ZOOM_100, true);

	tb->AddTool(IDC_PAUSE, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::Pause),
		wxNullBitmap, wxITEM_NORMAL, _T("Pause"),
		_T("Pause playback"));

	tb->AddTool(IDC_SEEK_NEXT, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::SeekNext),
		wxNullBitmap, wxITEM_NORMAL, _T("Seek to end"),
		_T("Go to the end of the song"));

	tb->AddSeparator();

	tb->AddTool(wxID_ZOOM_IN, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::ZoomIn),
		wxNullBitmap, wxITEM_NORMAL, _T("Zoom in"),
		_T("Space events further apart in the event list"));

	tb->AddTool(wxID_ZOOM_100, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::ZoomNormal),
		wxNullBitmap, wxITEM_NORMAL, _T("Zoom normal"),
		_T("Space events as close as possible in the event list without losing detail"));
	tb->ToggleTool(wxID_ZOOM_100, true);

	tb->AddTool(wxID_ZOOM_OUT, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::ZoomOut),
		wxNullBitmap, wxITEM_NORMAL, _T("Zoom out"),
		_T("Space events closer together in the event list"));

	tb->AddSeparator();

	tb->AddTool(IDC_IMPORT, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::Import),
		wxNullBitmap, wxITEM_NORMAL, _T("Import"),
		_T("Replace this song with one loaded from a file"));

	tb->AddTool(IDC_EXPORT, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::Export),
		wxNullBitmap, wxITEM_NORMAL, _T("Export"),
		_T("Save this song to a file"));

	// Figure out how many channels are in use and what the best value is to space
	// events evenly and as close together as possible.
	bool channels[256];
	memset(&channels, 0, sizeof(channels));
	this->optimalTicksPerRow = 10000;
	int lastTick = 0;
	for (EventVector::const_iterator i =
		this->music->events->begin(); i != this->music->events->end(); i++)
	{
		assert((*i)->channel < 256); // should be safe/uint8_t
		channels[(*i)->channel] = true;

		// See if this delta time is the smallest so far
		int deltaTick = (*i)->absTime - lastTick;
		if ((deltaTick > 0) && (deltaTick < this->optimalTicksPerRow)) {
			this->optimalTicksPerRow = deltaTick;
		}
		lastTick = (*i)->absTime;
	}
	this->ticksPerRow = this->optimalTicksPerRow;

	// Create and add a channel control for each channel, except for channel
	// zero (which is global for all channels)
	wxBoxSizer *szChannels = new wxBoxSizer(wxHORIZONTAL);
	for (int i = 1; i < 256; i++) {
		if (channels[i]) {
			EventPanel *p = new EventPanel(this, i);
			this->channelPanels.push_back(p);
			szChannels->Add(p, 1, wxEXPAND);
		}
	}
	wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
	s->Add(tb, 0, wxEXPAND);
	s->Add(szChannels, 1, wxEXPAND);
	this->SetSizer(s);

	this->player = new PlayerThread(this->editor->audio, this->music, this);
	this->thread = boost::thread(boost::ref(*this->player));
}

MusicDocument::~MusicDocument()
{
	// Tell the playback thread to gracefully terminate
	this->player->quit();

	// Wake the thread up if it's stuck in a delay
	this->thread.interrupt();

	// Wait for playback thread to terminate
	this->thread.join();

	// Unload everything
	delete this->player;
}

void MusicDocument::save()
	throw (camoto::stream::error)
{
	try {
		this->musFile->seekp(0, stream::start);
		this->musicType->write(this->musFile, this->suppData, this->music, 0);
		this->isModified = false;
	} catch (const format_limitation& e) {
		std::string msg("This song cannot be saved in its current form, due "
			"to a limitation imposed by the underlying file format:\n\n");
		msg.append(e.what());
		msg.append("\n\nYou will need to adjust the song accordingly and try "
			"again.");
		throw camoto::stream::error(msg);
	}
	return;
}

void MusicDocument::onSeekPrev(wxCommandEvent& ev)
{
	this->player->rewind();
	this->absTimeStart = 0;
	this->pushViewSettings();
	return;
}

void MusicDocument::onPlay(wxCommandEvent& ev)
{
	if (this->playing) return;

	// Resume playback thread
	this->player->resume();

	this->playing = true;

	return;
}

void MusicDocument::onPause(wxCommandEvent& ev)
{
	if (!this->playing) return;

	// Pause playback thread
	this->player->pause();

	this->playing = false;
	return;
}

void MusicDocument::onSeekNext(wxCommandEvent& ev)
{
	// TODO
	return;
}

void MusicDocument::onZoomIn(wxCommandEvent& ev)
{
	this->ticksPerRow /= 2;
	if (this->ticksPerRow < 1) this->ticksPerRow = 1;
	this->pushViewSettings();
	return;
}

void MusicDocument::onZoomNormal(wxCommandEvent& ev)
{
	this->ticksPerRow = this->optimalTicksPerRow;
	this->pushViewSettings();
	return;
}

void MusicDocument::onZoomOut(wxCommandEvent& ev)
{
	this->ticksPerRow *= 2;
	this->pushViewSettings();
	return;
}

void MusicDocument::onImport(wxCommandEvent& ev)
{
	DlgImportMusic importdlg(this->frame, this->editor->pManager);
	importdlg.filename = this->editor->settings.lastExportPath;
	importdlg.fileType = this->editor->settings.lastExportType;
	importdlg.setControls();

	for (;;) {
		if (importdlg.ShowModal() != wxID_OK) return;

		// Save some of the options to preset next time
		this->editor->settings.lastExportPath = importdlg.filename;
		this->editor->settings.lastExportType = importdlg.fileType;

		if (importdlg.fileType.empty()) {
			wxMessageDialog dlg(this, _T("You must select a file type."),
				_T("Import error"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			continue;
		}

		// Perform the actual import
		wxString errmsg;
		try {
			stream::input_file_sptr infile(new stream::input_file());
			infile->open(importdlg.filename.mb_str());

			MusicTypePtr pMusicInType(this->editor->pManager->getMusicTypeByCode(
				(const char *)importdlg.fileType.mb_str()));

			// We can't have a type chosen that we didn't supply in the first place
			assert(pMusicInType);

			// TODO: figure out whether we need to open any supplemental files
			// (e.g. Vinyl will need to create an external instrument file, whereas
			// Kenslab has one but won't need to use it for this.)
			SuppData inSuppData;

			MusicPtr newMusic = pMusicInType->read(infile, inSuppData);
			assert(newMusic);

			//this->music->events = newMusic->events;

			this->music = newMusic; // TODO: Does this update the player thread?
			this->isModified = true;

			// Success
			break;
		} catch (const stream::open_error& e) {
			errmsg = _T("Error reading file ");
			errmsg += importdlg.filename;
			errmsg += _T(": ");
			errmsg += wxString::FromUTF8(e.what());
		}
		wxMessageDialog dlg(this, errmsg, _T("Import error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
	return;
}

void MusicDocument::onExport(wxCommandEvent& ev)
{
	DlgExportMusic exportdlg(this->frame, this->editor->pManager);
	exportdlg.filename = this->editor->settings.lastExportPath;
	exportdlg.fileType = this->editor->settings.lastExportType;
	exportdlg.flags = this->editor->settings.lastExportFlags;
	exportdlg.setControls();

	for (;;) {
		if (exportdlg.ShowModal() != wxID_OK) return;

		// Save some of the options to preset next time
		this->editor->settings.lastExportPath = exportdlg.filename;
		this->editor->settings.lastExportType = exportdlg.fileType;
		this->editor->settings.lastExportFlags = exportdlg.flags;

		if (exportdlg.fileType.empty()) {
			wxMessageDialog dlg(this, _T("You must select a file type."),
				_T("Import error"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			continue;
		}

		// Perform the actual export
		wxString errmsg;
		stream::output_file_sptr outfile;
		try {
			outfile.reset(new stream::output_file());
			const char *outname = exportdlg.filename.mb_str();
			outfile->create(outname);

			MusicTypePtr pMusicOutType(this->editor->pManager->getMusicTypeByCode(outname));

			// We can't have a type chosen that we didn't supply in the first place
			assert(pMusicOutType);

			// TODO: figure out whether we need to open any supplemental files
			// (e.g. Vinyl will need to create an external instrument file, whereas
			// Kenslab has one but won't need to use it for this.)
			SuppData suppData;

			pMusicOutType->write(outfile, suppData, this->music, exportdlg.flags);

			// Success
			break;
		} catch (const stream::open_error& e) {
			errmsg = _T("Error creating file ");
			errmsg += exportdlg.filename;
			errmsg += _T(": ");
			errmsg += wxString::FromUTF8(e.what());
		} catch (const format_limitation& e) {
			errmsg = _T("This song cannot be exported in the selected format, due "
				"to a limitation imposed by the format itself:\n\n");
			errmsg += wxString::FromUTF8(e.what());
			errmsg += _T("\n\nYou will need to adjust the song accordingly and try "
				"again, or select a different export file format.");
		}
		if (outfile) outfile->remove(); // delete broken file
		outfile.reset(); // force file close/delete
		wxMessageDialog dlg(this, errmsg, _T("Export error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
	return;
}

void MusicDocument::onMouseWheel(wxMouseEvent& ev)
{
	unsigned int amount = ev.m_wheelRotation / ev.m_wheelDelta *
		ev.m_linesPerAction * this->ticksPerRow;
	if (amount > this->absTimeStart) this->absTimeStart = 0;
	else this->absTimeStart -= amount;
	this->pushViewSettings();
	return;
}

void MusicDocument::onResize(wxSizeEvent& ev)
{
	this->Layout();
	wxSize size = ev.GetSize();
	this->halfHeight = size.GetHeight() / this->fontHeight;
	return;
}

void MusicDocument::pushViewSettings()
{
	for (EventPanelVector::iterator i = this->channelPanels.begin();
		i != this->channelPanels.end();
		i++
	) {
		(*i)->Refresh();
	}
	return;
}

void MusicDocument::notifyPosition(unsigned long absTime)
	throw ()
{
	// This is called from the player thread so we can't directly use the UI

	this->playPos = absTime;
	if (this->absTimeStart + this->halfHeight < this->playPos) {
		// Scroll to keep highlight in the middle
		this->absTimeStart = this->playPos - this->halfHeight;
	}
	//this->pushViewSettings();
	return;
}
