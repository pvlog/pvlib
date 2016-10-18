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

	virtual int readEventData(uint32_t serial, time_t from, time_t to, UserType user);

private:
	int writeReplay(const Packet *packet, uint16_t transactionCntr);

	int write(const Packet *packet);

	int read(Packet *packet);

	int requestChannel(uint16_t channel, uint32_t fromIdx, uint32_t toIdx);

	int readRecords(uint16_t object, uint32_t from_idx, uint32_t to_idx,
			Record *records, int *len, RecordType type);

	int logout();

	int discoverDevices(int deviceNum);

	void addDevice(uint32_t serial, char *mac);

	int sendPassword(const char *password, UserType user);

	int ackAuth(uint32_t serial);

	int authenticate(const char *password, UserType user);

	int syncTime();

	Connection *connection;
	Smabluetooth sma;
	Smanet smanet;

	uint16_t transaction_cntr; // Packet counter
	bool transaction_active;

	std::vector<Device> devices;
};

#endif /* #ifndef SMADATA2PLUS_H */
