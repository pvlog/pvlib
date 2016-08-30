/*
 *   Pvlib - Log implementation
 *
 *   Copyright (C) 2011
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

#include <stdint.h>
#include <stdio.h>

typedef enum {
	LOG_DISABLE = 0,
	LOG_ERROR = 1,
	LOG_WARNING = 2,
	LOG_INFO = 4,
	LOG_DEBUG = 8,
	LOG_TRACE = 16,
	LOG_ALL = (int) (-1)
} log_severity_t;

void log_enable(FILE *file, int severity);

void log_disable();

void log_log(log_severity_t severity, const char *file, int line, const char *format, ...);

void log_hex(log_severity_t severity,
             const char *file,
             int line,
             const char *message,
             uint8_t *data,
             int len);

#define LOG_TRACE(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_log(LOG_ERROR, __FILE__, __LINE__,  __VA_ARGS__)
#define LOG_INFO(...) log_log(LOG_INFO, __FILE__, __LINE__,  __VA_ARGS__)
#define LOG_WARNING(...) log_log(LOG_WARNING, __FILE__, __LINE__,  __VA_ARGS__)
#define LOG_DEBUG(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_TRACE_HEX(MESSAGE, DATA, LEN) log_hex(LOG_TRACE, __FILE__, __LINE__, MESSAGE, DATA, LEN)
#define LOG_DEBUG_HEX(MESSAGE, DATA, LEN) log_hex(LOG_TRACE, __FILE__, __LINE__, MESSAGE, DATA, LEN)

#endif /* #ifndef LOG_H */
