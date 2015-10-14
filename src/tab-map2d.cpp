/**
 * @file  tab-map2d.cpp
 * @brief Tab for editing 2D tile-based maps.
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
#include "tab-map2d.hpp"

using namespace camoto::gamegraphics;
using namespace camoto::gamemaps;

const std::string Tab_Map2D::tab_id = "tab-map2d";

Tab_Map2D::ModelTilesetColumns::ModelTilesetColumns()
{
	this->add(this->name);
	this->add(this->icon);
	this->add(this->tileset);
	this->add(this->index);
}

Tab_Map2D::Tab_Map2D(BaseObjectType *obj,
	const Glib::RefPtr<Gtk::Builder>& refBuilder)
	:	Gtk::Box(obj),
		refBuilder(refBuilder),
		agItems(Gio::SimpleActionGroup::create())
{
	this->agItems->add_action("undo", sigc::mem_fun(this, &Tab_Map2D::on_undo));
	this->agItems->add_action("redo", sigc::mem_fun(this, &Tab_Map2D::on_redo));
	this->agItems->add_action("save", sigc::mem_fun(this, &Tab_Map2D::on_save));
	this->insert_action_group("doc", this->agItems);

	auto tvLayers = Glib::RefPtr<Gtk::TreeView>::cast_dynamic(
		this->refBuilder->get_object("tvLayers"));
	this->ctItems = Gtk::TreeStore::create(this->cols);
	tvLayers->set_model(this->ctItems);

	this->refBuilder->get_widget_derived("daCanvas", this->ctCanvas);
}

void Tab_Map2D::content(std::unique_ptr<Map2D> obj, DepData& depData)
{
	this->obj = std::move(obj);

	// Read all the depData objects and build a TilesetCollection from them
	TilesetCollection allTilesets;

	for (auto& d : depData) {
		auto& depType = d.first;
		auto& objInst = d.second;
		if (objInst->type != GameObjectInstance::Type::Tileset) continue;
		ImagePurpose purpose = dep2purpose(depType);
		allTilesets[purpose] = objInst->get_shared<Tileset>();
	}
	this->ctCanvas->content(this->obj, allTilesets);
	return;
}

void Tab_Map2D::on_undo()
{
	return;
}

void Tab_Map2D::on_redo()
{
	return;
}

void Tab_Map2D::on_save()
{
	return;
}
