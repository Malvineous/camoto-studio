/**
 * @file   gamelist.cpp
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

#include <iostream>
#include <glibmm/fileutils.h>
#include <glibmm/i18n.h>
#include <glibmm/pattern.h>
#include <glibmm/ustring.h>
#include <camoto/util.hpp> // make_unique
#include <camoto/gamegraphics/imagetype.hpp>
#include <camoto/gamegraphics/tilesettype.hpp>
#include <camoto/gamemaps/maptype.hpp>
#include <camoto/gamemaps/map2d.hpp>
#include "main.hpp"
#include "gamelist.hpp"

#define _X(a)  (const xmlChar *)(a)

using namespace camoto;
using namespace camoto::gamegraphics;
using namespace camoto::gamemaps;

std::string dep2string(DepType t)
{
	switch (t) {
		case DepType::GenericTileset1:    return "generic-tileset1";
		case DepType::GenericTileset2:    return "generic-tileset2";
		case DepType::GenericTileset3:    return "generic-tileset3";
		case DepType::GenericTileset4:    return "generic-tileset4";
		case DepType::GenericTileset5:    return "generic-tileset5";
		case DepType::GenericTileset6:    return "generic-tileset6";
		case DepType::GenericTileset7:    return "generic-tileset7";
		case DepType::GenericTileset8:    return "generic-tileset8";
		case DepType::GenericTileset9:    return "generic-tileset9";
		case DepType::BackgroundTileset1: return "background-tileset1";
		case DepType::BackgroundTileset2: return "background-tileset2";
		case DepType::BackgroundTileset3: return "background-tileset3";
		case DepType::BackgroundTileset4: return "background-tileset4";
		case DepType::BackgroundTileset5: return "background-tileset5";
		case DepType::BackgroundTileset6: return "background-tileset6";
		case DepType::BackgroundTileset7: return "background-tileset7";
		case DepType::BackgroundTileset8: return "background-tileset8";
		case DepType::BackgroundTileset9: return "background-tileset9";
		case DepType::ForegroundTileset1: return "foreground-tileset1";
		case DepType::ForegroundTileset2: return "foreground-tileset2";
		case DepType::ForegroundTileset3: return "foreground-tileset3";
		case DepType::ForegroundTileset4: return "foreground-tileset4";
		case DepType::ForegroundTileset5: return "foreground-tileset5";
		case DepType::ForegroundTileset6: return "foreground-tileset6";
		case DepType::ForegroundTileset7: return "foreground-tileset7";
		case DepType::ForegroundTileset8: return "foreground-tileset8";
		case DepType::ForegroundTileset9: return "foreground-tileset9";
		case DepType::SpriteTileset1:     return "sprite-tileset1";
		case DepType::SpriteTileset2:     return "sprite-tileset2";
		case DepType::SpriteTileset3:     return "sprite-tileset3";
		case DepType::SpriteTileset4:     return "sprite-tileset4";
		case DepType::SpriteTileset5:     return "sprite-tileset5";
		case DepType::SpriteTileset6:     return "sprite-tileset6";
		case DepType::SpriteTileset7:     return "sprite-tileset7";
		case DepType::SpriteTileset8:     return "sprite-tileset8";
		case DepType::SpriteTileset9:     return "sprite-tileset9";
		case DepType::FontTileset1:       return "font-tileset1";
		case DepType::FontTileset2:       return "font-tileset2";
		case DepType::FontTileset3:       return "font-tileset3";
		case DepType::FontTileset4:       return "font-tileset4";
		case DepType::FontTileset5:       return "font-tileset5";
		case DepType::FontTileset6:       return "font-tileset6";
		case DepType::FontTileset7:       return "font-tileset7";
		case DepType::FontTileset8:       return "font-tileset8";
		case DepType::FontTileset9:       return "font-tileset9";
		case DepType::BackgroundImage:    return "background-image";
		case DepType::Palette:            return "palette";
		case DepType::MaxValue:
			return "<invalid value: MaxValue>";
			break;
		//default: // no default so GCC complains if we miss one
	}
	return "<invalid value>";
}

ImagePurpose dep2purpose(DepType t)
{
	if (t <= DepType::BackgroundImage) return (ImagePurpose)t;
	return ImagePurpose::ImagePurposeCount; // invalid value
}

void GameInfo::populateFromXML(xmlDoc *xml)
{
	xmlNode *root = xmlDocGetRootElement(xml);
	xmlChar *title = xmlGetProp(root, _X("title"));
	if (title) {
		this->title = Glib::ustring((char *)title, xmlStrlen(title));
		xmlFree(title);
	}

	for (xmlNode *i = root->children; i; i = i->next) {
		if (xmlStrcmp(i->name, _X("info")) == 0) {
			for (xmlNode *j = i->children; j; j = j->next) {
				if (xmlStrEqual(j->name, _X("developer"))) {
					xmlChar *content = xmlNodeGetContent(j);
					this->developer = Glib::ustring((const char *)content, xmlStrlen(content));
					xmlFree(content);
				}
				if (xmlStrEqual(j->name, _X("reverser"))) {
					xmlChar *content = xmlNodeGetContent(j);
					this->reverser = Glib::ustring((const char *)content, xmlStrlen(content));
					xmlFree(content);
				}
			}
		}
	}
	return;
}

const GameObject* Game::findObjectByFilename(const std::string& filename,
	const std::string& editor) const
{
	for (auto& i : this->objects) {
		if (filename.compare(i.second.filename) == 0) {
			if (editor.empty() || (editor.compare(i.second.editor))) {
				return &i.second;
			}
		}
	}
	return nullptr;
}

const GameObject* Game::findObjectById(const itemid_t& id) const
{
	auto io = this->objects.find(id);
	if (io == this->objects.end()) return nullptr;
	return &io->second;
}

std::map<std::string, GameInfo> getAllGames()
{
	std::map<std::string, GameInfo> games;

	Glib::Dir dir(::path.gameData);
	Glib::PatternSpec spec("*.xml");
	for (const auto& i : dir) {
		if (!spec.match(i)) continue; // skip non XML files
		auto n = Glib::build_filename(::path.gameData, i);
		std::cout << "[gamelist] Parsing " << n << "\n";
		xmlDoc *xml = xmlParseFile(n.c_str());
		if (!xml) {
			std::cout << "[gamelist] Error parsing " << n << std::endl;
		} else {
			GameInfo gi;
			// ID is the base filename.  We can just chop off the last four characters
			// to remove ".xml" because we're only here if the filename ends in ".xml"
			gi.id = i.substr(0, i.length() - 4);
			gi.populateFromXML(xml);
			games[gi.id] = gi;
			xmlFreeDoc(xml);
		}
	}
	return games;
}

/// Recursively process the <display/> chunk.
void populateDisplay(xmlNode *n, tree<itemid_t>& t)
{
	for (xmlNode *i = n->children; i; i = i->next) {
		if (xmlStrcmp(i->name, _X("item")) == 0) {
			xmlChar *val = xmlGetProp(i, _X("ref"));
			if (val) {
				t.children.push_back(itemid_t((const char *)val, xmlStrlen(val)));
				xmlFree(val);
			}
		} else if (xmlStrcmp(i->name, _X("group")) == 0) {
			xmlChar *val = xmlGetProp(i, _X("name"));
			if (val) {
				tree<itemid_t> group(itemid_t((const char *)val, xmlStrlen(val)));
				xmlFree(val);
				populateDisplay(i, group);
				t.children.push_back(group);
			}
		}
	}
	return;
}

std::vector<camoto::gamegraphics::Rect> processTilesetFromSplitChunk(xmlNode *i)
{
	std::vector<camoto::gamegraphics::Rect> tileList;

	for (xmlNode *j = i->children; j; j = j->next) {
		if (!xmlStrEqual(j->name, _X("image"))) continue;

		camoto::gamegraphics::Rect tp;
		for (xmlAttr *a = j->properties; a; a = a->next) {
			xmlChar *val = xmlNodeGetContent(a->children);
			auto intval = strtod((const char *)val, NULL);
			xmlFree(val);
			if (xmlStrcmp(a->name, _X("x")) == 0) tp.x = intval;
			else if (xmlStrcmp(a->name, _X("y")) == 0) tp.y = intval;
			else if (xmlStrcmp(a->name, _X("width")) == 0) tp.width = intval;
			else if (xmlStrcmp(a->name, _X("height")) == 0) tp.height = intval;
		}
		tileList.push_back(tp);
	}
	return tileList;
}

void processTilesetFromImagesChunk(xmlNode *i, TilesetFromImagesInfo *tii)
{
	for (xmlNode *j = i->children; j; j = j->next) {
		if (!xmlStrEqual(j->name, _X("item"))) continue;

		std::string id, name;
		for (xmlAttr *a = j->properties; a; a = a->next) {
			if (xmlStrcmp(a->name, _X("ref")) == 0) {
				xmlChar *val = xmlNodeGetContent(a->children);
				id = std::string((const char *)val, xmlStrlen(val));
				xmlFree(val);
			} else if (xmlStrcmp(a->name, _X("title")) == 0) {
				xmlChar *val = xmlNodeGetContent(a->children);
				name = std::string((const char *)val, xmlStrlen(val));
				xmlFree(val);
			}
		}
		tii->ids.push_back(id);
		tii->names.push_back(name);
	}
	return;
}

void processFilesChunk(Game *g, xmlNode *i, const Glib::ustring& idParent)
{
	for (xmlNode *j = i->children; j; j = j->next) {
		bool isFileTag = xmlStrEqual(j->name, _X("file"));
		bool isArchiveTag = isFileTag ? false : xmlStrEqual(j->name, _X("archive"));
		bool isTilesetTag = (isFileTag || isArchiveTag) ? false : xmlStrEqual(j->name, _X("tileset"));
		if (isFileTag || isArchiveTag || isTilesetTag) {
			GameObject o;
			xmlChar *val = xmlNodeGetContent(j);
			o.idParent = idParent;
			xmlFree(val);
			Glib::ustring strImage;
			unsigned int layoutWidth = 0;
			for (xmlAttr *a = j->properties; a; a = a->next) {
				xmlChar *val = xmlNodeGetContent(a->children);
				if (xmlStrcmp(a->name, _X("id")) == 0) {
					o.id = Glib::ustring((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("title")) == 0) {
					o.friendlyName = Glib::ustring((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("editor")) == 0) {
					o.editor = Glib::ustring((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("format")) == 0) {
					o.format = Glib::ustring((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("filter")) == 0) {
					o.filter = Glib::ustring((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("name")) == 0) {
					o.filename = Glib::ustring((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("offset")) == 0) {
					o.offset = strtod((const char *)val, NULL);
				} else if (xmlStrcmp(a->name, _X("size")) == 0) {
					o.size = strtod((const char *)val, NULL);
				} else if (xmlStrcmp(a->name, _X("image")) == 0) {
					strImage = Glib::ustring((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("layoutWidth")) == 0) {
					layoutWidth = strtod((const char *)val, NULL);
				}
				xmlFree(val);
			}

			if (isArchiveTag) {
				o.editor = "archive";
				o.friendlyName = o.filename;
				processFilesChunk(g, j, o.id);
			} else if (isTilesetTag) {
				o.editor = "tileset";
				if (strImage.empty()) {
					// This is a tileset composed of multiple images
					o.format = TILESETTYPE_MINOR_FROMIMG;
					TilesetFromImagesInfo tii;
					tii.layoutWidth = layoutWidth;
					processTilesetFromImagesChunk(j, &tii);
					g->tilesetsFromImages[o.id] = tii;
				} else {
					// This is a tileset made by splitting an image into parts
					o.format = TILESETTYPE_MINOR_FROMSPLIT;
					TilesetFromSplitInfo tsi;
					tsi.idImage = strImage;
					tsi.layoutWidth = layoutWidth;
					tsi.tileList = processTilesetFromSplitChunk(j);
					g->tilesetsFromSplit[o.id] = tsi;
				}
			}

			// Look for supp or dep child nodes
			for (xmlNode *k = j->children; k; k = k->next) {
				bool isSuppTag = xmlStrEqual(k->name, _X("supp"));
				bool isDepTag = isSuppTag ? false : xmlStrEqual(k->name, _X("dep"));
				if (!isSuppTag && !isDepTag) continue;

				Glib::ustring sdRef; // id of supp or dep tag
				Glib::ustring sdType;
				for (xmlAttr *a = k->properties; a; a = a->next) {
					xmlChar *val = xmlNodeGetContent(a->children);
					if (xmlStrcmp(a->name, _X("ref")) == 0) {
						sdRef = Glib::ustring((const char *)val, xmlStrlen(val));
					} else if (xmlStrcmp(a->name, _X("reftype")) == 0) {
						sdType = Glib::ustring((const char *)val, xmlStrlen(val));
					}
					xmlFree(val);
				}
				if (isSuppTag) {
					// Convert attribute name into SuppItem type
					auto suppType = SuppItem::MaxValue;
					     if (sdType.compare("dictionary" ) == 0) suppType = SuppItem::Dictionary;
					else if (sdType.compare("fat"        ) == 0) suppType = SuppItem::FAT;
					else if (sdType.compare("palette"    ) == 0) suppType = SuppItem::Palette;
					else if (sdType.compare("instruments") == 0) suppType = SuppItem::Instruments;
					else if (sdType.compare("layer1"     ) == 0) suppType = SuppItem::Layer1;
					else if (sdType.compare("layer2"     ) == 0) suppType = SuppItem::Layer2;
					else if (sdType.compare("layer3"     ) == 0) suppType = SuppItem::Layer3;
					else if (sdType.compare("extra1"     ) == 0) suppType = SuppItem::Extra1;
					else if (sdType.compare("extra2"     ) == 0) suppType = SuppItem::Extra2;
					else if (sdType.compare("extra3"     ) == 0) suppType = SuppItem::Extra3;
					else if (sdType.compare("extra4"     ) == 0) suppType = SuppItem::Extra4;
					else if (sdType.compare("extra5"     ) == 0) suppType = SuppItem::Extra5;
					else {
						std::cout << "[gamelist] Invalid supplementary type \""
							<< sdType << "\"" << std::endl;
					}
					if (suppType != SuppItem::MaxValue) {
						o.supp[suppType] = sdRef;
					}
				} else if (isDepTag) {
					// Convert attribute name into DepType
					DepType depType = (DepType)-1;
					     if (sdType.compare("generic-tileset1"   ) == 0) depType = DepType::GenericTileset1;
					else if (sdType.compare("generic-tileset2"   ) == 0) depType = DepType::GenericTileset2;
					else if (sdType.compare("generic-tileset3"   ) == 0) depType = DepType::GenericTileset3;
					else if (sdType.compare("generic-tileset4"   ) == 0) depType = DepType::GenericTileset4;
					else if (sdType.compare("generic-tileset5"   ) == 0) depType = DepType::GenericTileset5;
					else if (sdType.compare("generic-tileset6"   ) == 0) depType = DepType::GenericTileset6;
					else if (sdType.compare("generic-tileset7"   ) == 0) depType = DepType::GenericTileset7;
					else if (sdType.compare("generic-tileset8"   ) == 0) depType = DepType::GenericTileset8;
					else if (sdType.compare("generic-tileset9"   ) == 0) depType = DepType::GenericTileset9;
					else if (sdType.compare("background-tileset1") == 0) depType = DepType::BackgroundTileset1;
					else if (sdType.compare("background-tileset2") == 0) depType = DepType::BackgroundTileset2;
					else if (sdType.compare("background-tileset3") == 0) depType = DepType::BackgroundTileset3;
					else if (sdType.compare("background-tileset4") == 0) depType = DepType::BackgroundTileset4;
					else if (sdType.compare("background-tileset5") == 0) depType = DepType::BackgroundTileset5;
					else if (sdType.compare("background-tileset6") == 0) depType = DepType::BackgroundTileset6;
					else if (sdType.compare("background-tileset7") == 0) depType = DepType::BackgroundTileset7;
					else if (sdType.compare("background-tileset8") == 0) depType = DepType::BackgroundTileset8;
					else if (sdType.compare("background-tileset9") == 0) depType = DepType::BackgroundTileset9;
					else if (sdType.compare("foreground-tileset1") == 0) depType = DepType::ForegroundTileset1;
					else if (sdType.compare("foreground-tileset2") == 0) depType = DepType::ForegroundTileset2;
					else if (sdType.compare("foreground-tileset3") == 0) depType = DepType::ForegroundTileset3;
					else if (sdType.compare("foreground-tileset4") == 0) depType = DepType::ForegroundTileset4;
					else if (sdType.compare("foreground-tileset5") == 0) depType = DepType::ForegroundTileset5;
					else if (sdType.compare("foreground-tileset6") == 0) depType = DepType::ForegroundTileset6;
					else if (sdType.compare("foreground-tileset7") == 0) depType = DepType::ForegroundTileset7;
					else if (sdType.compare("foreground-tileset8") == 0) depType = DepType::ForegroundTileset8;
					else if (sdType.compare("foreground-tileset9") == 0) depType = DepType::ForegroundTileset9;
					else if (sdType.compare("sprite-tileset1"    ) == 0) depType = DepType::SpriteTileset1;
					else if (sdType.compare("sprite-tileset2"    ) == 0) depType = DepType::SpriteTileset2;
					else if (sdType.compare("sprite-tileset3"    ) == 0) depType = DepType::SpriteTileset3;
					else if (sdType.compare("sprite-tileset4"    ) == 0) depType = DepType::SpriteTileset4;
					else if (sdType.compare("sprite-tileset5"    ) == 0) depType = DepType::SpriteTileset5;
					else if (sdType.compare("sprite-tileset6"    ) == 0) depType = DepType::SpriteTileset6;
					else if (sdType.compare("sprite-tileset7"    ) == 0) depType = DepType::SpriteTileset7;
					else if (sdType.compare("sprite-tileset8"    ) == 0) depType = DepType::SpriteTileset8;
					else if (sdType.compare("sprite-tileset9"    ) == 0) depType = DepType::SpriteTileset9;
					else if (sdType.compare("font-tileset1"      ) == 0) depType = DepType::FontTileset1;
					else if (sdType.compare("font-tileset2"      ) == 0) depType = DepType::FontTileset2;
					else if (sdType.compare("font-tileset3"      ) == 0) depType = DepType::FontTileset3;
					else if (sdType.compare("font-tileset4"      ) == 0) depType = DepType::FontTileset4;
					else if (sdType.compare("font-tileset5"      ) == 0) depType = DepType::FontTileset5;
					else if (sdType.compare("font-tileset6"      ) == 0) depType = DepType::FontTileset6;
					else if (sdType.compare("font-tileset7"      ) == 0) depType = DepType::FontTileset7;
					else if (sdType.compare("font-tileset8"      ) == 0) depType = DepType::FontTileset8;
					else if (sdType.compare("font-tileset9"      ) == 0) depType = DepType::FontTileset9;
					else if (sdType.compare("background-image"   ) == 0) depType = DepType::BackgroundImage;
					else if (sdType.compare("palette"            ) == 0) depType = DepType::Palette;
					else {
						std::cout << "[gamelist] Invalid dependent object type \""
							<< sdType << "\"" << std::endl;
					}
					if (depType != (DepType)-1) {
						o.dep[depType] = sdRef;
					}
				} // else ignore unknown tag
			}

			if (o.id.empty()) {
				std::cout << "[gamelist] Got a <" << j->name
					<< "/> with no 'id' attribute (" << o.friendlyName
					<< ")\n";
			} else {
				bool missingTypeMajor = o.editor.empty();
				bool missingTypeMinor = o.format.empty();
				bool missingFilename = o.filename.empty() && (isFileTag || isArchiveTag);
				if (missingTypeMajor || missingTypeMinor || missingFilename) {
					std::cout << "[gamelist] <" << (const char *)j->name
						<< "/> with id \"" << o.id
						<< "\" is missing attribute(s): ";
					if (missingTypeMajor) std::cout << "editor ";
					if (missingTypeMinor) std::cout << "format ";
					if (missingFilename) std::cout << "filename ";
					std::cout << "\n";
				}
				auto io = g->objects.find(o.id);
				if (io != g->objects.end()) {
					std::cerr << "[gamelist] <" << (const char *)j->name
						<< "/> with duplicate id: \"" << o.id << "\"\n";
				} else {
					g->objects[o.id] = o;
				}
			}
		}
	}
	return;
}

Game::Game(const itemid_t& id)
{
	this->id = id;

	auto n = Glib::build_filename(::path.gameData, id + ".xml");

	std::cout << "[gamelist] Parsing " << n << "\n";
	xmlDoc *xml = xmlParseFile(n.c_str());
	if (!xml) {
		throw EFailure(Glib::ustring::compose(
			// Translators: %1 is the filename of the offending XML file
			_("Error parsing game description XML file: %1"),
			n.c_str()
		));
	}

	this->GameInfo::populateFromXML(xml);

	xmlNode *root = xmlDocGetRootElement(xml);
	for (xmlNode *i = root->children; i; i = i->next) {
		if (xmlStrcmp(i->name, _X("display")) == 0) {
			populateDisplay(i, this->treeItems);
		} else if (xmlStrcmp(i->name, _X("files")) == 0) {
			// Process the <files/> chunk
			processFilesChunk(this, i, {});
		} else if (xmlStrcmp(i->name, _X("commands")) == 0) {
			// Process the <commands/> chunk
			for (xmlNode *j = i->children; j; j = j->next) {
				if (xmlStrEqual(j->name, _X("command"))) {
					Glib::ustring title;
					for (xmlAttr *a = j->properties; a; a = a->next) {
						if (xmlStrcmp(a->name, _X("title")) == 0) {
							xmlChar *val = xmlNodeGetContent(a->children);
							title = Glib::ustring((const char *)val, xmlStrlen(val));
							xmlFree(val);
						}
					}
					if (title.empty()) {
						std::cerr << "[gamelist] Game \"" << id
							<< "\" has a <command/> with no title attribute." << std::endl;
					} else {
						xmlChar *val = xmlNodeGetContent(j);
						this->dosCommands[title] = Glib::ustring((const char *)val, xmlStrlen(val));
						xmlFree(val);
					}
				}
			}
		} else if (xmlStrcmp(i->name, _X("map")) == 0) {
			// Process the <map/> chunk
			for (xmlNode *j = i->children; j; j = j->next) {
				if (xmlStrEqual(j->name, _X("objects"))) {
					for (xmlNode *k = j->children; k; k = k->next) {
						if (xmlStrEqual(k->name, _X("object"))) {
							MapObject o;

							// Set the defaults to no limits
							o.minWidth = 0;
							o.minHeight = 0;
							o.maxWidth = 0;
							o.maxHeight = 0;

							for (xmlAttr *a = k->properties; a; a = a->next) {
								if (xmlStrcmp(a->name, _X("name")) == 0) {
									xmlChar *val = xmlNodeGetContent(a->children);
									o.name = Glib::ustring((const char *)val, xmlStrlen(val));
									xmlFree(val);
								}
							}
							for (xmlNode *l = k->children; l; l = l->next) {
								if (xmlStrEqual(l->name, _X("min"))) {
									for (xmlAttr *a = l->properties; a; a = a->next) {
										if (xmlStrcmp(a->name, _X("width")) == 0) {
											xmlChar *val = xmlNodeGetContent(a->children);
											o.minWidth = strtod((const char *)val, NULL);
											xmlFree(val);
										} else if (xmlStrcmp(a->name, _X("height")) == 0) {
											xmlChar *val = xmlNodeGetContent(a->children);
											o.minHeight = strtod((const char *)val, NULL);
											xmlFree(val);
										}
									}
								} else if (xmlStrEqual(l->name, _X("max"))) {
									for (xmlAttr *a = l->properties; a; a = a->next) {
										if (xmlStrcmp(a->name, _X("width")) == 0) {
											xmlChar *val = xmlNodeGetContent(a->children);
											o.maxWidth = strtod((const char *)val, NULL);
											xmlFree(val);
										} else if (xmlStrcmp(a->name, _X("height")) == 0) {
											xmlChar *val = xmlNodeGetContent(a->children);
											o.maxHeight = strtod((const char *)val, NULL);
											xmlFree(val);
										}
									}
								} else {
									// See if this child element is <top>, <mid> or <bot>
									MapObject::RowVector *section = NULL;
									if (xmlStrEqual(l->name, _X("top"))) section = &o.section[MapObject::TopSection];
									else if (xmlStrEqual(l->name, _X("mid"))) section = &o.section[MapObject::MidSection];
									else if (xmlStrEqual(l->name, _X("bot"))) section = &o.section[MapObject::BotSection];

									// If it was, enumerate its children
									if (section) {
										for (xmlNode *m = l->children; m; m = m->next) {
											if (xmlStrEqual(m->name, _X("row"))) {
												MapObject::Row newRow;
												for (xmlNode *n = m->children; n; n = n->next) {
													MapObject::TileRun *tiles;

													// See if this element is <l>, <m> or <r>
													tiles = NULL;
													if (xmlStrEqual(n->name, _X("l"))) tiles = &newRow.segment[MapObject::Row::L];
													else if (xmlStrEqual(n->name, _X("m"))) tiles = &newRow.segment[MapObject::Row::M];
													else if (xmlStrEqual(n->name, _X("r"))) tiles = &newRow.segment[MapObject::Row::R];

													// If it was, load the tile codes from the <tile/> tags
													if (tiles) {
														for (xmlNode *o = n->children; o; o = o->next) {
															if (xmlStrEqual(o->name, _X("tile"))) {
																for (xmlAttr *a = o->properties; a; a = a->next) {
																	if (xmlStrcmp(a->name, _X("id")) == 0) {
																		xmlChar *val = xmlNodeGetContent(a->children);
																		int code = strtod((const char *)val, NULL);
																		tiles->push_back(code);
																		xmlFree(val);
																	}
																} // for <tile/> attributes
															}
														} // for <l/>, <m/> or <r/> children
													}

												} // for <row/> children

												// Add this <row> back to the <top>, <mid> or <bot>
												section->push_back(newRow);
											}
										} // for <top/>, <mid/> or <bot/> children
									}
								}
							} // for <object/> children
							this->mapObjects.push_back(o);
						}
					} // for <objects/> children
				}
			} // for <map/> children
		}
	}
	xmlFreeDoc(xml);
}

std::unique_ptr<GameObjectInstance> openObjectGeneric(Gtk::Window* win,
	const GameObject& o, std::unique_ptr<camoto::stream::inout> content,
	camoto::SuppData& suppData, DepData* depData, Project *proj)
{
	if ((o.editor.compare("image") == 0)
		|| (o.editor.compare("palette") == 0)
	) {
		auto i = std::make_unique<GOI_Unique<Image>>();
		i->type = GameObjectInstance::Type::Image;
		i->val_u = ::openObject<ImageType>(win, o, std::move(content),
			suppData, depData, proj);
		return i;

	} else if (o.editor.compare("tileset") == 0) {
		auto i = std::make_unique<GOI_Shared<Tileset>>();
		i->type = GameObjectInstance::Type::Tileset;
		i->val_s = ::openObject<TilesetType>(win, o, std::move(content),
			suppData, depData, proj);
		return i;

	} else if (o.editor.compare("map2d") == 0) {
		auto i = std::make_unique<GOI_Unique<Map2D>>();
		i->type = GameObjectInstance::Type::Map2D;
		auto mapObj = ::openObject<MapType>(win, o, std::move(content),
			suppData, depData, proj);
		auto map2d = dynamic_cast<Map2D*>(mapObj.get());
		if (map2d) {
			mapObj.release();
			i->val_u.reset(std::move(map2d));
		}
		return i;

	}
	return nullptr;
}
