/**
 * @file  tab-graphics.cpp
 * @brief Tab for editing images, tilesets and palettes.
 *
 * Copyright (C) 2013-2015 Adam Nielsen <malvineous@shikadi.net>
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

#include <cassert>
#include <gtkmm.h>
#include <glibmm/i18n.h>
#include "main.hpp"
#include "util-gfx.hpp"
#include "tab-graphics.hpp"

using namespace camoto::gamegraphics;

const std::string Tab_Graphics::tab_id = "tab-graphics";

Tab_Graphics::ModelTilesetColumns::ModelTilesetColumns()
{
	this->add(this->name);
	this->add(this->icon);
	this->add(this->tileset);
	this->add(this->index);
}

Tab_Graphics::Tab_Graphics(BaseObjectType *obj,
	const Glib::RefPtr<Gtk::Builder>& refBuilder)
	:	Gtk::Box(obj),
		refBuilder(refBuilder),
		agItems(Gio::SimpleActionGroup::create())
{
	this->agItems->add_action("tileset_add", sigc::mem_fun(this, &Tab_Graphics::on_tileset_add));
	this->agItems->add_action("tileset_remove", sigc::mem_fun(this, &Tab_Graphics::on_tileset_remove));
	this->agItems->add_action("image_import", sigc::mem_fun(this, &Tab_Graphics::on_image_import));
	this->agItems->add_action("image_export", sigc::mem_fun(this, &Tab_Graphics::on_image_export));
	this->agItems->add_action("palette_import", sigc::mem_fun(this, &Tab_Graphics::on_palette_import));
	this->agItems->add_action("palette_export", sigc::mem_fun(this, &Tab_Graphics::on_palette_export));

	this->agItems->add_action("undo", sigc::mem_fun(this, &Tab_Graphics::on_undo));
	this->agItems->add_action("redo", sigc::mem_fun(this, &Tab_Graphics::on_redo));
	this->agItems->add_action("palette_import", sigc::mem_fun(this, &Tab_Graphics::on_palette_import));
	this->agItems->add_action("palette_export", sigc::mem_fun(this, &Tab_Graphics::on_palette_export));
	this->agItems->add_action("image_import", sigc::mem_fun(this, &Tab_Graphics::on_image_import));
	this->agItems->add_action("image_export", sigc::mem_fun(this, &Tab_Graphics::on_image_export));
	this->insert_action_group("doc", this->agItems);

	this->ctTileset = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(
		this->refBuilder->get_object("tvItems"));
	assert(this->ctTileset);

	this->ctItems = Gtk::TreeStore::create(this->cols);
	this->ctTileset->set_model(this->ctItems);

	auto tvsel = this->ctTileset->get_selection();
	tvsel->signal_changed().connect(sigc::mem_fun(this, &Tab_Graphics::on_item_selected));

	this->ctTileset->signal_row_activated().connect(sigc::mem_fun(this, &Tab_Graphics::on_row_activated));
}

void Tab_Graphics::content(std::shared_ptr<Tileset> obj)
{
	this->obj_tileset = obj;

	auto studio = static_cast<Studio *>(this->get_toplevel());

	// Populate tree view with items
	auto row = *(this->ctItems->append());
	row[this->cols.name] = "0";
	try {
		row[this->cols.icon] = studio->getIcon(Studio::Icon::Folder);
	} catch (const Glib::FileError& e) {
	} catch (const Gdk::PixbufError& e) {
	}
	row[this->cols.tileset] = obj;
	row[this->cols.index] = -1;

	this->appendChildren("0", obj, row);
	this->ctTileset->expand_all();
	return;
}

void Tab_Graphics::content(std::unique_ptr<Image> obj)
{
	// Images don't need the tileset tree
	auto ctTileset = Glib::RefPtr<Gtk::Box>::cast_dynamic(
		this->refBuilder->get_object("boxTileset"));
	assert(ctTileset);
	ctTileset->hide();

	if (!(obj->caps() & Image::Caps::HasPalette)) {
		// Hide the palette view as this image doesn't have a palette
		auto ctPalette = Glib::RefPtr<Gtk::Box>::cast_dynamic(
			this->refBuilder->get_object("palette_pane"));
		assert(ctPalette);
		ctPalette->hide();
	}

	this->setImage(std::move(obj));
	return;
}

void Tab_Graphics::content(std::unique_ptr<Palette> obj)
{
	this->obj_palette = std::move(obj);

	// Palettes don't need the tileset tree
	auto ctTileset = Glib::RefPtr<Gtk::Box>::cast_dynamic(
		this->refBuilder->get_object("boxTileset"));
	assert(ctTileset);
	ctTileset->hide();

	// Palettes don't need the image canvas either
	auto ctImage = Glib::RefPtr<Gtk::Box>::cast_dynamic(
		this->refBuilder->get_object("boxImage"));
	assert(ctImage);
	ctImage->hide();

	return;
}

void Tab_Graphics::appendChildren(const Glib::ustring& prefix,
	std::shared_ptr<Tileset> tileset, Gtk::TreeModel::Row& root)
{
	auto studio = static_cast<Studio *>(this->get_toplevel());

	unsigned int index = 0;
	for (auto& i : tileset->files()) {
		auto row = *(this->ctItems->append(root->children()));
		auto name = Glib::ustring::compose("%1.%2", prefix, index);
		if (i->fAttr & Tileset::File::Attribute::Vacant) {
			auto nextPrefix = Glib::ustring::compose("%1.%2", prefix, index);
			row[this->cols.name] = name;
			row[this->cols.icon] = studio->getIcon(Studio::Icon::Generic);
			row[this->cols.tileset] = tileset;
			row[this->cols.index] = index;
		} else if (i->fAttr & Tileset::File::Attribute::Folder) {
			// This is a folder
			row[this->cols.name] = name;
			row[this->cols.icon] = studio->getIcon(Studio::Icon::Folder);
			auto tsChild = tileset->openTileset(i);
			row[this->cols.tileset] = tsChild;
			row[this->cols.index] = -1;
			this->appendChildren(name, tsChild, row);
		} else {
			// This is a tile
			auto nextPrefix = Glib::ustring::compose("%1.%2", prefix, index);
			row[this->cols.name] = name;
			row[this->cols.icon] = studio->getIcon(Studio::Icon::Image);
			row[this->cols.tileset] = tileset;
			row[this->cols.index] = index;
		}
		index++;
	}
	return;
}

void Tab_Graphics::setImage(std::unique_ptr<Image> img)
{
	assert(img);

	// Create a Cairo Surface from the libgamegraphics image
	auto cimg = createCairoSurface(img.get(), this->obj_tileset.get());

	auto ctImage = Glib::RefPtr<Gtk::Image>::cast_dynamic(
		this->refBuilder->get_object("ctImage"));
	assert(ctImage);
	ctImage->set(cimg);

	this->obj_image = std::move(img);
	return;
}

void Tab_Graphics::on_row_activated(const Gtk::TreeModel::Path& path,
	Gtk::TreeViewColumn* column)
{
	auto& row = *this->ctItems->get_iter(path);
	std::shared_ptr<Tileset> tileset = row[this->cols.tileset];
	int index = row[this->cols.index];

	if (index < 0) {
		// This is a tileset, rather than a single tile
	} else {
		// This is a single tile
		auto& tiles = tileset->files();
		this->setImage(
			tileset->openImage(tiles[index])
		);
	}
	return;
}

void Tab_Graphics::on_undo()
{
	return;
}

void Tab_Graphics::on_redo()
{
	return;
}

void Tab_Graphics::on_tileset_add()
{
	return;
}

void Tab_Graphics::on_tileset_remove()
{
	return;
}

void Tab_Graphics::on_image_import()
{
	return;
}

void Tab_Graphics::on_image_export()
{
	return;
}

void Tab_Graphics::on_palette_import()
{
	return;
}

void Tab_Graphics::on_palette_export()
{
	return;
}

void Tab_Graphics::on_item_selected()
{
//	this->syncControlStates();
	return;
}
