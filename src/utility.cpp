/*
 *   Pvlib - Connection interface
 *
 *   Copyright (C) 2016
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

#include "utility.h"

#include <cstdlib>
#include <ctime>

#include "config.h"

namespace pvlib {

const char *resources_path() {
	const char *env = getenv("PVLIB_RESOURCE_DIR");
	if (env != NULL) {
		return env;
	} else {
		return RESOURCE_DIR;
	}
}

//FIXME: not reentrant
std::string timeString(time_t time, int tz, bool dst) {
	time_t tmp = time + tz * 60 + static_cast<int>(dst) * 60;
	std::tm* now_tm= std::gmtime(&tmp);
	char buf[42];
	std::strftime(buf, 42, "%Y-%m-%d %X", now_tm);
	return buf;
}

} //namespace pvlib {
