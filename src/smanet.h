/*
 *   Pvlib - Smanet implementation
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

#ifndef SMANET_H
#define SMANET_H

#include <stdint.h>

#include "connection.h"
#include "smabluetooth.h"

/** smanet handle */
typedef struct smanet_s smanet_t;

/**
 * Setup smanet.
 * Note: Either con or sma must be NULL.
 *
 * @param protocol Protocol type to use.
 * @param con connection handle.
 * @param sma smabluetooth handle.
 *
 * @return smanet handle on success, else NULL.
 */
smanet_t *smanet_init(uint16_t protocol, connection_t *con, smabluetooth_t *sma);

/**
 * Close smanet.
 *
 * @param smanet smanet handle.
 */
void smanet_close(smanet_t *smanet);

/**
 * Write data.
 *
 * @param smanet smanet handle.
 * @param data data to write.
 * @param len length of data.
 * @param param to destination mac(6 byte), only used for smadata2plus bluetooth transmission,
 *        otherwise set it to NULL.
 *
 * @return >= 0 on success, else < 0.
 */
int smanet_write(smanet_t *smanet, uint8_t *data, int len, const uint8_t *to);

/**
 * Read data.
 * Note: Reads allways one and only one smabluetooth frame.
 * If given size is below smabluetooth frame size left bytes will be removed.
 * If given size greater than smabluetooth frame size only size of frame will be read.
 * So allways check return size!
 *
 * @param smanet smanet handle.
 * @param data buf to readt to.
 * @param len len of data.
 * @param[out] from device mac(6 byte), only used for smadata2plus bluetooth,
 *             otherwise set it to NULL.
 *
 * @return amount of bytes read on success, else < 0.
 */
int smanet_read(smanet_t *smanet, uint8_t *data, int len, uint8_t *from);

#endif /* #ifndef SMANET_H */
