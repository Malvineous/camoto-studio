/**
 * @file   main.hpp
 * @brief  Entry point for Camoto Studio.
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

#ifndef _MAIN_HPP_
#define _MAIN_HPP_

#define CAMOTO_HEADER \
	"Camoto Game Modding Studio\n" \
	"Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>\n" \
	"http://www.shikadi.net/camoto\n" \
	"\n" \
	"This program comes with ABSOLUTELY NO WARRANTY.  This \n" \
	"is free software, and you are welcome to change and \n" \
	"redistribute it under certain conditions; see \n" \
	"<http://www.gnu.org/licenses/> for details.\n"

#include <gtkmm.h>
#include "gamelist.hpp"
#include "project.hpp"

/// File paths
struct paths
{
	std::string dataRoot;          ///< Main data folder
	std::string gameData;          ///< Location of XML game description files
	std::string gameScreenshots;   ///< Game screenshots used in 'new project' dialog
	std::string gameIcons;         ///< Icons used to represent each game
	std::string guiIcons;          ///< Icons used for GUI elements
	std::string mapIndicators;     ///< Icons used for map editor indicators
	std::string lastUsed;          ///< Path last used in open/save dialogs
};

extern paths path;

/// User preferences
struct config_data
{
	/// Path to DOSBox binary
	std::string dosboxPath;

	/// True to add a DOS 'pause' command before exiting DOSBox
	bool dosboxExitPause;

	/// Index of MIDI device to use
	int midiDevice;

	/// Digital output delay (relative to MIDI output) in milliseconds
	int pcmDelay;
};

extern config_data config;

class Studio: public Gtk::Window
{
	public:
		enum class Icon {
			Folder,
			Generic,
			Invalid,
			Archive,
			B800,
			Image,
			Map2D,
			Music,
			Palette,
		};

		Studio(BaseObjectType *obj, const Glib::RefPtr<Gtk::Builder>& refBuilder);

		/// Open the given project in a new tab.
		/**
		 * @param proj
		 *   The project instance to create a new tab for.
		 */
		void openProject(std::unique_ptr<Project> proj);

		/// Open the given folder as a project, in a new tab.
		/**
		 * @param folder
		 *   The folder to open.  It must contain a project.camoto file.
		 */
		void openProjectByFilename(const std::string& folder);

		void openItem(const GameObject& item,
			std::unique_ptr<camoto::stream::inout> content,
			camoto::SuppData& suppData,
			Project *proj);

		void closeTab(Gtk::Box *tab);

		/// Display a message in the main window's infobar
		void infobar(const Glib::ustring& content);

		/// Convert a string ID into an Icon value
		Icon nameToIcon(const std::string& name);

		/// Convert an Icon value into a string ID
		std::string iconToName(Icon icon);

		/// Get an icon to use in a tree list
		Glib::RefPtr<Gdk::Pixbuf> getIcon(Icon icon);

	protected:
		void on_menuitem_file_new();
		void on_menuitem_file_open();
		void on_menuitem_file_exit();
		void on_menuitem_project_new();
		void on_menuitem_project_open();
		void on_infobar_button(int response_id);

		/// Open a .glade file in a new tab.
		template <class T>
		T* openTab(const Glib::ustring& title);

	protected:
		Glib::RefPtr<Gtk::Builder> refBuilder;
		std::map<std::string, Icon> mapName;
		std::map<Icon, Glib::RefPtr<Gdk::Pixbuf>> icons;
};

#endif // _MAIN_HPP_
