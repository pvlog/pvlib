#ifndef CONNECTION_PRIVATE_H
#define CONNECTION_PRIVATE_H

#include "protocol.h"

typedef struct connection_info_s {
    const char *name;
    const char *author;
    const char *comment;
    connection_t *(*open)();
} connection_info_t;

struct connection_s {
    void *handle;

    int (*write)(connection_t *con, const uint8_t *data, int len);
    int (*read)(connection_t *con, uint8_t *data, int max_len, int timeout);
    int (*info)(connection_t *con, connection_data_t * info);
    int (*connect)(connection_t *con, const char *, const void *);
    void (*disconnect)(connection_t *con);
    void (*close)(connection_t *con);
};

#endif //#ifndef CONNECTION_PRIVATE_H
