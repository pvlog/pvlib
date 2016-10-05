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

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "smabluetooth.h"
#include "log.h"
#include "thread.h"

#define HEADER_SIZE 18
#define TIMEOUT 5000

static const uint8_t MAC_BROADCAST[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
static const uint8_t MAC_NULL[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

//TODO: Better names for states
typedef enum {
	STATE_ERROR,
	STATE_NOT_CONNECTED,
	STATE_START,
	STATE_MAC,
	STATE_DEVICE_LIST,
	STATE_CONNECTED
} state_t;

typedef enum {
	EVENT_NO_EVENT,
	EVENT_STATE_CHANGE,
	EVENT_SIGNAL_STRENGTH
} event_t;

struct smabluetooth_s {
	connection_t *con;
	connection_data_t info;
	int connected;

	state_t state;
	int num_devices;
	uint8_t mac[6];
	uint8_t mac_inv[6];
	int signal;

	uint8_t buf[0xff - HEADER_SIZE];
	smabluetooth_packet_t packet;

	thread_t thread;
	bool quit;

	thread_cond_t event;
	event_t event_type;

	thread_sem_t used;
	thread_sem_t free;

	thread_mutex_t mutex;
	thread_mutex_t write_mutex;
};

static bool check_state(smabluetooth_t *sma, state_t state)
{
	state_t is_state;

	thread_mutex_lock(&sma->mutex);
	is_state = sma->state;
	thread_mutex_unlock(&sma->mutex);

	return (state == is_state);
}

static void set_state(smabluetooth_t *sma, state_t state)
{
	thread_mutex_lock(&sma->mutex);
	if (sma->state != state) {
		sma->state = state;
		sma->event_type = EVENT_STATE_CHANGE;
		thread_mutex_unlock(&sma->mutex);
		thread_cond_signal(&sma->event);
	} else {
		thread_mutex_unlock(&sma->mutex);
	}
}

//static int wait_event(smabluetooth_t *sma, event_t event, int timeout)
//{
//	thread_mutex_lock(&sma->mutex);
//	while (sma->event_type != event) {
//		if (thread_cond_timedwait(&sma->event, &sma->mutex, timeout) < 0) {
//			return -1;
//		}
//	}
//	sma->event_type = EVENT_NO_EVENT;
//	thread_mutex_unlock(&sma->mutex);
//
//	return 0;
//}

static int parse_header(uint8_t *buf, smabluetooth_packet_t *packet)
{
	if (buf[0] != 0x7e) {
		LOG_ERROR("Invalid header!");
		return -1;
	}
	if (buf[0] ^ buf[1] ^ buf[2] ^ buf[3]) {
		LOG_ERROR("Broken header!");
		return -1;
	}

	if (buf[1] < HEADER_SIZE) {
		LOG_ERROR("Broken header!");
		return -1;
	}
	packet->len = buf[1] - HEADER_SIZE;

	memcpy(packet->mac_src, &buf[4], 6);
	memcpy(packet->mac_dst, &buf[10], 6);

	packet->cmd = buf[16];

	return 0;
}

static int cmd_02(smabluetooth_t *sma, smabluetooth_packet_t *packet)
{
	uint8_t net_id;
	uint8_t buf[13];
	smabluetooth_packet_t answer;

	if (!check_state(sma, STATE_NOT_CONNECTED)) {
		return 0; //ignore packet
	}

	if (packet->len != 13) {
		LOG_ERROR("cmd 0x02 packet invalid length: %d instead of 13", packet->len);
		return -1;
	}

	net_id = packet->data[4];

	answer.cmd = 0x02;
	answer.data = buf;
	answer.len = 13;
	memcpy(answer.mac_dst, packet->mac_src, 6);
	memcpy(answer.mac_src, packet->mac_dst, 6);
	memcpy(answer.data, packet->data, 13);

	if (smabluetooth_write(sma, &answer) < 0) {
		return -1;
	}

	set_state(sma, STATE_START);
	return 0;
}

//Get our mac + mac of direct connection partner
static int cmd_0A(smabluetooth_t *sma, smabluetooth_packet_t *packet)
{
	if (!check_state(sma, STATE_START)) {
		return 0; //ignore packet
	}

	if (packet->len != 13) {
		LOG_ERROR("Ignoring packet, invalid length: %d instead of 13!", packet->len);
		return 0;
	}

	thread_mutex_lock(&sma->mutex);
	memcpy(sma->mac, &packet->data[7], 6);
	memcpy(sma->mac_inv, &packet->data[0], 6);
	thread_mutex_unlock(&sma->mutex);

	set_state(sma, STATE_MAC);

	return 0;
}

//Get all devices in network
static int cmd_05(smabluetooth_t *sma, smabluetooth_packet_t *packet)
{
	int num_devices;

	if (!check_state(sma, STATE_MAC)) {
		return 0; //ignore packet
	}
	if ((packet->len % 8) != 0) {
		LOG_ERROR("Invalid packet!");
		return -1;
	}

	num_devices = packet->len / 8 - 1;
	LOG_INFO("Found %d devices", num_devices);

	if (num_devices == 0) {
		LOG_ERROR("Invalid cmd 05 packet!");
		return -1;
	}

	thread_mutex_lock(&sma->mutex);
	sma->num_devices = num_devices;
	thread_mutex_unlock(&sma->mutex);

	set_state(sma, STATE_DEVICE_LIST);

	return 0;
}

//Signal query response
static int cmd_04(smabluetooth_t *sma, smabluetooth_packet_t *packet)
{
	if (packet->len != 6) {
		return 0; //Ignnore
	}
	if (packet->data[0] != 0x05) {
		return 0;
	}

	thread_mutex_lock(&sma->mutex);
	sma->signal = packet->data[4] * 100 / 0xff;
	sma->event_type = EVENT_SIGNAL_STRENGTH;
	thread_mutex_unlock(&sma->mutex);

	thread_cond_signal(&sma->event);

	return 0;
}

static int packet_event(smabluetooth_t *sma, smabluetooth_packet_t *packet)
{
	switch (packet->cmd) {
	case 0x02:
		if (cmd_02(sma, packet) < 0) return -1;
		break;
	case 0x0A:
		if (cmd_0A(sma, packet) < 0) return -1;
		break;
	case 0x05:
		if (cmd_05(sma, packet) < 0) return -1;
		break;
	case 0x04:
		if (cmd_04(sma, packet) < 0) return -1;
		break;
	default:
		LOG_ERROR("Got unknown cmd: %d", packet->cmd);
	}

	return 0;
}
static void *worker_thread(void *arg)
{
	smabluetooth_t *sma;
	uint8_t buf[0xff - HEADER_SIZE];
	smabluetooth_packet_t packet;
	int ret;
	bool for_us;

	packet.data = buf;
	sma = (smabluetooth_t*) arg;

	while (1) {
		int quit;
		thread_mutex_lock(&sma->mutex);
		quit = sma->quit;
		thread_mutex_unlock(&sma->mutex);
		if (quit) {
			return NULL;
		}

		ret = connection_read(sma->con, buf, HEADER_SIZE, TIMEOUT);

		if (ret < 0) {
			goto error;
		} else if (ret == 0) {
			continue;
		} else if (ret != HEADER_SIZE) {
			goto error;
		}

		if (parse_header(buf, &packet) < 0) {
			goto error;
		}

		//LOG_DEBUG("Got header");

		//no lock required for sma->mac, only we(this thread) change it.
		for_us = (memcmp(packet.mac_dst, sma->mac, 6) == 0) || (memcmp(packet.mac_dst, MAC_NULL, 6)
		        == 0) || (memcmp(packet.mac_dst, MAC_BROADCAST, 6) == 0);

		if (((packet.cmd == 0x01) || (packet.cmd == 0x08)) && for_us) {
				//smadata2plus data
			thread_sem_aquire(&sma->free);
			memcpy(&sma->packet, &packet, sizeof(packet));
			sma->packet.data = sma->buf;
			if ((sma->packet.len = connection_read(sma->con, sma->packet.data, sma->packet.len, TIMEOUT)) < 0) {
				goto error;
			}

			LOG_TRACE_HEX("received smabluetooth packet:", packet.data, packet.len);

			thread_sem_release(&sma->used);
		} else {
			if ((sma->packet.len = connection_read(sma->con, packet.data, packet.len, TIMEOUT)) < 0) {
				goto error;
			}

			LOG_TRACE_HEX("received non smadata2plus packet:", packet.data, packet.len);

			if (for_us) {
				if (packet_event(sma, &packet) < 0) {
					goto error;
				}
			}
		}
	}

error:
	set_state(sma, STATE_ERROR);
	LOG_ERROR("Leaving worker thread after error!");
	return NULL;
}

int smabluetooth_connect(smabluetooth_t *sma)
{
	if (thread_create(&sma->thread, worker_thread, sma) < 0) {
		return -1;
	}

	thread_mutex_lock(&sma->mutex);

	//Wait until we get list of all devices
	while (sma->state != STATE_DEVICE_LIST) {
		if (sma->state == STATE_ERROR) {
			thread_mutex_unlock(&sma->mutex);
			return -1;
		}

		if (thread_cond_timedwait(&sma->event, &sma->mutex, 15000) < 0) {
			LOG_ERROR("Connection timeout!");
			thread_mutex_unlock(&sma->mutex);
			return -1;
		}
	}
	thread_mutex_unlock(&sma->mutex);

	//we are connected!
	set_state(sma, STATE_CONNECTED);

	//Get connection quality
	// FIXME: we need lock for sma->mac_inv!!!!!
	smabluetooth_signal(sma, sma->mac_inv);

	return 0;
}

void smabluetooth_disconnect(smabluetooth_t *sma)
{
    thread_mutex_lock(&sma->mutex);
    sma->quit = true;
    thread_mutex_unlock(&sma->mutex);

    thread_join(sma->thread);

    set_state(sma, STATE_NOT_CONNECTED);
}

int smabluetooth_device_num(smabluetooth_t *sma)
{
	int ret;

	thread_mutex_lock(&sma->mutex);
	ret = sma->num_devices;
	thread_mutex_unlock(&sma->mutex);

	return ret;
}

smabluetooth_t *smabluetooth_init(connection_t *con)
{
	smabluetooth_t *sma;

	assert(con != NULL);

	sma = calloc(1, sizeof(*sma));
	sma->con = con;
	sma->state = STATE_NOT_CONNECTED;
	sma->packet.data = sma->buf;

	thread_mutex_create(&sma->mutex);
	thread_mutex_create(&sma->write_mutex);
	thread_cond_create(&sma->event);
	thread_sem_create(&sma->used, 0);
	thread_sem_create(&sma->free, 1);

	return sma;
}

void smabluetooth_close(smabluetooth_t *sma)
{
	if (sma->state != STATE_NOT_CONNECTED) {
		smabluetooth_disconnect(sma);
	}
	thread_mutex_destroy(&sma->mutex);
	thread_cond_destroy(&sma->event);
	thread_sem_destroy(&sma->used);
	thread_sem_destroy(&sma->free);
	free(sma);
}

int smabluetooth_write(smabluetooth_t *sma, smabluetooth_packet_t *packet)
{
	uint8_t buf[0xff];
	uint16_t len;

	len = packet->len + HEADER_SIZE;
	if (len > 0xff) {
		LOG_ERROR("data too long");
		return -1;
	}

	buf[0] = 0x7e;
	buf[1] = (uint8_t) len;
	buf[2] = 0x00;
	buf[3] = buf[0] ^ buf[1] ^ buf[2];

	thread_mutex_lock(&sma->mutex);
	memcpy(&buf[4], sma->mac, 6);
	thread_mutex_unlock(&sma->mutex);

	memcpy(&buf[10], packet->mac_dst, 6);

	buf[16] = packet->cmd;
	buf[17] = 0x00;

	memcpy(&buf[18], packet->data, packet->len);

	LOG_TRACE_HEX("smabluetooth, write", buf, len);

	thread_mutex_lock(&sma->write_mutex);
	if (connection_write(sma->con, buf, len) < 0) {
		thread_mutex_unlock(&sma->write_mutex);
		LOG_ERROR("Failed writing data.");
		return -1;
	}
	thread_mutex_unlock(&sma->write_mutex);
	return 0;
}

int smabluetooth_read(smabluetooth_t *sma, smabluetooth_packet_t *packet)
{
	int len;

	if (thread_sem_timedaquire(&sma->used, 5000) < 0) {
		LOG_ERROR("Packet timeout!");
		return -1;
	}

	len = (packet->len > sma->packet.len) ? sma->packet.len : packet->len;

	packet->cmd = sma->packet.cmd;
	packet->len = len;
	memcpy(packet->mac_dst, sma->packet.mac_dst, 6);
	memcpy(packet->mac_src, sma->packet.mac_src, 6);
	memcpy(packet->data, sma->packet.data, len);

	thread_sem_release(&sma->free);

	return 0;
}

int smabluetooth_signal(smabluetooth_t *sma, uint8_t *mac)
{
	smabluetooth_packet_t packet;
	uint8_t buf[3];
	int signal;

	if (!check_state(sma, STATE_CONNECTED)) {
		LOG_ERROR("Not Connected!");
		return -1;
	}

	packet.data = buf;
	packet.len = 2;
	packet.cmd = SMABLUETOOTH_ASKSIGNAL;
	memcpy(packet.mac_dst, mac, 6);

	buf[0] = 0x05;
	buf[1] = 0x00;

	if (smabluetooth_write(sma, &packet) < 0) {
		return -1;
	}

	thread_mutex_lock(&sma->mutex);
	while (sma->event_type != EVENT_SIGNAL_STRENGTH) {
		thread_cond_timedwait(&sma->event, &sma->mutex, 5000);
	}
	sma->event_type = EVENT_NO_EVENT;
	signal = sma->signal;
	thread_mutex_unlock(&sma->mutex);

	return signal;
}
