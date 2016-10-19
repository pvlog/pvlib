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

#ifndef SMABLUETOOTH_H
#define SMABLUETOOTH_H

#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

#include "ReadWrite.h"

#define SMABLUETOOTH_CONNECT 0x02
#define SMABLUETOOTH_ADDRESS 0x0a
#define SMABLUETOOTH_ADDRESS2 0x05
#define SMABLUETOOTH_ASKSIGNAL 0x03
#define SMABLUETOOTH_ANSWERSIGNAL 0x04

class Connection;

class Smabluetooth : public ReadWrite {
public:
	static const int HEADER_SIZE = 18;

	struct Packet {
		uint8_t mac_src[6];              //< source mac
		uint8_t mac_dst[6];              //< destination mac
		uint8_t cmd;                     //< packet commando
		uint8_t data[255 - HEADER_SIZE]; //< packet data
		uint8_t len;                     //< data len
	};


	/**
	 * Setup smabluetooth.
	 *
	 * @param con connection used for sending and receiving data.
	 *
	 */
	Smabluetooth(Connection *con);

	/**
	 * Close smabluetooth.
	 */
	virtual ~Smabluetooth();

	/**
	 * Send data.
	 *
	 * @param sma smabluetooth handle.
	 * @param packet @see smabluetooth_packet_t.
	 * @return < 0 if error occured, else >= 0.
	 */
	int writePacket(const Packet *packet);

	/**
	 * Write data.
	 */
	virtual int write(const uint8_t *data, int len, const std::string &to) override;

	/**
	 * Read data.
	 * Note: Reads always one and only one smabluetooth frame.
	 * If given size is below smabluetooth frame size left bytes will be removed.
	 * If given size greater than smabluetooth frame size only size of frame will be read.
	 * So always check return size!
	 *
	 * @param packet packet @see smabluetooth_packet_t.
	 * @return < 0 if error occured, else amount of bytes read.
	 */
	int readPacket(Packet *packet);

	/**
	 * Read data
	 */
	virtual int read(uint8_t *data, int maxlen, std::string &from) override;

	/**
	 * Connect to string convertet.
	 *
	 * @param sma smabluetooth handle.
	 * @return < 0 if error occurs, else > 0.
	 */
	int connect();

	/**
	 * Disconnect.
	 */
	void disconnect();

	/**
	 * Get number of devices in network.
	 *
	 * @param sma smabluetooth handle.
	 * @return number of devices
	 */
	 int getDeviceNum();

	/**
	 * Get signal strength.
	 *
	 * @param sma smabluetooth handle.
	 * @return signal strength from 0 to 100 inclusive, if error occurs < 0.
	 */
	int getSignalStrength(const uint8_t *mac);
private:
	int cmd_02(const Packet *packet);

	int cmd_0A(const Packet *packet);

	int cmd_04(const Packet *packet);

	int cmd_05(const Packet *packet);

	int packet_event(const Packet *packet);

	void worker_thread();

	enum State {
		STATE_ERROR,
		STATE_NOT_CONNECTED,
		STATE_START,
		STATE_MAC,
		STATE_DEVICE_LIST,
		STATE_CONNECTED
	};

	enum Event {
		EVENT_NO_EVENT        = 0,
		EVENT_STATE_CHANGE    = 1,
		EVENT_SIGNAL_STRENGTH = 2
	};


	Connection *con;
	bool connected;

	State state;
	int num_devices;
	uint8_t mac[6];
	uint8_t mac_inv[6];
	int signalStrength;


	std::mutex mutex;
	std::condition_variable event;
	std::thread thread;
	std::atomic_bool quit;

	int events;

	std::queue<Packet> packets;
	const static size_t MAX_PACKETS_SIZE = 40;
};

#endif /* #ifndef SMABLUETOOTH_H */
