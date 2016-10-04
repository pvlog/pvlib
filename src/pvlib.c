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
                          uint32_t protocol,
                          const void *connection_param,
                          const void *protocol_param)
{
	connection_t *con;
	protocol_t *prot;
	pvlib_plant_t *plant;

	con = connection_open(connection);
	if (con == NULL) {
		return NULL;
	}

	prot = protocol_open(protocol, con, protocol_param);
	if (prot == NULL) {
		connection_close(con);
		return NULL;
	}

	plant = malloc(sizeof(*plant));

	plant->con = con;
	plant->protocol = prot;

	return plant;
}

int pvlib_connect(pvlib_plant_t *plant,
                  const char *address,
                  const char *passwd,
                  const void *connection_param,
                  const void *protocol_param)
{
    int ret;
    if ((ret = connection_connect(plant->con, address, connection_param)) < 0) {
        return ret;
    }
	if ((ret = protocol_connect(plant->protocol, passwd, protocol_param)) < 0) {
	    connection_disconnect(plant->con);
	    return ret;
	}

	return 0;
}

void pvlib_disconnect(pvlib_plant_t *plant)
{
    protocol_disconnect(plant->protocol);
    connection_disconnect(plant->con);
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
	return protocol_get_devices(plant->protocol, ids, max_handles);
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

int pvlib_get_status(pvlib_plant_t *plant, uint32_t id, pvlib_status_t *status)
{
    return protocol_status(plant->protocol, id, status);
}

int pvlib_get_inverter_info(pvlib_plant_t *plant, uint32_t id, pvlib_inverter_info_t *inverter_info)
{
	return protocol_inverter_info(plant->protocol, id, inverter_info);
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

void pvlib_init_ac(pvlib_ac_t *ac) {
	ac->current_power     = PVLIB_INVALID_S32;
	ac->frequency         = PVLIB_INVALID_S32;
	ac->num_phases        = 0;

	for (int i = 0; i < sizeof(ac->power) / sizeof(ac->power[0]); ++i) {
		ac->power[i]   = PVLIB_INVALID_S32;
		ac->voltage[i] = PVLIB_INVALID_S32;
		ac->current[i] = PVLIB_INVALID_S32;
	}
}

void pvlib_init_dc(pvlib_dc_t *dc) {
	dc->current_power     = PVLIB_INVALID_S32;
	dc->num_lines         = 0;

	for (int i = 0; i < sizeof(dc->power) / sizeof(dc->power[0]); ++i) {
		dc->power[i]   = PVLIB_INVALID_S32;
		dc->voltage[i] = PVLIB_INVALID_S32;
		dc->current[i] = PVLIB_INVALID_S32;
	}
}

void pvlib_init_stats(pvlib_stats_t *stats) {
	stats->total_yield =  PVLIB_INVALID_S64;
	stats->day_yield   =  PVLIB_INVALID_S64;
	stats->operation_time = PVLIB_INVALID_S64;
	stats->feed_in_time   = PVLIB_INVALID_S64;
}

