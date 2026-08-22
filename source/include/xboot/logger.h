#ifndef XBOOT_LOGGER_H
#define XBOOT_LOGGER_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

typedef enum {
    XBOOT_LOG_TRACE = 0,
    XBOOT_LOG_DEBUG = 1,
    XBOOT_LOG_INFO = 2,
    XBOOT_LOG_NOTICE = 3,
    XBOOT_LOG_WARN = 4,
    XBOOT_LOG_ERROR = 5,
    XBOOT_LOG_FATAL = 6,
} xboot_log_level;

typedef void (*xboot_log_sink)(void *context,
                               xboot_log_level level,
                               const char *message);

typedef struct {
    xboot_log_sink sink;
    void *context;
    xboot_log_level threshold;
} xboot_log_sink_record;

#define XBOOT_MAX_LOG_SINKS 4U

typedef struct {
    xboot_log_sink_record sinks[XBOOT_MAX_LOG_SINKS];
    uint64_t session_id;
    xboot_log_level threshold;
} xboot_logger;

xboot_status_t xboot_logger_init(xboot_logger *logger, uint64_t session_id,
                                 xboot_log_level threshold);
xboot_status_t xboot_logger_add_sink(xboot_logger *logger,
                                     xboot_log_sink sink,
                                     void *context,
                                     xboot_log_level threshold);
void xboot_logger_write(xboot_logger *logger, xboot_log_level level,
                        const char *message);
const char *xboot_log_level_name(xboot_log_level level);

#endif
