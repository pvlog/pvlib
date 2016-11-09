#ifndef LOG_H
#define LOG_H

#include <sstream>
#include "Utility.h"

namespace pvlib {

enum Level {
	Error = 0, Info, Warning, Debug, Trace
};

class Log {
	DISABLE_COPY(Log)
public:
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
