/*
 *   Pvlib - Protocol interface
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

#include "protocol.h"

extern const protocol_info_t protocol_info_smadata2plus;

static const protocol_info_t * const protocols[] = { &protocol_info_smadata2plus, NULL };

int protocol_num(void)
{
	int i;
	for (i = 0; protocols[i] != NULL; i++)
		;

	return i;
}

int protocol_supported(uint32_t *handles, int max_handles)
{
	int i;
	for (i = 0; i < max_handles; i++) {
		if (protocols[i] == NULL) {
			break;
		}

		handles[i] = i;
	}

	return i;
}

const char *protocol_name(uint32_t handle)
{
	if (handle >= protocol_num()) {
		return NULL;
	}

	return protocols[handle]->name;
}

const char *protocol_author(uint32_t handle)
{
	if (handle >= protocol_num()) {
		return NULL;
	}

	return protocols[handle]->author;
}

const char *protocol_comment(uint32_t handle)
{
	if (handle >= protocol_num()) {
		return NULL;
	}

	return protocols[handle]->comment;
}

protocol_t *protocol_open(uint32_t handle, connection_t *con, const char *params)
{
	protocol_t protocol;
	protocol_t *ret;

	if (handle >= protocol_num()) {
		return NULL;
	}

	if (protocols[handle]->open(&protocol, con, params) < 0) {
		return NULL;
	}

	ret = malloc(sizeof(*ret));
	memcpy(ret, &protocol, sizeof(*ret));

	return ret;
}

void protocol_close(protocol_t *protocol)
{
	protocol->close(protocol);
}

int protocol_connect(protocol_t *protocol, const char *passwd, const void *param)
{
	return protocol->connect(protocol, passwd, param);
}

void protocol_disconnect(protocol_t *protocol)
{
    protocol->disconnect(protocol);
}

int protocol_inverter_num(protocol_t *protocol)
{
	return protocol->inverter_num(protocol);
}

int protocol_dc(protocol_t *protocol, uint32_t id, pvlib_dc_t *dc)
{
	return protocol->get_dc(protocol, id, dc);
}

int protocol_ac(protocol_t *protocol, uint32_t id, pvlib_ac_t *ac)
{
	return protocol->get_ac(protocol, id, ac);
}

int protocol_stats(protocol_t *protocol, uint32_t id, pvlib_stats_t *stats)
{
	return protocol->get_stats(protocol, id, stats);
}

int protocol_status(protocol_t *protocol, uint32_t id, pvlib_status_t *status)
{
    return protocol->get_status(protocol, id, status);
}
