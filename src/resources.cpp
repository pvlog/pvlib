#include "resources.h"

#include <cstdlib>

const char* default_resources = "/usr/share/pvlib/";

const char *resources_path() {
    const char *env = getenv("PVLIB_RESOURCES");
    if (env != NULL) {
        return env;
    } else {
        return default_resources;
    }
}
