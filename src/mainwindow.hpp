/**
 * @file   mainwindow.hpp
 * @brief  Interface definition for callback functions implemented by the main
 *         window.
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

#ifndef _MAINWINDOW_HPP_
#define _MAINWINDOW_HPP_

#include <wx/frame.h>
#include <wx/glcanvas.h>
#include <wx/imaglist.h>

#include <camoto/gamearchive/manager.hpp>
#include <camoto/gamegraphics/manager.hpp>
#include <camoto/gamemaps/manager.hpp>
#include <camoto/gamemusic/manager.hpp>

/// Order of images in IMainWindow::smallImages
struct ImageListIndex
{
	enum Type {
		Folder,     ///< Directory
		File,       ///< Generic file
		Game,       ///< Game being edited
		InstMute,   ///< Muted instrument
		InstOPL,    ///< Adlib instrument
		InstMIDI,   ///< MIDI patch
		InstPCM,    ///< Sampled instrument
		SeekPrev,   ///< Media: Seek backwards
		Play,       ///< Media: Start playback
		Pause,      ///< Media: Pause playback
		SeekNext,   ///< Media: Seek forwards
		ZoomIn,     ///< Zoom closer
		ZoomNormal, ///< Zoom to 1:1
		ZoomOut,    ///< Zoom out
		Import,     ///< Import document
		Export,     ///< Export to file
	};
};

/// Base class for main window.
class IMainWindow: public wxFrame
{
	public:
		/// Shared imagelist providing 16x16 icons to all controls that need them
		wxImageList *smallImages;

		IMainWindow(wxWindow *parent, wxWindowID winid, const wxString& title,
			const wxPoint& pos = wxDefaultPosition,
			const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE,
			const wxString& name = wxFrameNameStr);

		/// Set the text in the hint part of the status bar.
		/**
		 * @param text
		 *   Text to display, or wxString() to display nothing.
		 */
		virtual void setStatusText(const wxString& text) = 0;

		/// Set the text in the keyboard help part of the status bar.
		/**
		 * @param text
		 *   Text to display, or wxString() to display nothing.
		 */
		virtual void setHelpText(const wxString& text) = 0;

		/// Get a shared OpenGL context.
		/**
		 * @return OpenGL context to share among subsequent OpenGL canvases.
		 */
		virtual wxGLContext *getGLContext() = 0;

		/// Get the OpenGL attribute list to use for all shared surfaces.
		/**
		 * This returns the attribute list used to create the shared context
		 * returned by getGLContext() so that subsequent GL canvases can make
		 * use of the shared context.  If other canvases are created with
		 * different attribute lists then they will be unable to share the
		 * context returned by getGLContext().
		 *
		 * @return Integer array suitable for passing to wxGLCanvas c'tor.
		 */
		virtual int *getGLAttributes() = 0;

		/// Get the libgamearchive interface.
		virtual camoto::gamearchive::ManagerPtr getArchiveMgr() = 0;

		/// Get the libgamegraphics interface.
		virtual camoto::gamegraphics::ManagerPtr getGraphicsMgr() = 0;

		/// Get the libgamemaps interface.
		virtual camoto::gamemaps::ManagerPtr getMapsMgr() = 0;

		/// Get the libgamemusic interface.
		virtual camoto::gamemusic::ManagerPtr getMusicMgr() = 0;
};

#endif // _MAINWINDOW_HPP_
