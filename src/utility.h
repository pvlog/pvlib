/*
 *   Pvlib - Connection interface
 *
 *   Copyright (C) 2016 pvlogdev@gmail.com
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef UTILITY_H
#define UTILITY_H

#include <string>

#define DISABLE_COPY(CLASS) \
	CLASS(const CLASS&) = delete; \
	CLASS& operator=(const CLASS&) = delete;

namespace pvlib {

/**
 * Returns path to resource folder
 */
const char *resources_path();

std::string timeString(time_t time, int tz, bool dst);
}

#endif /* #ifndef UTILITY_H */
