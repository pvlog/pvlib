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
#include <string>
#include <unordered_set>

#include "utility.h"
#include "pvlib.h"

namespace pvlib {

enum Level {
	Error = PVLIB_LOG_ERROR,
	Warning = PVLIB_LOG_WARNING,
	Info = PVLIB_LOG_INFO,
	Debug = PVLIB_LOG_DEBUG,
	Trace = PVLIB_LOG_TRACE,
};

class Log {
public:
	DISABLE_COPY(Log)

	Log() {
		//nothing to do
	}
	virtual ~Log();

	std::ostringstream& get(Level level, const char* module, const char* file, int line);

	static void init(pvlib_log_func, const char* modules[], pvlib_log_level);

	static Level getReportingLevel() {
		return messageLevel;
	}

	static const std::unordered_set<std::string>& getLogModules() {
		return logModules;
	}

protected:
	std::ostringstream os;

	static const char *filename(const char *file);

	static Level messageLevel;

	static std::unordered_set<std::string> logModules;

	static pvlib_log_func logCallback;

	const char* module = nullptr;
	const char* file   = nullptr;
	int line  = 0;
	int level = 0;
};

struct print_array {
	const uint8_t *array;
	size_t size;

	print_array(const uint8_t *array, size_t size) : array(array), size(size) {}
};

std::ostream& operator<<(std::ostream& o, const print_array& a);

#ifndef PVLIB_LOG_MODULE
#	define PVLIB_LOG_MODULE "global"
#endif


#define LOG(LEVEL) \
if (LEVEL > Log::getReportingLevel() || ((Log::getLogModules().size() != 0) && (Log::getLogModules().count(PVLIB_LOG_MODULE) == 0))) \
; \
else \
Log().get(LEVEL, PVLIB_LOG_MODULE, __FILE__, __LINE__)

} //namespace pvlib {

#endif /* #ifndef LOG_H */
