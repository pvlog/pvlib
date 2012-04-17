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

#if defined __cplusplus
extern "C" {
#endif

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
	uint32_t current_power; ///< current power of string inverter in watts

	uint32_t power[3]; ///< current power of phase in watss
	uint32_t voltage[3]; ///< current voltage in millivolts
	uint32_t current[3]; ///< current current in milliampere

	uint8_t num_lines; ///< number of output phases

	uint32_t frequence; ///< frequence in milliherz
} pvlib_ac_t;

typedef struct pvlib_dc_s {
	uint32_t current_power; ///<current power in watts

	uint32_t power[3]; ///<current power in watts
	uint32_t voltage[3]; ///<current voltage in millivolts
	uint32_t current[3]; ///<current current in milliampere

	uint8_t num_lines; ///<number of input strings
} pvlib_dc_t;

typedef struct pvlib_stats_s {
	uint32_t total_yield; ///<total produced power in  watt-hours
	uint32_t day_yield; ///<total produced power today in  watt-hours

	uint32_t operation_time; /// <operation time in seconds
	uint32_t feed_in_time; ///<feed in time in seconds
} pvlib_stats_t;

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
                          const char *address,
                          const void *param,
                          uint32_t protocol);

/**
 * Connect to plant/string_inverter
 *
 * @param con_address connection specific for rfcomm bluetoooth mac of target.
 * @param con_param connection specific.
 * @param protocol_passwd password for plant.
 * @param protocol_param protocol specific.
 *
 */
int pvlib_connect(pvlib_plant_t *plant, const char *passwd, const void *param);

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
