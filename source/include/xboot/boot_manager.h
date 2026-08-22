#ifndef XBOOT_BOOT_MANAGER_H
#define XBOOT_BOOT_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <xboot/status.h>
#include <xboot/boot_config.h>

#define XBOOT_MAX_BOOT_ENTRIES 8U

typedef enum {
    XBOOT_BOOT_IDLE = 0,
    XBOOT_BOOT_SELECTED = 1,
    XBOOT_BOOT_FAILED = 2,
} xboot_boot_state;

typedef struct {
    xboot_boot_entry entries[XBOOT_MAX_BOOT_ENTRIES];
    size_t entry_count;
    size_t selected_index;
    xboot_boot_state state;
} xboot_boot_manager;

xboot_status_t xboot_boot_manager_init(xboot_boot_manager *manager);
xboot_status_t xboot_boot_manager_add(xboot_boot_manager *manager,
                                      const xboot_boot_entry *entry);
xboot_status_t xboot_boot_manager_select(xboot_boot_manager *manager,
                                          size_t index);
xboot_status_t xboot_boot_manager_select_first_valid(xboot_boot_manager *manager);
const xboot_boot_entry *xboot_boot_manager_selected(const xboot_boot_manager *manager);

#endif
