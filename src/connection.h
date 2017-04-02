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

#endif /* #ifndef CONNNECTION_H */
