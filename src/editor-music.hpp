/**
 * @file   editor-music.hpp
 * @brief  Music editor.
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

#ifndef _EDITOR_MUSIC_HPP_
#define _EDITOR_MUSIC_HPP_

#include "audio.hpp"
#include "editor.hpp"

class MusicEditor: public IEditor
{
	public:
		MusicEditor(IMainWindow *parent, AudioPtr audio)
			throw ();

		virtual IToolPanelVector createToolPanes() const
			throw ();

		virtual IDocument *openObject(const wxString& typeMinor,
			camoto::iostream_sptr data, const wxString& filename, SuppMap supp) const
			throw ();

	protected:
		IMainWindow *frame;
		AudioPtr audio;

};

#endif // _EDITOR_MUSIC_HPP_
