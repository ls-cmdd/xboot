#include <xboot/boot_manager.h>

static int valid_entry(const xboot_boot_entry *entry) {
    return entry != NULL && entry->has_linux != 0 && entry->linux_path[0] == '/';
}

static void copy_entry(xboot_boot_entry *destination, const xboot_boot_entry *source) {
    for (size_t i = 0; i < sizeof(*destination); i++) {
        ((uint8_t *)destination)[i] = ((const uint8_t *)source)[i];
    }
}

xboot_status_t xboot_boot_manager_init(xboot_boot_manager *manager) {
    if (manager == NULL) return XBOOT_INVALID_ARGUMENT;
    for (size_t i = 0; i < sizeof(*manager); i++) ((uint8_t *)manager)[i] = 0;
    manager->state = XBOOT_BOOT_IDLE;
    return XBOOT_OK;
}

xboot_status_t xboot_boot_manager_add(xboot_boot_manager *manager,
                                      const xboot_boot_entry *entry) {
    if (manager == NULL || !valid_entry(entry)) return XBOOT_INVALID_ARGUMENT;
    if (manager->entry_count >= XBOOT_MAX_BOOT_ENTRIES) return XBOOT_NO_MEMORY;
    copy_entry(&manager->entries[manager->entry_count++], entry);
    return XBOOT_OK;
}

xboot_status_t xboot_boot_manager_select(xboot_boot_manager *manager, size_t index) {
    if (manager == NULL || index >= manager->entry_count) return XBOOT_INVALID_ARGUMENT;
    if (!valid_entry(&manager->entries[index])) {
        manager->state = XBOOT_BOOT_FAILED;
        return XBOOT_CORRUPT;
    }
    manager->selected_index = index;
    manager->state = XBOOT_BOOT_SELECTED;
    return XBOOT_OK;
}

xboot_status_t xboot_boot_manager_select_first_valid(xboot_boot_manager *manager) {
    if (manager == NULL) return XBOOT_INVALID_ARGUMENT;
    for (size_t i = 0; i < manager->entry_count; i++) {
        if (valid_entry(&manager->entries[i])) return xboot_boot_manager_select(manager, i);
    }
    manager->state = XBOOT_BOOT_FAILED;
    return XBOOT_NOT_FOUND;
}

const xboot_boot_entry *xboot_boot_manager_selected(const xboot_boot_manager *manager) {
    if (manager == NULL || manager->state != XBOOT_BOOT_SELECTED ||
        manager->selected_index >= manager->entry_count) return NULL;
    return &manager->entries[manager->selected_index];
}
