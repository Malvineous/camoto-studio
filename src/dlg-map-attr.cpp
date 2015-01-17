/**
 * @file   dlg-map-attr.cpp
 * @brief  Dialog box for changing map attributes.
 *
 * Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>
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

#include <wx/filename.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <camoto/gamemaps/map.hpp>
#include "main.hpp"
#include "dlg-map-attr.hpp"

using namespace camoto;
using namespace camoto::gamemaps;

/// Control IDs
enum {
	IDC_LIST = wxID_HIGHEST + 1,  // attribute list
	IDC_INT,   // this->ctInt
	IDC_TEXT,  // this->ctText
	IDC_ENUM,  // this->ctEnum
	IDC_FILE,  // this->ctFilename
};

BEGIN_EVENT_TABLE(DlgMapAttr, wxDialog)
	EVT_BUTTON(wxID_OK, DlgMapAttr::onOK)
	EVT_BUTTON(wxID_CANCEL, DlgMapAttr::onCancel)
	EVT_LISTBOX(IDC_LIST, DlgMapAttr::onAttrSelected)
END_EVENT_TABLE()

DlgMapAttr::DlgMapAttr(Studio *parent, MapPtr map)
	:	wxDialog(parent, wxID_ANY, _("Map attributes"), wxDefaultPosition,
			wxDefaultSize, wxRESIZE_BORDER),
		studio(parent),
		map(map),
		newAttr(new Map::AttributePtrVector),
		curSel(-1)
{
	Map::AttributePtrVectorPtr attributes = map->getAttributes();
	wxArrayString items;
	if (attributes) {
		for (Map::AttributePtrVector::const_iterator
			i = attributes->begin(); i != attributes->end(); i++
		) {
			Map::AttributePtr a = *i;
			items.Add(wxString(a->name.c_str(), wxConvUTF8));
			Map::AttributePtr b(new Map::Attribute);
			*b = *a;
			this->newAttr->push_back(b);
		}
	}

	wxListBox *attrList = new wxListBox(this, IDC_LIST, wxDefaultPosition,
		wxSize(150, 200), items, wxLB_SINGLE);

	// Have to create the static box *before* the controls that go inside it
	this->szControls = new wxStaticBoxSizer(wxVERTICAL, this, _("Selected attribute"));

	// First text field
	this->txtDesc = new wxStaticText(this, wxID_ANY,
		_("Please select a map attribute from the list"));
	this->szControls->Add(this->txtDesc, 0, wxEXPAND | wxALIGN_LEFT | wxALL, 4);

	this->ctText = new wxTextCtrl(this, IDC_TEXT);
	this->ctInt = new wxSpinCtrl(this, IDC_INT);
	this->ctEnum = new wxComboBox(this, IDC_ENUM, wxEmptyString, wxDefaultPosition,
		wxDefaultSize, 0, NULL, wxCB_DROPDOWN | wxCB_READONLY);
	this->ctFilename = new wxComboBox(this, IDC_FILE, wxEmptyString, wxDefaultPosition,
		wxDefaultSize, 0, NULL, wxCB_DROPDOWN);
	this->szControls->Add(this->ctText, 0, wxEXPAND | wxALIGN_LEFT | wxALIGN_TOP | wxALL, 4);
	this->szControls->Add(this->ctInt, 0, wxEXPAND | wxALIGN_LEFT | wxALIGN_TOP | wxALL, 4);
	this->szControls->Add(this->ctEnum, 0, wxEXPAND | wxALIGN_LEFT | wxALIGN_TOP | wxALL, 4);
	this->szControls->Add(this->ctFilename, 0, wxEXPAND | wxALIGN_LEFT | wxALIGN_TOP | wxALL, 4);
	this->szControls->AddStretchSpacer(1);
	this->szControls->Show(this->ctText, false);
	this->szControls->Show(this->ctInt, false);
	this->szControls->Show(this->ctEnum, false);
	this->szControls->Show(this->ctFilename, false);

	wxBoxSizer *szSplit = new wxBoxSizer(wxHORIZONTAL);
	szSplit->Add(attrList, 1, wxEXPAND | wxALL, 4);
	szSplit->Add(this->szControls, 2, wxEXPAND | wxALL, 4);

	wxBoxSizer *szMain = new wxBoxSizer(wxVERTICAL);
	szMain->Add(szSplit, 1, wxEXPAND | wxALL, 4);

	szMain->Add(new wxStaticText(this, wxID_ANY, _("You may need to close and "
				"reopen the map for any changes to take effect.")), 0, wxALL, 4);

	szMain->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0,
		wxEXPAND | wxALIGN_CENTER_VERTICAL | wxALL, 8);
	this->SetSizer(szMain);
	szMain->Fit(this);
}

DlgMapAttr::~DlgMapAttr()
{
}

void DlgMapAttr::onOK(wxCommandEvent& ev)
{
	this->saveCurrent();

	// Copy attributes back to the map
	Map::AttributePtrVectorPtr attributes = map->getAttributes();
	for (Map::AttributePtrVector::const_iterator
		d = attributes->begin(), s = this->newAttr->begin(); d != attributes->end(); d++, s++
	) {
		switch ((*d)->type) {
			case Map::Attribute::Text:
				(*d)->textValue = (*s)->textValue;
				break;
			case Map::Attribute::Integer:
				(*d)->integerValue = (*s)->integerValue;
				break;
			case Map::Attribute::Enum:
				(*d)->enumValue = (*s)->enumValue;
				break;
			case Map::Attribute::Filename:
				(*d)->filenameValue = (*s)->filenameValue;
				break;
		}
	}
	this->EndModal(wxID_OK);
	return;
}

void DlgMapAttr::onCancel(wxCommandEvent& ev)
{
	this->EndModal(wxID_CANCEL);
	return;
}

void DlgMapAttr::onAttrSelected(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	this->saveCurrent();
	this->txtDesc->SetLabel(_("Please select a map attribute from the list"));
	if (sel >= 0) {
		// Load new selection
		Map::AttributePtr& a = this->newAttr->at(sel);
		this->txtDesc->SetLabel(wxString(a->desc.c_str(), wxConvUTF8));
		switch (a->type) {
			case Map::Attribute::Text:
				this->ctText->SetValue(wxString(a->textValue.c_str(), wxConvUTF8));
				this->szControls->Show(this->ctText, true);
				break;
			case Map::Attribute::Integer: {
				int minVal = a->integerMinValue;
				int maxVal = a->integerMaxValue;
				if ((minVal == 0) && (maxVal == 0)) {
					minVal = -32768;
					maxVal = 32767;
				}
				this->ctInt->SetValue(a->integerValue);
				this->ctInt->SetRange(minVal, maxVal);
				this->szControls->Show(this->ctInt, true);
				break;
			}
			case Map::Attribute::Enum: {
				// Load enum values
				wxArrayString items;
				for (std::vector<std::string>::const_iterator
					i = a->enumValueNames.begin(); i != a->enumValueNames.end(); i++
				) {
					items.Add(wxString(i->c_str(), wxConvUTF8));
				}
				this->ctEnum->Clear();
				this->ctEnum->Append(items);
				this->ctEnum->SetSelection(a->enumValue);
				this->szControls->Show(this->ctEnum, true);
				break;
			}
			case Map::Attribute::Filename: {
				int sel = -1, j = 0;
				this->ctFilename->Clear();
				wxString validExt(a->filenameValidExtension.c_str(), wxConvUTF8);
				wxString curName(a->filenameValue.c_str(), wxConvUTF8);
				for (GameObjectMap::const_iterator
					i = this->studio->game->objects.begin(); i != this->studio->game->objects.end(); i++
				) {
					if (validExt.IsSameAs(wxFileName(i->second->filename).GetExt(), false)) {
						this->ctFilename->Append(i->second->friendlyName + _(" [")
							+ i->second->filename + _("]"), (void *)i->second.get());
						if ((sel < 0) && (curName.IsSameAs(i->second->filename, false))) sel = j;
						j++;
					}
				}
				if (sel >= 0) {
					this->ctFilename->SetSelection(sel);
				} else {
					this->ctFilename->SetValue(curName);
				}
				this->szControls->Show(this->ctFilename, true);
				break;
			}
		}
	}
	this->szControls->Layout();
	yield();
	this->szControls->Layout();
	this->curSel = sel;
	return;
}

void DlgMapAttr::saveCurrent()
{
	if (this->curSel >= 0) {
		// Save current selection
		Map::AttributePtr& a = this->newAttr->at(this->curSel);
		switch (a->type) {
			case Map::Attribute::Text: {
				a->textValue = this->ctText->GetValue().mb_str();
				this->szControls->Show(this->ctText, false);
				break;
			}
			case Map::Attribute::Integer:
				a->integerValue = this->ctInt->GetValue();
				this->szControls->Show(this->ctInt, false);
				break;
			case Map::Attribute::Enum:
				a->enumValue = ctEnum->GetSelection();
				this->szControls->Show(this->ctEnum, false);
				break;
			case Map::Attribute::Filename: {
				int sel = ctFilename->GetSelection();
				if (sel >= 0) {
					GameObject *o = (GameObject *)ctFilename->GetClientData(sel);
					assert(o);
					a->filenameValue = o->filename.mb_str();
				} else {
					// not prepopulated, but typed in
					a->filenameValue = this->ctFilename->GetValue().mb_str();
				}
				this->szControls->Show(this->ctFilename, false);
				break;
			}
		}
		this->szControls->Layout();
	}
	return;
}
