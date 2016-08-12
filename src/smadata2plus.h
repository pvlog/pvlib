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

#include <stdint.h>
#include <stdbool.h>

#include "pvlib.h"

#define SMADATA2PLUS_BROADCAST 0xffffffff

typedef struct smadata2plus_s smadata2plus_t;

typedef struct smadata2plus_packet_s {
	uint8_t  src_mac[6];
	uint16_t pkt_cnt;
	uint8_t  ctrl;
	uint32_t dst;
	uint32_t src;
	uint8_t  flag; /* unknown */
	uint16_t cnt;
	bool     start;
	uint8_t  *data;
	int      len;
} smadata2plus_packet_t;

int smadata2plus_read_channel(smadata2plus_t *sma, uint16_t channel, uint32_t idx1, uint32_t idx2);

//int smadata2plus_read_channel_(smadata2plus_t *sma, uint16_t channel, uint32_t index);

//int smadata2plus_read_channel_string(smadata2plus_t *sma, uint16_t channel, uint32_t index);

//struct pvlib_interface_s *smadata2plus_init(connection_t *con);

#endif /* #ifndef SMADATA2PLUS_H */
