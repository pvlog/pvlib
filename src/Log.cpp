#include "Log.h"

#include <cstring>
#include <cstdio>
#include <ctime>
#include <iomanip>

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
	strftime(buffer,80,"%Y-%m-%d %I:%M:%S", timeinfo);
	std::string timeStr(buffer);

	os << levelName[level] << '[' << timeStr << fileName << " " << line << " " << ']' << " ";
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
