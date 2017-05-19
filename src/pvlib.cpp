/*
 *   Pvlib - PV logging library
 *
 *   Copyright (C) 2011, 2016 pvlogdev@gmail.com
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

#include "connection.h"
#include "protocol.h"
#include "pvlib.h"
#include "config.h"
#include "log.h"

using namespace pvlib;

struct pvlib_plant {
	Connection *con;
	Protocol *protocol;
};

pvlib_ac *pvlib_alloc_ac() {
	return new (std::nothrow) pvlib_ac;
}

void pvlib_free_ac(pvlib_ac *ac) {
	delete ac;
}

pvlib_dc  *pvlib_alloc_dc() {
	return new (std::nothrow) pvlib_dc;
}

void pvlib_free_dc(pvlib_dc *dc) {
	delete dc;
}

pvlib_status *pvlib_alloc_status() {
	return new (std::nothrow) pvlib_status;
}

void pvlib_free_status(pvlib_status *status) {
	delete status;
}

pvlib_stats *pvlib_alloc_stats() {
	return new (std::nothrow) pvlib_stats;
}

void pvlib_free_stats(pvlib_stats* stats) {
	delete stats;
}

pvlib_inverter_info *pvlib_alloc_inverter_info() {
	return new (std::nothrow) pvlib_inverter_info;
}

void pvlib_free_inverter_info(pvlib_inverter_info * inverter_info) {
	delete inverter_info;
}

void pvlib_version(int* major, int* minor, int* patch) {
	*major = MAJOR_VERSION;
	*minor = MINOR_VERSION;
	*patch = PATCH_VERSION;
}

const char* pvlib_get_version_string() {
	return VERSION_STRING;
}

int pvlib_connection_num(void) {
	return Connection::availableConnections.size();
}

const char *pvlib_connection_name(uint32_t handle) {
	return Connection::availableConnections.at(handle)->name;
}

int pvlib_connections(uint32_t *cons, int max_con) {
	int availableCons = Connection::availableConnections.size();

	int retCons = (availableCons < max_con) ? availableCons : max_con;
	for (int i = 0; i < retCons; ++i) {
		cons[i] = i;
	}

	return retCons;
}

int pvlib_protocol_num(void) {
	return Protocol::availableProtocols.size();
}

const char *pvlib_protocol_name(uint32_t handle) {
	if (handle >= Protocol::availableProtocols.size()) {
		return NULL;
	}

	return Protocol::availableProtocols.at(handle)->name;
}

int pvlib_protocols(uint32_t *protocols, int max_protos) {
	int availableProtos = Protocol::availableProtocols.size();

	int retProtos = (availableProtos < max_protos) ? availableProtos : max_protos;
	for (int i = 0; i < retProtos; ++i) {
		protocols[i] = i;
	}

	return retProtos;
}

pvlib_plant *pvlib_open(uint32_t connection,
                        uint32_t protocol,
                        const void *connection_param,
                        const void *protocol_param)
{
	Connection *con;
	Protocol *prot;
	pvlib_plant *plant;

	con = Connection::availableConnections[connection]->create();
	if (con == NULL) {
		return NULL;
	}

	prot = Protocol::availableProtocols[protocol]->create(con);
	if (prot == NULL) {
		return NULL;
	}

	plant = new pvlib_plant();

	plant->con = con;
	plant->protocol = prot;

	return plant;
}

int pvlib_connect(pvlib_plant *plant,
                  const char *address,
                  const char *passwd,
                  const void *connection_param,
                  const void *protocol_param)
{
	int ret;
	if ((ret = plant->con->connect(address, connection_param)) < 0) {
		return ret;
	}

	if ((ret = plant->protocol->connect(passwd, protocol_param)) < 0) {
		plant->con->disconnect();
		return ret;
	}

	return 0;
}

void pvlib_disconnect(pvlib_plant *plant) {
	plant->protocol->disconnect();
	plant->con->disconnect();
}

void pvlib_init(pvlib_log_func log_callback, const char *modules[], pvlib_log_level level) {
	Log::init(log_callback, modules, level);
}

void pvlib_shutdown(void) {
	//nothing to do
}

int pvlib_num_string_inverter(pvlib_plant *plant) {
	return plant->protocol->inverterNum();
}

int pvlib_device_handles(pvlib_plant *plant, uint32_t *ids, int max_handles) {
	int retInverters = std::min(max_handles, plant->protocol->inverterNum());
	plant->protocol->getDevices(ids, retInverters);
	return retInverters;
}

int pvlib_get_ac_values(pvlib_plant *plant, uint32_t id, pvlib_ac *ac) {
	return plant->protocol->readAc(id, ac);
}

int pvlib_get_dc_values(pvlib_plant *plant, uint32_t id, pvlib_dc *dc) {
	return plant->protocol->readDc(id, dc);
}

int pvlib_get_stats(pvlib_plant *plant, uint32_t id, pvlib_stats *stats) {
	return plant->protocol->readStats(id, stats);
}

int pvlib_get_status(pvlib_plant *plant, uint32_t id, pvlib_status *status) {
    return plant->protocol->readStatus(id, status);
}

int pvlib_get_inverter_info(pvlib_plant *plant, uint32_t id, pvlib_inverter_info *inverter_info) {
	return plant->protocol->readInverterInfo(id, inverter_info);
}

int pvlib_get_day_yield(pvlib_plant *plant, uint32_t id, time_t from, time_t to, pvlib_day_yield **dayYield) {
	return plant->protocol->readDayYield(id, from, to, dayYield);
}

int pvlib_get_events(pvlib_plant *plant, uint32_t id, time_t from, time_t to, pvlib_event **events) {
	return plant->protocol->readEvents(id, from, to, events);
}

void *pvlib_protocol_handle(pvlib_plant *plant) {
	return plant->protocol;
}

void pvlib_close(pvlib_plant *plant) {
	delete plant->protocol;
	delete plant->con;

	free(plant);
}

void pvlib_init_ac(pvlib_ac *ac) {
	ac->totalPower = PVLIB_INVALID_S32;
	ac->frequency  = PVLIB_INVALID_S32;
	ac->phaseNum     = 0;

	for (size_t i = 0; i < sizeof(ac->power) / sizeof(ac->power[0]); ++i) {
		ac->power[i]   = PVLIB_INVALID_S32;
		ac->voltage[i] = PVLIB_INVALID_S32;
		ac->current[i] = PVLIB_INVALID_S32;
	}
}

void pvlib_init_dc(pvlib_dc *dc) {
	dc->totalPower = PVLIB_INVALID_S32;
	dc->trackerNum      = 0;

	for (size_t i = 0; i < sizeof(dc->power) / sizeof(dc->power[0]); ++i) {
		dc->power[i]   = PVLIB_INVALID_S32;
		dc->voltage[i] = PVLIB_INVALID_S32;
		dc->current[i] = PVLIB_INVALID_S32;
	}
}

void pvlib_init_stats(pvlib_stats *stats) {
	stats->totalYield    = PVLIB_INVALID_S64;
	stats->dayYield      = PVLIB_INVALID_S64;
	stats->operationTime = PVLIB_INVALID_S64;
	stats->feedInTime    = PVLIB_INVALID_S64;
}

