/**
 * @file   editor.cpp
 * @brief  Interface definition for a format editor.
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

#include "editor.hpp"

using namespace camoto;

IDocument::IDocument(Studio *studio, const wxString& typeMajor)
	:	wxPanel(studio),
		isModified(false),
		studio(studio),
		typeMajor(typeMajor)
{
}

const wxString& IDocument::getTypeMajor() const
{
	return this->typeMajor;
}

void IDocument::setStatusText(const wxString& text)
{
	this->studio->setStatusText(text);
	return;
}

void IDocument::setHelpText(const wxString& text)
{
	this->studio->setHelpText(text);
	return;
}

IToolPanel::IToolPanel(Studio *studio)
	:	wxPanel(studio)
{
}

IEditor::~IEditor()
{
}
