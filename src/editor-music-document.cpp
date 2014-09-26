/**
 * @file   editor-music-document.cpp
 * @brief  Music IDocument interface.
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
	EVT_SIZE(MusicDocument::onResize)
	EVT_COMMAND(wxID_ANY, MUSICSTREAM_UPDATE, MusicDocument::updatePlaybackStatus)
END_EVENT_TABLE()

MusicDocument::MusicDocument(MusicEditor *editor, MusicPtr music,
	fn_write fnWriteMusic)
	:	IDocument(editor->studio, _T("music")),
		editor(editor),
		music(music),
		fnWriteMusic(fnWriteMusic),
		font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL)
{
	Studio *parent = this->editor->studio;

	wxClientDC dc(this);
	dc.SetFont(this->font);
	dc.GetTextExtent(_T("X"), &this->fontWidth, &this->fontHeight);

	wxToolBar *tb = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
	tb->SetToolBitmapSize(wxSize(16, 16));

	tb->AddTool(IDC_SEEK_PREV, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::SeekPrev),
		wxNullBitmap, wxITEM_NORMAL, _("Seek to start"),
		_("Go back to the beginning of the song"));

	tb->AddTool(IDC_PLAY, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::Play),
		wxNullBitmap, wxITEM_NORMAL, _("Play"),
		_("Start playback"));
	tb->ToggleTool(wxID_ZOOM_100, true);

	tb->AddTool(IDC_PAUSE, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::Pause),
		wxNullBitmap, wxITEM_NORMAL, _("Pause"),
		_("Pause playback"));

	tb->AddTool(IDC_SEEK_NEXT, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::SeekNext),
		wxNullBitmap, wxITEM_NORMAL, _("Seek to end"),
		_("Go to the end of the song"));

	tb->AddSeparator();

	tb->AddTool(wxID_ZOOM_IN, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::ZoomIn),
		wxNullBitmap, wxITEM_NORMAL, _("Zoom in"),
		_("Space events further apart in the event list"));

	tb->AddTool(wxID_ZOOM_100, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::ZoomNormal),
		wxNullBitmap, wxITEM_NORMAL, _("Zoom normal"),
		_("Space events as close as possible in the event list without losing detail"));
	tb->ToggleTool(wxID_ZOOM_100, true);

	tb->AddTool(wxID_ZOOM_OUT, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::ZoomOut),
		wxNullBitmap, wxITEM_NORMAL, _("Zoom out"),
		_("Space events closer together in the event list"));

	tb->AddSeparator();

	tb->AddTool(IDC_IMPORT, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::Import),
		wxNullBitmap, wxITEM_NORMAL, _("Import"),
		_("Replace this song with one loaded from a file"));

	tb->AddTool(IDC_EXPORT, wxEmptyString,
		parent->smallImages->GetBitmap(ImageListIndex::Export),
		wxNullBitmap, wxITEM_NORMAL, _("Export"),
		_("Save this song to a file"));

	tb->AddSeparator();

	this->labelPlayback = new wxStaticText(tb, wxID_ANY, _("Paused"));
	tb->AddControl(this->labelPlayback);

	tb->Realize();

	// Figure out how many channels are in use and what the best value is to space
	// events evenly and as close together as possible.
	this->optimalTicksPerRow = 1;
	this->ticksPerRow = this->optimalTicksPerRow;

	// Create and add a channel control for each channel, except for channel
	// zero (which is global for all channels)
	wxBoxSizer *szChannels = new wxBoxSizer(wxHORIZONTAL);
	unsigned int trackIndex = 0;
	for (gamemusic::TrackInfoVector::iterator
		ti = music->trackInfo.begin(); ti != music->trackInfo.end(); ti++, trackIndex++
	) {
			EventPanel *p = new EventPanel(this, trackIndex);
			this->channelPanels.push_back(p);
			szChannels->Add(p, 1, wxEXPAND);
	}
	wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
	s->Add(tb, 0, wxEXPAND);
	s->Add(szChannels, 1, wxEXPAND);
	this->SetSizer(s);

	this->musicStream = this->editor->audio->addMusicStream(this->music);
	this->musicStream->notifyWindow(this);
}

MusicDocument::~MusicDocument()
{
	// Tell the playback thread to gracefully terminate
	this->editor->audio->removeStream(this->musicStream);
}

void MusicDocument::save()
{
	try {
		this->fnWriteMusic();
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
	this->musicStream->rewind();
	this->pushViewSettings();
	return;
}

void MusicDocument::onPlay(wxCommandEvent& ev)
{
	if (!this->musicStream->isPaused()) return;
	this->musicStream->pause(false);
	return;
}

void MusicDocument::onPause(wxCommandEvent& ev)
{
	if (this->musicStream->isPaused()) return;
	this->musicStream->pause(true);
	this->labelPlayback->SetLabel(_("Paused"));
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
	DlgImportMusic importdlg(this->editor->studio, this->editor->studio->mgrMusic);
	importdlg.filename = this->editor->settings.lastExportPath;
	importdlg.fileType = this->editor->settings.lastExportType;
	importdlg.setControls();

	for (;;) {
		if (importdlg.ShowModal() != wxID_OK) return;

		// Save some of the options to preset next time
		this->editor->settings.lastExportPath = importdlg.filename;
		this->editor->settings.lastExportType = importdlg.fileType;

		if (importdlg.fileType.empty()) {
			wxMessageDialog dlg(this, _("You must select a file type."),
				_("Import error"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			continue;
		}

		// Perform the actual import
		wxString errmsg;
		try {
			stream::input_file_sptr infile(new stream::input_file());
			infile->open(importdlg.filename.mb_str());

			MusicTypePtr pMusicInType(this->editor->studio->mgrMusic->getMusicTypeByCode(
				(const char *)importdlg.fileType.mb_str()));

			// We can't have a type chosen that we didn't supply in the first place
			assert(pMusicInType);

			// TODO: figure out whether we need to open any supplemental files
			// (e.g. Vinyl will need to create an external instrument file, whereas
			// Kenslab has one but won't need to use it for this.)
			SuppData inSuppData;

			MusicPtr newMusic = pMusicInType->read(infile, inSuppData);
			assert(newMusic);

			*this->music = *newMusic;
			this->isModified = true;

			// Success
			break;
		} catch (const stream::open_error& e) {
			errmsg = wxString::Format(_("Error reading file \"%s\": %s"),
				importdlg.filename.c_str(),
				wxString(e.what(), wxConvUTF8).c_str()
			);
		}
		wxMessageDialog dlg(this, errmsg, _("Import error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
	return;
}

void MusicDocument::onExport(wxCommandEvent& ev)
{
	DlgExportMusic exportdlg(this->editor->studio, this->editor->studio->mgrMusic);
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
			wxMessageDialog dlg(this, _("You must select a file type."),
				_("Export error"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			continue;
		}

		// Perform the actual export
		wxString errmsg;
		stream::output_file_sptr outfile;
		try {
			outfile.reset(new stream::output_file());
			std::string outname(exportdlg.filename.mb_str());
			outfile->create(outname);

			MusicTypePtr pMusicOutType(this->editor->studio->mgrMusic->getMusicTypeByCode(
				(const char *)exportdlg.fileType.mb_str()));

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
			errmsg = wxString::Format(_("Error creating file \"%s\": %s"),
				exportdlg.filename.c_str(),
				wxString(e.what(), wxConvUTF8).c_str()
			);
		} catch (const format_limitation& e) {
			errmsg = wxString::Format(_("This song cannot be exported in the "
				"selected format, due to a limitation imposed by the format itself:"
				"\n\n%s\n\nYou will need to adjust the song accordingly and try "
				"again, or select a different export file format."),
				wxString(e.what(), wxConvUTF8).c_str()
			);
		} catch (const std::exception& e) {
			errmsg = wxString::Format(_("Unexpected error during export:\n\n%s"),
				wxString(e.what(), wxConvUTF8).c_str()
			);
		}
		if (outfile) outfile->remove(); // delete broken file
		outfile.reset(); // force file close/delete
		wxMessageDialog dlg(this, errmsg, _("Export error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
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
	for (EventPanelVector::iterator
		i = this->channelPanels.begin(); i != this->channelPanels.end(); i++
	) {
		(*i)->Refresh();
	}
	return;
}

void MusicDocument::updatePlaybackStatus(wxCommandEvent& ev)
{
	gamemusic::Playback::Position audiblePos;
	bool audiblePosValid = false;

	{
		// This mutex protects both queuePos and waitUntil
		boost::lock_guard<boost::mutex> lock(this->musicStream->mutex_queuePos);

		PaTime streamTime = this->editor->audio->getTime();

		// Check to see whether the oldest pos has played yet
		while (!this->musicStream->queuePos.empty()) {
			MusicStream::PositionTime& nextH = this->musicStream->queuePos.front();
			if (nextH.time <= streamTime) {
				// This pos has been played now
				audiblePos = nextH.pos;
				audiblePosValid = true;
				this->musicStream->queuePos.pop();
				this->musicStream->waitUntil = 0; // wait until next event
			} else {
				// The next event hasn't happened yet, leave it for later
				//this->musicStream->waitUntil = this->queuePos.front().time;
				this->musicStream->waitUntil = nextH.time;
				break;
			}
		}
	}

	if (audiblePosValid && (audiblePos != this->lastAudiblePos)) {
		// The position being played out of the speakers has just changed
		long pattern = -1;
		if (audiblePos.order < this->music->patternOrder.size()) {
			pattern = this->music->patternOrder[audiblePos.order];
		}
		unsigned int beat = (audiblePos.row / audiblePos.tempo.ticksPerBeat)
			% audiblePos.tempo.beatsPerBar;

		wxString str = wxString::Format(
			"Order: %02X\tPattern: %02lX\tRow: %02lX\tBeat: %02X",
			audiblePos.order,
			pattern,
			audiblePos.row,
			beat);
		this->labelPlayback->SetLabel(str);
		this->lastAudiblePos = audiblePos;

		// Scroll the event list
		this->pushViewSettings();
	}

	return;
}
