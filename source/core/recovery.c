#include <xboot/recovery.h>

static void clear_bytes(void *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) ((uint8_t *)buffer)[i] = 0;
}

static void copy_message(char destination[XBOOT_DIAGNOSTIC_MESSAGE_MAX], const char *source) {
    size_t i = 0;
    while (source[i] != '\0' && i + 1 < XBOOT_DIAGNOSTIC_MESSAGE_MAX) {
        destination[i] = source[i];
        i++;
    }
    destination[i] = '\0';
}

xboot_status_t xboot_recovery_init(xboot_recovery_state *state) {
    if (state == NULL) return XBOOT_INVALID_ARGUMENT;
    clear_bytes(state, sizeof(*state));
    return XBOOT_OK;
}

xboot_status_t xboot_recovery_record(xboot_recovery_state *state,
                                     xboot_status_t status,
                                     const char *message) {
    xboot_diagnostic_event *event;
    if (state == NULL || message == NULL) return XBOOT_INVALID_ARGUMENT;
    event = &state->events[state->next];
    clear_bytes(event, sizeof(*event));
    event->sequence = state->count + state->failed_attempts + 1U;
    event->status = status;
    copy_message(event->message, message);
    state->next = (state->next + 1U) % XBOOT_DIAGNOSTIC_CAPACITY;
    if (state->count < XBOOT_DIAGNOSTIC_CAPACITY) state->count++;
    if (status != XBOOT_OK) state->failed_attempts++;
    return XBOOT_OK;
}

xboot_status_t xboot_recovery_begin_boot(xboot_recovery_state *state) {
    if (state == NULL) return XBOOT_INVALID_ARGUMENT;
    if (state->boot_attempts == UINT32_MAX) return XBOOT_NO_MEMORY;
    state->boot_attempts++;
    return xboot_recovery_record(state, XBOOT_OK, "boot attempt started");
}

xboot_status_t xboot_recovery_mark_success(xboot_recovery_state *state,
                                           uint32_t entry_index) {
    if (state == NULL) return XBOOT_INVALID_ARGUMENT;
    state->last_successful_entry = entry_index;
    return xboot_recovery_record(state, XBOOT_OK, "boot attempt succeeded");
}

const xboot_diagnostic_event *xboot_recovery_event(const xboot_recovery_state *state,
                                                   size_t reverse_index) {
    size_t index;
    if (state == NULL || reverse_index >= state->count) return NULL;
    index = (state->next + XBOOT_DIAGNOSTIC_CAPACITY - 1U - reverse_index) % XBOOT_DIAGNOSTIC_CAPACITY;
    return &state->events[index];
}
