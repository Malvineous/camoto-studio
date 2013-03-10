/**
 * @file   camotolibs.cpp
 * @brief  Interface to the Camoto libraries.
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

#include <boost/bind.hpp>
#include <wx/filename.h>
#include <camoto/stream.hpp>
#include <camoto/stream_file.hpp>
#include <camoto/gamearchive/fixedarchive.hpp>
#include <camoto/gamearchive/util.hpp>
#include "camotolibs.hpp"
#include "exceptions.hpp"

using namespace camoto;
using namespace camoto::gamearchive;
using namespace camoto::gamegraphics;
using namespace camoto::gamemaps;
using namespace camoto::gamemusic;

/// Callback function to set expanded/native file size.
void setRealSize(gamearchive::ArchivePtr arch,
	gamearchive::Archive::EntryPtr id, stream::len newRealSize)
{
	arch->resize(id, id->storedSize, newRealSize);
	return;
}

CamotoLibs::CamotoLibs(wxFrame *parent)
	:	mgrArchive(gamearchive::getManager()),
		mgrGraphics(gamegraphics::getManager()),
		mgrMaps(gamemaps::getManager()),
		mgrMusic(gamemusic::getManager()),
		game(NULL),
		parent(parent)
{
}

stream::inout_sptr CamotoLibs::openFile(const GameObjectPtr& o, bool useFilters)
{
	stream::inout_sptr s;

	// Open the file containing the item's data
	if (!o->idParent.empty()) {
		// This file is contained within an archive
		try {
			s = this->openFileFromArchive(o->idParent, o->filename, useFilters);
		} catch (const stream::error& e) {
			throw EFailure(wxString::Format(_("Could not open this item:\n\n%s"),
				wxString(e.what(), wxConvUTF8).c_str()));
		}
	} else {
		// This is an actual file to open
		wxFileName fn;
		fn.AssignDir(this->getGameFilesPath());
		fn.SetFullName(o->filename);

		// Make sure the filename isn't empty.  If it is, either the XML file
		// has a blank filename (not allowed) or some code has accidentally
		// accessed the object map with an invalid ID, creating an empty entry
		// for that ID which we're now trying to load.
		if (o->filename.empty()) {
			throw EFailure(_("Cannot open this item.  The game "
				"description XML file is missing the filename for this item!"));
		}

		std::cout << "[main] Opening " << fn.GetFullPath().ToAscii() << "\n";
		if (!::wxFileExists(fn.GetFullPath())) {
			throw EFailure(wxString::Format(_("Cannot open this item.  There is a "
				"file missing from the project's copy of the game data files:\n\n%s"),
				fn.GetFullPath().c_str()));
		}
		stream::file_sptr pf(new stream::file());
		try {
			pf->open(fn.GetFullPath().mb_str());
			s = pf;
		} catch (stream::open_error& e) {
			throw EFailure(wxString::Format(_("Unable to open %s\n\nReason: %s"),
				fn.GetFullPath().c_str(), wxString(e.what(), wxConvUTF8).c_str()));
		}
	}
	return s;
}

void CamotoLibs::openObject(const GameObjectPtr& o, stream::inout_sptr *stream,
	SuppData *supp)
{
	*stream = this->openFile(o, true); // true == apply filters
	assert(*stream); // should have thrown an exception on error

	if (supp) {
		// Load any supplementary files specified in the XML
		for (SuppIDs::iterator i = o->supp.begin(); i != o->supp.end(); i++) {
			stream::inout_sptr data;
			GameObjectPtr os = this->game->findObjectById(i->second);
			if (!os) {
				throw EFailure(wxString::Format(_("Cannot open this item.  It has a "
					"supplementary item in the game description XML file with an ID "
					"of \"%s\", but there is no item with this ID."),
					i->second.c_str()));
			}
			this->openObject(os, &data, NULL);
			// Save this item as the main object's supp
			(*supp)[i->first] = data;
		}
	}

	return;
}

void CamotoLibs::openSuppItems(const SuppFilenames& suppItem, SuppData *suppOut)
{
	for (SuppFilenames::const_iterator
		i = suppItem.begin(); i != suppItem.end(); i++
	) {
		if (suppOut->find(i->first) != suppOut->end()) {
			std::cout << "[camotolibs] SuppItem type #" << i->first
				<< " has already been loaded, not loading again from "
				<< i->second << ".\n"
				"[camotolibs]   This could be because a SuppItem was specified in "
				"the game description XML file and it doesn't need to be."
				<< std::endl;
			continue;
		}
		wxString filename = wxString(i->second.c_str(), wxConvUTF8);
		GameObjectPtr os = this->game->findObjectByFilename(filename, wxString());
		if (!os) {
			throw EFailure(wxString::Format(_("Cannot open this item - a "
				"required file is missing.\n\n[An entry is missing from the game "
				"description XML file for the filename \"%s\"]"),
				filename.c_str()));
		}
		stream::inout_sptr data;
		this->openObject(os, &data, NULL);
		(*suppOut)[i->first] = data;
	}
	return;
}

gamearchive::ArchivePtr CamotoLibs::getArchive(const wxString& idArchive)
{
	// See if idArchive is open
	ArchiveMap::iterator itArch = this->archives.find(idArchive);
	ArchivePtr arch;
	if (itArch == this->archives.end()) {
		// Not open, so open it, possibly recursing back here if it's inside
		// another archive

		GameObjectPtr o = this->game->findObjectById(idArchive);
		if (!o) {
			throw EFailure(wxString::Format(_("This item is supposed to be inside "
				"an archive with an ID of \"%s\", but there's no entry in the game "
				"description XML for an archive with that ID!"),
				idArchive.c_str()));
		}

		camoto::stream::inout_sptr data;
		SuppData supp;
		this->openObject(o, &data, &supp);
		assert(data);

		// Now the archive file is open, so create an Archive object around it

		// No need to check if idArchive is valid, as openObject() just did that
		if (o->typeMinor.Cmp(_T(ARCHTYPE_MINOR_FIXED)) == 0) {
			// This is a fixed archive, with its files described in the XML
			std::vector<FixedArchiveFile> items;
			for (GameObjectMap::iterator
				i = this->game->objects.begin(); i != this->game->objects.end(); i++
			) {
				if (idArchive.Cmp(i->second->idParent) == 0) {
					// This item is a subfile
					FixedArchiveFile next;
					next.offset = i->second->offset;
					next.size = i->second->size;
					next.name = i->second->filename.ToUTF8();
					// next.filter is unused
					items.push_back(next);
				}
			}
			arch = gamearchive::createFixedArchive(data, items);
		} else {
			// Normal archive file
			std::string strType(o->typeMinor.ToUTF8());
			ArchiveTypePtr pArchType(this->mgrArchive->getArchiveTypeByCode(strType));
			if (!pArchType) {
				throw EFailure(wxString::Format(_("Cannot open this item.  The "
					"archive \"%s\" is in the unsupported format \"%s\"\n\n"
					"[No handler in libgamearchive]"),
					idArchive.c_str(), o->typeMinor.c_str()));
			}

			// Collect any supplemental files required by the format
			this->openSuppItems(pArchType->getRequiredSupps(
				data, std::string(o->filename.mb_str())
			), &supp);

			try {
				arch = pArchType->open(data, supp);
			} catch (const camoto::stream::error& e) {
				throw EFailure(wxString::Format(_("Library exception opening "
					"archive \"%s\":\n\n%s"),
					idArchive.c_str(),
					wxString(e.what(), wxConvUTF8).c_str()
				));
			}
		}

		// Cache for future access
		this->archives[idArchive] = arch;
	} else {
		arch = itArch->second;
	}

	return arch;
}

camoto::stream::inout_sptr CamotoLibs::openFileFromArchive(const wxString& idArchive,
	const wxString& filename, bool useFilters)
{
	ArchivePtr arch = this->getArchive(idArchive);

	// Now we have the archive containing our file, so find and open it
	std::string nativeFilename(filename.mb_str());
	Archive::EntryPtr f = findFile(arch, nativeFilename);
	if (!f) {
		throw EFailure(wxString::Format(_("Cannot open this item.  The file "
			"\"%s\" could not be found inside the archive \"%s\""),
			filename.c_str(), idArchive.c_str()));
	}

	// Open the file
	camoto::stream::inout_sptr file = arch->open(f);
	assert(file);

	// If it has any filters, apply them
	if (useFilters && (!f->filter.empty())) {
		// The file needs to be filtered first
		FilterTypePtr pFilterType(this->mgrArchive->getFilterTypeByCode(f->filter));
		if (!pFilterType) {
			throw EFailure(wxString::Format(_("This file requires decoding, but "
				"the \"%s\" filter to do this couldn't be found (is your installed "
				"version of libgamearchive too old?)"),
				f->filter.c_str()));
		}
		try {
			file = pFilterType->apply(file,
				// Set the truncation function for the prefiltered (uncompressed) size.
				boost::bind<void>(&setRealSize, arch, f, _1)
			);
		} catch (const camoto::filter_error& e) {
			throw EFailure(wxString::Format(_("Error decoding this file: %s"),
				wxString(e.what(), wxConvUTF8).c_str()));
		}
	}

	return file;
}

gamegraphics::ImagePtr CamotoLibs::openImage(const GameObjectPtr& o)
{
	if (o->typeMinor.IsEmpty()) {
		throw EFailure(_("No file type was specified for this item!"));
	}

	std::string strType("img-");
	strType.append(o->typeMinor.ToUTF8());
	ImageTypePtr pImageType(this->mgrGraphics->getImageTypeByCode(strType));
	if (!pImageType) {
		wxString wxtype(strType.c_str(), wxConvUTF8);
		throw EFailure(wxString::Format(_("Sorry, it is not possible to edit this "
			"image as the \"%s\" format is unsupported.\n\n"
			"[No libgamegraphics handler for \"%s\"]"),
			o->typeMinor.c_str(), wxtype.c_str()));
	}

	assert(pImageType);
	std::cout << "[camotolibs] Using handler for " << pImageType->getFriendlyName() << "\n";

	// Open the main file
	camoto::stream::inout_sptr data;
	SuppData supp;
	this->openObject(o, &data, &supp);
	assert(data);

	// Check to see if the file is actually in this format
	if (pImageType->isInstance(data) < ImageType::PossiblyYes) {
		std::string friendlyType = pImageType->getFriendlyName();
		wxString wxtype(friendlyType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_("This file is supposed to be in \"%s\" "
			"format, but it seems this may not be the case.  Would you like to try "
			"opening it anyway?"), wxtype.c_str());
		wxMessageDialog dlg(this->parent, msg, _("Open item"), wxYES_NO | wxICON_ERROR);
		int r = dlg.ShowModal();
		if (r != wxID_YES) return ImagePtr();
	}

	// Collect any supplemental files required by the format
	this->openSuppItems(pImageType->getRequiredSupps(
		std::string(o->filename.mb_str())
	), &supp);

	return pImageType->open(data, supp);
}

gamegraphics::TilesetPtr CamotoLibs::openTileset(const GameObjectPtr& o)
{
	if (o->typeMinor.IsEmpty()) {
		throw EFailure(_("No file type was specified for this item!"));
	}

	if (o->typeMinor.IsSameAs(_T(TILESETTYPE_MINOR_FROMIMG))) {
		// Create a tileset from an existing image
		TilesetsFromLists::iterator ti = this->game->tilesets.find(o->id);
		if (ti == this->game->tilesets.end()) {
			// This should never happen
			throw EFailure(wxString::Format(_("Tileset \"%s\" is supposed to be "
				"created from a list, but the list doesn't seem to exist!"),
				o->id.c_str()));
		}
		TilesetInfo& tsi = ti->second;
		GameObjectPtr oImage = this->game->findObjectById(tsi.idImage);
		ImagePtr img = this->openImage(oImage);
		return gamegraphics::createTilesetFromList(tsi.tileList, img, tsi.layoutWidth);
	} else {
		std::string strType("tls-");
		strType.append(o->typeMinor.ToUTF8());
		gamegraphics::TilesetTypePtr pTilesetType(this->mgrGraphics->getTilesetTypeByCode(strType));
		if (!pTilesetType) {
			wxString wxtype(strType.c_str(), wxConvUTF8);
			throw EFailure(wxString::Format(_("Sorry, it is not possible to edit "
				"this tileset as the \"%s\" format is unsupported.\n\n"
				"[No libgamegraphics handler for \"%s\"]"),
				o->typeMinor.c_str(), wxtype.c_str()));
		}
		std::cout << "[camotolibs] Using handler for " << pTilesetType->getFriendlyName() << "\n";

		// Open the main file
		camoto::stream::inout_sptr data;
		SuppData supp;
		this->openObject(o, &data, &supp);
		assert(data);

		// Check to see if the file is actually in this format
		if (pTilesetType->isInstance(data) < gamegraphics::TilesetType::PossiblyYes) {
			std::string friendlyType = pTilesetType->getFriendlyName();
			wxString wxtype(friendlyType.c_str(), wxConvUTF8);
			wxString msg = wxString::Format(_("\"%s\" is supposed to be in \"%s\" "
				"format, but it seems this may not be the case.  Would you like to "
				"try opening it anyway?"), o->filename.c_str(), wxtype.c_str());

			wxMessageDialog dlg(this->parent, msg, _("Open item"),
				wxYES_NO | wxICON_ERROR);
			int r = dlg.ShowModal();
			if (r != wxID_YES) return gamegraphics::TilesetPtr();
		}

		// Collect any supplemental files required by the format
		this->openSuppItems(pTilesetType->getRequiredSupps(
			std::string(o->filename.mb_str())
		), &supp);

		try {
			// Open the tileset file
			gamegraphics::TilesetPtr pTileset(pTilesetType->open(data, supp));
			assert(pTileset);
			return pTileset;
		} catch (const stream::error& e) {
			throw EFailure(wxString::Format(_("Library exception: %s"),
				wxString(e.what(), wxConvUTF8).c_str()));
		}
	}
}

gamegraphics::PaletteTablePtr CamotoLibs::openPalette(const GameObjectPtr& o)
{
	if (o->typeMinor.IsEmpty()) {
		throw EFailure(_("No file type was specified for this item!"));
	}

	std::string strType("pal-");
	strType.append(o->typeMinor.ToUTF8());
	ImageTypePtr pImageType(this->mgrGraphics->getImageTypeByCode(strType));
	if (!pImageType) {
		wxString wxtype(strType.c_str(), wxConvUTF8);
		throw EFailure(wxString::Format(_("Sorry, it is not possible to open this "
			"file as the \"%s\" palette format is unsupported.\n\n"
			"[No libgamegraphics handler for \"%s\"]"),
			o->typeMinor.c_str(), wxtype.c_str()));
	}

	assert(pImageType);
	std::cout << "[camotolibs] Using handler for " << pImageType->getFriendlyName() << "\n";

	// Open the main file
	camoto::stream::inout_sptr data;
	SuppData supp;
	this->openObject(o, &data, &supp);
	assert(data);

	// Check to see if the file is actually in this format
	if (pImageType->isInstance(data) < ImageType::PossiblyYes) {
		std::string friendlyType = pImageType->getFriendlyName();
		wxString wxtype(friendlyType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_("This file (or one required by it) is "
			"supposed to be in \"%s\" format, but it seems this may not be the "
			"case.  Would you like to try opening it anyway?"), wxtype.c_str());
		wxMessageDialog dlg(this->parent, msg, _("Open item"), wxYES_NO | wxICON_ERROR);
		int r = dlg.ShowModal();
		if (r != wxID_YES) return PaletteTablePtr();
	}

	// Collect any supplemental files required by the format
	this->openSuppItems(pImageType->getRequiredSupps(
		std::string(o->filename.mb_str())
	), &supp);

	gamegraphics::ImagePtr img = pImageType->open(data, supp);
	return img->getPalette();
}

void writeMap(const MapTypePtr& mapType, const MapPtr& map,
	stream::output_sptr mapFile, SuppData suppData)
{
	mapType->write(map, mapFile, suppData);
	return;
}

gamemaps::MapPtr CamotoLibs::openMap(const GameObjectPtr& o,
	fn_write *fnWriteMap)
{
	if (o->typeMinor.IsEmpty()) {
		throw EFailure(_("No file type was specified for this item!"));
	}

	std::string strType("map-");
	strType.append(o->typeMinor.ToUTF8());
	MapTypePtr pMapType(this->mgrMaps->getMapTypeByCode(strType));
	if (!pMapType) {
		wxString wxtype(strType.c_str(), wxConvUTF8);
		throw EFailure(wxString::Format(_("Sorry, it is not possible to edit this "
			"map as the \"%s\" format is unsupported.\n\n"
			"[No libgamemaps handler for \"%s\"]"),
			o->typeMinor.c_str(), wxtype.c_str()));
	}
	std::cout << "[camotolibs] Using handler for " << pMapType->getFriendlyName() << "\n";

	// Open the main file
	camoto::stream::inout_sptr data;
	SuppData supp;
	this->openObject(o, &data, &supp);
	assert(data);

	// Check to see if the file is actually in this format
	if (pMapType->isInstance(data) < MapType::PossiblyYes) {
		std::string friendlyType = pMapType->getFriendlyName();
		wxString wxtype(friendlyType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_("This file is supposed to be in \"%s\" "
			"format, but it seems this may not be the case.  Would you like to try "
			"opening it anyway?"), wxtype.c_str());
		wxMessageDialog dlg(this->parent, msg, _("Open item"), wxYES_NO | wxICON_ERROR);
		int r = dlg.ShowModal();
		if (r != wxID_YES) return MapPtr();
	}

	// Collect any supplemental files required by the format
	this->openSuppItems(pMapType->getRequiredSupps(
		data, o->filename.mb_str().data()
	), &supp);

	// Open the map file
	try {
		MapPtr map = pMapType->open(data, supp);
		assert(map);

		if (fnWriteMap) {
			*fnWriteMap = boost::bind(&writeMap, pMapType, map, data, supp);
		}

		return map;
	} catch (const camoto::stream::error& e) {
		throw EFailure(wxString::Format(_("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}
}

void writeMusic(const MusicTypePtr& musicType, const MusicPtr& music,
	stream::output_sptr musicFile, SuppData suppData)
{
	musicFile->seekp(0, stream::start);
	musicType->write(musicFile, suppData, music, 0);
	return;
}

gamemusic::MusicPtr CamotoLibs::openMusic(const GameObjectPtr& o,
	fn_write *fnWriteMusic)
{
	if (o->typeMinor.IsEmpty()) {
		throw EFailure(_("No file type was specified for this item!"));
	}

	std::string strType;
	strType.append(o->typeMinor.ToUTF8());
	MusicTypePtr pMusicType(this->mgrMusic->getMusicTypeByCode(strType));
	if (!pMusicType) {
		wxString wxtype(strType.c_str(), wxConvUTF8);
		throw EFailure(wxString::Format(_("Sorry, it is not possible to edit this "
			"song as the \"%s\" format is unsupported.\n\n"
			"[No libgamemusic handler for \"%s\"]"),
			o->typeMinor.c_str(), wxtype.c_str()));
	}
	std::cout << "[camotolibs] Using handler for " << pMusicType->getFriendlyName() << "\n";

	// Open the main file
	camoto::stream::inout_sptr data;
	SuppData supp;
	this->openObject(o, &data, &supp);
	assert(data);

	// Check to see if the file is actually in this format
	if (pMusicType->isInstance(data) < MusicType::PossiblyYes) {
		std::string friendlyType = pMusicType->getFriendlyName();
		wxString wxtype(friendlyType.c_str(), wxConvUTF8);
		wxString msg = wxString::Format(_("This file is supposed to be in \"%s\" "
			"format, but it seems this may not be the case.  Would you like to try "
			"opening it anyway?"), wxtype.c_str());
		wxMessageDialog dlg(this->parent, msg, _("Open item"), wxYES_NO | wxICON_ERROR);
		int r = dlg.ShowModal();
		if (r != wxID_YES) return MusicPtr();
	}

	// Collect any supplemental files required by the format
	this->openSuppItems(pMusicType->getRequiredSupps(
		data, o->filename.mb_str().data()
	), &supp);

	// Open the music file
	try {
		MusicPtr music = pMusicType->read(data, supp);
		assert(music);

		if (fnWriteMusic) {
			*fnWriteMusic = boost::bind(&writeMusic, pMusicType, music, data, supp);
		}

		return music;
	} catch (const camoto::stream::error& e) {
		throw EFailure(wxString::Format(_("Library exception: %s"),
			wxString(e.what(), wxConvUTF8).c_str()));
	}
}
