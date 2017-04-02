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

#ifndef SRC_PVLIB_SRC_READWRITE_H_
#define SRC_PVLIB_SRC_READWRITE_H_

#include <string>

namespace pvlib {

class ReadWrite {
public:
	virtual ~ReadWrite() {}

	/**
	 * Write data.
	 *
	 * @param con connection handle.
	 * @param data data to write.
	 * @param len length of data.
	 *
	 * @return < 0 if error occurs.
	 */
	virtual int write(const uint8_t *data, int len, const std::string &to) = 0;

	int write(const uint8_t *data, int len) {
		return write(data, len, "");
	}

	/**
	 * Read data.
	 *
	 * @param con connection handle.
	 * @param data buffer to read to.
	 * @param len length of data to read.
	 * @param timeout timeout in ms
	 *
	 * @return < 0 if error occurs, else amount of bytes read.
	 */
	virtual int read(uint8_t *data, int maxlen, std::string &from) = 0;

	int read(uint8_t *data, int max_len) {
		std::string str;
		return read(data, max_len, str);
	}
};

} //namespace pvlib {

#endif /* SRC_PVLIB_SRC_READWRITE_H_ */
