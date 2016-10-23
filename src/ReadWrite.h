#ifndef SRC_PVLIB_SRC_READWRITE_H_
#define SRC_PVLIB_SRC_READWRITE_H_

#include <string>

namespace pvlib {

class ReadWrite {
public:
	virtual ~ReadWrite() {}

	/**
	 * Write data.
	 *
	 * @param con connection handle.
	 * @param data data to write.
	 * @param len length of data.
	 *
	 * @return < 0 if error occurs.
	 */
	virtual int write(const uint8_t *data, int len, const std::string &to) = 0;

	int write(const uint8_t *data, int len) {
		return write(data, len, "");
	}

	/**
	 * Read data.
	 *
	 * @param con connection handle.
	 * @param data buffer to read to.
	 * @param len length of data to read.
	 * @param timeout timeout in ms
	 *
	 * @return < 0 if error occurs, else amount of bytes read.
	 */
	virtual int read(uint8_t *data, int maxlen, std::string &from) = 0;

	int read(uint8_t *data, int max_len) {
		std::string str;
		return read(data, max_len, str);
	}
};

} //namespace pvlib {

#endif /* SRC_PVLIB_SRC_READWRITE_H_ */
