/**
 * @file   editor-map-document.hpp
 * @brief  Map editor document.
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

#ifndef _EDITOR_MAP_DOCUMENT_HPP_
#define _EDITOR_MAP_DOCUMENT_HPP_

#include <camoto/gamemaps.hpp>
#include <camoto/gamegraphics.hpp>

class MapDocument;

#include "exceptions.hpp"
#include "editor.hpp"
#include "editor-map.hpp"
#include "editor-map-canvas.hpp"

class MapDocument: public IDocument
{
	public:
		MapDocument(Studio *parent, MapEditor::Settings *settings,
			camoto::gamemaps::Map2DPtr map, fn_write fnWriteMap,
			const camoto::gamemaps::TilesetCollectionPtr tilesets,
			const MapObjectVector *mapObjectVector);

		virtual void save();

		void onZoomSmall(wxCommandEvent& ev);
		void onZoomNormal(wxCommandEvent& ev);
		void onZoomLarge(wxCommandEvent& ev);
		void onToggleGrid(wxCommandEvent& ev);
		void onTileMode(wxCommandEvent& ev);
		void onObjMode(wxCommandEvent& ev);
		void onSetAttributes(wxCommandEvent& ev);

		void setZoomFactor(int f);

	protected:
		MapCanvas *canvas;
		MapEditor::Settings *settings;
		camoto::gamemaps::Map2DPtr map;
		fn_write fnWriteMap;
		const camoto::gamemaps::TilesetCollectionPtr tilesets;

		friend class LayerPanel;
		friend class TilePanel;

		enum {
			IDC_TOGGLEGRID = wxID_HIGHEST + 1,
			IDC_MODE_TILE,
			IDC_MODE_OBJ,
		};
		DECLARE_EVENT_TABLE();
};

#endif // _EDITOR_MAP_DOCUMENT_HPP_
