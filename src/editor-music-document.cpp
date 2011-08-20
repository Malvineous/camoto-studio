/**
 * @file   editor-music-document.cpp
 * @brief  Music IDocument interface.
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

#include "main.hpp"
#include "editor-music-document.hpp"
#include "MusicWriter_OPLSynth.hpp"

using namespace camoto;
using namespace camoto::gamemusic;

class PlayerThread
{
	public:
		PlayerThread(MusicDocument *doc)//PatchBankPtr instruments, EventVector *events, Audio::OPLPtr opl) :
			throw () :
				doc(doc),
				playpause_lock(playpause_mutex), // initial state is locked (paused)
				doquit(false)
		{
		}

		void operator()()
		{
			MusicWriter_OPLSynth out(this->doc->opl);
			out.setPatchBank(this->doc->music->getPatchBank());
			out.start();
			EventVector::iterator pos = this->doc->events.begin();
			for (;;) {
				// Block if we've been paused.  This has to be in a control block as we
				// don't want to hold the mutex while we're blocking in the delay()
				// call, which is in processEvent().
				{
					boost::mutex::scoped_lock loop_lock(this->playpause_mutex);
				}

				// If we've just been resumed, check to see if it's because we're exiting
				if (this->doquit) break;

				try {
					// Update the playback time so the highlight gets moved
					this->doc->setHighlight((*pos)->absTime);

					(*pos++)->processEvent(&out); // Will block for song delays

					// Loop once we reach the end of the song
					if (pos == this->doc->events.end()) {
						// Notify output that the event time is about to loop back to zero
						out.finish();
						out.start();

						pos = this->doc->events.begin();
					}
				} catch (...) {
					std::cerr << "Error converting event into OPL data!  Ignoring.\n";
				}
			}
			out.finish();
			return;
		}

		void resume()
			throw ()
		{
			this->playpause_lock.unlock();
			return;
		}

		void pause()
			throw ()
		{
			this->playpause_lock.lock();
			return;
		}

		void quit()
			throw ()
		{
			this->doquit = true;
			// Wake up the thread if need be
			if (this->playpause_lock.owns_lock()) this->resume();
			return;
		}

	protected:
		MusicDocument *doc;           ///< Music data to play (and notify about progress)
		Audio::OPLPtr opl;            ///< OPL device
		boost::mutex playpause_mutex; ///< Mutex to pause playback
		boost::unique_lock<boost::mutex> playpause_lock; ///< Main play/pause lock
		bool doquit;                  ///< Set to true to make thread terminate

};


BEGIN_EVENT_TABLE(MusicDocument, IDocument)
	EVT_TOOL(IDC_SEEK_PREV, MusicDocument::onSeekPrev)
	EVT_TOOL(IDC_PLAY, MusicDocument::onPlay)
	EVT_TOOL(IDC_PAUSE, MusicDocument::onPause)
	EVT_TOOL(IDC_SEEK_NEXT, MusicDocument::onSeekNext)
	EVT_TOOL(wxID_ZOOM_IN, MusicDocument::onZoomIn)
	EVT_TOOL(wxID_ZOOM_100, MusicDocument::onZoomNormal)
	EVT_TOOL(wxID_ZOOM_OUT, MusicDocument::onZoomOut)
	EVT_MOUSEWHEEL(MusicDocument::onMouseWheel)
	EVT_SIZE(EventPanel::onResize)
END_EVENT_TABLE()

MusicDocument::MusicDocument(IMainWindow *parent, MusicReaderPtr music, AudioPtr audio)
	throw () :
		IDocument(parent, _T("music")),
		music(music),
		audio(audio),
		absTimeStart(0),
		playing(false),
		font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL)
{
	wxClientDC dc(this);
	dc.SetFont(this->font);
	dc.GetTextExtent(_T("X"), &this->fontWidth, &this->fontHeight);

	wxToolBar *tb = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
	tb->SetToolBitmapSize(wxSize(16, 16));

	tb->AddTool(IDC_SEEK_PREV, wxEmptyString,
		wxImage(::path.guiIcons + _T("gtk-media-previous-ltr.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Seek to start"),
		_T("Go back to the beginning of the song"));

	tb->AddTool(IDC_PLAY, wxEmptyString,
		wxImage(::path.guiIcons + _T("gtk-media-play-ltr.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Play"),
		_T("Start playback"));
	tb->ToggleTool(wxID_ZOOM_100, true);

	tb->AddTool(IDC_PAUSE, wxEmptyString,
		wxImage(::path.guiIcons + _T("gtk-media-pause.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Pause"),
		_T("Pause playback"));

	tb->AddTool(IDC_SEEK_NEXT, wxEmptyString,
		wxImage(::path.guiIcons + _T("gtk-media-next-ltr.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Seek to end"),
		_T("Go to the end of the song"));

	tb->AddSeparator();

	tb->AddTool(wxID_ZOOM_IN, wxEmptyString,
		wxImage(::path.guiIcons + _T("gtk-zoom-in.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Zoom in"),
		_T("Space events further apart in the event list"));

	tb->AddTool(wxID_ZOOM_100, wxEmptyString,
		wxImage(::path.guiIcons + _T("gtk-zoom-100.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Zoom normal"),
		_T("Space events as close as possible in the event list without losing detail"));
	tb->ToggleTool(wxID_ZOOM_100, true);

	tb->AddTool(wxID_ZOOM_OUT, wxEmptyString,
		wxImage(::path.guiIcons + _T("gtk-zoom-out.png"), wxBITMAP_TYPE_PNG),
		wxNullBitmap, wxITEM_NORMAL, _T("Zoom out"),
		_T("Space events closer together in the event list"));

	// Need this later, but have to do it before reading any events.
	PatchBankPtr patches = this->music->getPatchBank();

	// Read all the events into a buffer, and while we're there figure out
	// how many channels are in use and what the best value is to space
	// events evenly and as close together as possible.
	EventPtr next;
	bool channels[256];
	memset(&channels, 0, sizeof(channels));
	this->optimalTicksPerRow = 10000;
	int lastTick = 0;
	while ((next = music->readNextEvent())) {
		this->events.push_back(next);//->processEvent(&out); // Will block for song delays
		assert(next->channel < 256); // should be safe/uint8_t
		channels[next->channel] = true;

		// See if this delta time is the smallest so far
		int deltaTick = next->absTime - lastTick;
		if ((deltaTick > 0) && (deltaTick < this->optimalTicksPerRow)) this->optimalTicksPerRow = deltaTick;
		lastTick = next->absTime;
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

	this->opl = this->audio->createOPL();
	this->player = new PlayerThread(this);//patches, &this->events, this->opl);
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

	this->audio->releaseOPL(this->opl);
}

void MusicDocument::save()
	throw (std::ios::failure)
{
	throw std::ios::failure("Saving has not been implemented yet!");
}

void MusicDocument::onSeekPrev(wxCommandEvent& ev)
{
	// TODO: rewind song
	//this->music->rewind();
	this->absTimeStart = 0;
	this->pushViewSettings();
	return;
}

void MusicDocument::onPlay(wxCommandEvent& ev)
{
	// TODO
	if (this->playing) return;

	this->audio->pause(this->opl, false); // this makes delays start blocking

	// Resume playback thread
	this->player->resume();

	this->playing = true;

	return;
}

void MusicDocument::onPause(wxCommandEvent& ev)
{
	// TODO
	if (!this->playing) return;

	// Pause playback thread
	this->player->pause();

	this->audio->pause(this->opl, true); // this makes delays expire immediately
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

void MusicDocument::onMouseWheel(wxMouseEvent& ev)
{
	this->absTimeStart -= ev.m_wheelRotation / ev.m_wheelDelta *
		ev.m_linesPerAction * this->ticksPerRow;
	if (this->absTimeStart < 0) this->absTimeStart = 0;
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

void MusicDocument::setHighlight(int ticks)
	throw ()
{
	// This is called from the player thread so we can't directly use the UI

	this->playPos = ticks;
	if (this->absTimeStart + this->halfHeight < this->playPos) {
		// Scroll to keep highlight in the middle
		this->absTimeStart = this->playPos - this->halfHeight;
	}
	//this->pushViewSettings();
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
