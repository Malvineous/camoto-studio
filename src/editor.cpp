/**
 * @file   editor.cpp
 * @brief  Interface definition for a format editor.
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

#include "editor.hpp"

using namespace camoto;

IDocument::IDocument(IMainWindow *parent, const wxString& typeMajor)
	throw () :
		wxPanel(parent),
		isModified(false),
		frame(parent),
		typeMajor(typeMajor)
{
}

const wxString& IDocument::getTypeMajor() const
	throw ()
{
	return this->typeMajor;
}

void IDocument::setStatusText(const wxString& text)
	throw ()
{
	this->frame->setStatusText(text);
	return;
}

void IDocument::setHelpText(const wxString& text)
	throw ()
{
	this->frame->setHelpText(text);
	return;
}

IToolPanel::IToolPanel(IMainWindow *parent)
	throw () :
		wxPanel(parent)
{
}

IEditor::~IEditor()
	throw ()
{
}

#define SUPP_MAP(name, type) \
	s = supp.find(name); \
	if (s != supp.end()) { \
		suppData[SuppItem::type] = s->second.stream; \
	}

void suppMapToData(SuppMap& supp, SuppData &suppData)
	throw ()
{
	SuppMap::iterator s;
	SUPP_MAP(_T("dict"), Dictionary);
	SUPP_MAP(_T("fat"), FAT);
	SUPP_MAP(_T("pal"), Palette);
	SUPP_MAP(_T("instruments"), Instruments);
	SUPP_MAP(_T("layer1"), Layer1);
	SUPP_MAP(_T("layer2"), Layer2);
	SUPP_MAP(_T("extra1"), Extra1);
	SUPP_MAP(_T("extra2"), Extra2);
	SUPP_MAP(_T("extra3"), Extra3);
	SUPP_MAP(_T("extra4"), Extra4);
	SUPP_MAP(_T("extra5"), Extra5);
	return;
}
