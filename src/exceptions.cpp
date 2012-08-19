/**
 * @file   exceptions.cpp
 * @brief  Base/common exceptions.
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

#include <exception>
#include <wx/wx.h>

#include "exceptions.hpp"

EFailure::EFailure(const wxString& msg)
	:	msg(msg)
{
}

EFailure::~EFailure()
	throw ()
{
}

const wxString& EFailure::getMessage() const
{
	return this->msg;
}

const char *EFailure::what() const
	throw ()
{
	this->buffer = this->msg.utf8_str();
	return this->buffer.data();
}
