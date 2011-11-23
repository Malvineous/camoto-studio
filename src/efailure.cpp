/**
 * @file   efailure.cpp
 * @brief  EFailure exception.
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

#include "efailure.hpp"

EFailure::EFailure(const wxString& msg)
	throw () :
	msg(msg)
{
}

EFailure::~EFailure()
	throw ()
{
}

const char *EFailure::what() const
	throw ()
{
	// Have to store the wxCharBuffer otherwise it gets deallocated and we end up
	// returning an invalid pointer.
	this->buf = this->msg.ToUTF8();
	return this->buf;
}

const wxString& EFailure::getMessage() const
	throw ()
{
	return this->msg;
}
