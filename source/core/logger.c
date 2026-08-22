#include <xboot/logger.h>

const char *xboot_log_level_name(xboot_log_level level) {
    switch (level) {
    case XBOOT_LOG_TRACE: return "TRACE";
    case XBOOT_LOG_DEBUG: return "DEBUG";
    case XBOOT_LOG_INFO: return "INFO";
    case XBOOT_LOG_NOTICE: return "NOTICE";
    case XBOOT_LOG_WARN: return "WARN";
    case XBOOT_LOG_ERROR: return "ERROR";
    case XBOOT_LOG_FATAL: return "FATAL";
    default: return "UNKNOWN";
    }
}

xboot_status_t xboot_logger_init(xboot_logger *logger, uint64_t session_id,
                                 xboot_log_level threshold) {
    if (logger == NULL || session_id == 0 || threshold > XBOOT_LOG_FATAL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    logger->session_id = session_id;
    logger->threshold = threshold;
    for (size_t i = 0; i < XBOOT_MAX_LOG_SINKS; i++) {
        logger->sinks[i].sink = NULL;
        logger->sinks[i].context = NULL;
        logger->sinks[i].threshold = XBOOT_LOG_FATAL;
    }
    return XBOOT_OK;
}

xboot_status_t xboot_logger_add_sink(xboot_logger *logger,
                                     xboot_log_sink sink,
                                     void *context,
                                     xboot_log_level threshold) {
    if (logger == NULL || sink == NULL || threshold > XBOOT_LOG_FATAL) {
        return XBOOT_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < XBOOT_MAX_LOG_SINKS; i++) {
        if (logger->sinks[i].sink == NULL) {
            logger->sinks[i].sink = sink;
            logger->sinks[i].context = context;
            logger->sinks[i].threshold = threshold;
            return XBOOT_OK;
        }
    }
    return XBOOT_NO_MEMORY;
}

void xboot_logger_write(xboot_logger *logger, xboot_log_level level,
                        const char *message) {
    if (logger == NULL || message == NULL || level > XBOOT_LOG_FATAL ||
        level < logger->threshold) {
        return;
    }
    for (size_t i = 0; i < XBOOT_MAX_LOG_SINKS; i++) {
        if (logger->sinks[i].sink != NULL && level >= logger->sinks[i].threshold) {
            logger->sinks[i].sink(logger->sinks[i].context, level, message);
        }
    }
}
