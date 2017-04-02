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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdint>
#include <string>
#include <vector>

#include "readWrite.h"

//#include "pvlib.h"

//#define CONNECTION_MAX_NAME 128
//
///** connection handle */
//typedef struct connection_s connection_t;
//
//typedef struct connection_data_s {
//	char src_name[CONNECTION_MAX_NAME]; ///< Our name.
//	uint8_t src_address[6]; ///< Our address.
//
//	uint8_t dst_address[6]; ///< Address of destination.
//	char dst_name[CONNECTION_MAX_NAME]; ///< Name of destination.
//
//	int address_len; ///< Length of address, depends on connection type.
//} connection_data_t;
//
//int connection_num(void);
//
//int connection_supported(uint32_t *handle, int max_handles);
//
//const char *connection_name(uint32_t handle);
//
//const char *connection_author(uint32_t handle);
//
//const char *connection_comment(uint32_t handle);
//
//connection_t * connection_open(uint32_t handle);

namespace pvlib {

struct ConnectionInfo;

class Connection : public ReadWrite {
public:
	virtual ~Connection() {};


	/**
	 * Connect to given address.
	 *
	 * @param address address to connect to.
	 */
	virtual int connect(const char *address, const void *param) = 0;

	virtual void disconnect() = 0;


	/**
	 * Give some usefull connection info.
	 *
	 * @param con connection handle.
	 * @param[out] info @see connection_info_t.
	 *
	 * @return < 0, if error occurs.
	 */
	//int info(connection_data_t *info) = 0;

	static const std::vector<const ConnectionInfo*> availableConnections;

};


struct ConnectionInfo {
	typedef Connection* (*CreateFunc)();

    const CreateFunc create;
    const char *name;
    const char *author;
    const char *comment;

    ConnectionInfo(const CreateFunc create, const char *name, const char *author, const char *comment) :
    		create(create),
    		name(name),
			author(author),
			comment(comment) {
    }
};

} //namespace pvlib {


//int connection_connect(connection_t *con, const char *address, const void *param);
//
//void connection_disconnect(connection_t *con);
//
///**
// * Write data.
// *
// * @param con connection handle.
// * @param data data to write.
// * @param len length of data.
// *
// * @return < 0 if error occurs.
// */
//int connection_write(connection_t *con, const uint8_t *data, int len);
//
///**
// * Read data.
// *
// * @param con connection handle.
// * @param data buffer to read to.
// * @param len length of data to read.
// * @param timeout timeout in ms
// *
// * @return < 0 if error occurs, else amount of bytes read.
// */
//int connection_read(connection_t *con, uint8_t *data, int max_len, int timeout);
//
///**
// * Give some usefull connection info.
// *
// * @param con connection handle.
// * @param[out] info @see connection_info_t.
// *
// * @return < 0, if error occurs.
// */
//int connection_info(connection_t *con, connection_data_t *info);
//
///**
// * Close connection.
// *
// * @param con connection handle.
// */
//void connection_close(connection_t *con);

#endif /* #ifndef CONNNECTION_H */
