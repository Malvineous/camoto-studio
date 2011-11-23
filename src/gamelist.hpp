/**
 * @file   gamelist.hpp
 * @brief  Interface to the list of supported games.
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

#ifndef _GAMELIST_HPP_
#define _GAMELIST_HPP_

#include <boost/shared_ptr.hpp>
#include <wx/wx.h>
#include <vector>
#include <map>

/// Minor type for an archive where the file offsets are listed in the XML
#define ARCHTYPE_MINOR_FIXED "fixed"

/// A basic tree implementation for storing the game item structure
template <typename T>
struct tree
{
	tree() throw () { };
	tree(const T& item) throw () : item(item) { };
	T item;
	typedef std::vector< tree<T> > children_t;
	children_t children;
};

/// Details about a single game object, such as a map or a song.
struct GameObject
{
	inline virtual ~GameObject() {};

	wxString id;           ///< Unique ID for this object
	wxString filename;     ///< Object's filename
	wxString idParent;     ///< ID of containing object, or empty for local file
	wxString typeMajor;    ///< Major type (editor to use)
	wxString typeMinor;    ///< Minor type (file format)
	wxString friendlyName; ///< Name to show user
	std::map<wxString, wxString> supp;  ///< SuppItem -> ID mapping

	int offset;            ///< [Fixed archive only] Offset of this file
	int size;              ///< [Fixed archive only] Size of this file
};

/// Shared pointer to a GameObject
typedef boost::shared_ptr<GameObject> GameObjectPtr;

/// Map between id and game object
typedef std::map<wxString, GameObjectPtr> GameObjectMap;

/// Game details for the UI
struct GameInfo
{
	wxString id;        ///< Game ID, used for resource filenames
	wxString title;     ///< User-visible title
	wxString developer; ///< Game dev
	wxString reverser;  ///< Who reverse engineered the file formats
};

/// Object descriptions for map editor
struct MapObject
{
	/// Run of tile codes
	typedef std::vector<unsigned int> TileRun;

	/// A row of tiles in the object.
	/**
	 * Each row (going left-to-right) starts with one or more "left" tiles,
	 * followed by one or more "mid" tiles.  The mid tile list is repeated
	 * as necessary to fill up the available space.  The row then finishes
	 * with one or more "right" tiles.
	 */
	struct Row {

		/// Array indices for segment.
		enum DirX {
			L = 0,      ///< Tiles on the left
			M = 1,      ///< Tiles in the middle, repeated
			R = 2,      ///< Tiles on the right
			Count = 3,  ///< Number of elements for array sizes
		};

		/// Actual tile runs.  Array index is a DirX value.
		TileRun segment[Count];
	};

	typedef std::vector<Row> RowVector;

	/// User-visible name of this object for tooltips, etc.
	wxString name;

	/// Minimum width for this object in tiles, or zero for no minimum.
	unsigned int minWidth;

	/// Minimum height for this object in tiles, or zero for no minimum.
	unsigned int minHeight;

	/// Maximum width for this object in tiles, or zero for no maximum.
	unsigned int maxWidth;

	/// Maximum height for this object in tiles, or zero for no maximum.
	unsigned int maxHeight;

	/// Array indices for section.
	enum DirY {
		TopSection = 0,    ///< Rows at the top of the object
		MidSection = 1,    ///< Rows in the middle of the object, repeated
		BotSection = 2,    ///< Rows at the bottom of the object
		SectionCount = 3,  ///< Number of elements for array sizes
	};

	/// List of rows for each section (top/middle/bottom).  Index is a DirY value.
	RowVector section[SectionCount];
};

typedef std::vector<MapObject> MapObjectVector;

/// All data about a game that can be edited.
struct Game: public GameInfo
{
	GameObjectMap objects;
	tree<wxString> treeItems;
	MapObjectVector mapObjects;
	std::map<wxString, wxString> dosCommands;
};

/// List of basic game info.
typedef std::map<wxString, GameInfo> GameInfoMap;

/// Load a list of games from the XML files in the given location.
/**
 * @return List of all supported games.
 */
GameInfoMap getAllGames()
	throw ();

/// Load a single game's data from the XML files in the given location.
/**
 * @param id
 *   ID of the game.
 *
 * @return Game instance containing information about the given game, or
 *   NULL on error.
 */
Game *loadGameStructure(const wxString& id)
	throw ();

#endif // _GAMELIST_HPP_
