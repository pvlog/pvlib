/*
 *   Pvlib - big, little endian conversion
 *
 *   Copyright (C) 2011 pvlogdev@gmail.com
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

#ifndef BYTE_H
#define BYTE_H

#include <cstdint>

//TODO: Do not use macros
#ifdef _MSC_VER
#   define BYTE_FAST_SWAP
#	include <stdlib.h>
#	define BSWAP_16(X) _byteswap_ushort(X)
#	define BSWAP_32(X) _byteswap_ulong(X)
#	define BSWAP_64(X) _byteswap_uint64(X)
#elif (((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 3))  | (__GNUC__ > 4))
#   define BYTE_FAST_SWAP
#	define BSWAP_16(X) (((uint16_t)(X) << 8) | ((uint16_t)(X) >> 8))
#	define BSWAP_32(X) __builtin_bswap32(X)
#	define BSWAP_64(X) __builtin_bswap64(X)
#else
#   define BYTE_SLOW_SWAP
#	define BSWAP_16(X) (((uint16_t)(X) << 8) | ((uint16_t)(X) >> 8))
#	define BSWAP_32(X) (((uint32_t)(X) << 24)              | \
					   (((uint32_t)(X) << 8) & 0x00FF0000) | \
					   (((uint32_t)(X) >> 8) & 0x0000FF00) | \
					   (((uint32_t)(X) >> 24)))
#	define BSWAP_64(X) (((uint64_t)(X) << 56) | \
                       (((uint64_t)(X) << 40) & UINT64_C(0x00ff000000000000)) | \
                       (((uint64_t)(X) << 24) & UINT64_C(0x0000ff0000000000)) | \
                       (((uint64_t)(X) << 8)  & UINT64_C(0x000000ff00000000)) | \
                       (((uint64_t)(X) >> 8)  & UINT64_C(0x00000000ff000000)) | \
                       (((uint64_t)(X) >> 24) & UINT64_C(0x0000000000ff0000)) | \
                       (((uint64_t)(X) >> 40) & UINT64_C(0x000000000000ff00)) | \
                       ((uint64_t)(X)  >> 56))
#endif

#if defined (__GLIBC__)
#   include <endian.h>
#   if (__BYTE_ORDER == __LITTLE_ENDIAN)
#       define BYTE_LITTLE_ENDIAN
#   elif (__BYTE_ORDER == __BIG_ENDIAN)
#      define BYTE_BIG_ENDIAN
#   else
#       define BYTE_UNKNOWN_ORDER
#   endif
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#   define BYTE_BIG_ENDIAN
#elif defined (_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#   define BYTE_LITTLE_ENDIAN
#elif defined (BIG_ENDIAN) && !defined(LITTLE_ENDIAN)
#   define  define BYTE_BIG_ENDIAN
#elif defined (LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#   define BYTE_LITTLE_ENDIAN
#else
#   define BYTE_UNKNOWN_ORDER
#endif

#if (defined BYTE_UNKNOWN_ORDER || ((!defined __i386__) && (!defined _M_IX86) && (!defined _X86_) \
		&& (!defined __X86__) && (!__x86_64)))
#   define BYTE_NO_CAST
#endif

namespace byte {


/**
 * Parse little endian stored unsigned 16 bit integer.
 *
 * @param data to parse.
 * @return parsed integer.
 */
static inline uint16_t parseU16le(const uint8_t *data)
{
	return (uint16_t) data[1] << 8 | (uint16_t) data[0];
}

/**
 * Parse big endian stored unsigned 16 bit integer.
 *
 * @param data to parse.
 * @return parsed integer.
 */
static inline uint16_t parseU16be(const uint8_t *data)
{
	return (uint16_t) data[0] << 8 | (uint16_t) data[1];
}

/**
 * Parse little endian stored unsigned 32 bit integer.
 *
 * @param data to parse.
 * @return parsed integer.
 */
static inline uint32_t parseU32le(const uint8_t *data)
{
#ifdef BYTE_NO_CAST
	return ((uint32_t) data[3] << 24) | ((uint32_t) data[2] << 16) | ((uint32_t) data[1] << 8)
	        | ((uint32_t) data[0]);
#elif defined BYTE_LITTLE_ENDIAN
	return *((uint32_t*)data);
#elif defined BYTE_BIG_ENDIAN
	return BSWAP_32(*((uint32_t*)data));
#endif
}

/**
 * Parse big endian stored unsigned 32 bit integer.
 *
 * @param data to parse.
 * @return parsed integer.
 */
static inline uint32_t parseU32be(const uint8_t *data)
{
#ifdef BYTE_NO_CAST
	return ((uint32_t) data[0] << 24) | ((uint32_t) data[1] << 16) | ((uint32_t) data[2] << 8)
	        | ((uint32_t) data[3]);
#elif defined BYTE_LITTLE_ENDIAN
	return BSWAP_32(*((uint32_t*)data));
#elif defined BYTE_BIG_ENDIAN
	return *((uint32_t*)data);
#endif
}

/**
 * Parse little endian stored unsigned 64 bit integer.
 *
 * @param data to parse.
 * @return parsed integer.
 */
static inline uint64_t parseU64le(const uint8_t *data)
{
#ifdef BYTE_NO_CAST
	return ((uint64_t) data[7] << 56) | ((uint64_t) data[6] << 48) | ((uint64_t) data[5] << 40)
	        | ((uint64_t) data[4] << 32) | ((uint64_t) data[3] << 24) | ((uint64_t) data[2] << 16)
	        | ((uint64_t) data[1] << 8) | ((uint64_t) data[0]);
#elif defined BYTE_LITTLE_ENDIAN
	return *((uint64_t*)data);
#elif defined BYTE_BIG_ENDIAN
	return BSWAP_64(*((uint64_t*)data));
#endif
}

/**
 * Parse big endian stored unsigned 64 bit integer.
 *
 * @param data to parse.
 * @return parsed integer.
 */
static inline uint64_t parseU64be(const uint8_t *data)
{
#ifdef BYTE_NO_CAST
	return ((uint64_t) data[0] << 56) | ((uint64_t) data[1] << 48) | ((uint64_t) data[2] << 40)
	        | ((uint64_t) data[3] << 32) | ((uint64_t) data[4] << 24) | ((uint64_t) data[5] << 16)
	        | ((uint64_t) data[5] << 8) | ((uint64_t) data[7]);
#elif defined BYTE_LITTLE_ENDIAN
	return BSWAP_64(*((uint64_t*)data));
#elif defined BYTE_BIG_ENDIAN
	return *((uint64_t*)data);
#endif
}

/**
 * Store 16 bit integer in little endian format.
 *
 * @param data buf to store integer.
 * @param word integer to store.
 */
static inline void storeU16le(uint8_t *data, uint16_t word)
{
	data[0] = (uint8_t)(word & 0xff);
	data[1] = (uint8_t)((word >> 8) & 0xff);
}

/**
 * Store 16 bit integer in big endian format.
 *
 * @param data buf to store integer.
 * @param word integer to store.
 */
static inline void storeU16be(uint8_t *data, uint16_t word)
{
	data[0] = (uint8_t)((word >> 8) & 0xff);
	data[1] = (uint8_t)(word & 0xff);
}

/**
 * Store 32 bit integer in little endian format.
 *
 * @param data buf to store integer.
 * @param dword integer to store.
 */
static inline void storeU32le(uint8_t *data, uint32_t dword)
{
#ifdef BYTE_NO_CAST
	data[3] = (uint8_t)((dword >> 24) & 0xff);
	data[2] = (uint8_t)((dword >> 16) & 0xff);
	data[1] = (uint8_t)((dword >> 8) & 0xff);
	data[0] = (uint8_t)(dword & 0xff);
#elif defined BYTE_LITTLE_ENDIAN
	(*(uint32_t*)data) = dword;
#elif defined BYTE_BIG_ENDIAN
	(*(uint32_t*)data) = BSWAP_32(dword);
#endif
}

/**
 * Store 32 bit integer in big endian format.
 *
 * @param data buf to store integer.
 * @param dword integer to store.
 */
static inline void storeU32be(uint8_t *data, uint32_t dword)
{
#ifdef BYTE_NO_CAST
	data[0] = (uint8_t)((dword >> 24) & 0xff);
	data[1] = (uint8_t)((dword >> 16) & 0xff);
	data[2] = (uint8_t)((dword >> 8) & 0xff);
	data[3] = (uint8_t)(dword & 0xff);
#elif defined BYTE_LITTLE_ENDIAN
	(*(uint32_t*)data) = BSWAP_32(dword);
#elif defined BYTE_BIG_ENDIAN
	(*(uint32_t*)data) = dword;
#endif
}

/**
 * Store 64 bit integer in little endian format.
 *
 * @param data buf to store integer.
 * @param qword integer to store.
 */
static inline void storeU64le(uint8_t *data, uint64_t qword)
{
#ifdef BYTE_NO_CAST
	data[7] = (uint8_t)((qword >> 56) & 0xff);
	data[6] = (uint8_t)((qword >> 48) & 0xff);
	data[5] = (uint8_t)((qword >> 40) & 0xff);
	data[4] = (uint8_t)((qword >> 32) & 0xff);
	data[3] = (uint8_t)((qword >> 24) & 0xff);
	data[2] = (uint8_t)((qword >> 16) & 0xff);
	data[1] = (uint8_t)((qword >> 8) & 0xff);
	data[0] = (uint8_t)(qword & 0xff);
#elif defined BYTE_LITTLE_ENDIAN
	(*(uint64_t*)data) = qword;
#elif defined BYTE_BIG_ENDIAN
	(*(uint64_t*)data) = BSWAP_64(qword);
#endif
}

/**
 * Store 64 bit integer in big endian format.
 *
 * @param data buf to store integer.
 * @param qword integer to store.
 */
static inline void storeU64be(uint8_t *data, uint64_t qword)
{
#ifdef BYTE_NO_CAST
	data[0] = (uint8_t)((qword >> 56) & 0xff);
	data[1] = (uint8_t)((qword >> 48) & 0xff);
	data[2] = (uint8_t)((qword >> 40) & 0xff);
	data[3] = (uint8_t)((qword >> 32) & 0xff);
	data[4] = (uint8_t)((qword >> 24) & 0xff);
	data[5] = (uint8_t)((qword >> 16) & 0xff);
	data[6] = (uint8_t)((qword >> 8) & 0xff);
	data[7] = (uint8_t)(qword & 0xff);
#elif defined BYTE_LITTLE_ENDIAN
	(*(uint64_t*)data) = BSWAP_64(qword);
#elif defined BYTE_BIG_ENDIAN
	(*(uint64_t*)data) = qword;
#endif
}

class DataWriter {
	uint8_t *buf;
	uint8_t *end;
public:
	inline DataWriter(uint8_t *buf, int len) : buf(buf), end(buf + len) {
		//nothing to do
	}

	inline void u8(uint8_t val) {
		assert(buf + 1 <= end);
		*buf = val; ++buf;
	}

	inline void u16le(uint16_t val) {
		assert(buf + 2 <= end);
		storeU16le(buf, val); buf += 2;
	}


	inline void u32le(uint32_t val) {
		assert(buf + 4 <= end);
		storeU32le(buf, val); buf += 4;
	}

	inline void u64le(uint64_t val) {
		assert(buf + 8 <= end);
		storeU64le(buf, val); buf += 8;
	}

	inline void i16le(int16_t val){
		u16le(static_cast<int16_t>(val));
	}

	inline void i32le(int32_t val) {
		i32le(static_cast<int32_t>(val));
	}

	inline void i64le(int64_t val) {
		i64le(static_cast<int64_t>(val));
	}

	inline void u16be(uint16_t val) {
		assert(buf + 2 <= end);
		storeU16be(buf, val); buf += 2;
	}


	inline void u32be(uint32_t val) {
		assert(buf + 4 <= end);
		storeU32be(buf, val); buf += 4;
	}

	inline void u64be(uint64_t val) {
		assert(buf + 8 <= end);
		storeU64be(buf, val); buf += 8;
	}

	inline void i16be(int16_t val){
		u16be(static_cast<int16_t>(val));
	}

	inline void i32be(int32_t val) {
		i32be(static_cast<int32_t>(val));
	}

	inline void i64be(int64_t val) {
		i64be(static_cast<int64_t>(val));
	}

	inline void skip(int len) {
		assert(buf + len <= end);
		buf += len;
	}
};

class DataReader {
	const uint8_t *buf;
	const uint8_t *end;

public:
	inline DataReader(const uint8_t *buf, int len) : buf(buf), end(buf + len) {
		//nothing to do
	}

	inline uint16_t u16le() {
		assert(buf + 2 <= end);
		return parseU16le(buf); buf += 2;
	}


	inline uint32_t u32le() {
		assert(buf + 4 <= end);
		return parseU32le(buf); buf += 4;
	}

	inline uint64_t u64le() {
		assert(buf + 8 <= end);
		return parseU64le(buf); buf += 8;
	}

	inline int16_t i16le(){
		return static_cast<int16_t>(u16le());
	}

	inline int32_t i32le() {
		return static_cast<int32_t>(u32le());
	}

	inline int64_t i64le() {
		return static_cast<int64_t>(u64le());
	}

	inline uint16_t u16be() {
		assert(buf + 2 <= end);
		return parseU16be(buf); buf += 2;
	}


	inline uint32_t u32be() {
		assert(buf + 4 <= end);
		return parseU32be(buf); buf += 4;
	}

	inline uint64_t u64be() {
			assert(buf + 8 <= end);
			return parseU64be(buf); buf += 8;
	}

	inline int16_t i16be() {
		return static_cast<int16_t>(u16be());
	}


	inline int32_t i32be() {
		return static_cast<int32_t>(u32be());
	}

	inline int64_t i64be() {
		return static_cast<int64_t>(u64be());
	}

	inline uint8_t u8() {
		assert(buf + 1 < end);
		return *buf++;
	}

	inline int8_t i8() {
		return static_cast<int8_t>(u8());
	}

	inline void skip(int len) {
		assert(buf + len <= end);
		buf += len;
	}
};

} //namespace byte {

#undef BYTE_NO_CAST /* not needed anymore */

#endif /* #ifndef BYTE_H */
