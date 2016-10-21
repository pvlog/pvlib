#ifndef LOG_H
#define LOG_H

#include <cstdio>
#include <sstream>
#include <cstring>
#include <ostream>

enum Level {
	Error = 0, Info, Warning, Debug, Trace
};

const static char *levelName[] = { "ERROR", "INFO", "WARNING", "DEBUG", "TRACE" };

class Log {
public:
	Log()
	{
		//nothing to do
	}
	virtual ~Log()
	{
		os << std::endl;
		fprintf(stderr, "%s", os.str().c_str());
		fflush(stderr);
	}

	std::ostringstream& Get(Level level, const char* file, int line)
	{
		const char *fileName = filename(file);

		os << levelName[level] << '[' << fileName << " " << line << ']' << " ";
		return os;
	}
	static Level& ReportingLevel()
	{
		return messageLevel;
	}

protected:
	std::ostringstream os;

private:
	static const char *filename(const char *file)
	{
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

	Log(const Log&);
	Log& operator =(const Log&);
private:
	static Level messageLevel;
};

struct print_array {
	const uint8_t *array;
	size_t size;

	print_array(const uint8_t *array, size_t size) : array(array), size(size) {}
};

inline std::ostream& operator<<(std::ostream& o, const print_array& a) {
	for (size_t i = 0; i < a.size; ++i) {
		o << a.array[i] << " ";
		if (!(i + 1) % 16 ||( i + 1 == a.size)) {
			o << "\n";
		}
	}

	return o;
}

#define LOG(LEVEL) \
if (LEVEL > Log::ReportingLevel()) \
; \
else \
Log().Get(LEVEL, __FILE__, __LINE__)

#endif /* #ifndef LOG_H */
