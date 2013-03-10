/**
 * @file   camotolibs.hpp
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

#ifndef _CAMOTOLIBS_HPP_
#define _CAMOTOLIBS_HPP_

#include <boost/function.hpp>
#include <wx/string.h>
#include <wx/frame.h>
#include <camoto/gamearchive/manager.hpp>
#include <camoto/gamegraphics/manager.hpp>
#include <camoto/gamemaps/manager.hpp>
#include <camoto/gamemusic/manager.hpp>

class CamotoLibs;
typedef boost::function<void()> fn_write;

#include "gamelist.hpp"

/// Interface to the Camoto libraries.
class CamotoLibs
{
	public:
		camoto::gamearchive::ManagerPtr mgrArchive;   ///< Manager object for libgamearchive
		camoto::gamegraphics::ManagerPtr mgrGraphics; ///< Manager object for libgamegraphics
		camoto::gamemaps::ManagerPtr mgrMaps;         ///< Manager object for libgamemaps
		camoto::gamemusic::ManagerPtr mgrMusic;       ///< Manager object for libgamemusic

		Game *game;        ///< Game being edited in current project

		CamotoLibs(wxFrame *parent);

		/// Return the path to the game's files.
		virtual wxString getGameFilesPath() = 0;

		/// Open an object's file, without suppitems.
		camoto::stream::inout_sptr openFile(const GameObjectPtr& o,
			bool useFilters);

		/// Open an object by ID, complete with supp items.
		/**
		 * @param o
		 *   The item to open.
		 *
		 * @param stream
		 *   On return, contains the open stream for the main file holding the
		 *   requested object's data.
		 *
		 * @param supp
		 *   On return, contains open streams for each of this item's supplementary
		 *   data files.  This can be NULL if supplementary files are not required.
		 *
		 * @note If any of the supplementary files themselves have supplementary
		 *   files, they are not opened.  Only the direct supps of the object
		 *   identified by id are opened.  This is because supps are only file
		 *   streams anyway, and it's up to the final format handler to figure out
		 *   what to do with the content.
		 */
		void openObject(const GameObjectPtr& o, camoto::stream::inout_sptr *stream,
			camoto::SuppData *supp);

		/// Open each of the given supplementary items.
		void openSuppItems(const camoto::SuppFilenames& suppItem,
			camoto::SuppData *suppOut);

		/// Get an Archive instance for the given ID.
		/**
		 * This function will return the existing instance if the archive has
		 * already been opened, otherwise it will open it and return the new
		 * instance, caching it for future use.
		 *
		 * @param idArchive
		 *   ID of the archive to open.
		 *
		 * @return A shared pointer to the archive instance.
		 */
		camoto::gamearchive::ArchivePtr getArchive(const wxString& idArchive);

		camoto::stream::inout_sptr openFileFromArchive(const wxString& idArchive,
			const wxString& filename, bool useFilters);

		/// Open the image identified by the given game object.
		camoto::gamegraphics::ImagePtr openImage(const GameObjectPtr& o);

		/// Open the tileset identified by the given game object.
		camoto::gamegraphics::TilesetPtr openTileset(const GameObjectPtr& o);

		/// Open the palette identified by the given game object.
		camoto::gamegraphics::PaletteTablePtr openPalette(const GameObjectPtr& o);

		/// Open the map identified by the given game object.
		/**
		 * @param o
		 *   Object to open.
		 *
		 * @param fnWriteMap
		 *   Function to call when the map needs to be saved.
		 *
		 * @return A pointer to a Map instance for this object, or a null pointer
		 *   to abort the operation (e.g. file was in the wrong format and the user
		 *   chose not to continue).
		 *
		 * @throw EFailure
		 *   The file could not be opened or was in an invalid format.
		 *
		 * @note If the returned value is a null pointer, no error message should be
		 *   displayed as the user has chosen to abort the operation.
		 *
		 * @note This function may display a popup message to get user confirmation.
		 */
		camoto::gamemaps::MapPtr openMap(const GameObjectPtr& o,
			fn_write *fnWriteMap);

		/// Open the song identified by the given game object.
		/**
		 * @param o
		 *   Object to open.
		 *
		 * @param fnWriteMusic
		 *   Function to call when the song needs to be saved.
		 *
		 * @return A pointer to a Music instance for this object, or a null pointer
		 *   to abort the operation (e.g. file was in the wrong format and the user
		 *   chose not to continue).
		 *
		 * @throw EFailure
		 *   The file could not be opened or was in an invalid format.
		 *
		 * @note If the returned value is a null pointer, no error message should be
		 *   displayed as the user has chosen to abort the operation.
		 *
		 * @note This function may display a popup message to get user confirmation.
		 */
		camoto::gamemusic::MusicPtr openMusic(const GameObjectPtr& o,
			fn_write *fnWriteMusic);

	protected:
		wxFrame *parent;

		typedef std::map<wxString, camoto::gamearchive::ArchivePtr> ArchiveMap;
		ArchiveMap archives; ///< List of currently open archives
};

#endif // _CAMOTOLIBS_HPP_
