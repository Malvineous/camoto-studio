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

#ifndef _EFAILURE_HPP_
#define _EFAILURE_HPP_

#include <exception>
#include <wx/string.h>

/// Exception thrown for general errors that should trigger popup boxes.
class EFailure: virtual public std::exception {
	protected:
		wxString msg;

	public:
		EFailure(const wxString& msg)
			throw ();

		~EFailure()
			throw ();

		virtual const char *what() const
			throw ();

		const wxString& getMessage() const
			throw ();
};

#endif // _EFAILURE_HPP_
