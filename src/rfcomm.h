/*
 *   Pvlib - Connection interface
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

#ifndef RFCOMM_H
#define RFCOMM_H

#include <memory>

#include "connection.h"

namespace pvlib {

class Rfcomm : public Connection {
public:
	Rfcomm();

	virtual ~Rfcomm() override;

	virtual int connect(const char *address, const void *param) override;

	virtual void disconnect() override;

	virtual int write(const uint8_t *data, int len, const std::string &to) override;

	virtual int read(uint8_t *data, int max_len, std::string& from) override;
private:
	bool connected;
	int timeout;
	int socket;
	uint8_t src_mac[6];
	uint8_t dst_mac[6];
	char src_name[128];
	char dst_name[128];
};

} //namespace pvlib {

#endif /* #ifndef RFCOMM_H */
