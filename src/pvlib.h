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

#ifndef PVLIB_H
#define PVLIB_H

#include <stdint.h>
#include <stdio.h>
#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

static const int64_t PVLIB_INVALID_S64 = INT64_MIN;
static const int32_t PVLIB_INVALID_S32 = INT32_MIN;
static const int16_t PVLIB_INVALID_S16 = INT16_MIN;

static const uint64_t PVLIB_INVALID_U64 = UINT64_MAX;
static const uint32_t PVLIB_INVALID_U32 = UINT32_MAX;
static const uint16_t PVLIB_INVALID_U16 = UINT16_MAX;

typedef struct pvlib_plant_s pvlib_plant_t;

enum {
	PVLIB_UNSUPPORTED_CONNECTION, PVLIB_ERROR
};

typedef enum {
	PVLIB_RFCOMM
} pvlib_connection_t;

typedef enum {
	PVLIB_SMADATA2PLUS, PVLIB_SMADATA
} pvlib_protocol_t;

typedef struct pvlib_ac_s {
	int32_t current_power; ///< current power of string inverter in watts

	int32_t power[3]; ///< current power of phase in watss
	int32_t voltage[3]; ///< current voltage in millivolts
	int32_t current[3]; ///< current current in milliampere

	uint8_t num_phases; ///< number of output phases

	int32_t frequency; ///< frequence in milliherz
} pvlib_ac_t;

void pvlib_init_ac(pvlib_ac_t *ac);

typedef struct pvlib_dc_s {
	int32_t current_power; ///<current power in watts

	int32_t power[3]; ///<current power in watts
	int32_t voltage[3]; ///<current voltage in millivolts
	int32_t current[3]; ///<current current in milliampere

	uint8_t num_lines; ///<number of input strings
} pvlib_dc_t;

void pvlib_init_dc(pvlib_dc_t *dc);

typedef struct pvlib_stats_s {
	int64_t total_yield; ///<total produced power in  watt-hours
	int64_t day_yield; ///<total produced power today in  watt-hours

	int64_t operation_time; /// <operation time in seconds
	int64_t feed_in_time; ///<feed in time in seconds
} pvlib_stats_t;

void pvlib_init_stats(pvlib_stats_t *stats);

typedef enum {
	PVLIB_STATUS_OK,
	PVLIB_STATUS_WARNING,
	PVLIB_STATUS_ERROR,
	PVLIB_STATUS_OFF,
	PVLIB_STATUS_UNKNOWN
}pvlib_status_value_t;

typedef struct pvlib_status_s {
	pvlib_status_value_t status;
	uint32_t number;
} pvlib_status_t;

typedef struct pvlib_inverter_info_s {
	char manufacture[64];
	char type[64];
	char name[64];
	char firmware_version[64];
} pvlib_inverter_info_t;

/**
 * Initialize pvlib.
 *
 * @param fd file descriptor for logging.
 */
void pvlib_init(FILE *fd);

/**
 * Shutdown pvlib.
 */
void pvlib_shutdown(void);

/**
 * Returns number of availabel connections.
 *
 * @return number of availabel connections.
 */
int pvlib_connection_num(void);

/**
 * Returns connection handles.
 *
 * @param[out] cons connection handles.
 * @param max_cons number of handles, which should be placed in cons
 *        return value of pvlib_connection_num should be used.
 *
 * @return number of connection handles placed in cons.
 */
int pvlib_connections(uint32_t *cons, int max_cons);

/**
 * Returns name of connection.
 *
 * @param con connection handle.
 *
 * @return name of connection.
 */
const char *pvlib_connection_name(uint32_t con);

/**
 * Returns number of availabel protocols.
 *
 * @return number of availabel protocols.
 */
int pvlib_protocol_num();

/**
 * Returns protocol handles.
 *
 * @param[out] protocol protocol handles.
 * @param max_protocols number of handles, which should be placed in protocols
 *        return value of pvlib_protocol_num should be used.
 *
 * @return number of protocol handles placed in protocols.
 */
int pvlib_protocols(uint32_t *protocols, int max_protocols);

/**
 * Returns name of protocol.
 *
 * @param con protocol handle.
 *
 * @return name of protocol.
 */
const char *pvlib_protocol_name(uint32_t protocol);

/**
 * Initialise connection and protocol.
 *
 * @param pvlib pvlib_t handle
 * @param connection connection type
 * @param protocol protocol type
 *
 * @return on error(invalid connection or protocol handle) NULL.
 */
pvlib_plant_t *pvlib_open(uint32_t connection,
                          uint32_t protocol,
                          const void *connection_param,
                          const void *protocol_param);

/**
 * Connect to plant/string_inverter
 *
 * @param con_address connection specific for rfcomm bluetoooth mac of target.
 * @param con_param connection specific.
 * @param protocol_passwd password for plant.
 * @param protocol_param protocol specific.
 *
 */
int pvlib_connect(pvlib_plant_t *plant,
                  const char *address,
                  const char *passwd,
                  const void *connection_param,
                  const void *protocol_param);

/**
 * Disconnect plant/string_inverter.
 */
void pvlib_disconnect(pvlib_plant_t *plant);

/**
 * Close connection to plant or string inverter.
 *
 * @param pvlib pvlib_t handle
 * @param num
 */
void pvlib_close(pvlib_plant_t *plant);

/**
 * Returns total number of stringinverter.
 *
 * @param pvlib pvlib_t handle
 * @return number of string inverter.
 */
int pvlib_num_string_inverter(pvlib_plant_t *plant);

/**
 * Returns string inverter handles.
 *
 * @param plant plant.
 * @param[out] id string inverter handles.
 * @param max_devices number of handles, which should be placed in ids
 *        return value of pvlib_num_string_inverter should be used.
 *
 */
int pvlib_device_handles(pvlib_plant_t *plant, uint32_t *ids, int max_devices);

/**
 * Get dc values from string converter.
 *
 * @param pvlib pvlib_t handle
 * @param[out] dc dc values
 * @param[out] id string inverter id
 */
int pvlib_get_dc_values(pvlib_plant_t *plant, uint32_t id, pvlib_dc_t *dc);

/**
 * Get dc values from string converter.
 *
 * @param pvlib pvlib_t handle
 * @param[out] ac dc values
 * @param[out] id string inverterr id
 */
int pvlib_get_ac_values(pvlib_plant_t *plant, uint32_t id, pvlib_ac_t *ac);

/**
 * Get dc values from string converter.
 *
 * @param pvlib pvlib_t handle
 * @param[out] stats statistics of string inverter
 * @param[out] id string converter id
 */
int pvlib_get_stats(pvlib_plant_t *plant, uint32_t id, pvlib_stats_t *stats);

/**
 * Get status from inverter
 *
 * @param plant plant handle
 * @param id inverter id
 * @param[out] status inverter status
 * @return negative on failure 0 on success and good status and positive on success and bad status.
 */
int pvlib_get_status(pvlib_plant_t *plant, uint32_t id, pvlib_status_t *status);

/**
 * Get inverter informations
 *
 * @param plant plant handle
 * @param id inverter id
 * @param[out] inveter_info inverter information
 * @return negative on failue 0 on success.
 */
int pvlib_get_inverter_info(pvlib_plant_t *plant, uint32_t id, pvlib_inverter_info_t *inverter_info);

/**
 * Returns protocol handle.
 * This must not be supported by protocol, so NULL does not mean an error occured.
 *
 * @return protocol handle.
 */
void *pvlib_protocol_handle(pvlib_plant_t *plant);

#if defined __cplusplus
}
#endif

#endif /* #ifndef PVLIB_H */
