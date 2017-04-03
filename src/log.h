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

#ifndef LOG_H
#define LOG_H

#include <sstream>

#include "utility.h"

namespace pvlib {

enum Level {
	Error = 0, Info, Warning, Debug, Trace
};

class Log {
public:
	DISABLE_COPY(Log)

	Log() {
		//nothing to do
	}
	virtual ~Log();

	std::ostringstream& get(Level level, const char* file, int line);

	static Level& reportingLevel() {
		return messageLevel;
	}

protected:
	std::ostringstream os;

	static const char *filename(const char *file);

	static Level messageLevel;
};

struct print_array {
	const uint8_t *array;
	size_t size;

	print_array(const uint8_t *array, size_t size) : array(array), size(size) {}
};

std::ostream& operator<<(std::ostream& o, const print_array& a);

#define LOG(LEVEL) \
if (LEVEL > Log::reportingLevel()) \
; \
else \
Log().get(LEVEL, __FILE__, __LINE__)

} //namespace pvlib {

#endif /* #ifndef LOG_H */
