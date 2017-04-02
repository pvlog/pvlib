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

const static char *levelName[] = { "ERROR", "INFO", "WARNING", "DEBUG", "TRACE" };

Level Log::messageLevel = Trace;

Log::~Log() {
	os << std::endl;
	fprintf(stderr, "%s", os.str().c_str());
	fflush(stderr);
}

std::ostringstream& Log::get(Level level, const char* file, int line) {
	const char *fileName = filename(file);
	time_t curTime = time(nullptr);
	char buffer[30];

	tm* timeinfo = localtime(&curTime);
	strftime(buffer,80,"%Y-%m-%d %T", timeinfo);
	std::string timeStr(buffer);

	os << levelName[level] << '[' << timeStr << " " << fileName << ":" << line << ']' << " ";
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
		if (!(i + 1) % 16 ||( i + 1 == a.size)) {
			o << "\n";
		}
	}

	return o;
}

} //namespace pvlib {
