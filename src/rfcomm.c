/*
 *   Pvlib - Bluetooth rfcomm implementation
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>

//workaround for libbluetooth
//gcc compiled with c99 or c++0x does nor define typeof
//which is needed by libbluetooth
//
#ifdef __GNUC__
#	ifndef typeof
#		define typeof __typeof__
#		define RFCOMM_TYPEOF_DEFINED
#	endif
#endif// #ifndef __GNUCC__

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#ifdef __GNUC__
#	ifdef RFCOMM_TYPEOF_DEFINED
#		undef typeof
#	endif
#endif

#include "log.h"
#include "connection.h"

#define TIMEOUT 5 /* in seconds */

struct rfcomm_handle {
	int socket;
	uint8_t src_mac[6];
	uint8_t dst_mac[6];
	char src_name[128];
	char dst_name[128];
};

static void rfcomm_close(connection_t *con)
{
	struct rfcomm_handle *rfcomm;

	rfcomm = (struct rfcomm_handle *) con->handle;
	if (rfcomm == NULL) return;

	close(rfcomm->socket);
	free(rfcomm);
	free(con);
}

static int rfcomm_write(connection_t *con, const uint8_t *data, int len)
{
	return send(((struct rfcomm_handle*) con->handle)->socket, data, len, 0);
}

static int rfcomm_read(connection_t *con, uint8_t *data, int max_len, int timeout)
{
	struct timeval tv;
	fd_set rdfds;
	int s;

	s = ((struct rfcomm_handle*) con->handle)->socket;

	FD_ZERO(&rdfds);
	FD_SET(s, &rdfds);

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	if (select(s + 1, &rdfds, NULL, NULL, &tv) < 0) {
		LOG_ERROR("rfcomm select error!");
		return -1;
	}

	if (FD_ISSET(s, &rdfds)) {
		int ret;
		ret = recv(s, data, max_len, 0);
		if (ret <= 0) {
			LOG_ERROR("Error reading data: %s", strerror(errno));
			return -1;
		}
		return ret;
	} else {
		return 0;
	}
}

static int rfcomm_info(connection_t *con, connection_data_t *info)
{
	struct rfcomm_handle *rfcomm;
	rfcomm = (struct rfcomm_handle *) con->handle;

	memcpy(info->dst_address, rfcomm->dst_mac, 6);
	strncpy(info->dst_name, rfcomm->dst_name, CONNECTION_MAX_NAME - 1);
	info->dst_name[CONNECTION_MAX_NAME - 1] = '\0';

	memcpy(info->src_address, rfcomm->src_mac, 6);
	strncpy(info->src_name, rfcomm->src_name, CONNECTION_MAX_NAME - 1);
	info->src_name[CONNECTION_MAX_NAME - 1] = '\0';

	info->address_len = 6;

	return 0;
}

static int rfcomm_connect(connection_t *con, const char *address, const void *param)
{
	int s;
	struct sockaddr_rc addr;
	int dev_id;
	struct rfcomm_handle *rfcomm;

	dev_id = hci_get_route(NULL);
	if (dev_id < 0) {
		LOG_ERROR("Failed finding bluetooth device: %s!", strerror(errno));
		return -1;
	}

	s = hci_open_dev(dev_id);
	if (s < 0) {
		LOG_ERROR("Opening bluetooth device failed: %s!", strerror(errno));
		return -1;
	}

	rfcomm = malloc(sizeof(*rfcomm));

	if (str2ba(address, (bdaddr_t*) rfcomm->dst_mac) < 0) {
		LOG_ERROR("Failed reading device bluetooth address: %s!", strerror(errno));
		goto err;
	}

	if (hci_read_local_name(s, 128, rfcomm->src_name, 1000) < 0) {
		LOG_INFO("Failed reading local bluetooth device name: %s!", strerror(errno));
		rfcomm->src_name[0] = '\0';
		goto err;
	}

	if (hci_read_bd_addr(s, (bdaddr_t*) rfcomm->src_mac, 1000) < 0) {
		LOG_ERROR("Failed reading local mac address, %s!", strerror(errno));
		goto err;
	}

	if (hci_read_remote_name(s, (bdaddr_t*) rfcomm->dst_mac, 128, rfcomm->dst_name, TIMEOUT * 1000)
	        < 0) {
		LOG_ERROR("Failed reading remote name: %s", strerror(errno));
		goto err;
	}

	close(s);
	s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (s < 0) {
		LOG_ERROR("Failed opening bluetooth socket: %s", strerror(errno));
		free(rfcomm);
		return -1;
	}

	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = (uint8_t) 1;

	if (str2ba(address, &addr.rc_bdaddr) < 0) {
		goto err;
	}

	if (connect(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		LOG_ERROR("Failed connecting to remote: %s", strerror(errno));
		goto err;
	}

	rfcomm->socket = s;

	con->handle = rfcomm;

	LOG_INFO("RFCOMM: Successfully established connection.");
	return 0;

	err: free(rfcomm);
	close(s);
	return -1;
}

int rfcomm_open(connection_t *con, const char *address, const void *param)
{
	con->handle = NULL;
	con->write = rfcomm_write;
	con->read = rfcomm_read;
	con->info = rfcomm_info;
	con->close = rfcomm_close;
	//    con->connect = rfcomm_connect;

	return rfcomm_connect(con, address, param);
}

const connection_info_t connection_info_rfcomm = { "rfcomm", "pvlogdev", "", rfcomm_open };
