/*
 *   Pvlib - Smabluetooth implementation
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

#include <Smabluetooth.h>
#include <cstring>
#include <cstddef>
#include <cstdlib>
#include <cstdbool>
#include <cassert>
#include <algorithm>

#include "Log.h"
#include "Connection.h"

namespace pvlib {

#define HEADER_SIZE 18
#define TIMEOUT 5000

static const uint8_t MAC_BROADCAST[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
static const uint8_t MAC_NULL[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


using LockGuard = std::lock_guard<std::mutex>;
using UniqueLock = std::unique_lock<std::mutex>;

static int parse_header(uint8_t *buf, Smabluetooth::Packet *packet) {
	if (buf[0] != 0x7e) {
		LOG(Error) << "Invalid header!";
		return -1;
	}
	if (buf[0] ^ buf[1] ^ buf[2] ^ buf[3]) {
		LOG(Error) << "Broken header!";
		return -1;
	}

	if (buf[1] < HEADER_SIZE) {
		LOG(Error) << "Broken header!";
		return -1;
	}
	packet->len = buf[1] - HEADER_SIZE;

	memcpy(packet->mac_src, &buf[4], 6);
	memcpy(packet->mac_dst, &buf[10], 6);

	packet->cmd = buf[16];

	return 0;
}

int Smabluetooth::cmd_02(const Packet *packet) {
	uint8_t net_id;
	Packet answer;

	if (packet->len != 13) {
		LOG(Error) << "cmd 0x02 packet invalid length: " << packet->len << " instead of 13";
		return -1;
	}

	LOG(Trace) << "Got command 02";

	UniqueLock lock(mutex);
	if (state != STATE_NOT_CONNECTED) {
		return 0; //ignore packet
	}


	net_id = packet->data[4];
	lock.unlock();

	answer.cmd = 0x02;
	answer.len = 13;
	memcpy(answer.mac_dst, packet->mac_src, 6);
	memcpy(answer.mac_src, packet->mac_dst, 6);
	memcpy(answer.data, packet->data, 13);

	if (writePacket(&answer) < 0) {
		return -1;
	}

	lock.lock();
	state = STATE_START;
	lock.unlock();

	event.notify_all();

	return 0;
}

//Get our mac + mac of direct connection partner
int Smabluetooth::cmd_0A(const Packet *packet) {
	if (packet->len != 13) {
		LOG(Error) << "Ignoring packet, invalid length: " << packet->len << " instead of 13!";
		return 0;
	}

	LOG(Trace) << "Got command 0A";

	LockGuard lock(mutex);
	if (state != STATE_START) {
		return 0; //Ignore packet
	}
	memcpy(mac, &packet->data[7], 6);
	memcpy(mac_inv, &packet->data[0], 6);

	state = STATE_MAC;

	return 0;
}

//Get all devices in network
int Smabluetooth::cmd_05(const Packet *packet) {
	int num_devices;
	if ((packet->len % 8) != 0) {
		LOG(Error) <<"Invalid packet!";
		return -1;
	}

	LOG(Trace) << "Got command 05";

	num_devices = packet->len / 8 - 1;
	LOG(Info) << "Found " << num_devices <<" devices";

	if (num_devices == 0) {
		LOG(Error) << "Invalid cmd 05 packet!";
		return -1;
	}

	UniqueLock lock(mutex);
	if (state != STATE_MAC) {
		return 0; //ignore packet
	}
	this->num_devices = num_devices;
	state = STATE_DEVICE_LIST;
	lock.unlock();

	event.notify_all();

	return 0;
}

//Signal query response
int Smabluetooth::cmd_04(const Packet *packet) {
	if (packet->len != 6) {
		return 0; //Ignnore
	}
	if (packet->data[0] != 0x05) {
		return 0;
	}

	LOG(Trace) << "Got command 04";

	mutex.lock();
	signalStrength = packet->data[4] * 100 / 0xff;
	events |= EVENT_SIGNAL_STRENGTH;
	mutex.unlock();

	event.notify_all();

	return 0;
}

int Smabluetooth::packet_event(const Packet *packet) {
	switch (packet->cmd) {
	case 0x02:
		if (cmd_02(packet) < 0) return -1;
		break;
	case 0x0A:
		if (cmd_0A(packet) < 0) return -1;
		break;
	case 0x05:
		if (cmd_05(packet) < 0) return -1;
		break;
	case 0x04:
		if (cmd_04(packet) < 0) return -1;
		break;
	default:
		LOG(Error) << "Got unknown cmd: " << packet->cmd;
	}

	return 0;
}

//read requested number of bytes
//if we got null bytes and timeout return 0
//if we got bytes but not requested size return -1
static int read_complete_len(Connection *con, uint8_t *data, int len, int timeout) {
	int ret = 0;
	int pos = 0;

	do {
		if ((ret = con->read(data + pos, len)) < 0) {
			return -1;
		}
		len -= ret;
		pos += ret;

		if (ret == 0 && pos != 0) {
			return -1; //we got part of data and then timeout
		}

	} while (pos != 0 && len > 0);

	return pos;
}

void Smabluetooth::worker_thread() {
	uint8_t buf[HEADER_SIZE];
	Packet packet;
	int ret;
	bool for_us;

	while (!quit.load()) {
		ret = read_complete_len(con, buf, HEADER_SIZE, TIMEOUT);

		if (ret < 0) {
			goto error;
		} else if (ret == 0) {
			continue;
		}

		if (parse_header(buf, &packet) < 0) {
			goto error;
		}

		if ((ret = read_complete_len(con, packet.data, packet.len, TIMEOUT)) < 0) {
			goto error;
		}

		//LOG_DEBUG("Got header");

		//no lock required for sma->mac, only we(this thread) change it.
		for_us = (memcmp(packet.mac_dst, mac, 6) == 0) || (memcmp(packet.mac_dst, MAC_NULL, 6)
		        == 0) || (memcmp(packet.mac_dst, MAC_BROADCAST, 6) == 0);

		if (((packet.cmd == 0x01) || (packet.cmd == 0x08)) && for_us) {
			LOG(Trace) << "received smadata2plus packet:\n" << print_array(packet.data, packet.len);
			mutex.lock();
			if (packets.size() >= MAX_PACKETS_SIZE) { //remove old packets
				packets.pop();
			}
			packets.push(packet);
			mutex.unlock();

			event.notify_all();
		} else {
			LOG(Trace) << "received non smadata2plus packet:\n" << print_array(packet.data, packet.len);

			if (for_us) {
				if (packet_event(&packet) < 0) {
					goto error;
				}
			}
		}
	}

	return;

error:
	mutex.lock();
	state = STATE_ERROR;
	mutex.unlock();

}

Smabluetooth::Smabluetooth(Connection *con) :
		con(con),
		state(STATE_NOT_CONNECTED),
		num_devices(0),
		signalStrength(0),
		events(0) {

}

Smabluetooth::~Smabluetooth() {
	disconnect();
}

int Smabluetooth::connect() {
	disconnect();

	//start thread
	quit.store(false);
	thread = std::thread([this] { worker_thread(); });

	LOG(Info) << "Connecting to device!";
	UniqueLock lock(mutex);
	//Wait until we get list of all devices
	while (state != STATE_DEVICE_LIST) {
		if (state == STATE_ERROR) {
			quit.store(true);
			thread.join();
			state = STATE_NOT_CONNECTED;
			return -1;
		}

		if (event.wait_for(lock, std::chrono::seconds(5)) == std::cv_status::timeout) {
			LOG(Error) << "Connection timeout!";
			quit.store(true);
			thread.join();
			state = STATE_NOT_CONNECTED;
			return -1;
		}
	}
	state = STATE_CONNECTED;
	lock.unlock();

	LOG(Info) << "Connected to device!";

	int signalStrength = getSignalStrength(mac_inv);
	LOG(Info) <<"Signal strength: " << signalStrength;

	return 0;
}

void Smabluetooth::disconnect() {
	State state;
	UniqueLock lock(mutex);
	state = this->state;
	lock.unlock();

	if (state == STATE_NOT_CONNECTED) {
		return;
	}

	quit.store(true);
	thread.join();

	lock.lock();
	state = STATE_NOT_CONNECTED;
}

int Smabluetooth::getDeviceNum() {
	LockGuard lock(mutex);
	return num_devices;
}

int Smabluetooth::writePacket(const Packet *packet)
{
	uint8_t buf[0xff];
	uint16_t len;
	int ret;

	len = packet->len + HEADER_SIZE;
	if (len > 0xff) {
		LOG(Error) << "Invalid data length: " << packet->len;
		return -1;
	}

	buf[0] = 0x7e;
	buf[1] = (uint8_t) len;
	buf[2] = 0x00;
	buf[3] = buf[0] ^ buf[1] ^ buf[2];

	memcpy(buf + 4, mac, 6);
	memcpy(buf + 10, packet->mac_dst, 6);

	buf[16] = packet->cmd;
	buf[17] = 0x00;

	memcpy(&buf[18], packet->data, packet->len);

	LOG(Trace) << "smabluetooth, write:\n" << print_array(buf, len);

	if ((ret = con->write(buf, len)) < 0) {
		LOG(Error) << "Failed writing data.";
		return ret;
	}

	return 0;
}

int Smabluetooth::write(const uint8_t *data, int len, const std::string &to) {
	Packet packet;

	//packet.data = data;
	packet.len = len;
	packet.cmd = 0x01;

	if (to.size() != 6) {
		LOG(Error) << "Invalid destination: " << to;
		return -1;
	}

	if (len > (int)sizeof(packet.data)) {
		LOG(Error) << "Invalid data size: " << len << ", can be maximal: " << sizeof(packet.data);
		return -1;
	}

	memcpy(packet.data, data, len);
	memcpy(packet.mac_dst, to.c_str(), 6);
	return writePacket(&packet);
}

int Smabluetooth::readPacket(Packet *packet) {
	UniqueLock lock(mutex);

	if (state != STATE_CONNECTED) {
		return -1;
	}

	while (packets.empty()) {
		if(event.wait_for(lock, std::chrono::seconds(5)) == std::cv_status::timeout) {
			return 0;
		}
	}

	*packet = packets.front(); packets.pop();

	return packet->len;
}

int Smabluetooth::read(uint8_t *data, int maxlen, std::string &from) {
	Packet packet;
	int ret;

	if ((ret = readPacket(&packet)) <= 0) {
		return ret;
	}

	//FIXME: data buffering do not discard left packet data
	int dataLen = std::min(static_cast<int>(packet.len), maxlen);
	from = std::string((char*)packet.mac_src, 6);
	memcpy(data, packet.data, maxlen);

	return dataLen;
}

int Smabluetooth::getSignalStrength(const uint8_t *mac)
{
	Packet packet;

	UniqueLock lock(mutex);
	if (state != STATE_CONNECTED) {
		return -1;
	}
	events &= ~EVENT_SIGNAL_STRENGTH;
	lock.unlock();

	packet.len = 2;
	packet.cmd = SMABLUETOOTH_ASKSIGNAL;
	memcpy(packet.mac_dst, mac, 6);

	packet.data[0] = 0x05;
	packet.data[1] = 0x00;

	if (writePacket(&packet) < 0) {
		return -1;
	}

	lock.lock();
	while (!(events & EVENT_SIGNAL_STRENGTH)) {
		if(event.wait_for(lock, std::chrono::seconds(5)) == std::cv_status::timeout) {
			return -1;
		}
	}
	events &= ~EVENT_SIGNAL_STRENGTH;
	return signalStrength;
}

} //namespace pvlib {
