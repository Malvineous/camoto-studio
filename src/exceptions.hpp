/**
 * @file   exceptions.hpp
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

#ifndef _EXCEPTIONS_HPP_
#define _EXCEPTIONS_HPP_

#include <exception>
#include <wx/wx.h>

class EBase: public std::exception
{
	public:
		EBase(const wxString& msg);

		~EBase();

		const wxString& getMessage() const;

		virtual const char *what() const;

	protected:
		wxString msg;                 ///< Original message
		mutable wxCharBuffer buffer;  ///< Buffer used for what() return value

};

#endif // _EXCEPTIONS_HPP_
