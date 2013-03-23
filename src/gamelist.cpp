/**
 * @file   gamelist.cpp
 * @brief  Interface to the list of supported games.
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

#include <iostream>
#include <wx/dir.h>
#include <wx/filename.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "main.hpp"
#include "gamelist.hpp"

#define _X(a)  (const xmlChar *)(a)

wxString dep2string(DepType t)
{
	switch (t) {
		case GenericTileset1:    return _T("generic-tileset1");
		case BackgroundTileset1: return _T("background-tileset1");
		case BackgroundTileset2: return _T("background-tileset2");
		case ForegroundTileset1: return _T("foreground-tileset1");
		case ForegroundTileset2: return _T("foreground-tileset2");
		case SpriteTileset1:     return _T("sprite-tileset1");
		case FontTileset1:       return _T("font-tileset1");
		case FontTileset2:       return _T("font-tileset2");
		case BackgroundImage:    return _T("background-image");
		case Palette:            return _T("palette");
		case DepTypeCount: // fall through
		default:
			return _T("<invalid value>");
	}
}

GameObjectPtr Game::findObjectByFilename(const wxString& filename,
	const wxString& typeMajor)
{
	for (GameObjectMap::iterator
		i = this->objects.begin(); i != this->objects.end(); i++
	) {
		if (filename.compare(i->second->filename) == 0) {
			if (typeMajor.IsEmpty() || (typeMajor.IsSameAs(i->second->typeMajor))) {
				return i->second;
			}
		}
	}
	return GameObjectPtr();
}

GameObjectPtr Game::findObjectById(const wxString& id)
{
	GameObjectMap::iterator io = this->objects.find(id);
	if (io == this->objects.end()) return GameObjectPtr();
	return io->second;
}

/// Process the <info/> chunk.
void populateGameInfo(xmlDoc *xml, GameInfo *gi)
{
	xmlNode *root = xmlDocGetRootElement(xml);
	xmlChar *title = xmlGetProp(root, _X("title"));
	if (title) {
		gi->title = wxString::FromUTF8((char *)title, xmlStrlen(title));
		xmlFree(title);
	}

	for (xmlNode *i = root->children; i; i = i->next) {
		if (xmlStrcmp(i->name, _X("info")) == 0) {
			for (xmlNode *j = i->children; j; j = j->next) {
				if (xmlStrEqual(j->name, _X("developer"))) {
					xmlChar *content = xmlNodeGetContent(j);
					gi->developer = wxString::FromUTF8((const char *)content, xmlStrlen(content));
					xmlFree(content);
				}
				if (xmlStrEqual(j->name, _X("reverser"))) {
					xmlChar *content = xmlNodeGetContent(j);
					gi->reverser = wxString::FromUTF8((const char *)content, xmlStrlen(content));
					xmlFree(content);
				}
			}
		}
	}
	return;
}

GameInfoMap getAllGames()
{
	GameInfoMap games;

	wxDir dir(::path.gameData);
	wxString filename;
	bool more = dir.GetFirst(&filename, _T("*.xml"), wxDIR_FILES);
	while (more) {
		wxFileName fn;
		fn.AssignDir(::path.gameData);
		fn.SetFullName(filename);
		const wxString n = fn.GetFullPath();
		std::cout << "[gamelist] Parsing " << n << "\n";
		xmlDoc *xml = xmlParseFile(n.mb_str());
		if (!xml) {
			std::cout << "[gamelist] Error parsing " << n << std::endl;
		} else {
			GameInfo gi;
			gi.id = fn.GetName();
			populateGameInfo(xml, &gi);
			games[gi.id] = gi;
			xmlFreeDoc(xml);
		}
		more = dir.GetNext(&filename);
	}

	return games;
}

/// Recursively process the <display/> chunk.
void populateDisplay(xmlNode *n, tree<wxString>& t)
{
	for (xmlNode *i = n->children; i; i = i->next) {
		if (xmlStrcmp(i->name, _X("item")) == 0) {
			xmlChar *val = xmlGetProp(i, _X("ref"));
			if (val) {
				t.children.push_back(wxString::FromUTF8((const char *)val, xmlStrlen(val)));
				xmlFree(val);
			}
		} else if (xmlStrcmp(i->name, _X("group")) == 0) {
			xmlChar *val = xmlGetProp(i, _X("name"));
			if (val) {
				tree<wxString> group(wxString::FromUTF8((const char *)val, xmlStrlen(val)));
				xmlFree(val);
				populateDisplay(i, group);
				t.children.push_back(group);
			}
		}
	}
	return;
}

camoto::gamegraphics::TileList processTilesetChunk(xmlNode *i)
{
	camoto::gamegraphics::TileList tileList;

	for (xmlNode *j = i->children; j; j = j->next) {
		if (!xmlStrEqual(j->name, _X("image"))) continue;

		camoto::gamegraphics::TilePos tp;
		for (xmlAttr *a = j->properties; a; a = a->next) {
			if (xmlStrcmp(a->name, _X("x")) == 0) {
				xmlChar *val = xmlNodeGetContent(a->children);
				tp.xOffset = strtod((const char *)val, NULL);
				xmlFree(val);
			} else if (xmlStrcmp(a->name, _X("y")) == 0) {
				xmlChar *val = xmlNodeGetContent(a->children);
				tp.yOffset = strtod((const char *)val, NULL);
				xmlFree(val);
			} else if (xmlStrcmp(a->name, _X("width")) == 0) {
				xmlChar *val = xmlNodeGetContent(a->children);
				tp.width = strtod((const char *)val, NULL);
				xmlFree(val);
			} else if (xmlStrcmp(a->name, _X("height")) == 0) {
				xmlChar *val = xmlNodeGetContent(a->children);
				tp.height = strtod((const char *)val, NULL);
				xmlFree(val);
			}
		}
		tileList.push_back(tp);
	}
	return tileList;
}

void processFilesChunk(Game *g, xmlNode *i, const wxString& idParent)
{
	for (xmlNode *j = i->children; j; j = j->next) {
		bool isFileTag = xmlStrEqual(j->name, _X("file"));
		bool isArchiveTag = isFileTag ? false : xmlStrEqual(j->name, _X("archive"));
		bool isTilesetTag = (isFileTag || isArchiveTag) ? false : xmlStrEqual(j->name, _X("tileset"));
		if (isFileTag || isArchiveTag || isTilesetTag) {
			GameObjectPtr o(new GameObject());
			xmlChar *val = xmlNodeGetContent(j);
			o->idParent = idParent;
			xmlFree(val);
			wxString strImage;
			unsigned int layoutWidth = 0;
			for (xmlAttr *a = j->properties; a; a = a->next) {
				xmlChar *val = xmlNodeGetContent(a->children);
				if (xmlStrcmp(a->name, _X("id")) == 0) {
					o->id = wxString::FromUTF8((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("title")) == 0) {
					o->friendlyName = wxString::FromUTF8((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("typeMajor")) == 0) {
					o->typeMajor = wxString::FromUTF8((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("typeMinor")) == 0) {
					o->typeMinor = wxString::FromUTF8((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("name")) == 0) {
					o->filename = wxString::FromUTF8((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("offset")) == 0) {
					o->offset = strtod((const char *)val, NULL);
				} else if (xmlStrcmp(a->name, _X("size")) == 0) {
					o->size = strtod((const char *)val, NULL);
				} else if (xmlStrcmp(a->name, _X("image")) == 0) {
					strImage = wxString::FromUTF8((const char *)val, xmlStrlen(val));
				} else if (xmlStrcmp(a->name, _X("layoutWidth")) == 0) {
					layoutWidth = strtod((const char *)val, NULL);
				}
				xmlFree(val);
			}

			bool missingImage = false;
			if (isArchiveTag) {
				o->typeMajor = _T("archive");
				o->friendlyName = o->filename;
				processFilesChunk(g, j, o->id);
			} else if (isTilesetTag) {
				o->typeMajor = _T("tileset");
				o->typeMinor = _T(TILESETTYPE_MINOR_FROMIMG);
				TilesetInfo tsi;
				tsi.idImage = strImage;
				tsi.layoutWidth = layoutWidth;
				tsi.tileList = processTilesetChunk(j);
				g->tilesets[o->id] = tsi;
				missingImage = strImage.IsEmpty();
			}

			// Look for supp or dep child nodes
			for (xmlNode *k = j->children; k; k = k->next) {
				bool isSuppTag = xmlStrEqual(k->name, _X("supp"));
				bool isDepTag = isSuppTag ? false : xmlStrEqual(k->name, _X("dep"));
				if (!isSuppTag && !isDepTag) continue;

				wxString sdRef; // id of supp or dep tag
				wxString sdType;
				for (xmlAttr *a = k->properties; a; a = a->next) {
					xmlChar *val = xmlNodeGetContent(a->children);
					if (xmlStrcmp(a->name, _X("ref")) == 0) {
						sdRef = wxString::FromUTF8((const char *)val, xmlStrlen(val));
					} else if (xmlStrcmp(a->name, _X("reftype")) == 0) {
						sdType = wxString::FromUTF8((const char *)val, xmlStrlen(val));
					}
					xmlFree(val);
				}
				if (isSuppTag) {
					// Convert attribute name into SuppItem type
					camoto::SuppItem::Type suppType = (camoto::SuppItem::Type)-1;
					     if (sdType.IsSameAs(_T("dictionary" ))) suppType = camoto::SuppItem::Dictionary;
					else if (sdType.IsSameAs(_T("fat"        ))) suppType = camoto::SuppItem::FAT;
					else if (sdType.IsSameAs(_T("palette"    ))) suppType = camoto::SuppItem::Palette;
					else if (sdType.IsSameAs(_T("instruments"))) suppType = camoto::SuppItem::Instruments;
					else if (sdType.IsSameAs(_T("layer1"     ))) suppType = camoto::SuppItem::Layer1;
					else if (sdType.IsSameAs(_T("layer2"     ))) suppType = camoto::SuppItem::Layer2;
					else if (sdType.IsSameAs(_T("extra1"     ))) suppType = camoto::SuppItem::Extra1;
					else if (sdType.IsSameAs(_T("extra2"     ))) suppType = camoto::SuppItem::Extra2;
					else if (sdType.IsSameAs(_T("extra3"     ))) suppType = camoto::SuppItem::Extra3;
					else if (sdType.IsSameAs(_T("extra4"     ))) suppType = camoto::SuppItem::Extra4;
					else if (sdType.IsSameAs(_T("extra5"     ))) suppType = camoto::SuppItem::Extra5;
					else {
						std::cout << "[gamelist] Invalid supplementary type \""
							<< sdType.mb_str().data() << "\"" << std::endl;
					}
					if (suppType != (camoto::SuppItem::Type)-1) {
						o->supp[suppType] = sdRef;
					}
				} else if (isDepTag) {
					// Convert attribute name into DepType
					DepType depType = (DepType)-1;
					     if (sdType.IsSameAs(_T("generic-tileset1"   ))) depType = GenericTileset1;
					else if (sdType.IsSameAs(_T("background-tileset1"))) depType = BackgroundTileset1;
					else if (sdType.IsSameAs(_T("background-tileset2"))) depType = BackgroundTileset2;
					else if (sdType.IsSameAs(_T("foreground-tileset1"))) depType = ForegroundTileset1;
					else if (sdType.IsSameAs(_T("foreground-tileset2"))) depType = ForegroundTileset2;
					else if (sdType.IsSameAs(_T("sprite-tileset1"    ))) depType = SpriteTileset1;
					else if (sdType.IsSameAs(_T("font-tileset1"      ))) depType = FontTileset1;
					else if (sdType.IsSameAs(_T("font-tileset2"      ))) depType = FontTileset2;
					else if (sdType.IsSameAs(_T("background-image"   ))) depType = BackgroundImage;
					else if (sdType.IsSameAs(_T("palette"            ))) depType = Palette;
					else {
						std::cout << "[gamelist] Invalid dependent object type \""
							<< sdType.mb_str().data() << "\"" << std::endl;
					}
					if (depType != (DepType)-1) {
						o->dep[depType] = sdRef;
					}
				} // else ignore unknown tag
			}

			if (o->id.IsEmpty()) {
				std::cout << "[gamelist] Got a <" << j->name
					<< "/> with no 'id' attribute (" << o->friendlyName.ToAscii()
					<< ")\n";
			} else {
				bool missingTypeMajor = o->typeMajor.IsEmpty();
				bool missingTypeMinor = o->typeMinor.IsEmpty();
				bool missingFilename = o->filename.IsEmpty() && (isFileTag || isArchiveTag);
				if (missingTypeMajor || missingTypeMinor || missingFilename || missingImage) {
					std::cout << "[gamelist] <" << (const char *)j->name
						<< "/> with id \"" << o->id.ToAscii()
						<< "\" is missing attribute(s): ";
					if (missingTypeMajor) std::cout << "typeMajor ";
					if (missingTypeMinor) std::cout << "typeMinor ";
					if (missingFilename) std::cout << "filename ";
					if (missingImage) std::cout << "image ";
					std::cout << "\n";
				}
				g->objects[o->id] = o;
			}
		}
	}
	return;
}

Game *loadGameStructure(const wxString& id)
{
	wxFileName fn;
	fn.AssignDir(::path.gameData);
	fn.SetName(id);
	fn.SetExt(_T("xml"));

	const wxString n = fn.GetFullPath();
	std::cout << "[gamelist] Parsing " << n.mb_str().data() << "\n";
	xmlDoc *xml = xmlParseFile(n.mb_str());
	if (!xml) {
		std::cerr << "[gamelist] Error parsing " << n.mb_str().data() << std::endl;
		return NULL;
	}

	Game *g = new Game();
	g->id = id;
	populateGameInfo(xml, g);

	xmlNode *root = xmlDocGetRootElement(xml);
	for (xmlNode *i = root->children; i; i = i->next) {
		if (xmlStrcmp(i->name, _X("display")) == 0) {
			populateDisplay(i, g->treeItems);
		} else if (xmlStrcmp(i->name, _X("files")) == 0) {
			// Process the <files/> chunk
			processFilesChunk(g, i, wxString());
		} else if (xmlStrcmp(i->name, _X("commands")) == 0) {
			// Process the <commands/> chunk
			for (xmlNode *j = i->children; j; j = j->next) {
				if (xmlStrEqual(j->name, _X("command"))) {
					wxString title;
					for (xmlAttr *a = j->properties; a; a = a->next) {
						if (xmlStrcmp(a->name, _X("title")) == 0) {
							xmlChar *val = xmlNodeGetContent(a->children);
							title = wxString::FromUTF8((const char *)val, xmlStrlen(val));
							xmlFree(val);
						}
					}
					if (title.empty()) {
						std::cerr << "[gamelist] Game \"" << id.ToAscii()
							<< "\" has a <command/> with no title attribute." << std::endl;
					} else {
						xmlChar *val = xmlNodeGetContent(j);
						g->dosCommands[title] = wxString::FromUTF8((const char *)val, xmlStrlen(val));
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
									o.name = wxString::FromUTF8((const char *)val, xmlStrlen(val));
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
							g->mapObjects.push_back(o);
						}
					} // for <objects/> children
				}
			} // for <map/> children
		}
	}
	xmlFreeDoc(xml);

	return g;
}
