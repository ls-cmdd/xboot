#include <assert.h>
#include <stdio.h>
#include <xboot/logger.h>
#include <xboot/resource.h>

static unsigned int cleanup_calls;
static xboot_status_t cleanup_resource(void *context, uint64_t address, uint64_t size) {
    (void)context;
    assert(address == 0x1000);
    assert(size == 0x1000);
    cleanup_calls++;
    return XBOOT_OK;
}

typedef struct {
    unsigned int calls;
    xboot_log_level last_level;
    const char *last_message;
} sink_state;

static void collect_log(void *context, xboot_log_level level, const char *message) {
    sink_state *state = context;
    state->calls++;
    state->last_level = level;
    state->last_message = message;
}

int main(void) {
    xboot_resource_manager resources;
    xboot_logger logger;
    sink_state sink = {0, XBOOT_LOG_TRACE, NULL};
    uint64_t id = 0;
    const xboot_resource_record *record;

    assert(xboot_resource_manager_init(&resources) == XBOOT_OK);
    assert(xboot_resource_acquire(&resources, XBOOT_RESOURCE_MEMORY, "test",
                                  0x1000, 0x1000, NULL, cleanup_resource, &id) == XBOOT_OK);
    assert(id != 0);
    assert(xboot_resource_active_count(&resources) == 1);
    record = xboot_resource_find(&resources, id);
    assert(record != NULL && record->state == XBOOT_RESOURCE_ACQUIRED);
    assert(xboot_resource_release(&resources, id) == XBOOT_OK);
    assert(cleanup_calls == 1);
    assert(xboot_resource_active_count(&resources) == 0);
    assert(xboot_resource_release(&resources, id) == XBOOT_NOT_FOUND);

    assert(xboot_logger_init(&logger, 1, XBOOT_LOG_INFO) == XBOOT_OK);
    assert(xboot_logger_add_sink(&logger, collect_log, &sink, XBOOT_LOG_WARN) == XBOOT_OK);
    xboot_logger_write(&logger, XBOOT_LOG_INFO, "filtered");
    assert(sink.calls == 0);
    xboot_logger_write(&logger, XBOOT_LOG_ERROR, "accepted");
    assert(sink.calls == 1);
    assert(sink.last_level == XBOOT_LOG_ERROR);
    assert(sink.last_message[0] == 'a');

    puts("resource/logger host tests: PASS");
    return 0;
}
