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

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "Rfcomm.h"

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

#include "Log.h"
#include <Connection.h>

const static int TIMEOUT = 5; /* in seconds */

Rfcomm::Rfcomm() :
		connected(false),
		timeout(TIMEOUT),
		socket(0) {

}

Rfcomm::~Rfcomm() {
	if (connected) {
		disconnect();
	}
}

int Rfcomm::connect(const char *address, const void *param) {
	struct sockaddr_rc addr;
	int dev_id;
	int s;

	if (connected) {
		disconnect();
	}

	dev_id = hci_get_route(NULL);
	if (dev_id < 0) {
		LOG(Error) << "Failed finding bluetooth device: " << strerror(errno);
		return -1;
	}

	s = hci_open_dev(dev_id);
	if (socket < 0) {
		LOG(Error) << "Opening bluetooth device failed: " << strerror(errno);
		return -1;
	}

	//rfcomm = malloc(sizeof(*rfcomm));

	if (str2ba(address, (bdaddr_t*)dst_mac) < 0) {
		LOG(Error) << "Failed reading device bluetooth address: " << strerror(errno);
		goto err;
	}

	if (hci_read_local_name(s, 128, src_name, 1000) < 0) {
		LOG(Error) << "Failed reading local bluetooth device name: " << strerror(errno);
		src_name[0] = '\0';
		goto err;
	}

	if (hci_read_bd_addr(s, (bdaddr_t*)src_mac, 1000) < 0) {
		LOG(Error) << "Failed reading local mac address: " << strerror(errno);
		goto err;
	}

	if (hci_read_remote_name(s, (bdaddr_t*)dst_mac, 128, dst_name, TIMEOUT * 1000)
	        < 0) {
		LOG(Error) << "Failed reading remote name: " << strerror(errno);
		goto err;
	}

	close(s);
	s = ::socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (s < 0) {
		LOG(Error) << "Failed opening bluetooth socket: " << strerror(errno);
		return -1;
	}

	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = (uint8_t) 1;

	if (str2ba(address, &addr.rc_bdaddr) < 0) {
		goto err;
	}

	if (::connect(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		LOG(Error) << "Failed connecting to remote: " << strerror(errno);
		goto err;
	}

	socket = s;
	connected = true;
	LOG(Info) << "RFCOMM: Successfully established connection.";
	return 0;

err:
	close(s);
	return -1;
}



int Rfcomm::write(const uint8_t *data, int len, const std::string& to)
{
	return send(socket, data, len, 0);
}

int Rfcomm::read(uint8_t *data, int max_len, std::string& from)
{
	struct timeval tv;
	fd_set rdfds;
	int s;

	s = socket;

	FD_ZERO(&rdfds);
	FD_SET(s, &rdfds);

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	if (select(s + 1, &rdfds, NULL, NULL, &tv) < 0) {
		LOG(Error) << "rfcomm select error!";
		return -1;
	}

	if (FD_ISSET(s, &rdfds)) {
		int ret;
		ret = recv(s, data, max_len, 0);
		if (ret <= 0) {
			LOG(Error) << "Error reading data: " << strerror(errno);
			return -1;
		}
		return ret;
	} else {
		return 0;
	}
}

//static int Rfcomm::info( connection_data_t *info)
//{
//	struct rfcomm_handle *rfcomm;
//	rfcomm = (struct rfcomm_handle *) con->handle;
//
//	memcpy(info->dst_address, rfcomm->dst_mac, 6);
//	strncpy(info->dst_name, rfcomm->dst_name, CONNECTION_MAX_NAME - 1);
//	info->dst_name[CONNECTION_MAX_NAME - 1] = '\0';
//
//	memcpy(info->src_address, rfcomm->src_mac, 6);
//	strncpy(info->src_name, rfcomm->src_name, CONNECTION_MAX_NAME - 1);
//	info->src_name[CONNECTION_MAX_NAME - 1] = '\0';
//
//	info->address_len = 6;
//
//	return 0;
//}


void Rfcomm::disconnect() {
	if (connected) {
		close(socket);
	}
}

static Connection *createRfcomm() {
	return new Rfcomm();
}

extern const ConnectionInfo rfcommConnectionInfo;
const ConnectionInfo rfcommConnectionInfo(createRfcomm, "rfcomm", "pvlogdev,", "");
