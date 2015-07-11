/**
 * @file   exceptions.hpp
 * @brief  Base/common exceptions.
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

#ifndef _EXCEPTIONS_HPP_
#define _EXCEPTIONS_HPP_

#include <exception>
#include <glibmm/ustring.h>

class EFailure: public std::exception
{
	public:
		EFailure(const Glib::ustring& msg);

		~EFailure()
			throw ();

		const Glib::ustring& getMessage() const;

		virtual const char *what() const
			throw ();

	protected:
		Glib::ustring msg;     ///< Original message
};

#endif // _EXCEPTIONS_HPP_
