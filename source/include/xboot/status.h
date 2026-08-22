#ifndef XBOOT_STATUS_H
#define XBOOT_STATUS_H

#include <stdint.h>

typedef enum {
    XBOOT_OK = 0,
    XBOOT_INVALID_ARGUMENT = 1,
    XBOOT_NO_MEMORY = 2,
    XBOOT_NOT_FOUND = 3,
    XBOOT_UNSUPPORTED = 4,
    XBOOT_IO_ERROR = 5,
    XBOOT_TIMEOUT = 6,
    XBOOT_CORRUPT = 7,
    XBOOT_SECURITY_ERROR = 8,
    XBOOT_HARDWARE_ERROR = 9,
    XBOOT_PROTOCOL_ERROR = 10,
    XBOOT_RESOURCE_BUSY = 11,
    XBOOT_BUFFER_TOO_SMALL = 12,
} xboot_status_t;

static inline const char *xboot_status_name(xboot_status_t status) {
    switch (status) {
    case XBOOT_OK: return "ok";
    case XBOOT_INVALID_ARGUMENT: return "invalid argument";
    case XBOOT_NO_MEMORY: return "no memory";
    case XBOOT_NOT_FOUND: return "not found";
    case XBOOT_UNSUPPORTED: return "unsupported";
    case XBOOT_IO_ERROR: return "I/O error";
    case XBOOT_TIMEOUT: return "timeout";
    case XBOOT_CORRUPT: return "corrupt";
    case XBOOT_SECURITY_ERROR: return "security error";
    case XBOOT_HARDWARE_ERROR: return "hardware error";
    case XBOOT_PROTOCOL_ERROR: return "protocol error";
    case XBOOT_RESOURCE_BUSY: return "resource busy";
    case XBOOT_BUFFER_TOO_SMALL: return "buffer too small";
    default: return "unknown status";
    }
}

#endif
