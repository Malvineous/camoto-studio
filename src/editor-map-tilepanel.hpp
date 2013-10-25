/**
 * @file   editor-map-tilepanel.hpp
 * @brief  List of available tiles for the map editor.
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

#ifndef _EDITOR_MAP_TILEPANEL_HPP_
#define _EDITOR_MAP_TILEPANEL_HPP_

class TilePanel;

#include "editor.hpp"
#include "editor-map-document.hpp"
#include "editor-map-tilepanel-canvas.hpp"

/// Panel window allowing a tile to be selected from a tileset.
class TilePanel: public IToolPanel
{
	public:
		TilePanel(Studio *parent);

		virtual void getPanelInfo(wxString *id, wxString *label) const;
		virtual void switchDocument(IDocument *doc);
		virtual void loadSettings(Project *proj);
		virtual void saveSettings(Project *proj) const;

		void onZoomSmall(wxCommandEvent& ev);
		void onZoomNormal(wxCommandEvent& ev);
		void onZoomLarge(wxCommandEvent& ev);
		void onToggleGrid(wxCommandEvent& ev);
		void onIncWidth(wxCommandEvent& ev);
		void onDecWidth(wxCommandEvent& ev);
		void onIncOffset(wxCommandEvent& ev);
		void onDecOffset(wxCommandEvent& ev);

		/// Called from the layer panel when the primary layer has been changed.
		void notifyLayerChanged();

	protected:
		void setZoomFactor(int f);

		/// View settings for this editor which are saved with the project.
		struct Settings {
			int zoomFactor; ///< Amount of zoom (1,2,4)
		};
		Settings settings;

		MapDocument *doc;
		TilePanelCanvas *canvas;
		wxToolBar *tb;

		unsigned int tilesX;    ///< Number of tiles to draw before wrapping to the next row
		unsigned int offset;    ///< Number of tiles to skip drawing from the start of the tileset

		friend class TilePanelCanvas;

		enum {
			IDC_TOGGLEGRID = wxID_HIGHEST + 1,
			IDC_INC_WIDTH,
			IDC_DEC_WIDTH,
			IDC_INC_OFFSET,
			IDC_DEC_OFFSET,
		};
		DECLARE_EVENT_TABLE();
};

#endif // _EDITOR_MAP_TILEPANEL_HPP_
