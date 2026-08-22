#ifndef XBOOT_RECOVERY_H
#define XBOOT_RECOVERY_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>

#define XBOOT_DIAGNOSTIC_CAPACITY 16U
#define XBOOT_DIAGNOSTIC_MESSAGE_MAX 96U

typedef struct {
    uint64_t sequence;
    xboot_status_t status;
    char message[XBOOT_DIAGNOSTIC_MESSAGE_MAX];
} xboot_diagnostic_event;

typedef struct {
    xboot_diagnostic_event events[XBOOT_DIAGNOSTIC_CAPACITY];
    size_t next;
    size_t count;
    uint32_t boot_attempts;
    uint32_t failed_attempts;
    uint32_t last_successful_entry;
} xboot_recovery_state;

xboot_status_t xboot_recovery_init(xboot_recovery_state *state);
xboot_status_t xboot_recovery_record(xboot_recovery_state *state,
                                     xboot_status_t status,
                                     const char *message);
xboot_status_t xboot_recovery_begin_boot(xboot_recovery_state *state);
xboot_status_t xboot_recovery_mark_success(xboot_recovery_state *state,
                                           uint32_t entry_index);
const xboot_diagnostic_event *xboot_recovery_event(const xboot_recovery_state *state,
                                                   size_t reverse_index);

#endif
