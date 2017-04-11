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

#include <cstring>
#include <cstdio>
#include <ctime>
#include <iomanip>

#include "log.h"

namespace pvlib {

Level Log::messageLevel = Trace;
pvlib_log_func Log::logCallback = nullptr;
std::unordered_set<std::string> Log::logModules;

Log::~Log() {
	std::string str = os.str();
	if (logCallback != nullptr) {
		logCallback(module, file, line, static_cast<pvlib_log_level>(level), str.c_str());
	}
}

void Log::init(pvlib_log_func logCallback, const char* modules[], pvlib_log_level level) {
	if (modules != nullptr) {
		for (int i = 0; modules != nullptr && modules[i] != nullptr; ++i) {
			logModules.emplace(modules[i]);
		}
	}

	Log::logCallback  = logCallback;
	Log::messageLevel = static_cast<Level>(level);
}

std::ostringstream& Log::get(Level level, const char* module, const char* file, int line) {
	this->module = module;
	this->file   = filename(file);
	this->line   = line;
	this->level  = level;

	return os;
}

const char *Log::filename(const char *file) {
	const char *i;
	int len = strlen(file);
	if (len <= 1) return file;

	for (i = &file[len - 1]; i != file; i--) {
		if (*i == '/' || *i == '\\') {
			return &i[1];
		}
	}
	return file;
}

std::ostream& operator<<(std::ostream& o, const print_array& a) {
	o <<  std::hex << std::setfill('0');
	for (size_t i = 0; i < a.size; ++i) {
		o << std::setw(2) << (int)a.array[i] << " ";
		if (((i + 1) % 16 == 0) || ( i + 1 == a.size)) {
			o << "\n";
		}
	}

	return o;
}

} //namespace pvlib {
