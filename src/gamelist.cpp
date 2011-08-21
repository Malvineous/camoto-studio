/**
 * @file   gamelist.cpp
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

#include <wx/dir.h>
#include <wx/filename.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "main.hpp"
#include "gamelist.hpp"

#define XMLNS_SUPP  "http://www.shikadi.net/camoto/xml/supp"

#define _X(a)  (const xmlChar *)(a)

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
	throw ()
{
	GameInfoMap games;

	wxDir dir(::path.gameData);
	wxString filename;
	bool more = dir.GetFirst(&filename, _T("*.xml"), wxDIR_FILES);
	while (more) {
		wxFileName fn;
		fn.AssignDir(::path.gameData);
		fn.SetFullName(filename);
		const wxCharBuffer n = fn.GetFullPath().fn_str();
		std::cout << "[gamelist] Parsing " << n << "\n";
		xmlDoc *xml = xmlParseFile(n);
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
	throw ()
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

void processFilesChunk(Game *g, xmlNode *i, const wxString& idParent)
{
	for (xmlNode *j = i->children; j; j = j->next) {
		if (xmlStrEqual(j->name, _X("file")) || xmlStrEqual(j->name, _X("archive"))) {
			GameObject o;
			xmlChar *val = xmlNodeGetContent(j);
			o.friendlyName = wxString::FromUTF8((const char *)val, xmlStrlen(val));
			o.idParent = idParent;
			xmlFree(val);
			for (xmlAttr *a = j->properties; a; a = a->next) {
				if ((a->ns) && (xmlStrcmp(a->ns->href, _X(XMLNS_SUPP)) == 0)) {
					wxString suppName = wxString::FromUTF8((const char *)a->name, xmlStrlen(a->name));
					xmlChar *val = xmlNodeGetContent(a->children);
					o.supp[suppName] = wxString::FromUTF8((const char *)val, xmlStrlen(val));
					xmlFree(val);
				} else if (xmlStrcmp(a->name, _X("id")) == 0) {
					xmlChar *val = xmlNodeGetContent(a->children);
					o.id = wxString::FromUTF8((const char *)val, xmlStrlen(val));
					xmlFree(val);
				} else if (xmlStrcmp(a->name, _X("typeMajor")) == 0) {
					xmlChar *val = xmlNodeGetContent(a->children);
					o.typeMajor = wxString::FromUTF8((const char *)val, xmlStrlen(val));
					xmlFree(val);
				} else if (xmlStrcmp(a->name, _X("typeMinor")) == 0) {
					xmlChar *val = xmlNodeGetContent(a->children);
					o.typeMinor = wxString::FromUTF8((const char *)val, xmlStrlen(val));
					xmlFree(val);
				} else if (xmlStrcmp(a->name, _X("name")) == 0) {
					xmlChar *val = xmlNodeGetContent(a->children);
					o.filename = wxString::FromUTF8((const char *)val, xmlStrlen(val));
					xmlFree(val);
				} else if (xmlStrcmp(a->name, _X("title")) == 0) {
					xmlChar *val = xmlNodeGetContent(a->children);
					o.friendlyName = wxString::FromUTF8((const char *)val, xmlStrlen(val));
					xmlFree(val);
				}
			}

			if (xmlStrEqual(j->name, _X("archive"))) {
				o.typeMajor = _T("archive");
				o.friendlyName = o.filename;
				processFilesChunk(g, j, o.id);
			}

			if (!o.id.IsEmpty()) {
				if (o.typeMajor.IsEmpty() || o.typeMinor.IsEmpty() || o.filename.IsEmpty()) {
					std::cout << "[gamelist] <file/> with id \"" << o.id.ToAscii() <<
						"\" is missing attribute(s): ";
					if (o.typeMajor.IsEmpty()) std::cout << "typeMajor ";
					if (o.typeMinor.IsEmpty()) std::cout << "typeMinor ";
					if (o.filename.IsEmpty()) std::cout << "filename ";
					std::cout << "\n";
				}
				g->objects[o.id] = o;
			} else {
				std::cout << "[gamelist] Got a <file/> or <archive/> with no 'id' (" <<
					o.friendlyName.ToAscii() << ")\n";
			}
		}
	}
	return;
}

Game *loadGameStructure(const wxString& id)
	throw ()
{
	wxFileName fn;
	fn.AssignDir(::path.gameData);
	fn.SetName(id);
	fn.SetExt(_T("xml"));

	const wxCharBuffer n = fn.GetFullPath().fn_str();
	std::cout << "[gamelist] Parsing " << n << "\n";
	xmlDoc *xml = xmlParseFile(n);
	if (!xml) {
		std::cout << "[gamelist] Error parsing " << n << std::endl;
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
