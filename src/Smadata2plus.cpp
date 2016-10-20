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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <cassert>
#include <chrono>
#include <thread>
#include <cinttypes>
#include <algorithm>

#include <Protocol.h>
#include <Smabluetooth.h>
#include <Smadata2plus.h>
#include <Smanet.h>

#include "log.h"
#include "byte.h"
#include "pvlib.h"
#include "uthash.h"
#include "resources.h"

using std::this_thread::sleep_for;
using std::chrono::seconds;

using byte::DataReader;
using byte::DataWriter;

static const uint32_t SMADATA2PLUS_BROADCAST = 0xffffffff;

static const uint16_t PROTOCOL = 0x6560;
static const unsigned int HEADER_SIZE = 24;

/* ctrl */
static const int CTRL_MASTER = 1 << 7 | 1 << 5;
static const int  CTRL_NO_BROADCAST = 1 << 6;
static const int CTRL_UNKNOWN = 1 << 3;

/* address */
#define SERIAL_BROADCAST 0xffffffff
static const uint8_t MAC_BROADCAST[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static const int VOLTAGE_DIVISOR = 100;     //to volts
static const int CURRENT_DIVISOR = 1000;    // to ampere
static const int FREQUENCY_DIVISOR = 100;   // to herz
//static const uint32_t smadata2plus_serial = 0x3b225946;
static const uint32_t smadata2plus_serial = 0x3a8b74b6;

static const int NUM_RETRIES = 3;
static const uint16_t TRANSACTION_CNTR_START = 0x8000;

struct Packet {
	char     src_mac[6];
	uint16_t transaction_cntr;
	uint8_t  ctrl;
	uint32_t dst;
	uint32_t src;
	uint8_t  flag; /* unknown */
	uint16_t packet_num;
	bool     start;
	uint8_t  *data;
	int      len;
};

enum {
	TOTAL_POWER    = 0x263f,
	MAX_PHASE1     = 0x411e,
	MAX_PHASE2     = 0x411f,
	MAX_PHASE3     = 0x4120,
	UNKNOWN_1      = 0x4166,
	UNKNWON_2      = 0x417f,
	POWER_PHASE1   = 0x4640,
	POWER_PHASE2   = 0x4641,
	POWER_PHASE3   = 0x4642,
	VOLTAGE_PHASE1 = 0x4648,
	VOLTAGE_PHASE2 = 0x4649,
	VOLTAGE_PHASE3 = 0x464a,
	CURRENT_PHASE1 = 0x4650,
	CURRENT_PHASE2 = 0x4651,
	CURRENT_PHASE3 = 0x4652,
	FREQUENCE      = 0x4657
};

enum {
	DC_POWER = 0x251e, DC_VOLTAGE = 0x451f, DC_CURRENT = 0x4521
};

enum {
	STAT_OPERATION_TIME = 0x462E,
	STAT_FEED_IN_TIME   = 0x462F,
	STAT_TOTAL_YIELD    = 0x2601,
	STAT_DAY_YIELD      = 0x2622,
};

enum {
	DEVICE_NAME  = 0x821E,
	DEVICE_CLASS = 0x821F,
	DEVICE_TYPE  = 0x8220,
	DEVICE_UNKNOWN = 0x8221,
	DEVICE_SWVER = 0x8234,
};

enum {
	DEVICE_STATUS = 0x2148
};

struct tag_hash {
    int num;
    char tag[128];
    char message[256];

    UT_hash_handle hh;
};


//struct smadata2plus_s {
//	connection_t *con;
//	smabluetooth_t *sma;
//	smanet_t *smanet;
//	uint16_t transaction_cntr; // Packet counter
//	bool transaction_active;
//
//	device_t *devices;
//	int device_num;
//
//	struct tag_hash *tags;
//
//};

struct Attribute {
	uint32_t attribute;
	bool     selected;
};


struct RecordHeader {
	uint8_t  cnt;
	uint32_t idx;
	uint8_t  type; // 00, 04 -> type 1 or 2, 08 -> attributes, 0x10 -> string
	uint32_t time;
};

struct Record1 {
	uint32_t value1;
	uint32_t value2;
	uint32_t value3;
	uint32_t value4;
	uint32_t unknonwn;
};

struct Record2 {
	uint64_t value;
};

struct Record3 {
	uint8_t data[32];
};


struct  Record {
	RecordHeader header;

	Smadata2plus::RecordType record_type;

	union {
		Record1 r1;
		Record2 r2;
		Record3 r3;
	} record;
};

static void inc_transaction_cntr(uint16_t &transaction_cntr)
{
	if ((transaction_cntr < TRANSACTION_CNTR_START) || (transaction_cntr == 0xffff)) {
		transaction_cntr = TRANSACTION_CNTR_START;
	} else {
		transaction_cntr++;
	}
}

class Transaction {
	Smadata2plus *sma;

public:
	void begin() {
		assert(sma->transaction_active == true);
		sma->transaction_active = true;
	}

	void end() {
		sma->transaction_active = false;
		inc_transaction_cntr(sma->transaction_cntr);
	}

	Transaction(Smadata2plus *sma) : sma(sma) {
		assert(sma->transaction_active == false);
		sma->transaction_active = true;
	}

	~Transaction() {
		if (sma->transaction_active) {
			end();
		}
	}

};


static void parseAttributes(const uint8_t *data, int dataLen, Attribute *attributes, int *len)
{
	int attributeIdx = 0;

	for (int idx = 0; idx < dataLen && attributeIdx < *len; idx += 4, attributeIdx++) {
		uint32_t attribute = byte::parseU32le(data + idx) & 0x00ffffff;
		uint8_t selected = data[idx + 3];

		if (attribute == 0xfffffe) {
			break; //end of enums
		}

		attributes[attributeIdx].attribute = attribute;
		attributes[attributeIdx].selected  = selected;
	}

	*len = attributeIdx;
}

static void parseRecordHeader(const uint8_t* buf, RecordHeader *header) {
	DataReader dr(buf, 8);

	header->cnt  = dr.u8();
	header->idx  = dr.u16le();
	header->type = dr.u8();
	header->time = dr.u32le();
}

static void parseRecord1(const uint8_t *buf, Record1 *r1) {
	DataReader dr(buf, 20);

	r1->value1   = dr.u32le();
	r1->value2   = dr.u32le();
	r1->value3   = dr.u32le();
	r1->value4   = dr.u32le();
	r1->unknonwn = dr.u32le();
}

static void parseRecord2(const uint8_t *buf, Record2 *r2)
{
	r2->value = byte::parseU64le(buf);
}

static void parseRecord3(const uint8_t *buf, Record3 *r3)
{
	memcpy(r3->data, buf, 40);
}


static int parseChannelRecords(const uint8_t *buf,
                               int len, Record *records,
                               int *maxRecords,
                               Smadata2plus::RecordType type,
                               int16_t requestedObject)
{
	DataReader dr(buf, len);

	if (len < 8) {
		LOG_ERROR("Invalid record length %d", len);
		return -1; //invalid length
	}

	if (dr.u8() != 0x1 || dr.u8() != 0x02) {
		LOG_ERROR("Unexpected data in record header!");
		return -1; //invalid data
	}

	uint16_t object = dr.u16le();
	LOG_DEBUG("Object id %02x", object);
	if (object != requestedObject) {
		LOG_ERROR("Invalid object requested %d got %d", object, requestedObject);
		return -1;
	}

	uint32_t unknown1 = dr.u32le();
	uint32_t unknown2 = dr.u32le();
	LOG_DEBUG("record data unknonwn1: %d", unknown1);
	LOG_DEBUG("record data unknonwn2: %d", unknown2);

	size_t record_length = 0;
	switch (type) {
		case Smadata2plus::RECORD_1 : record_length = 28; break;
		case Smadata2plus::RECORD_2 : record_length = 16; break;
		case Smadata2plus::RECORD_3 : record_length = 40; break;
		default: assert(0 && "Invalid record!"); break;
	}

	int rec_idx = 0;
	for (int i = 12; i < len && rec_idx < *maxRecords; i += record_length, rec_idx++) {
		Record *r = &records[rec_idx];

		parseRecordHeader(buf + i, &r->header);
		switch (type) {
			case Smadata2plus::RECORD_1 : parseRecord1(buf + i + 8, &r->record.r1); break;
			case Smadata2plus::RECORD_2 : parseRecord2(buf + i + 8, &r->record.r2); break;
			case Smadata2plus::RECORD_3 : parseRecord3(buf + i + 8, &r->record.r3);; break;
			default: assert(0 && "Invalid record!"); break;
		}
	}

	*maxRecords = rec_idx;

	return 0;
}


//static int add_tag(smadata2plus_t *sma, int tag_num, const char* tag, const char* tag_message)
//{
//    struct tag_hash *s;
//    s = malloc(sizeof(*s));
//    if (s == NULL) {
//        LOG_ERROR("malloc failed!");
//        return -1;
//    }
//
//    s->num = tag_num;
//    strncpy(s->tag, tag, sizeof(s->tag) - 1);
//    strncpy(s->message, tag_message, sizeof(s->message) - 1);
//
//    HASH_ADD_INT(sma->tags, num, s);
//
//    return 0;
//}
//
//static struct tag_hash *find_tag(smadata2plus_t *sma, int num)
//{
//    struct tag_hash *s;
//
//    HASH_FIND_INT(sma->tags, &num, s);
//
//    return s;
//}
//
//static int read_tags(smadata2plus_t *sma, FILE *file) {
//	int ret;
//
//	char buf[256];
//	char tag_num[256];
//	char tag[256];
//	char tag_name[256];
//
//	while (fgets(buf, sizeof(buf), file) != NULL ) {
//		if (buf[0] == '#') { //remove comments
//			continue;
//		}
//
//		if (sscanf(buf, "%[^'=']=%[^';'];%s", tag_num, tag, tag_name) != 3) {
//			LOG_ERROR("Invalid line %s (Ignoring it)", buf);
//			continue;
//		}
//
//		errno = 0;
//		long num = strtol(tag_num, NULL, 10);
//		if (errno != 0) {
//			LOG_ERROR("Error parsing tag number from %s", tag_num);
//			return -1;
//		}
//
//		if ((ret = add_tag(sma, num, tag, tag_name)) < 0) {
//			return ret;
//		}
//	}
//
//	if (ferror(file)) {
//		LOG_ERROR("Error reading tag file: %s", strerror(errno));
//		fclose(file);
//		return -1;
//	}
//
//	fclose(file);
//	return 0;
//}


static Smadata2plus::Device *getDevice(std::vector<Smadata2plus::Device> &devices, uint32_t serial) {
	for (Smadata2plus::Device& device : devices) {
		if (device.serial == serial) {
			return &device;
		}
	}

	return nullptr;
}

static int serialToMac(const std::vector<Smadata2plus::Device> &devices, char *mac, uint32_t serial) {
	for (const Smadata2plus::Device &device : devices) {
		if (device.serial == serial) {
			memcpy(mac, device.mac, 6);
			return 0;
		}
	}

	return -1;
}

int Smadata2plus::writeReplay(const Packet *packet, uint16_t transactionCntr)
{
	uint8_t buf[511 + HEADER_SIZE];
	uint8_t size = HEADER_SIZE;
	char mac_dst[6];
	DataWriter dw(buf, HEADER_SIZE);

	assert(packet->len + HEADER_SIZE <= sizeof(buf));
	assert(packet->len % 4 == 0);

	memset(buf, 0x00, HEADER_SIZE);

	dw.u8((packet->len + HEADER_SIZE) / 4);
	dw.u8(packet->ctrl);

	if (packet->dst == SMADATA2PLUS_BROADCAST) {
		dw.u16le(0xffff);
		dw.u32le(0xffffffff);
		memcpy(mac_dst, MAC_BROADCAST, 6);
	} else {
		if (serialToMac(devices, mac_dst, packet->dst)
				< 0) {
			LOG_ERROR("device: %d not in device list!", packet->dst);
			return -1;
		}
		dw.u16le(0x004e);
		dw.u32le(packet->dst);
	}
	dw.u8(0x00);
	dw.u8(packet->flag);
	dw.u16le(0x0078);
	dw.u32le(smadata2plus_serial);
	dw.u8(0x00);

	if (packet->ctrl == 0xe8)
		dw.u8(0);
	else
		dw.u8(packet->flag);

	if (packet->start)
		buf[20] = packet->packet_num;

	byte::storeU16le(&buf[22], transactionCntr);

	memcpy(&buf[size], packet->data, packet->len);

	LOG_TRACE_HEX("write smadata2plus packet", buf, packet->len + size);

	std::string to(mac_dst, 6);
	return smanet.write(buf, size + packet->len, to);
}

//static void begin_transaction(smadata2plus_t *sma)
//{
//	assert(sma->transaction_active == false);
//	sma->transaction_active = true;
//}
//
//static void end_transaction(smadata2plus_t *sma)
//{
//	assert(sma->transaction_active == true);
//	sma->transaction_active = false;
//	inc_transaction_cntr(sma);
//}

int Smadata2plus::write(const Packet *packet) {
	return writeReplay(packet, transaction_cntr);
}

int Smadata2plus::read(Packet *packet) {
	uint8_t buf[512 + HEADER_SIZE];
	int size = HEADER_SIZE;
	int len;

	assert(packet->len <= 512);

	std::string src;
	len = smanet.read(buf, packet->len + HEADER_SIZE, src);
	if (len <= 0) { //handle timeout as failure
		LOG_ERROR("smanet_read failed.");
		return -1;
	}
	int macsize = std::min(6, (int)src.size());
	memcpy(packet->src_mac, src.c_str(), macsize);

	LOG_TRACE_HEX("read smadata2plus packet", buf, len);

	packet->ctrl = buf[1];
	packet->dst = byte::parseU32le(&buf[4]);
	packet->src = byte::parseU32le(&buf[12]);
	packet->flag = buf[9];
	packet->start = (buf[23] == 0x80) ? 1 : 0; //Fix
	packet->packet_num = byte::parseU16le(buf + 20);
	packet->transaction_cntr = byte::parseU16le(&buf[22]);

	len -= HEADER_SIZE;
	if (len < packet->len) packet->len = len;

	memcpy(packet->data, &buf[size], packet->len);

	return 0;
}

/*
 * Request a channel.
 */
int Smadata2plus::requestChannel(uint32_t serial, uint16_t channel, uint32_t fromIdx, uint32_t toIdx) {
	Packet packet;
	uint8_t buf[12];
	int ret;
	DataWriter dw(buf, sizeof(buf));

	memset(buf, 0x00, sizeof(buf));

	packet.ctrl = CTRL_MASTER;
	packet.dst = serial;
	packet.flag = 0x00;
	packet.data = buf;
	packet.len = sizeof(buf);
	packet.packet_num = 0;
	packet.start = 1;

	dw.u8(0x00);
	dw.u8(0x02);
	dw.u16le(channel);
	dw.u32le(fromIdx);
	dw.u32le(toIdx);

	ret = write(&packet);
	//inc_transaction_cntr();

	return ret;
}

int Smadata2plus::readRecords(uint32_t serial,
                              uint16_t object,
                              uint32_t from_idx,
                              uint32_t to_idx,
                              Record *records,
                              int *len,
                              RecordType type)
{
	int ret = 0;
	Packet packet;
	uint8_t data[512];


	//begin_transaction(sma);

	if ((ret = requestChannel(serial, object, from_idx, to_idx)) < 0) {
		LOG_ERROR("Failed requesting %04X %04X % 04X", object, from_idx, to_idx);
		//end_transaction(sma);
		return ret;
	}

	memset(&packet, 0x00, sizeof(packet));
	packet.data = data;
	packet.len = sizeof(data);

	if ((ret = read(&packet)) < 0) {
		//end_transaction(sma);
		return ret;
	}

	//end_transaction(sma);

	if ((ret = parseChannelRecords(data, packet.len, records, len, type, object)) < 0) {
		LOG_ERROR("Error parsing record of %04X %04X % 04X", object, from_idx, to_idx);
		return ret;
	}

	return 0;
}

void Smadata2plus::addDevice(uint32_t serial, char *mac) {
	devices.emplace_back(serial, mac, false);
}

/*
 * This is done ones to start connection.
 * This packet does not get a response from inverters.
 */
//static int reset_devices(smadata2plus_t *sma)
//{
//	return request_channel(sma, 0, 0, 0);
//}

int Smadata2plus::logout() {
	Packet packet;
	uint8_t buf[8];
	int ret;
	DataWriter dw(buf, sizeof(buf));

	packet.ctrl = CTRL_MASTER;
	packet.dst = SERIAL_BROADCAST;
	packet.flag = 0x03;
	packet.data = buf;
	packet.len = sizeof(buf);
	packet.packet_num = 0;
	packet.start = true;

	dw.u32le(0xfffd010e);
	dw.u32le(0xffffffff);

	Transaction t(this);
	if ((ret = write(&packet)) < 0) {
		return ret;
	}

	return 0;
}


/*
 * Find all devices in net and extract serial and mac.
 */
int Smadata2plus::discoverDevices(int device_num)
{
	Packet packet;
	uint8_t buf[52];

	Transaction t(this);

	if (requestChannel(SERIAL_BROADCAST, 0, 0, 0) < 0) {
		return -1;
	}

	for (int i = 0; i < device_num; i++) {
		packet.data = buf;
		packet.len = sizeof(buf);

		if (read(&packet) < 0) {
			return -1;
		}

		addDevice(packet.src, packet.src_mac);
	}

	return 0;
}

/*
 * Send password to all devices in network.
 */
int Smadata2plus::sendPassword(const char *password, Smadata2plus::UserType user) {
	Packet packet;
	uint8_t buf[32];
	int i = 0;
	time_t cur_time;
	DataWriter dw(buf, sizeof(buf));

	packet.ctrl = CTRL_MASTER;
	packet.dst = SERIAL_BROADCAST;
	packet.flag = 0x01;
	packet.data = buf;
	packet.len = sizeof(buf);
	packet.packet_num = 0;
	packet.start = true;

	memset(buf, 0x00, sizeof(buf));

	cur_time = time(NULL);
	LOG_INFO("Sending password %s at %s.", password, ctime(&cur_time));

	dw.u32le(0xfffd040c);
	dw.u8(0x07);
	dw.skip(3);
	dw.u32le(40 * 365 * 24 * 60 * 60);
	dw.u32le(cur_time);

	memset(&buf[20], 0x88, 12);
	for (i = 0; (i < 12) && (password[i] != '\0'); i++) {
		buf[20 + i] = password[i] ^ 0x88;
	}

	return write(&packet);
}

/*
 * Seems to be only needed for single inverter (netid 1)
 */
int Smadata2plus::ackAuth(uint32_t serial)
{
	uint8_t buf[8];
	Packet packet;

	memset(buf, 0x00, sizeof(buf));

	packet.ctrl = CTRL_MASTER | CTRL_NO_BROADCAST | CTRL_UNKNOWN;
	packet.dst  = serial;
	packet.flag = 0x01;
	packet.data = buf;
	packet.len  = sizeof(buf);
	packet.packet_num  = 0;
	packet.start = 1;

	byte::storeU32le(buf, 0xfffd040d);
	buf[4] = 0x01;

	return write(&packet);
}

/*
 * Send password and interpret response from inverter.
 * Answer packet seems to be different depending on inverter number or netid
 * (On multinverter connection password is missing in answer packet, everything else is exactly the same)
 * On netid 1 (single inverter we need 2 send a second packet!?
 */
int Smadata2plus::authenticate(const char *password, UserType user)
{
	uint8_t buf[52];
	Packet packet;

	packet.data = buf;
	packet.len = 52;

	Transaction t(this);

	if (sendPassword(password, user) < 0) {
		LOG_ERROR("Failed sending password!");
		return -1;
	}

	for (size_t j = 0; j < devices.size(); j++) {
		if (read(&packet) < 0) {
			return -1;
		}

		for (int i = 0; i < (i < 12) && (password[i] != '\0'); i++) {
			Device *device;

			if ((buf[20 + i] ^ 0x88) != password[i]) {
				LOG_INFO("Plant authentication error, serial: %d", packet.src);
			}

			device = getDevice(devices, packet.src);
			if (device == NULL) {
				LOG_WARNING("Got authentication answer of non registered device: %d", packet.src);
			}
			device->authenticated = true;
		}
	}

	if (devices.size() == 1) {
		if (ackAuth(devices[0].serial) < 0) {
			return -1;
		}
	}

	return 0;
}

//static smadata2plus_t *init(connection_t *con)
//{
//	smadata2plus_t *sma;
//	smabluetooth_t *smabluetooth;
//	smanet_t *smanet;
//
//	smabluetooth = smabluetooth_init(con);
//	if (smabluetooth == NULL) return NULL;
//
//	smanet = smanet_init(PROTOCOL, NULL, smabluetooth);
//	if (smanet == NULL) return NULL;
//
//	sma = calloc(1, sizeof(*sma));
//
//	sma->con = con;
//	sma->sma = smabluetooth;
//	sma->smanet = smanet;
//	sma->tags = NULL;
//
//	sma->transaction_active = false;
//	reset_transaction_cntr(&sma->transaction_cntr);
//
//	return sma;
//}
//
//void smadata2plus_close(protocol_t *protocol)
//{
//	smadata2plus_t *sma = protocol->handle;
//	smabluetooth_close(sma->sma);
//	smanet_close(sma->smanet);
//
//	free(sma);
//	free(protocol);
//}

int Smadata2plus::syncTime() {
	Packet packet;
	uint8_t buf[40];
	int ret;

	packet.ctrl = CTRL_MASTER;
	packet.dst = SERIAL_BROADCAST;
	packet.flag = 0x00;
	packet.data = buf;
	packet.len = 40;
	packet.packet_num = 0;
	packet.start = 1;
	DataWriter dw(buf, sizeof(buf));

	dw.u32le(0xf000020a);
	dw.u32le(0x00236d00);
	dw.u32le(0x00236d00);
	dw.u32le(0x00236d00);
	dw.u32le(0);
	dw.u32le(0);
	dw.u32le(0);
	dw.u32le(0);
	dw.u32le(1);
	dw.u32le(1);

	Transaction t(this);
	if ((ret = write(&packet)) < 0) {
		LOG_ERROR("Error reading inverter date!");
		//end_transaction(sma);
		return ret;
	}

	t.end();

	//This packet is not an replay
	//It's transaction counter is completely different
	//and the reply flag is not set
	if ((ret = read(&packet)) < 0) {
		LOG_ERROR("smadata2plus_read failed!");
		return ret;
	}

	if (packet.len != 40) {
		LOG_ERROR("Invalid packet!");
		return -1;
	}

	time_t last_adjusted = byte::parseU32le(buf + 20);
	LOG_INFO("Time last adjusted: %s", ctime(&last_adjusted));

	time_t inverter_time1 = byte::parseU32le(buf + 16);
	time_t inverter_time2 = byte::parseU32le(buf + 24);
	LOG_INFO("Inverter time 1: %s", ctime(&inverter_time1));
	LOG_INFO("Inverter time 2: %s", ctime(&inverter_time2));

	uint32_t tz_dst = byte::parseU32le(buf + 28);
	int tz = tz_dst & 0xfffffe;
	int dst = tz_dst & 0x1;
	uint32_t unknown = byte::parseU32le(buf + 32);
	uint16_t transaction_cntr = packet.transaction_cntr;

	memset(&packet, 0x00, sizeof(packet));
	memset(buf, 0x00, sizeof(buf));

	packet.ctrl = CTRL_MASTER | CTRL_UNKNOWN | CTRL_NO_BROADCAST;
	packet.dst = devices[0].serial; //FIXME: destination!!!!!!
	packet.flag = 0x00;
	packet.data = buf;
	packet.len = 8;
	packet.packet_num = 0;
	packet.start = 0;

	byte::storeU32le(buf,     0xf000010a);
	byte::storeU32le(buf + 4, 0x1);

	if ((ret = writeReplay(&packet, transaction_cntr)) < 0) {
		LOG_ERROR("Error writing time ack!");
		return ret;
	}

	//FIXME: SunnyExplorer sends an packet with inverter time
	//

	time_t cur_time = time(NULL);

	if ((abs(cur_time - inverter_time1)) > 10) {
		memset(&packet, 0x00, sizeof(packet));
		memset(buf, 0x00, sizeof(buf));
		DataWriter dw(buf, sizeof(buf));

		dw.u32le(0xf000020a);
		dw.u32le(0x00236d00);
		dw.u32le(0x00236d00);
		dw.u32le(0x00236d00);
		dw.u32le(cur_time);
		dw.u32le(cur_time);
		dw.u32le(cur_time);
		dw.u32le(dst | tz);
		dw.u32le(unknown);
		dw.u32le(1);

		packet.ctrl = CTRL_MASTER;
		packet.dst = SERIAL_BROADCAST;
		packet.flag = 0x00;
		packet.data = buf;
		packet.len = sizeof(buf);
		packet.packet_num = 0;
		packet.start = 1;

		t.begin();
		if ((ret = write(&packet)) < 0) {
			LOG_ERROR("Error setting date!");
			return ret;
		}
		t.end();
	}

	return 0;
}

Smadata2plus::Smadata2plus(Connection *con) :
		connection(con),
		sma(con),
		smanet(PROTOCOL, &sma),
		transaction_cntr(TRANSACTION_CNTR_START),
		transaction_active(false) {

}

int Smadata2plus::connect(const char *password, const void *param)
{
	int deviceNum;
	int ret;
	int cnt = 0;

	if ((ret = sma.connect()) < 0) {
	    LOG_ERROR("Connecting bluetooth failed!");
	    return ret;
	}

	deviceNum = sma.getDeviceNum();
	LOG_INFO("%d devices!", deviceNum);

	if ((ret = logout()) < 0) {
		return ret;
	}

	do {
		ret = discoverDevices(deviceNum);
		if (cnt > NUM_RETRIES && ret < 0) {
			LOG_ERROR("Device discover failed!");
			return ret;
		} else if (ret < 0){
			LOG_WARNING("Device discover failed! Retrying ...");
			cnt++;
			sleep_for(seconds(cnt));
		}
	} while (ret < 0);
	cnt = 0;

	do {
		ret = authenticate(password, USER);
		if (cnt > NUM_RETRIES && ret < 0) {
			LOG_ERROR("Authentication  failed!");
			return ret;
		} else if (ret < 0){
			LOG_WARNING("Authentication failed! Retrying ...");
			cnt++;
			sleep_for(seconds(cnt));
		}
	} while (ret < 0);
	cnt = 0;

	do {
		ret = syncTime();
		if (cnt > NUM_RETRIES && ret < 0) {
			LOG_ERROR("Sync time failed!");
			return ret;
		} else if (ret < 0){
			LOG_WARNING("Sync time failed! Retrying ...");
			cnt++;
			sleep_for(seconds(cnt));
		}
	} while (ret < 0);
	cnt = 0;
	LOG_INFO("Synchronized time!");

	return 0;
}

static inline int32_t convertAcPower(uint32_t value) {
	if ((int32_t)value != PVLIB_INVALID_S32) {
		return (int32_t)value;
	} else {
		return PVLIB_INVALID_S32;
	}
}

static inline int32_t convertAcVoltage(uint32_t value) {
	if (value != PVLIB_INVALID_U32) {
		return (int32_t)value * 1000 / VOLTAGE_DIVISOR;
	} else {
		return PVLIB_INVALID_S32;
	}
}

static inline int32_t convertAcCurrent(uint32_t value) {
	if (value != PVLIB_INVALID_U32) {
		return (int32_t)value * 1000 / CURRENT_DIVISOR;
	} else {
		return PVLIB_INVALID_S32;
	}
}

static inline int32_t convertFrequency(uint32_t value) {
	if (value != PVLIB_INVALID_U32) {
		return value * 1000 / FREQUENCY_DIVISOR;
	} else {
		return PVLIB_INVALID_S32;
	}
}

int Smadata2plus::readAc(uint32_t id, pvlib_ac *ac)
{
	int ret;
	int cnt = 0;
	Record records[20];
	int num_recs = 20;

	pvlib_init_ac(ac);

	do {
		ret = readRecords(id, 0x5100, 0x200000, 0x50ffff, records, &num_recs, RECORD_1);
		if (cnt > NUM_RETRIES && ret < 0) {
			LOG_ERROR("Reading dc spot data  failed!");
			return ret;
		} else if (ret < 0){
			LOG_WARNING("Reading dc spot data failed! Retrying ...");
			cnt++;
			sleep_for(seconds(cnt));
		}
	} while (ret < 0);

	ac->phaseNum = 3;
	for (int i = 0; i < num_recs; i++) {
		Record *r = &records[i];

		uint32_t value = r->record.r1.value2;
		LOG_DEBUG("Read idx %d value: %d", r->header.idx, value);

		switch(r->header.idx) {
		case TOTAL_POWER:
			ac->totalPower = convertAcPower(value);
			break;
		case MAX_PHASE1:
			break;
		case MAX_PHASE2:
			break;
		case MAX_PHASE3:
			break;
		case UNKNOWN_1:
			LOG_DEBUG("UNKNOWN_1, %d", value);
			break;
		case UNKNWON_2:
			LOG_DEBUG("UNKNOWN_2, %d", value);
			break;
		case POWER_PHASE1:
			ac->power[0] = convertAcPower(value);
			break;
		case POWER_PHASE2:
			ac->power[1] = convertAcPower(value);;
			break;
		case POWER_PHASE3:
			ac->power[2] = convertAcPower(value);;
			break;
		case VOLTAGE_PHASE1:
			ac->voltage[0] = convertAcVoltage(value);
			break;
		case VOLTAGE_PHASE2:
			ac->voltage[1] = convertAcVoltage(value);;
			break;
		case VOLTAGE_PHASE3:
			ac->voltage[2] =  convertAcVoltage(value);
			break;
		case CURRENT_PHASE1:
			ac->current[0] = convertAcCurrent(value);
			break;
		case CURRENT_PHASE2:
			ac->current[1] = convertAcCurrent(value);
			break;
		case CURRENT_PHASE3:
			ac->current[2] = convertAcCurrent(value);
			break;
		case FREQUENCE:
			ac->frequency = convertFrequency(value);
			break;
		default:
			break;
		}
	}

	return 0;
}

int32_t convert_dc_power(uint32_t value) {
	if ((int32_t)value != PVLIB_INVALID_S32) {
		return (int32_t)value;
	} else {
		return PVLIB_INVALID_S32;
	}
}

int32_t convert_dc_voltage(uint32_t value) {
	if ((int32_t)value != PVLIB_INVALID_S32) {
		return (int32_t)value * 1000 / VOLTAGE_DIVISOR;
	} else {
		return PVLIB_INVALID_S32;
	}
}

int32_t convert_dc_current(uint32_t value) {
	if ((int32_t)value != PVLIB_INVALID_S32) {
		return (int32_t)value * 1000 / VOLTAGE_DIVISOR;
	} else {
		return PVLIB_INVALID_S32;
	}
}


int Smadata2plus::readDc(uint32_t id, pvlib_dc *dc)
{
	int ret;
	int cnt = 0;
	Record records[9];
	int num_recs = 9;

	pvlib_init_dc(dc);

	do {
		ret = readRecords(id, 0x5380, 0x200000, 0x5000ff, records, &num_recs, RECORD_1);
		if (cnt > NUM_RETRIES && ret < 0) {
			LOG_ERROR("Reading dc spot data  failed!");
			return ret;
		} else if (ret < 0){
			LOG_WARNING("Reading dc spot data failed! Retrying ...");
			cnt++;
			sleep_for(seconds(cnt));
		}
	} while (ret < 0);

	dc->trackerNum = 0;

	for (int i = 0; i < num_recs; i++) {
		Record *r = &records[i];
		uint32_t value = r->record.r1.value2;

		LOG_DEBUG("Read idx %d value: %d", r->header.idx, value);

		int tracker = r->header.cnt;
		if (tracker < 1) {
			LOG_ERROR("Invalid tracker number: %d", tracker);
			continue;
		}

		if (tracker > dc->trackerNum) {
			dc->trackerNum = tracker;
		}

		switch (r->header.idx) {
		case DC_POWER:
			dc->power[tracker - 1] = convert_dc_power(value);
			break;
		case DC_VOLTAGE:
			dc->voltage[tracker - 1] = convert_dc_voltage(value);
			break;
		case DC_CURRENT:
			dc->current[tracker - 1] = convert_dc_current(value);
			break;
		default:
			break;
		}
	}

	bool validPower = false;
	for (int i = 0; i < dc->trackerNum; ++i) {
		if (dc->power[i] != PVLIB_INVALID_S32) {
			validPower = true;
		}
	}

	if (validPower) {
		dc->totalPower = 0;
		for (int i = 0; i < dc->trackerNum; ++i) {
			if (dc->power[i] != PVLIB_INVALID_S32) {
				dc->totalPower += dc->power[i];
			}
		}
	}

	return 0;
}

int64_t convertStatsValue(uint64_t value) {
	if (value != PVLIB_INVALID_U64) {
		return (int64_t)value * 1000 / VOLTAGE_DIVISOR;
	} else {
		return PVLIB_INVALID_S64;
	}
}

int Smadata2plus::readStats(uint32_t id, pvlib_stats *stats) {
	int ret;
	int cnt = 0;
	Record records[4];
	int num_recs = 4;

	pvlib_init_stats(stats);

	do {
		ret = readRecords(id, 0x5400, 0x20000, 0x50ffff, records, &num_recs, RECORD_2);
		if (cnt > NUM_RETRIES && ret < 0) {
			LOG_ERROR("Reading stats  failed!");
			return ret;
		} else if (ret < 0){
			LOG_WARNING("Reading stats failed! Retrying ...");
			cnt++;
			sleep_for(seconds(cnt));
		}
	} while (ret < 0);

	for (int i = 0; i < num_recs; i++) {
		Record *r = &records[i];

		int64_t value = (int64_t)r->record.r2.value;

		LOG_DEBUG("Read stats: idx %x value: %" PRIu64, r->header.idx, value);

		switch (r->header.idx) {
		case STAT_TOTAL_YIELD:
			stats->totalYield = convertStatsValue(value);
			break;
		case STAT_DAY_YIELD:
			stats->dayYield = convertStatsValue(value);
			break;
		case STAT_OPERATION_TIME:
			stats->operationTime = convertStatsValue(value);
			break;
		case STAT_FEED_IN_TIME:
			stats->feedInTime = convertStatsValue(value);
			break;
		default:
			break;
		}
	}

	return 0;
}

int Smadata2plus::readStatus(uint32_t id, pvlib_status *status)
{
	int ret;
	int cnt = 0;
	Record records[1];
	int num_recs = 1;

	do {
		ret = readRecords(SERIAL_BROADCAST, 0x5180, 0x214800, 0x2148ff, records, &num_recs, RECORD_3);
		if (cnt > NUM_RETRIES && ret < 0) {
			LOG_ERROR("Reading inverter status  failed!");
			return ret;
		} else if (ret < 0){
			LOG_WARNING("Reading inverter status failed! Retrying ...");
			cnt++;
			sleep_for(seconds(cnt));
		}
	} while (ret < 0);

	status->number = 0;
	status->status = PVLIB_STATUS_UNKNOWN;

	for (int i = 0; i < num_recs; i++) {
		Record *r = &records[i];
		uint8_t *d = r->record.r3.data;

		switch(r->header.idx) {
		case DEVICE_STATUS: {
			Attribute attributes[8];
			int num_attributes = 8;
			parseAttributes(d, sizeof(r->record.r3.data), attributes, &num_attributes);
			for (int i = 0; i < num_attributes; i++) {
				if (attributes[i].selected) {
					status->number = attributes[i].attribute;
					switch (status->number) {
					case 307: status->status = PVLIB_STATUS_OK;      break;
					case 35:  status->status = PVLIB_STATUS_ERROR;   break;
					case 303: status->status = PVLIB_STATUS_OFF;     break;
					case 455: status->status = PVLIB_STATUS_WARNING; break;
					default:  status->status = PVLIB_STATUS_UNKNOWN; break;
					}
				}
			}
			break;
		}
		default:
			LOG_ERROR("Unexpected idx: %x", r->header.idx);
			break;
		}

	}

	return 0;
}

//version needs to be 10 at least 10 bytes
static int parseFirmwareVersion(uint8_t *data, char *version)
{
	//firmware version is stored from byte 16 + to 19

	if (data[18] > 9 || data[19] > 9) {
		LOG_ERROR("Invalid firmware version: 0x%02x%02x%02x%02x", data[16], data[17], data[18], data[19]);
		return -1;
	}

	char release_type[3] = {0};

	switch (data[16]) {
		case 0:release_type[0] = 'N'; break;
		case 1:release_type[0] = 'E'; break;
		case 2:release_type[0] = 'A'; break;
		case 3:release_type[0] = 'B'; break;
		case 4:release_type[0] = 'R'; break;
		case 5:release_type[0] = 'S'; break;
		default: snprintf(release_type, 3, "%02d", data[16]); break;
	}

	sprintf(version, "%d.%02d.%02d.%s", data[19], data[18], data[17], release_type);

	return 0;
}

int Smadata2plus::readInverterInfo(uint32_t id, pvlib_inverter_info *inverter_info)
{
	int ret;
	int cnt = 0;
	Record records[10];
	int num_recs = 10;

	do {
		ret = readRecords(id, 0x5800, 0x821e00, 0x8234FF, records, &num_recs, RECORD_3);
		if (cnt > NUM_RETRIES && ret < 0) {
			LOG_ERROR("Reading inverter info  failed!");
			return ret;
		} else if (ret < 0){
			LOG_WARNING("Reading inverter info failed! Retrying ...");
			cnt++;
			sleep_for(seconds(cnt));
		}
	} while (ret < 0);

	memset(inverter_info, 0, sizeof(*inverter_info));
	strcpy(inverter_info->manufacture, "SMA");

	for (int i = 0; i < num_recs; i++) {
		Record *r = &records[i];
		uint8_t * d = r->record.r3.data;

		switch (r->header.idx) {
		case DEVICE_NAME:
			if (strncmp((char*)d, "SN: ", 4) != 0) {
				LOG_WARNING("Unexpected device name!");
			}
			strncpy(inverter_info->name, (const char*)d, 32);
			break;
		case DEVICE_CLASS: {
			Attribute attributes[8];
			int num_attributes = 8;
			parseAttributes(d, sizeof(r->record.r3.data), attributes, &num_attributes);
			for (int i = 0; i < num_attributes; i++) {
				if (attributes[i].selected) {
					LOG_DEBUG("Device class: %d", attributes[i].attribute);
				}
			}
			break;
		}
		case DEVICE_TYPE: {
			Attribute attributes[8];
			int num_attributes = 8;
			parseAttributes(d, sizeof(r->record.r3.data), attributes, &num_attributes);
			for (int i = 0; i < num_attributes; i++) {
				if (attributes[i].selected) {
					LOG_DEBUG("Device type: %d", attributes[i].attribute);
					snprintf(inverter_info->type, sizeof(inverter_info->type), "%d", attributes[i].attribute);

				}
			}
			break;
		}
		case DEVICE_UNKNOWN:
			break;
		case DEVICE_SWVER:
			if (parseFirmwareVersion(d, inverter_info->firmware_version) < 0) {
				LOG_WARNING("Invalid firmware format. Ignoring it!");
			}
			break;
		default:
			break;
		}
	}

	return 0;
}

//struct EventData {
//    int32_t  time;
//	uint16_t entryId;
//	uint16_t sysId;
//	uint32_t serial;
//	uint16_t eventCode;
//	uint16_t eventFlags;
//	uint32_t group;
//	uint32_t unknown;
//	uint32_t tag;
//	uint32_t counter;
//	uint32_t dtChange;
//	uint32_t parameter;
//	uint32_t newVal;
//	uint32_t oldVal;
//};

static Smadata2plus::EventData parseEventData(uint8_t *buf, int len) {
	Smadata2plus::EventData ed;

	DataReader dr(buf, len);

	ed.time       = dr.i32le();
	ed.entryId    = dr.u16le();
	ed.sysId      = dr.u16le();
	ed.serial     = dr.u32le();
	ed.eventCode  = dr.u16le();
	ed.eventFlags = dr.u16le();
	ed.group      = dr.u32le();
	ed.unknown    = dr.u32le();
	ed.tag        = dr.u32le();
	ed.counter    = dr.u32le();
	ed.dtChange   = dr.u32le();
	ed.parameter  = dr.u32le();
	ed.newVal     = dr.u32le();
	ed.oldVal     = dr.u32le();

	return ed;
}

static Smadata2plus::TotalDayData parseTotalDayData(uint8_t *buf, int len) {
	Smadata2plus::TotalDayData tdd;

	DataReader dr(buf, len);

	tdd.time       = dr.u32le();
	tdd.totalYield = dr.u64le();

	return tdd;
}

int Smadata2plus::requestArchiveData(uint32_t serial, uint16_t obj, time_t from, time_t to) {
	Packet packet;
	uint8_t buf[12];
	int ret;
	DataWriter dw(buf, sizeof(buf));

	memset(buf, 0x00, sizeof(buf));

	packet.ctrl = CTRL_MASTER | CTRL_NO_BROADCAST;
	packet.dst = serial;
	packet.flag = 0x00;
	packet.data = buf;
	packet.len = sizeof(buf);
	packet.packet_num = 0;
	packet.start = 1;

	uint16_t reqObject = obj;//(user == USER) ? 0x7010 : 0x7012;

	dw.u16le(0x0200);
	dw.u16le(reqObject);
	dw.u32le(from);
	dw.u32le(to);

	if ((ret = write(&packet)) < 0) {
		return ret;
	}

	return 0;
}

int Smadata2plus::readEventData(uint32_t serial, time_t from, time_t to, UserType user, std::vector<EventData> &eventData) {
	int ret;
	uint16_t reqObj = (user == USER) ? 0x7010 : 0x7012;

	Transaction t(this);
	if ((ret = requestArchiveData(serial, reqObj, from, to)) < 0) {
		return ret;
	}

	uint8_t buf[512];
	Packet packet;

	packet.data = buf;
	packet.len = sizeof(buf);

	std::vector<EventData> events;
	do {
		if ((ret = read(&packet)) < 0)  {
			return ret;
		}

		//check data len
		if (packet.len < 12) {
			LOG_ERROR("Got packet with unexpected length!");
			return -1;
		}

		//check object
		uint16_t obj = byte::parseU16le(buf + 2);
		if (obj != reqObj) {
			LOG_ERROR("Unexpected object, expected: %x, got %x", reqObj, obj);
			return -1;
		}

		uint32_t dataFrom = byte::parseU32le(buf + 4);
		uint32_t dataTo   = byte::parseU32le(buf + 8);
		int entrys = dataTo - dataFrom;
		if (entrys <= 0) {
			LOG_ERROR("Unexpected entry number: %d", entrys);
			return -1;
		}

		for (int i = 12; i + 48 < packet.len && ((i - 12) / 48 < entrys); i += 48) {
			EventData eventData = parseEventData(buf + i, 48);
			if ((from <= eventData.time) && ( eventData.time <= to)) {
				//some or all inverter ignore the from and to time stamps and
				//return the complete event history, so filter know
				events.push_back(eventData);
			}
		}

	} while (packet.packet_num > 0);

	eventData = events;

	return 0;
}

int Smadata2plus::readTotalDayData(uint32_t serial, time_t from,
		time_t to, std::vector<TotalDayData> &totalDayData) {
	int ret;

	uint16_t reqObj = 0x7020;
	Transaction t(this);
	if ((ret = requestArchiveData(serial, reqObj, from, to)) < 0) {
		return ret;
	}

	uint8_t buf[512];
	Packet packet;

	packet.data = buf;
	packet.len = sizeof(buf);

	std::vector<TotalDayData> dayData;
	do {
		if ((ret = read(&packet)) < 0)  {
			return ret;
		}

		//check data len
		if (packet.len < 12) {
			LOG_ERROR("Got packet with unexpected length!");
			return -1;
		}

		//check object
		uint16_t obj = byte::parseU16le(buf + 2);
		if (obj != reqObj) {
			LOG_ERROR("Unexpected object, expected: %x, got %x", reqObj, obj);
			return -1;
		}

		uint32_t dataFrom = byte::parseU32le(buf + 4);
		uint32_t dataTo   = byte::parseU32le(buf + 8);
		int entrys = dataTo - dataFrom;
		if (entrys <= 0) {
			LOG_ERROR("Unexpected entry number: %d", entrys);
			return -1;
		}

		for (int i = 12; i + 12 < packet.len && ((i - 12) / 12 < entrys); i += 12) {
			TotalDayData day = parseTotalDayData(buf + i, 12);
			if ((from <= day.time) && (day.time <= to)) {
				//some or all inverter ignore the from and to time stamps and
				//return the complete event history, so filter know
				dayData.push_back(day);
			}
		}

	} while (packet.packet_num > 0);

	totalDayData = dayData;

	return 0;
}

int Smadata2plus::inverterNum() {
	return devices.size();
}

int Smadata2plus::getDevices(uint32_t* ids, int max_num) {
	for (int i = 0; i < max_num && i < static_cast<int>(devices.size()); i++) {
		ids[i] = devices[i].serial;
	}

	return 0;
}

//int Smadata2plus::readChannel(uint16_t channel, uint32_t idx1, uint32_t idx2)
//{
//	int ret;
//	smadata2plus_packet_t packet;
//	uint8_t data[512];
//
//	if ((ret = request_channel(channel, idx1, idx2)) < 0) {
//		return ret;
//	}
//
//	memset(&packet, 0x00, sizeof(packet));
//	packet.data = data;
//	packet.len = sizeof(data);
//
//	if ((ret = read(&packet)) < 0) {
//		return ret;
//	}
//
//	printf("\n\n\n");
//	for (int i = 0; i < packet.len; i++) {
//		printf("%02x ", packet.data[i]);
//		if ((i + 1) % 20 == 0) {
//			printf("\n");
//		}
//	}
//	printf("\n");
//	for (int i = 0; i < packet.len; i++) {
//		if (packet.data[i] >= 20)
//			printf("%c  ", packet.data[i]);
//		else
//			printf("   ");
//		if ((i + 1) % 20 == 0) {
//			printf("\n");
//		}
//	}
//	printf("\n\n\n");
//
//	return 0;
//}

void Smadata2plus::disconnect() {
	sma.disconnect();
}

//int smadata2plus_open(protocol_t *prot, connection_t *con, const char* params) {
//	smadata2plus_t *sma;
//	int ret;
//
//	sma = init(con);
//
//	if (sma == NULL)
//		return -1;
//
//	FILE *tag_file = NULL;
//	size_t file_length = strlen("en_US_tags.txt");
//	char tag_path[strlen(resources_path()) + 1 + file_length + 1];
//	if (params != NULL && strlen(params) == 5) {
//		;
//		strcpy(tag_path, resources_path());
//		strcat(tag_path, params);
//		strcat(tag_path, "_tags.txt");
//
//		tag_file = fopen(tag_path, "r");
//		if (tag_file == NULL) {
//			LOG_ERROR("tag file for local %s doesn't exist.", params);
//		}
//	}
//
//	if (tag_file == NULL) {
//		strcpy(tag_path, resources_path());
//		strcat(tag_path, "en_US_tags.txt");
//		tag_file = fopen(tag_path, "r");
//	}
//
//	if (tag_file == NULL) {
//		LOG_ERROR("tag file  %s doesn't exist.", tag_path);
//		return -1;
//	}
//
//	//params only contains filename to tag file
//	if ((ret = read_tags(sma, tag_file)) < 0) {
//		return ret;
//	}
//
//	prot->handle = sma;
//	prot->inverter_num = smadata2plus_device_num;
//	prot->get_devices = smadata2plus_get_devices;
//	prot->connect = smadata2plus_connect;
//	prot->disconnect = disconnect;
//	prot->get_stats = get_stats;
//	prot->get_ac = get_ac;
//	prot->get_dc = get_dc;
//	prot->get_status = get_status;
//	prot->close = smadata2plus_close;
//	prot->get_inverter_info = get_inverter_info;
//
//	return 0;
//}

static Protocol *createSmadata2plus(Connection *con) {
	return new Smadata2plus(con);
}

extern const ProtocolInfo smadata2plusProtocolInfo;
const ProtocolInfo smadata2plusProtocolInfo(createSmadata2plus, "smadata2plus", "pvlogdev,", "");
