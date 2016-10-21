/*
 *   Pvlib - Smadata2plus implementation
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

#ifndef SMADATA2PLUS_H
#define SMADATA2PLUS_H

#include <Protocol.h>
#include <cstring>
#include <unordered_map>

#include "Smanet.h"
#include "Smabluetooth.h"

struct Smabluetooth;
class Connection;
//struct Device;
struct Smanet;
struct Packet;
struct Record;
class Transaction;


class Smadata2plus : public Protocol {
	friend Transaction;
public:
	Smadata2plus(Connection *connection);

	virtual ~Smadata2plus() {};

	virtual int connect(const char *password, const void *param) override;

	virtual void disconnect() override;

	virtual int inverterNum() override;

	virtual int getDevices(uint32_t *id, int maxInverters) override;

	virtual int readDc(uint32_t id, pvlib_dc *dc) override;

	virtual int readAc(uint32_t id, pvlib_ac *dc) override;

	virtual int readStats(uint32_t id, pvlib_stats *stats) override;

	virtual int readStatus(uint32_t id, pvlib_status *status) override;

	virtual int readInverterInfo(uint32_t id, pvlib_inverter_info *inverter_info) override;

	virtual int readDayYield(uint32_t id, time_t from, time_t to, pvlib_day_yield **dayYield) override;

	virtual int readEvents(uint32_t id, time_t from, time_t to, pvlib_event **events) override;

	struct Device {
		uint32_t serial;
		char     mac[6];
		bool     authenticated;

		Device(uint32_t serial, const char *mac, bool authenticated) :
				serial(serial),
				authenticated(authenticated) {
			memcpy(this->mac, mac, sizeof(this->mac));
		}
	};

	enum UserType {
		USER,
		INSTALLER
	};

	enum RecordType {
		RECORD_1,
		RECORD_2,
		RECORD_3
	};

	struct EventData {
	    int32_t  time;
		uint16_t entryId;
		uint16_t sysId;
		uint32_t serial;
		uint16_t eventCode;
		uint16_t eventFlags;
		uint32_t group;
		uint32_t unknown;
		uint32_t tag;
		uint32_t counter;
		uint32_t dtChange;
		uint32_t parameter;
		uint32_t newVal;
		uint32_t oldVal;
	};

	struct TotalDayData {
		int32_t time;
		int64_t totalYield; //in Wh
	};

	virtual int readEventData(uint32_t serial, time_t from, time_t to,
			UserType user, std::vector<EventData> &evenData);

	virtual int readTotalDayData(uint32_t serial, time_t from,
			time_t to, std::vector<TotalDayData> &eventData);

private:
	int writeReplay(const Packet *packet, uint16_t transactionCntr);

	int write(const Packet *packet);

	int read(Packet *packet);

	int requestChannel(uint32_t serial, uint16_t channel, uint32_t fromIdx, uint32_t toIdx);

	int readRecords(uint32_t serial, uint16_t object, uint32_t from_idx, uint32_t to_idx,
			Record *records, int *len, RecordType type);

	int requestArchiveData(uint32_t serial, uint16_t objId, time_t from, time_t to);

	int logout();

	int discoverDevices(int deviceNum);

	void addDevice(uint32_t serial, char *mac);

	int sendPassword(const char *password, UserType user);

	int ackAuth(uint32_t serial);

	int authenticate(const char *password, UserType user);

	int syncTime();

	int readTags(const std::string& file);

	Connection *connection;
	Smabluetooth sma;
	Smanet smanet;

	uint16_t transaction_cntr; // Packet counter
	bool transaction_active;

	std::vector<Device> devices;

	struct Tag {
		std::string shortDesc;
		std::string longDesc;

		Tag(std::string shortDesc, std::string longDesc) :
			shortDesc(std::move(shortDesc)),
			longDesc(std::move(longDesc)) { }
	};

	std::unordered_map<uint32_t, Tag> tagMap;
};

#endif /* #ifndef SMADATA2PLUS_H */
