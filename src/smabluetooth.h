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

#include <stdint.h>
#include "connection.h"

#define SMABLUETOOTH_CONNECT 0x02
#define SMABLUETOOTH_ADDRESS 0x0a
#define SMABLUETOOTH_ADDRESS2 0x05
#define SMABLUETOOTH_ASKSIGNAL 0x03
#define SMABLUETOOTH_ANSWERSIGNAL 0x04

/*
typedef struct smabluetooth_header_s {
    uint8_t     len;        ///< you don't need to set it, it is done for you.
    uint8_t     mac_src[6]; ///< you don't need to set it, it is done for you.
    uint8_t     mac_dst[6]; ///< you don't need to set it, it is done for you.
    uint8_t     cmd;        ///< command to send.
}smabluetooth_header_t;

typedef struct smabluetooth_packet_s {
    smabluetooth_header_t header; ///< packet header.
    uint8_t     *data;       ///< data to send.
    uint8_t     data_len;    ///< lenght of data,
    int         unknown_src; ///< used as boolean, if set to 1 source addr is not sent, only used for sending.
    int         broadcast;   ///< used as boolean, if set to false data is only send to connection partner.
}smabluetooth_packet_t;
*/
typedef struct smabluetooth_packet_s {
	uint8_t mac_src[6];
	uint8_t mac_dst[6];
	uint8_t cmd;
	uint8_t *data;
	uint8_t len;
}smabluetooth_packet_t;

/** smabluetoth handle */
typedef struct smabluetooth_s smabluetooth_t;

/**
 * Setup smabluetooth.
 *
 * @param con connection used for sending data.
 * @return handle.
 */
smabluetooth_t *smabluetooth_init(connection_t *con);

/**
 * Close smabluetooth.
 *
 * @param sma smabluetooth handle.
 */
void smabluetooth_close(smabluetooth_t *sma);

/**
 * Send data.
 *
 * @param sma smabluetooth handle.
 * @param packet @see smabluetooth_packet_t.
 * @return < 0 if error occured, else >= 0.
 */
int smabluetooth_write(smabluetooth_t *sma, smabluetooth_packet_t *packet);

/**
 * Read data.
 * Note: Reads allways one and only one smabluetooth frame.
 * If given size is below smabluetooth frame size left bytes will be removed.
 * If given size greater than smabluetooth frame size only size of frame will be read.
 * So allways check return size!
 *
 * @param sma smabluetooth handle.
 * @param packet packet @see smabluetooth_packet_t.
 * @return < 0 if error occured, else amount of bytes read.
 */
int smabluetooth_read(smabluetooth_t *sma, smabluetooth_packet_t *packet);

/**
 * Connect to string convertet.
 *
 * @param sma smabluetooth handle.
 * @return < 0 if error occurs, else > 0.
 */
int smabluetooth_connect(smabluetooth_t *sma);

/**
 * Get number of devices in network.
 *
 * @param sma smabluetooth handle.
 * @return number of devices
 */
 int smabluetooth_device_num(smabluetooth_t *sma);

/**
 * Get signal strength.
 *
 * @param sma smabluetooth handle.
 * @return signal strength from 0 to 100 inclusive, if error occurs < 0.
 */
int smabluetooth_signal(smabluetooth_t *sma, uint8_t *mac);

#endif /* #ifndef SMABLUETOOTH_H */
