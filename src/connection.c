/*
 *   Pvlib - Connection interface
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
#include <stdlib.h>

#include "connection.h"
#include "connection_private.h"

extern const connection_info_t connection_info_rfcomm;

static const connection_info_t* const connections[] = {
		&connection_info_rfcomm,
		NULL
};

int connection_num(void)
{
	int i;
	for (i = 0; connections[i] != 0; i++)
		;
	return i;
}

int connection_supported(uint32_t *handles, int max_handles)
{
	uint32_t i;
	for (i = 0; i < max_handles; i++) {
		if (connections[i] == NULL) {
			break;
		}

		handles[i] = i;
	}

	return i;
}

const char *connection_name(uint32_t handle)
{
	if (handle >= connection_num()) {
		return NULL;
	}

	return connections[handle]->name;
}

const char *connection_author(uint32_t handle)
{
	if (handle >= connection_num()) {
		return NULL;
	}

	return connections[handle]->author;
}

const char *connection_comment(uint32_t handle)
{
	if (handle >= connection_num()) {
		return NULL;
	}

	return connections[handle]->comment;
}

connection_t *connection_open(uint32_t handle)
{
	if (handle >= connection_num()) {
		return NULL;
	}

	return connections[handle]->open();
}

int connection_connect(connection_t *con, const char *address, const void *param)
{
    return con->connect(con, address, param);
}

void connection_disconnect(connection_t *con)
{
    con->disconnect(con);
}

int connection_write(connection_t *con, const uint8_t *data, int len)
{
	return con->write(con, data, len);
}

int connection_read(connection_t *con, uint8_t *data, int len, int timeout)
{
	return con->read(con, data, len, timeout);
}

int connection_info(connection_t *con, connection_data_t *info)
{
	return con->info(con, info);
}

void connection_close(connection_t *con)
{
	con->close(con);
}
