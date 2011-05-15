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

#include <wx/wx.h>
#include <vector>
#include <map>

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
	wxString id;
	wxString filename;
	wxString typeMajor;
	wxString typeMinor;
	wxString friendlyName;
	std::map<wxString, wxString> supp;  ///< SuppItem -> ID mapping
};

/// Game details for the UI
struct GameInfo
{
	wxString id;
	wxString title;
	wxString developer;
	wxString reverser;
};

/// All data about a game that can be edited.
struct Game: public GameInfo
{
	std::map<wxString, GameObject> objects;
	tree<wxString> treeItems;
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
