/*
 *   Pvlib - PV logging library
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
#include <malloc.h>

#include "pvlib.h"
#include "protocol.h"
#include "connection.h"
#include "log.h"

struct pvlib_plant_s {
	connection_t *con;
	protocol_t *protocol;
};

int pvlib_connection_num(void)
{
	return connection_num();
}

const char *pvlib_connection_name(uint32_t handle)
{
	return connection_name(handle);
}

int pvlib_connections(uint32_t *cons, int max_con)
{
	return connection_supported(cons, max_con);
}

int pvlib_protocol_num(void)
{
	return protocol_num();
}

const char *pvlib_protocol_name(uint32_t handle)
{
	return protocol_name(handle);
}

int pvlib_protocols(uint32_t *protocols, int max_protocol)
{
	return protocol_supported(protocols, max_protocol);
}

pvlib_plant_t *pvlib_open(uint32_t connection,
                          const char *address,
                          const void *param,
                          uint32_t protocol)
{
	connection_t *con;
	protocol_t *prot;
	pvlib_plant_t *plant;

	con = connection_open(connection, address, param);
	if (con == NULL) {
		return NULL;
	}

	prot = protocol_open(protocol, con);
	if (prot == NULL) {
		LOG_ERROR("Failed opening protocol!");
		connection_close(con);
		return NULL;
	}

	plant = malloc(sizeof(*plant));

	plant->con = con;
	plant->protocol = prot;

	return plant;
}

int pvlib_connect(pvlib_plant_t *plant, const char *passwd, const void *protocol_param)
{
	if (plant->protocol->connect(plant->protocol, passwd, protocol_param) < 0) {
		return -1;
	}

	return 0;
}

void pvlib_init(FILE *file)
{

	log_enable(file, LOG_ALL);
}

void pvlib_shutdown(void)
{
	//nothing to do
}

int pvlib_num_string_inverter(pvlib_plant_t *plant)
{
	return protocol_inverter_num(plant->protocol);
}

int pvlib_device_handles(pvlib_plant_t *plant, uint32_t *ids, int max_handles)
{
	return -1; //TODO: implement
}

int pvlib_get_ac_values(pvlib_plant_t *plant, uint32_t id, pvlib_ac_t *ac)
{
	return protocol_ac(plant->protocol, id, ac);
}

int pvlib_get_dc_values(pvlib_plant_t *plant, uint32_t id, pvlib_dc_t *dc)
{
	return protocol_dc(plant->protocol, id, dc);
}

int pvlib_get_stats(pvlib_plant_t *plant, uint32_t id, pvlib_stats_t *stats)
{
	return protocol_stats(plant->protocol, id, stats);
}

void *pvlib_protocol_handle(pvlib_plant_t *plant)
{
	return plant->protocol->handle;
}

void pvlib_close(pvlib_plant_t *plant)
{
	protocol_close(plant->protocol);
	connection_close(plant->con);
	free(plant);
}
