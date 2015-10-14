/**
 * @file   gamelist.hpp
 * @brief  Interface to the list of supported games.
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

#ifndef _GAMELIST_HPP_
#define _GAMELIST_HPP_

#include <vector>
#include <map>
#include <glibmm/i18n.h>
#include <glibmm/ustring.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/window.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <camoto/stream.hpp>
#include <camoto/suppitem.hpp>
#include <camoto/gamegraphics/tileset.hpp>
#include <camoto/gamemaps/map.hpp>

/// Minor type for an archive where the file offsets are listed in the XML
#define ARCHTYPE_MINOR_FIXED "fixed"

/// Minor type for a tileset where the tile positions within an image are
/// listed in the XML
#define TILESETTYPE_MINOR_FROMSPLIT "_split"

/// Minor type for a tileset where the tiles are images listed in the XML
#define TILESETTYPE_MINOR_FROMIMG "_img"

typedef std::string itemid_t;

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

/// SuppItem -> game object ID mapping.
typedef std::map<camoto::SuppItem, itemid_t> SuppIDs;

/// Types of dependent objects.
/**
 * Dependent objects are full blown object instances (like a TilesetPtr) that
 * are required by other objects (like a map needing a tileset.)  Unlike
 * supplementary items, which are just streams, dependent objects are full
 * instances of specific Camoto types.
 */
enum class DepType
{
	GenericTileset1    = camoto::gamemaps::ImagePurpose::GenericTileset1,
	GenericTileset2    = camoto::gamemaps::ImagePurpose::GenericTileset2,
	GenericTileset3    = camoto::gamemaps::ImagePurpose::GenericTileset3,
	GenericTileset4    = camoto::gamemaps::ImagePurpose::GenericTileset4,
	GenericTileset5    = camoto::gamemaps::ImagePurpose::GenericTileset5,
	GenericTileset6    = camoto::gamemaps::ImagePurpose::GenericTileset6,
	GenericTileset7    = camoto::gamemaps::ImagePurpose::GenericTileset7,
	GenericTileset8    = camoto::gamemaps::ImagePurpose::GenericTileset8,
	GenericTileset9    = camoto::gamemaps::ImagePurpose::GenericTileset9,
	BackgroundTileset1 = camoto::gamemaps::ImagePurpose::BackgroundTileset1,
	BackgroundTileset2 = camoto::gamemaps::ImagePurpose::BackgroundTileset2,
	BackgroundTileset3 = camoto::gamemaps::ImagePurpose::BackgroundTileset3,
	BackgroundTileset4 = camoto::gamemaps::ImagePurpose::BackgroundTileset4,
	BackgroundTileset5 = camoto::gamemaps::ImagePurpose::BackgroundTileset5,
	BackgroundTileset6 = camoto::gamemaps::ImagePurpose::BackgroundTileset6,
	BackgroundTileset7 = camoto::gamemaps::ImagePurpose::BackgroundTileset7,
	BackgroundTileset8 = camoto::gamemaps::ImagePurpose::BackgroundTileset8,
	BackgroundTileset9 = camoto::gamemaps::ImagePurpose::BackgroundTileset9,
	ForegroundTileset1 = camoto::gamemaps::ImagePurpose::ForegroundTileset1,
	ForegroundTileset2 = camoto::gamemaps::ImagePurpose::ForegroundTileset2,
	ForegroundTileset3 = camoto::gamemaps::ImagePurpose::ForegroundTileset3,
	ForegroundTileset4 = camoto::gamemaps::ImagePurpose::ForegroundTileset4,
	ForegroundTileset5 = camoto::gamemaps::ImagePurpose::ForegroundTileset5,
	ForegroundTileset6 = camoto::gamemaps::ImagePurpose::ForegroundTileset6,
	ForegroundTileset7 = camoto::gamemaps::ImagePurpose::ForegroundTileset7,
	ForegroundTileset8 = camoto::gamemaps::ImagePurpose::ForegroundTileset8,
	ForegroundTileset9 = camoto::gamemaps::ImagePurpose::ForegroundTileset9,
	SpriteTileset1     = camoto::gamemaps::ImagePurpose::SpriteTileset1,
	SpriteTileset2     = camoto::gamemaps::ImagePurpose::SpriteTileset2,
	SpriteTileset3     = camoto::gamemaps::ImagePurpose::SpriteTileset3,
	SpriteTileset4     = camoto::gamemaps::ImagePurpose::SpriteTileset4,
	SpriteTileset5     = camoto::gamemaps::ImagePurpose::SpriteTileset5,
	SpriteTileset6     = camoto::gamemaps::ImagePurpose::SpriteTileset6,
	SpriteTileset7     = camoto::gamemaps::ImagePurpose::SpriteTileset7,
	SpriteTileset8     = camoto::gamemaps::ImagePurpose::SpriteTileset8,
	SpriteTileset9     = camoto::gamemaps::ImagePurpose::SpriteTileset9,
	FontTileset1       = camoto::gamemaps::ImagePurpose::FontTileset1,
	FontTileset2       = camoto::gamemaps::ImagePurpose::FontTileset2,
	FontTileset3       = camoto::gamemaps::ImagePurpose::FontTileset3,
	FontTileset4       = camoto::gamemaps::ImagePurpose::FontTileset4,
	FontTileset5       = camoto::gamemaps::ImagePurpose::FontTileset5,
	FontTileset6       = camoto::gamemaps::ImagePurpose::FontTileset6,
	FontTileset7       = camoto::gamemaps::ImagePurpose::FontTileset7,
	FontTileset8       = camoto::gamemaps::ImagePurpose::FontTileset8,
	FontTileset9       = camoto::gamemaps::ImagePurpose::FontTileset9,
	BackgroundImage    = camoto::gamemaps::ImagePurpose::BackgroundImage,
	Palette,
	MaxValue // must always be last
};

/// Convert a DepType value into a string, for error messages.
std::string dep2string(DepType t);

/// Convert a DepType value into an ImagePurpose value.
camoto::gamemaps::ImagePurpose dep2purpose(DepType t);

/// Dependency type -> game object ID mapping.
typedef std::map<DepType, Glib::ustring> Deps;

/// Details about a single game object, such as a map or a song.
struct GameObject
{
	itemid_t id;           ///< Unique ID for this object
	std::string filename;  ///< Object's filename
	itemid_t idParent;     ///< ID of containing object, or empty for local file
	std::string editor;    ///< Major type (editor to use)
	std::string format;    ///< Minor type (file format)
	std::string filter;    ///< Decompression/decryption filter ID, blank for none
	Glib::ustring friendlyName; ///< Name to show user
	SuppIDs supp;          ///< SuppItem -> id mapping
	Deps dep;              ///< Which objects this one is dependent upon

	int offset;            ///< [Fixed archive only] Offset of this file
	int size;              ///< [Fixed archive only] Size of this file
};

/// Structure of a tileset defined directly in the XML, where the content is
/// from an image split into parts
struct TilesetFromSplitInfo
{
	itemid_t id;           ///< Unique ID for this object
	itemid_t idImage;      ///< ID of the underlying image to split into tiles
	unsigned int layoutWidth; ///< Ideal width of the tileset, in number of tiles
	std::vector<camoto::gamegraphics::Rect> tileList; ///< List of tile coordinates in the parent image
};

/// Map of tileset IDs to tileset data
typedef std::map<Glib::ustring, TilesetFromSplitInfo> TilesetsFromSplit;

/// Structure of a tileset defined directly in the XML, where the content is
/// from multiple images
struct TilesetFromImagesInfo
{
	itemid_t id;                       ///< Unique ID for this object
	unsigned int layoutWidth;          ///< Ideal width of the tileset, in number of tiles
	std::vector<itemid_t> ids;         ///< List of IDs for each tile
	std::vector<Glib::ustring> names;  ///< List of names for each tile
};

/// Map of tileset IDs to tileset data
typedef std::map<Glib::ustring, TilesetFromImagesInfo> TilesetsFromImages;

/// Game details for the UI
struct GameInfo
{
	itemid_t id;             ///< Game ID, used for resource filenames
	Glib::ustring title;     ///< User-visible title
	Glib::ustring developer; ///< Game dev
	Glib::ustring reverser;  ///< Who reverse engineered the file formats

	/// Process the <info/> chunk.
	void populateFromXML(xmlDoc *xml);
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
	Glib::ustring name;

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
class Game: public GameInfo
{
	public:
		/// List of game objects indexed by their XML IDs
		std::map<itemid_t, GameObject> objects;
		TilesetsFromSplit tilesetsFromSplit;
		TilesetsFromImages tilesetsFromImages;
		tree<itemid_t> treeItems;
		MapObjectVector mapObjects;
		std::map<Glib::ustring, std::string> dosCommands;

		/// Load a single game's data from its XML description file.
		/**
		 * @param id
		 *   ID of the game.
		 */
		Game(const itemid_t& id);

		/// Find an object by filename.
		const GameObject* findObjectByFilename(const std::string& filename,
			const std::string& editor) const;

		/// Find an object by its ID.
		/**
		 * This must be used instead of the array operator[], as this function
		 * won't create an empty entry when checking for an invalid ID, causing
		 * the ID to subsequently become valid.
		 */
		const GameObject* findObjectById(const itemid_t& id) const;

	protected:
//		void processFilesChunk(xmlNode *i, const Glib::ustring& idParent);
};

/// Load a list of games from the XML description files.
/**
 * @return List of all supported games.
 */
std::map<std::string, GameInfo> getAllGames();

class GameObjectInstance;

template<class T>
class GOI_Unique;

template<class T>
class GOI_Shared;

/// Class instance of whatever type is used after opening a GameObject
class GameObjectInstance
{
	public:
		enum class Type {
			Invalid,
			Image,
			Tileset,
			Map2D,
		};

		Type type;

		virtual ~GameObjectInstance() {};

		template<class T>
		std::unique_ptr<T> get_unique()
		{
			auto c = dynamic_cast<GOI_Unique<T> *>(this);
			return std::move(c->val_u);
		}

		template<class T>
		std::shared_ptr<T> get_shared()
		{
			auto c = dynamic_cast<GOI_Shared<T> *>(this);
			return c->val_s;
		}
};

template<class T>
class GOI_Unique: public GameObjectInstance
{
	public:
		virtual ~GOI_Unique() {};
		std::unique_ptr<T> val_u;
};

template<class T>
class GOI_Shared: public GameObjectInstance
{
	public:
		virtual ~GOI_Shared() {};
		std::shared_ptr<T> val_s;
};

typedef std::map<DepType, std::unique_ptr<GameObjectInstance> > DepData;

#include <camoto/gamearchive/manager.hpp>
#include <camoto/gamegraphics/manager.hpp>
#include "project.hpp"

std::unique_ptr<GameObjectInstance> openObjectGeneric(Gtk::Window* win,
	const GameObject& o, std::unique_ptr<camoto::stream::inout> content,
	camoto::SuppData& suppData, DepData* depData, Project *proj);

/// Open a Camoto object
/**
 * @param win
 *   GTK window to set as parent for warning prompts/questions.
 *
 * @param o
 *   Details about object to open.
 *
 * @param content
 *   Stream holding main file content.
 *
 * @param suppData
 *   Additional data streams as required.
 *
 * @param proj
 *   Pointer to optional project (may be null).  If present, the format handler
 *   is queried for any additional supp items, and if filenames for any are
 *   returned, the project is used to find and open them.
 */
template <class Type>
auto openObject(Gtk::Window* win,
	const GameObject& o, std::unique_ptr<camoto::stream::inout> content,
	camoto::SuppData& suppData, DepData* depData, Project *proj)
	-> decltype(((Type*)nullptr)->open(std::move(content), suppData))
{
	if (o.format.empty()) {
		throw EFailure(_("No file type was specified for this item!"));
	}

	auto fmtHandler = camoto::FormatEnumerator<Type>::byCode(o.format);
	if (!fmtHandler) {
		throw EFailure(Glib::ustring::compose(
			"%1\n\n[%2]",
			Glib::ustring::compose(
				_("Sorry, it is not possible to edit this item as the \"%1\" format "
					"is unsupported."),
				o.format.c_str()
			),
			Glib::ustring::compose(
				// Translators: %1 is the object type (Image, Tileset, etc.) and %2 is
				// the format code (e.g. img-pcx)
				_("No %1 handler for \"%2\""),
				Type::obj_t_name,
				o.format.c_str()
			)
		));
	}

	// Check to see if the file is actually in this format
	if (fmtHandler->isInstance(*content) < Type::PossiblyYes) {
		Gtk::MessageDialog dlg(*win,
			Glib::ustring::compose(
				_("This file is supposed to be in \"%1\" format, but it seems this may "
					"not be the case.  You can continue, but you may experience strange "
					"results.  If Camoto crashes when you proceed, please report it as a "
					"bug."),
				fmtHandler->friendlyName().c_str()
			),
			false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
		dlg.set_title(_("Warning"));
		if (dlg.run() != Gtk::RESPONSE_OK) return nullptr;
	}

	if (proj) {
		// Collect any supplemental files required by the format
		proj->openSuppsByFilename(win, &suppData,
			fmtHandler->getRequiredSupps(*content, o.filename));

		// Also open any dep data (full objects) needed by this format.
		proj->openDeps(win, o, suppData, depData);
	}

	try {
		return fmtHandler->open(std::move(content), suppData);
	} catch (const camoto::stream::error& e) {
		throw EFailure(Glib::ustring::compose(
			_("Camoto library exception: %1"),
			e.what()
		));
	}
}

#endif // _GAMELIST_HPP_
