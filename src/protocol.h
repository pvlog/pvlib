/*
 *   Pvlib - Pvlib interface
 *
 *   Copyright (C) 2011, 2016 Protocol interface
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

#ifndef PVLIB_PROTOCOL_H
#define PVLIB_PROTOCOL_H

#include <vector>
#include <cstdint>

#include "pvlib.h"

namespace pvlib {

class Connection;
struct ProtocolInfo;

class Protocol {
public:
	virtual ~Protocol() {}

	virtual int connect(const char *password, const void *param) = 0;

	virtual void disconnect() = 0;

	virtual int inverterNum() = 0;

	virtual int getDevices(uint32_t *id, int maxInverters) = 0;

	virtual int readDc(uint32_t id, pvlib_dc *dc) = 0;

	virtual int readAc(uint32_t id, pvlib_ac *dc) = 0;

	virtual int readStats(uint32_t id, pvlib_stats *stats) = 0;

	virtual int readStatus(uint32_t id, pvlib_status *status) = 0;

	virtual int readInverterInfo(uint32_t id, pvlib_inverter_info *inverter_info) = 0;

	//archive support
	virtual int readDayYield(uint32_t id, time_t from, time_t to, pvlib_day_yield **dayYield) = 0;

	virtual int readEvents(uint32_t id, time_t from, time_t to, pvlib_event **events) = 0;

	static const std::vector<const ProtocolInfo*> availableProtocols;
};

struct ProtocolInfo {
	typedef Protocol* (*CreateProtocol)(Connection *con);

	const CreateProtocol create;
	const char *name;
	const char *author;
	const char *comment;

	ProtocolInfo(const CreateProtocol create, const char *name, const char *author, const char *comment) :
				create(create),
				name(name),
				author(author),
				comment(comment) {
	}
};

} //namespace pvlib {

#endif /* PVLIB_PROTOCOL_H */
