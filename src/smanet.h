/*
 *   Pvlib - Smanet implementation
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

#ifndef SMANET_H
#define SMANET_H

#include <cstdint>

#include "readWrite.h"

namespace pvlib {


class Smanet : public ReadWrite {
public:
	/**
	 * Setup smanet.
	 *
	 * @param protocol Protocol type to use.
	 * @param Connection to send data
	 *
	 * @return smanet handle on success, else NULL.
	 */
	Smanet(uint16_t protocol, ReadWrite *con);

	/**
	 * Close smanet.
	 *
	 */
	virtual ~Smanet() {};

	/**
	 * Write data.
	 *
	 * @param smanet smanet handle.
	 * @param data data to write.
	 * @param len length of data.
	 * @param param to destination mac(6 byte), only used for smadata2plus bluetooth transmission,
	 *        otherwise set it to NULL.
	 *
	 * @return >= 0 on success, else < 0.
	 */
	virtual int write(const uint8_t *data, int len, const std::string &to) override;

	/**
	 * Read data.
	 * Note: Reads always one and only one smabluetooth frame.
	 * If given size is below smabluetooth frame size left bytes will be removed.
	 * If given size greater than smabluetooth frame size only size of frame will be read.
	 * So allways check return size!
	 *
	 * @param smanet smanet handle.
	 * @param data buf to readt to.
	 * @param len len of data.
	 * @param[out] from device mac(6 byte), only used for smadata2plus bluetooth,
	 *             otherwise set it to NULL.
	 *
	 * @return amount of bytes read on success, else < 0.
	 */
	virtual int read(uint8_t *data, int len, std::string &from) override;

private:
	int readFrame(uint8_t *data, int len, std::string &from);

	static constexpr int BUF_SIZE = 255;
	uint16_t protocol;
	ReadWrite *con;
	uint8_t read_buf[BUF_SIZE];
	uint8_t size;
};

} //namespace pvlib {

#endif /* #ifndef SMANET_H */
