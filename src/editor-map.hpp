/**
 * @file   editor-map.hpp
 * @brief  Map editor.
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

#ifndef _EDITOR_MAP_HPP_
#define _EDITOR_MAP_HPP_

#include "editor.hpp"

class MapEditor: public IEditor
{
	public:
		MapEditor(wxWindow *parent)
			throw ();

		virtual IToolPanelVector createToolPanes() const
			throw ();

		virtual IDocument *openObject(const wxString& typeMinor,
			camoto::iostream_sptr data, const wxString& filename, SuppMap supp) const
			throw ();

	protected:
		wxWindow *parent;

};

#endif // _EDITOR_MAP_HPP_
