/**
 * @file   mainwindow.cpp
 * @brief  Interface definition for callback functions implemented by the main
 *         window.
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

#include "mainwindow.hpp"

IMainWindow::IMainWindow(wxWindow *parent, wxWindowID winid,
	const wxString& title, const wxPoint& pos,	const wxSize& size, long style,
	const wxString& name)
	:	wxFrame(parent, winid, title, pos, size, style, name)
{
}
